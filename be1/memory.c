/**
 * Bitemu - Game Boy Memory
 *
 * Responsabilidades:
 * - Mapa de direcciones: ROM (bank 0/1), VRAM, WRAM, OAM, I/O, HRAM, IE.
 * - MBC1/MBC3: cambio de bancos ROM/RAM, habilitación de RAM externa.
 * - I/O especial: JOYP (multiplexado D-pad/buttons), IF (bits reservados), DIV (reseteo), DMA (copia a OAM).
 * - Guardado/carga de batería (.sav) para cartuchos con RAM persistente.
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "memory.h"
#include "gb_constants.h"
#include "core/utils/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ---------------------------------------------------------------------------
 * Cartucho: detección de RAM con batería (para .sav)
 * --------------------------------------------------------------------------- */
static int cart_has_battery(uint8_t cart_type)
{
    return cart_type == 0x03 || cart_type == GB_CART_MBC2_BAT ||
           (cart_type >= 0x0F && cart_type <= 0x13) ||
           cart_type == 0x1B || cart_type == 0x1E;
}

static int cart_is_mbc2(uint8_t cart_type)
{
    return cart_type == GB_CART_MBC2 || cart_type == GB_CART_MBC2_BAT;
}

static int cart_has_rtc(uint8_t cart_type)
{
    return cart_type >= 0x0F && cart_type <= 0x13;
}

/* Avanza RTC MBC3 según tiempo real transcurrido desde rtc_last_sync */
static void rtc_advance(gb_mem_t *mem)
{
    if (!cart_has_rtc(mem->cart_type))
        return;
    if (mem->rtc_dh & GB_MBC3_RTC_HALT)
        return;

    time_t now = time(NULL);
    if (now < 0)
        return;
    uint64_t now_u = (uint64_t)now;
    uint64_t elapsed = (now_u > mem->rtc_last_sync) ? (now_u - mem->rtc_last_sync) : 0;
    mem->rtc_last_sync = now_u;

    if (elapsed == 0)
        return;

    /* Sumar elapsed segundos a rtc_s, rtc_m, rtc_h, días */
    uint32_t sec = mem->rtc_s + (uint32_t)(elapsed % 60);
    mem->rtc_s = sec % 60;
    uint32_t carry = sec / 60;

    uint32_t min = mem->rtc_m + carry;
    mem->rtc_m = min % 60;
    carry = min / 60;

    uint32_t hr = mem->rtc_h + carry;
    mem->rtc_h = hr % 24;
    carry = hr / 24;

    uint32_t days = mem->rtc_dl + ((mem->rtc_dh & 1) << 8) + carry;
    mem->rtc_dl = days & 0xFF;
    mem->rtc_dh = (mem->rtc_dh & 0xFE) | ((days >> 8) & 1);
    if (days >= 512)
        mem->rtc_dh |= 0x80;  /* overflow flag */
}

/* Actualiza rom0_ptr/rom1_ptr para acceso rápido (hot path) */
void gb_mem_update_rom_ptrs(gb_mem_t *mem)
{
    if (!mem->rom)
    {
        mem->rom0_ptr = NULL;
        mem->rom1_ptr = NULL;
        return;
    }
    uint32_t bank0 = 0;
    if (mem->mbc1_mode && mem->cart_type >= GB_CART_MBC1 && mem->cart_type <= 0x03)
        bank0 = (mem->rom_bank & GB_MBC_ROM_BANK_HI);
    mem->rom0_ptr = mem->rom + bank0 * GB_ROM_BANK_SIZE;

    uint32_t bank1;
    if (mem->cart_type == GB_CART_ROM_ONLY)
        bank1 = 1;
    else if (mem->cart_type >= GB_CART_MBC5 && mem->cart_type <= GB_CART_MBC5_MAX)
        bank1 = (mem->mbc5_rom_bank_high << 8) | mem->rom_bank;
    else if (mem->cart_type >= 0x0F && mem->cart_type <= 0x13)
        bank1 = (mem->rom_bank & 0x7F) ? (mem->rom_bank & 0x7F) : 1;
    else
    {
        bank1 = mem->rom_bank & (GB_MBC_ROM_BANK_MASK | GB_MBC_ROM_BANK_HI);
        if ((bank1 & GB_MBC_ROM_BANK_MASK) == 0)
            bank1 |= 1;
    }
    uint32_t phys1 = bank1 * GB_ROM_BANK_SIZE;
    mem->rom1_ptr = (phys1 < mem->rom_size) ? mem->rom + phys1 : NULL;
}

/* ---------------------------------------------------------------------------
 * Init / Reset
 * --------------------------------------------------------------------------- */
