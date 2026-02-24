/**
 * Bitemu - Game Boy Memory
 * Mapa de memoria y bancos
 */

#include "memory.h"
#include "gb_constants.h"
#include "core/utils/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int cart_has_battery(uint8_t cart_type)
{
    return cart_type == 0x03 || (cart_type >= 0x0F && cart_type <= 0x13);
}

void gb_mem_init(gb_mem_t *mem)
{
    memset(mem, 0, sizeof(gb_mem_t));
}

void gb_mem_reset(gb_mem_t *mem)
{
    mem->rom_bank = 1;
    mem->ext_ram_bank = 0;
    mem->ext_ram_enabled = 0;
    /* cart_type set by load_rom from header 0x147 */
    mem->ie = 0;
    mem->joypad_state = 0xFF;
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

uint8_t gb_mem_read(gb_mem_t *mem, uint16_t addr)
{
    if (addr <= GB_ROM0_END)
    {
        if (mem->rom && addr < mem->rom_size)
            return mem->rom[addr];
        return GB_READ_UNMAPPED;
    }
    if (addr <= GB_ROM1_END)
    {
        uint8_t bank;
        if (mem->cart_type == GB_CART_ROM_ONLY)
            bank = 0;
        else if (mem->cart_type >= 0x0F && mem->cart_type <= 0x13)
            bank = (mem->rom_bank & 0x7F) ? (mem->rom_bank & 0x7F) : 1;
        else
            bank = (mem->rom_bank & GB_MBC_ROM_BANK_MASK)
                       ? (mem->rom_bank & GB_MBC_ROM_BANK_MASK)
                       : 1;
        uint32_t phys = (uint32_t)bank * GB_ROM_BANK_SIZE + (addr - GB_ROM1_START);
        if (mem->rom && phys < mem->rom_size)
            return mem->rom[phys];
        return GB_READ_UNMAPPED;
    }
    if (addr <= GB_VRAM_END)
        return mem->vram[addr - GB_VRAM_START];
    if (addr <= GB_EXT_RAM_END)
    {
        if (!mem->ext_ram_enabled)
            return GB_READ_UNMAPPED;
        if (mem->cart_type >= 0x0F && mem->cart_type <= 0x13 && mem->ext_ram_bank > 3)
            return 0xFF; /* RTC read (stubbed) */
        return mem->ext_ram[(addr - GB_EXT_RAM_START) + (mem->ext_ram_bank & GB_MBC_RAM_BANK_MASK) * GB_EXT_RAM_BANK];
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
            uint8_t sel = mem->io[GB_IO_JOYP] & 0x30;
            uint8_t bits = 0x0F;
            if (!(sel & (1 << GB_JOYP_DIR_BIT)))
                bits &= ~(mem->joypad_state & 0x0F);
            else if (!(sel & (1 << GB_JOYP_BTN_BIT)))
                bits &= ~((mem->joypad_state >> 4) & 0x0F);
            return sel | bits;
        }
        if (off == GB_IO_IF)
            return (mem->io[GB_IO_IF] & 0x1F) | 0xE0;
        return mem->io[off];
    }
    if (addr <= GB_HRAM_END)
        return mem->hram[addr - GB_HRAM_START];
    return mem->ie;
}

void gb_mem_write(gb_mem_t *mem, uint16_t addr, uint8_t val)
{
    if (addr <= GB_ROM1_END)
    {
        if (mem->cart_type == GB_CART_ROM_ONLY)
            return;
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
            }
            else if (addr <= GB_MBC_RAM_BANK_END)
                mem->ext_ram_bank = val & 0x0F; /* 0-3 RAM, 4-0x0C RTC (stubbed) */
            /* 0x6000-0x7FFF: RTC latch, ignore */
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
            if (mem->rom_bank == 0)
                mem->rom_bank = 1;
        }
        else if (addr <= GB_MBC_RAM_BANK_END)
        {
            mem->rom_bank = (mem->rom_bank & GB_MBC_ROM_BANK_MASK) | ((val & GB_MBC_RAM_BANK_MASK) << 5);
            mem->ext_ram_bank = (val & GB_MBC_RAM_BANK_MASK);
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
        if (mem->ext_ram_enabled && (!(mem->cart_type >= 0x0F && mem->cart_type <= 0x13) || mem->ext_ram_bank <= 3))
            mem->ext_ram[(addr - GB_EXT_RAM_START) + (mem->ext_ram_bank & GB_MBC_RAM_BANK_MASK) * GB_EXT_RAM_BANK] = val;
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
            mem->io[GB_IO_JOYP] = (mem->io[GB_IO_JOYP] & 0x0F) | (val & 0x30);
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
            mem->io[GB_IO_IF] = (mem->io[GB_IO_IF] & 0xE0) | (val & 0x1F);
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

uint16_t gb_mem_read16(gb_mem_t *mem, uint16_t addr)
{
    return (uint16_t)gb_mem_read(mem, addr) | ((uint16_t)gb_mem_read(mem, addr + 1) << 8);
}

void gb_mem_write16(gb_mem_t *mem, uint16_t addr, uint16_t val)
{
    gb_mem_write(mem, addr, (uint8_t)(val & 0xFF));
    gb_mem_write(mem, addr + 1, (uint8_t)(val >> 8));
}

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
    fclose(f);
    log_info("Save written: %s", path);
}