void gb_mem_init(gb_mem_t *mem)
{
    memset(mem, 0, sizeof(gb_mem_t));
}

void gb_mem_reset(gb_mem_t *mem)
{
    mem->rom_bank = 1;
    mem->ext_ram_bank = 0;
    mem->ext_ram_enabled = 0;
    mem->mbc5_rom_bank_high = 0;
    mem->mbc1_mode = 0;
    mem->rtc_latch_prev = 0;
    if (cart_has_rtc(mem->cart_type))
        mem->rtc_last_sync = (uint64_t)time(NULL);
    mem->ie = 0;
    mem->joypad_state = GB_JOYP_RELEASED;
    gb_mem_update_rom_ptrs(mem);
    mem->io[GB_IO_LCDC] = 0x91;
    mem->io[GB_IO_STAT] = 0x85;
    mem->io[GB_IO_LY] = 0;
    mem->io[GB_IO_LYC] = 0;
    mem->io[GB_IO_SCY] = 0;
    mem->io[GB_IO_SCX] = 0;
    mem->io[GB_IO_BGP] = 0xFC;
    mem->io[GB_IO_OBP0] = 0xFF;
    mem->io[GB_IO_OBP1] = 0xFF;
    mem->io[GB_IO_NR52] = 0xF1;
}

#define GB_READ_UNMAPPED 0xFF

/* ---------------------------------------------------------------------------
 * Lectura por direcciones (incluye I/O y casos especiales)
 * --------------------------------------------------------------------------- */
__attribute__((hot))
uint8_t gb_mem_read(gb_mem_t *mem, uint16_t addr)
{
    /* Fast path: ROM (mayoría de accesos CPU) */
    if (__builtin_expect(addr <= GB_ROM1_END, 1))
    {
        if (mem->rom)
        {
            if (addr <= GB_ROM0_END && mem->rom0_ptr)
                return mem->rom0_ptr[addr];
            if (addr >= GB_ROM1_START && mem->rom1_ptr)
                return mem->rom1_ptr[addr - GB_ROM1_START];
        }
        return GB_READ_UNMAPPED;
    }
    if (addr <= GB_VRAM_END)
        return mem->vram[addr - GB_VRAM_START];
    if (addr <= GB_EXT_RAM_END)
    {
        if (!mem->ext_ram_enabled)
            return GB_READ_UNMAPPED;
        if (cart_is_mbc2(mem->cart_type))
        {
            uint16_t off = (addr - GB_EXT_RAM_START) & (GB_MBC2_RAM_SIZE - 1);
            return mem->ext_ram[off] | 0xF0;
        }
        if (mem->cart_type >= 0x0F && mem->cart_type <= 0x13)
        {
            if (mem->ext_ram_bank >= GB_MBC3_RTC_S && mem->ext_ram_bank <= GB_MBC3_RTC_DH)
                return mem->rtc_latched[mem->ext_ram_bank - GB_MBC3_RTC_S];
            if (mem->ext_ram_bank > 3)
                return GB_READ_UNMAPPED;
        }
        {
            uint8_t ram_bank = mem->ext_ram_bank & GB_MBC_RAM_BANK_MASK;
            if (mem->cart_type >= GB_CART_MBC1 && mem->cart_type <= 0x03 && !mem->mbc1_mode)
                ram_bank = 0;
            return mem->ext_ram[(addr - GB_EXT_RAM_START) + ram_bank * GB_EXT_RAM_BANK];
        }
    }
    if (addr <= GB_WRAM_END)
        return mem->wram[addr - GB_WRAM_START];
    if (addr <= GB_ECHO_END)
        return mem->wram[addr - GB_ECHO_START];
    if (addr <= GB_OAM_END)
        return mem->oam[addr - GB_OAM_START];
    if (addr <= GB_PROHIBITED_END)
        return GB_READ_UNMAPPED;
    if (addr <= GB_IO_END)
    {
        uint16_t off = addr - GB_IO_START;
        if (off == GB_IO_JOYP)
        {
            uint8_t sel = mem->io[GB_IO_JOYP] & (GB_JOYP_SEL_DIR | GB_JOYP_SEL_BTN);
            uint8_t bits = GB_JOYP_READ_MASK;
            if (!(sel & GB_JOYP_SEL_DIR))
                bits &= ~(mem->joypad_state & GB_JOYP_READ_MASK);
            if (!(sel & GB_JOYP_SEL_BTN))
                bits &= ~((mem->joypad_state >> 4) & GB_JOYP_READ_MASK);
            return sel | bits;
        }
        if (off == GB_IO_IF)
            return (mem->io[GB_IO_IF] & GB_IF_IE_MASK) | GB_IF_RESERVED;
        return mem->io[off];
    }
    if (addr <= GB_HRAM_END)
        return mem->hram[addr - GB_HRAM_START];
    return mem->ie;
}

/* ---------------------------------------------------------------------------
 * Escritura por direcciones (MBC, I/O especial: DIV, DMA, IF, JOYP)
 * --------------------------------------------------------------------------- */
void gb_mem_write(gb_mem_t *mem, uint16_t addr, uint8_t val)
{
    if (addr <= GB_ROM1_END)
    {
        if (mem->cart_type == GB_CART_ROM_ONLY)
            return;
        if (cart_is_mbc2(mem->cart_type))
        {
            if (addr <= GB_MBC_ROM_BANK_END)
            {
                if (addr & 0x0100)
                {
                    mem->rom_bank = val & 0x0F;
                    if (mem->rom_bank == 0)
                        mem->rom_bank = 1;
                    gb_mem_update_rom_ptrs(mem);
                }
                else
                    mem->ext_ram_enabled = (val & GB_MBC_RAM_ENABLE_MASK) == GB_MBC_RAM_ENABLE_VAL;
            }
            return;
        }
        if (mem->cart_type >= GB_CART_MBC5 && mem->cart_type <= GB_CART_MBC5_MAX)
        {
            /* MBC5 */
            if (addr <= GB_MBC_RAM_ENABLE_END)
                mem->ext_ram_enabled = (val & GB_MBC_RAM_ENABLE_MASK) == GB_MBC_RAM_ENABLE_VAL;
            else if (addr <= 0x2FFF)
                mem->rom_bank = val;  /* 0x2000-0x2FFF: banco bajo (0-255) */
            else if (addr <= 0x3FFF)
                mem->mbc5_rom_bank_high = val & 1;  /* 0x3000-0x3FFF: bit 9 */
            else if (addr <= GB_MBC_RAM_BANK_END)
                mem->ext_ram_bank = val & 0x0F;  /* 0x4000-0x5FFF: RAM bank 0-15 */
            gb_mem_update_rom_ptrs(mem);
            return;
        }
        if (mem->cart_type >= 0x0F && mem->cart_type <= 0x13)
        {
            /* MBC3 */
            if (addr <= GB_MBC_RAM_ENABLE_END)
                mem->ext_ram_enabled = (val & GB_MBC_RAM_ENABLE_MASK) == GB_MBC_RAM_ENABLE_VAL;
            else if (addr <= GB_MBC_ROM_BANK_END)
            {
                mem->rom_bank = val & 0x7F;
                if (mem->rom_bank == 0)
                    mem->rom_bank = 1;
                gb_mem_update_rom_ptrs(mem);
            }
            else if (addr <= GB_MBC_RAM_BANK_END)
                mem->ext_ram_bank = val & 0x0F;  /* 0-3 RAM, 4-0xC RTC */
            else if (addr >= 0x6000)
            {
                /* 0x6000-0x7FFF: RTC latch. 0x01 luego 0x00 copia RTC a latched */
                if (mem->rtc_latch_prev && val == 0x00)
                {
                    rtc_advance(mem);
                    mem->rtc_latched[0] = mem->rtc_s;
                    mem->rtc_latched[1] = mem->rtc_m;
                    mem->rtc_latched[2] = mem->rtc_h;
                    mem->rtc_latched[3] = mem->rtc_dl;
                    mem->rtc_latched[4] = mem->rtc_dh;
                }
                mem->rtc_latch_prev = (val == 0x01);
            }
            return;
        }
        /* MBC1 */
        if (addr <= GB_MBC_RAM_ENABLE_END)
        {
            mem->ext_ram_enabled = (val & GB_MBC_RAM_ENABLE_MASK) == GB_MBC_RAM_ENABLE_VAL;
        }
        else if (addr <= GB_MBC_ROM_BANK_END)
        {
            mem->rom_bank = (mem->rom_bank & GB_MBC_ROM_BANK_HI) | (val & GB_MBC_ROM_BANK_MASK);
            if ((mem->rom_bank & GB_MBC_ROM_BANK_MASK) == 0)
                mem->rom_bank |= 1;
            gb_mem_update_rom_ptrs(mem);
        }
        else if (addr <= GB_MBC_RAM_BANK_END)
        {
            mem->rom_bank = (mem->rom_bank & GB_MBC_ROM_BANK_MASK) | ((val & GB_MBC_RAM_BANK_MASK) << 5);
            mem->ext_ram_bank = (val & GB_MBC_RAM_BANK_MASK);
            gb_mem_update_rom_ptrs(mem);
        }
        else
        {
            mem->mbc1_mode = val & 1;
            gb_mem_update_rom_ptrs(mem);
        }
        return;
    }
    if (addr <= GB_VRAM_END)
    {
        mem->vram[addr - GB_VRAM_START] = val;
        return;
    }
    if (addr <= GB_EXT_RAM_END)
    {
        if (!mem->ext_ram_enabled)
            return;
        if (cart_is_mbc2(mem->cart_type))
        {
            uint16_t off = (addr - GB_EXT_RAM_START) & (GB_MBC2_RAM_SIZE - 1);
            mem->ext_ram[off] = val & 0x0F;
            return;
        }
        if (mem->cart_type >= 0x0F && mem->cart_type <= 0x13)
        {
            if (mem->ext_ram_bank >= GB_MBC3_RTC_S && mem->ext_ram_bank <= GB_MBC3_RTC_DH)
            {
                switch (mem->ext_ram_bank)
                {
                    case GB_MBC3_RTC_S:  mem->rtc_s = val; break;
                    case GB_MBC3_RTC_M:  mem->rtc_m = val; break;
                    case GB_MBC3_RTC_H:  mem->rtc_h = val; break;
                    case GB_MBC3_RTC_DL: mem->rtc_dl = val; break;
                    case GB_MBC3_RTC_DH: mem->rtc_dh = val; break;
                    default: break;
                }
                mem->rtc_last_sync = (uint64_t)time(NULL);  /* juego establece hora */
            }
            else if (mem->ext_ram_bank <= 3)
                mem->ext_ram[(addr - GB_EXT_RAM_START) + mem->ext_ram_bank * GB_EXT_RAM_BANK] = val;
        }
        else if (mem->cart_type >= GB_CART_MBC5 && mem->cart_type <= GB_CART_MBC5_MAX)
        {
            if (mem->ext_ram_bank <= 15)
                mem->ext_ram[(addr - GB_EXT_RAM_START) + (mem->ext_ram_bank & GB_MBC_RAM_BANK_MASK) * GB_EXT_RAM_BANK] = val;
        }
        else
        {
            uint8_t ram_bank = mem->ext_ram_bank & GB_MBC_RAM_BANK_MASK;
            if (mem->cart_type >= GB_CART_MBC1 && mem->cart_type <= 0x03 && !mem->mbc1_mode)
                ram_bank = 0;
            mem->ext_ram[(addr - GB_EXT_RAM_START) + ram_bank * GB_EXT_RAM_BANK] = val;
        }
        return;
    }
    if (addr <= GB_WRAM_END)
    {
        mem->wram[addr - GB_WRAM_START] = val;
        return;
    }
    if (addr <= GB_ECHO_END)
    {
        mem->wram[addr - GB_ECHO_START] = val;
        return;
    }
    if (addr <= GB_OAM_END)
    {
        mem->oam[addr - GB_OAM_START] = val;
        return;
    }
    if (addr <= GB_PROHIBITED_END)
        return;
    if (addr <= GB_IO_END)
    {
        uint16_t off = addr - GB_IO_START;
        if (off == GB_IO_JOYP)
            mem->io[GB_IO_JOYP] = (mem->io[GB_IO_JOYP] & GB_JOYP_READ_MASK) | (val & (GB_JOYP_SEL_DIR | GB_JOYP_SEL_BTN));
        else if (off == GB_IO_SC && val == 0x81)
        {
            mem->io[GB_IO_SC] = val;
            /* Serial transfer: SB contiene el carácter (Blargg tests) */
            if (getenv("BITEMU_SERIAL"))
                fputc(mem->io[GB_IO_SB], stderr);
        }
        else if (off == GB_IO_DIV)
        {
            mem->timer_div = 0;
        }
        else if (off == GB_IO_DMA)
        {
            mem->io[GB_IO_DMA] = val;
            /* DMA OAM: copia 160 bytes de (val<<8) a OAM */
            uint16_t src = (uint16_t)val << 8;
            for (int i = 0; i < GB_OAM_SIZE; i++)
                mem->oam[i] = gb_mem_read(mem, src + i);
        }
        else if (off == GB_IO_IF)
        {
            mem->io[GB_IO_IF] = (mem->io[GB_IO_IF] & GB_IF_RESERVED) | (val & GB_IF_IE_MASK);
        }
        else if (off == GB_IO_NR52)
        {
            if (!(val & APU_NR52_POWER))
            {
                for (int r = GB_IO_NR10; r < GB_IO_NR52; r++)
                    mem->io[r] = 0;
                mem->io[GB_IO_NR52] = 0;
            }
            else
            {
                mem->io[GB_IO_NR52] = (val & APU_NR52_POWER)
                                    | (mem->io[GB_IO_NR52] & APU_NR52_STATUS_MASK);
            }
        }
        else if (off == GB_IO_NR14 || off == GB_IO_NR24
              || off == GB_IO_NR34 || off == GB_IO_NR44)
        {
            mem->io[off] = val;
            if (val & APU_TRIGGER_BIT)
            {
                uint8_t ch_bit = 0;
                if (off == GB_IO_NR14) ch_bit = APU_NR52_CH1;
                else if (off == GB_IO_NR24) ch_bit = APU_NR52_CH2;
                else if (off == GB_IO_NR34) ch_bit = APU_NR52_CH3;
                else ch_bit = APU_NR52_CH4;
                mem->io[GB_IO_NR52] |= ch_bit;
                mem->apu_trigger_flags |= ch_bit;
            }
        }
        else
            mem->io[off] = val;
        return;
    }
    if (addr <= GB_HRAM_END)
    {
        mem->hram[addr - GB_HRAM_START] = val;
        return;
    }
    mem->ie = val;
}

/* ---------------------------------------------------------------------------
 * Acceso de 16 bits (little-endian)
 * --------------------------------------------------------------------------- */
uint16_t gb_mem_read16(gb_mem_t *mem, uint16_t addr)
{
    return (uint16_t)gb_mem_read(mem, addr) | ((uint16_t)gb_mem_read(mem, addr + 1) << 8);
}

void gb_mem_write16(gb_mem_t *mem, uint16_t addr, uint16_t val)
{
    gb_mem_write(mem, addr, (uint8_t)(val & GB_BYTE_LO));
    gb_mem_write(mem, addr + 1, (uint8_t)(val >> 8));
}

/* ---------------------------------------------------------------------------
 * Ruta ROM -> ruta .sav (misma base, extensión .sav)
 * --------------------------------------------------------------------------- */
static void rom_path_to_sav(char *out, size_t out_size, const char *rom_path)
{
    size_t n = strlen(rom_path);
    if (n >= out_size)
        return;
    memcpy(out, rom_path, n + 1);
    char *dot = strrchr(out, '.');
    if (dot)
        strcpy(dot, ".sav");
    else
        snprintf(out + n, out_size - n, ".sav");
}

#define GB_MBC3_RTC_SAV_SIZE  (5 + 8)  /* rtc_s..dh + timestamp */

void gb_mem_load_sav(gb_mem_t *mem, const char *rom_path)
{
    if (!mem || !rom_path || !cart_has_battery(mem->cart_type))
        return;
    char path[1024];
    rom_path_to_sav(path, sizeof(path), rom_path);
    FILE *f = fopen(path, "rb");
    if (!f)
        return;
    size_t read = fread(mem->ext_ram, 1, GB_EXT_RAM_SIZE, f);
    if (cart_has_rtc(mem->cart_type) && read == GB_EXT_RAM_SIZE)
    {
        uint8_t rtc_buf[5];
        uint64_t ts;
        if (fread(rtc_buf, 1, 5, f) == 5 && fread(&ts, sizeof(ts), 1, f) == 1)
        {
            mem->rtc_s = rtc_buf[0];
            mem->rtc_m = rtc_buf[1];
            mem->rtc_h = rtc_buf[2];
            mem->rtc_dl = rtc_buf[3];
            mem->rtc_dh = rtc_buf[4];
            mem->rtc_last_sync = ts;
        }
    }
    fclose(f);
    if (read > 0)
        log_info("Save loaded: %s (%zu bytes)", path, read);
}

void gb_mem_save_sav(const gb_mem_t *mem, const char *rom_path)
{
    if (!mem || !rom_path || !cart_has_battery(mem->cart_type))
        return;
    char path[1024];
    rom_path_to_sav(path, sizeof(path), rom_path);
    FILE *f = fopen(path, "wb");
    if (!f)
        return;
    fwrite(mem->ext_ram, 1, GB_EXT_RAM_SIZE, f);
    if (cart_has_rtc(mem->cart_type))
    {
        gb_mem_t *m = (gb_mem_t *)mem;
        rtc_advance(m);  /* sync before save */
        uint8_t rtc_buf[5] = { m->rtc_s, m->rtc_m, m->rtc_h, m->rtc_dl, m->rtc_dh };
        uint64_t ts = m->rtc_last_sync;
        fwrite(rtc_buf, 1, 5, f);
        fwrite(&ts, sizeof(ts), 1, f);
    }
    fclose(f);
    log_info("Save written: %s", path);
}
