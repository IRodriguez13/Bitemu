/**
 * BE2 - Sega Genesis: implementación del bus de memoria
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "memory.h"
#include "cpu_sync/cpu_sync.h"
#include "vdp/vdp.h"
#include "ym2612/ym2612.h"
#include "psg/psg.h"
#include "core/utils/log.h"
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

void genesis_mem_init(genesis_mem_t *mem)
{
    memset(mem, 0, sizeof(*mem));
    mem->sram_bytes = GEN_SRAM_SIZE;
}

void genesis_mem_apply_sram_header_ie32(genesis_mem_t *mem, const uint8_t *range_be32)
{
    mem->sram_bytes = GEN_SRAM_SIZE;
    if (!mem->sram_present || !range_be32)
        return;
    uint32_t s = ((uint32_t)range_be32[0] << 24) | ((uint32_t)range_be32[1] << 16)
               | ((uint32_t)range_be32[2] << 8) | range_be32[3];
    uint32_t e = ((uint32_t)range_be32[4] << 24) | ((uint32_t)range_be32[5] << 16)
               | ((uint32_t)range_be32[6] << 8) | range_be32[7];
    if (s == 0 && e == 0)
        return;
    if (e < s)
        return;
    uint64_t n = (uint64_t)e - (uint64_t)s + 1u;
    if (n == 0 || n > GEN_SRAM_SIZE)
        return;
    mem->sram_bytes = (uint32_t)n;
}

void genesis_mem_reset(genesis_mem_t *mem)
{
    memset(mem->ram, 0, GEN_RAM_SIZE);
    mem->joypad[0] = GEN_JOYPAD_IDLE;
    mem->joypad[1] = GEN_JOYPAD_IDLE;
    mem->joypad_ctrl[0] = mem->joypad_ctrl[1] = 0;
    mem->joypad_cycle[0] = mem->joypad_cycle[1] = 0;
    memset(mem->tmss, 0, sizeof(mem->tmss));
    mem->tmss_unlocked = 0;
    if (mem->mapper_ssf2)
    {
        for (int i = 0; i < GEN_SSF2_SLOT_COUNT; i++)
            mem->ssf2_bank[i] = (uint8_t)i;
    }
    /* sram no se borra; sram_enabled se setea en load_rom si header tiene "RA" */
}

void genesis_mem_set_vdp(genesis_mem_t *mem, struct gen_vdp *vdp)
{
    mem->vdp = vdp;
}

void genesis_mem_set_ym(genesis_mem_t *mem, struct gen_ym2612 *ym)
{
    mem->ym2612 = ym;
}

void genesis_mem_set_psg(genesis_mem_t *mem, struct gen_psg *psg)
{
    mem->psg = psg;
}

void genesis_mem_set_z80(genesis_mem_t *mem, uint8_t *z80_ram, uint8_t *bus_req, uint8_t *reset,
                         uint32_t *bus_ack_cycles)
{
    mem->z80_ram = z80_ram;
    mem->z80_bus_req = bus_req;
    mem->z80_reset = reset;
    mem->z80_bus_ack_cycles = bus_ack_cycles;
}

static uint32_t rom_addr(uint32_t addr)
{
    return addr & GEN_ROM_MASK;
}

/* Lock-on: 0x000000-0x1FFFFF = S&K, 0x200000-0x3FFFFF = locked game.
 * A130F1 bit 0 = 1: Sonic 3 & K → SRAM en 0x200000-0x20FFFF;
 *                   Sonic 2 & K → patch ROM en 0x300000-0x3FFFFF. */
static uint32_t lockon_rom_offset(genesis_mem_t *mem, uint32_t addr)
{
    if (addr < 0x200000)
        return addr;
    if (addr >= 0x300000 && mem->sram_enabled && mem->lockon_has_patch)
        return GEN_LOCKON_SIZE + ((addr - 0x300000) & (GEN_LOCKON_PATCH - 1));
    return addr;
}

/* Offset en archivo ROM para dirección de cartucho cart_addr (0…0x3FFFFF). */
static uint32_t rom_phys_offset(genesis_mem_t *mem, uint32_t cart_addr)
{
    if (mem->mapper_ssf2)
    {
        unsigned slot = (unsigned)(cart_addr >> GEN_SSF2_SLOT_SHIFT);
        uint32_t off = cart_addr & (GEN_SSF2_SLOT_SIZE - 1u);
        if (slot >= GEN_SSF2_SLOT_COUNT)
            return (uint32_t)mem->rom_size;
        uint32_t page = mem->ssf2_bank[slot];
        uint64_t phys = (uint64_t)page * GEN_SSF2_SLOT_SIZE + off;
        if (phys >= mem->rom_size)
            return (uint32_t)mem->rom_size;
        return (uint32_t)phys;
    }
    if (mem->lockon)
        return lockon_rom_offset(mem, cart_addr);
    return cart_addr;
}

static uint32_t ram_addr(uint32_t addr)
{
    return addr & GEN_RAM_MASK;
}

/* SRAM: ventana 0x200000–0x20FFFF; dirección física = offset mod sram_bytes (header). */
static uint32_t sram_offset(genesis_mem_t *mem, uint32_t addr)
{
    uint32_t off = (uint32_t)(addr - GEN_ADDR_SRAM_START);
    uint32_t sz = mem->sram_bytes ? mem->sram_bytes : GEN_SRAM_SIZE;
    return off % sz;
}

/* Protocolo 6-button: ciclo 1=TH high (D-pad,B,C), 2=TH low (D-pad,A,Start), 7=TH high (Z,Y,X,Mode).
 * Genesis: 0 = pressed (active low). raw: 1 = pressed. raw bits: 0=R,1=L,2=D,3=U,4=A,5=B,6=C,7=Start,8=X,9=Y,10=Z,11=Mode */
static uint8_t joypad_read_byte(genesis_mem_t *mem, int port, int byte_sel)
{
    (void)byte_sel;
    uint8_t ctrl = mem->joypad_ctrl[port];
    uint8_t cycle = mem->joypad_cycle[port];
    uint16_t raw = mem->joypad_raw[port];
    uint8_t out = 0xFF;
    if (cycle == 0)
        return 0x3F;

    if (cycle == 1 || cycle == 3 || cycle == 5)
    {
        if (ctrl & GEN_JOYPAD_TH)
        {
            uint8_t m = (uint8_t)(((raw>>3)&1) | ((raw>>2)&1)<<1 | ((raw>>1)&1)<<2 | ((raw>>0)&1)<<3
                       | ((raw>>5)&1)<<4 | ((raw>>6)&1)<<5);
            out = (uint8_t)(0xFF & ~m);
        }
    }
    else if (cycle == 2 || cycle == 4 || cycle == 6 || cycle == 8)
    {
        if (!(ctrl & GEN_JOYPAD_TH))
        {
            uint8_t m = (uint8_t)(((raw>>3)&1) | ((raw>>2)&1)<<1 | ((raw>>1)&1)<<2 | ((raw>>0)&1)<<3
                       | ((raw>>4)&1)<<4 | ((raw>>7)&1)<<5);
            out = (uint8_t)(0xFF & ~m);
        }
    }
    else if (cycle == 7 || cycle == 9)
    {
        if (ctrl & GEN_JOYPAD_TH)
        {
            uint8_t m = (uint8_t)(((raw>>8)&1) | ((raw>>9)&1)<<1 | ((raw>>10)&1)<<2 | ((raw>>11)&1)<<3);
            out = (uint8_t)(0x0F & ~m);
        }
    }

    return out;
}

void genesis_joypad_write_ctrl(genesis_mem_t *mem, int port, uint8_t val)
{
    uint8_t old_th = mem->joypad_ctrl[port] & GEN_JOYPAD_TH;
    uint8_t new_th = val & GEN_JOYPAD_TH;
    mem->joypad_ctrl[port] = val;
    if (old_th != new_th)
    {
        mem->joypad_cycle[port]++;
        if (mem->joypad_cycle[port] > 9)
            mem->joypad_cycle[port] = 1;
    }
}

uint8_t genesis_joypad_read_byte(genesis_mem_t *mem, int port, int byte_sel)
{
    uint8_t b = joypad_read_byte(mem, port, byte_sel);
    return b;
}

/* Open bus 68k en zona sin chip seleccionado: pull-ups ≈ 0xFF; sin prefetch/último dato en bus. */
static uint8_t genesis_open_bus_read_u8(uint32_t addr)
{
    (void)addr;
    return (uint8_t)GEN_IO_UNMAPPED_READ;
}

uint8_t genesis_mem_read8(genesis_mem_t *mem, uint32_t addr)
{
    /* SRAM: 0x200000-0x20FFFF. Con lock-on Sonic 2 & K, A130F1=1 mapea patch en 0x300000, no SRAM. */
    if (genesis_addr_in_sram(addr) && !mem->lockon_has_patch)
    {
        if (mem->sram_present && mem->sram_enabled)
            return mem->sram[sram_offset(mem, addr)];
        if (mem->sram_present && !mem->sram_enabled)
            return GEN_IO_UNMAPPED_READ;
    }
    if (genesis_addr_in_rom(addr) && mem->rom)
    {
        uint32_t off = rom_phys_offset(mem, rom_addr(addr));
        if (off < mem->rom_size)
            return mem->rom[off];
        return 0xFF;
    }
    if (genesis_addr_in_ram(addr))
        return mem->ram[ram_addr(addr)];
    if (genesis_addr_in_tmss(addr))
    {
        int ti = (int)(addr - GEN_ADDR_TMSS_START);
        if (ti >= 0 && ti < 4)
            return mem->tmss[ti];
        return GEN_IO_UNMAPPED_READ;
    }
    if (genesis_addr_in_io(addr))
    {
        if (addr == GEN_IO_JOYPAD1_DATA || addr == GEN_IO_JOYPAD1_DATA + 1)
        {
            uint8_t b = genesis_joypad_read_byte(mem, 0, (addr & 1));
            return b;
        }
        if (addr == GEN_IO_JOYPAD2_DATA || addr == GEN_IO_JOYPAD2_DATA + 1)
        {
            uint8_t b = genesis_joypad_read_byte(mem, 1, (addr & 1));
            return b;
        }
        if (addr == GEN_IO_VERSION)
            return GEN_IO_VERSION_VAL;
        return GEN_IO_UNMAPPED_READ;
    }
    if (genesis_addr_in_ym(addr) && mem->ym2612)
        return gen_ym2612_read_port(mem->ym2612, (int)(addr - GEN_ADDR_YM_START));
    if (genesis_addr_in_z80_ram(addr) && mem->z80_ram)
    {
        if (!gen_cpu_sync_m68k_may_access_z80_work_ram(mem->z80_bus_req, mem->z80_bus_ack_cycles))
            return gen_cpu_sync_z80_ram_contention_read(addr);
        return mem->z80_ram[addr & (GEN_Z80_RAM_SIZE - 1)];
    }
    if (genesis_addr_in_z80_bus(addr) && mem->z80_bus_req && mem->z80_reset)
    {
        if ((addr & GEN_ADDR_OFFSET_MASK) == GEN_Z80_BUSREQ_OFF)
            return *mem->z80_bus_req;
        if ((addr & GEN_ADDR_OFFSET_MASK) == GEN_Z80_RESET_OFF)
            return *mem->z80_reset;
    }
    if (genesis_addr_in_psg(addr))
        return GEN_IO_UNMAPPED_READ;  /* PSG es write-only */
    if (genesis_addr_in_vdp(addr) && mem->vdp)
    {
        if (addr == GEN_ADDR_VDP_HV || addr == GEN_ADDR_VDP_HV + 1)
        {
            uint16_t hv = gen_vdp_read_hv(mem->vdp);
            return (addr & 1) ? (uint8_t)(hv >> 8) : (uint8_t)(hv & 0xFF);
        }
        if (addr == GEN_ADDR_VDP_CTRL || addr == GEN_ADDR_VDP_CTRL + 1)
        {
            int fetch = (addr == GEN_ADDR_VDP_CTRL);
            return gen_vdp_read_status_byte(mem->vdp, fetch);
        }
        return GEN_IO_VERSION_VAL;
    }
    return genesis_open_bus_read_u8(addr);
}

uint16_t genesis_mem_read16(genesis_mem_t *mem, uint32_t addr)
{
    uint16_t hi = (uint16_t)genesis_mem_read8(mem, addr);
    uint16_t lo = (uint16_t)genesis_mem_read8(mem, addr + 1);
    return (hi << 8) | lo;
}

uint32_t genesis_mem_read32(genesis_mem_t *mem, uint32_t addr)
{
    uint32_t hi = (uint32_t)genesis_mem_read16(mem, addr);
    uint32_t lo = (uint32_t)genesis_mem_read16(mem, addr + 2);
    return (hi << 16) | lo;
}

void genesis_mem_write8(genesis_mem_t *mem, uint32_t addr, uint8_t val)
{
    if (mem->mapper_ssf2 && (addr & 1u) && addr >= GEN_ADDR_SSF2_SLOT0 && addr <= GEN_ADDR_SSF2_SLOT7)
    {
        int slot = (int)(addr - GEN_ADDR_SSF2_SLOT0) / 2;
        if (slot >= 0 && slot < GEN_SSF2_SLOT_COUNT && mem->rom_size > 0)
        {
            uint32_t npg = (uint32_t)((mem->rom_size + GEN_SSF2_SLOT_SIZE - 1u) / GEN_SSF2_SLOT_SIZE);
            if (npg < 1u)
                npg = 1u;
            mem->ssf2_bank[slot] = (uint8_t)(val % npg);
        }
        return;
    }
    if (addr == GEN_ADDR_SRAM_ENABLE)
    {
        mem->sram_enabled = (val & 1) ? 1 : 0;
        return;
    }
    if (genesis_addr_in_sram(addr) && !mem->lockon_has_patch)
    {
        if (mem->sram_present && mem->sram_enabled)
        {
            mem->sram[sram_offset(mem, addr)] = val;
            return;
        }
        if (mem->sram_present && !mem->sram_enabled)
            return;
    }
    if (genesis_addr_in_ram(addr))
    {
        mem->ram[ram_addr(addr)] = val;
        return;
    }
    if (genesis_addr_in_ym(addr) && mem->ym2612)
    {
        int port = (int)(addr - GEN_ADDR_YM_START);
        gen_ym2612_write_port(mem->ym2612, port, val);
        return;
    }
    if (genesis_addr_in_z80_ram(addr) && mem->z80_ram)
    {
        if (!gen_cpu_sync_m68k_may_access_z80_work_ram(mem->z80_bus_req, mem->z80_bus_ack_cycles))
            return;
        mem->z80_ram[addr & (GEN_Z80_RAM_SIZE - 1)] = val;
        return;
    }
    if (genesis_addr_in_z80_bus(addr))
    {
        if ((addr & GEN_ADDR_OFFSET_MASK) == GEN_Z80_BUSREQ_OFF && mem->z80_bus_req)
        {
            *mem->z80_bus_req = val;
            if (mem->z80_bus_ack_cycles)
            {
                if ((val & 1) != 0)
                    *mem->z80_bus_ack_cycles = GEN_Z80_BUSACK_CYCLES_68K;
                else
                    *mem->z80_bus_ack_cycles = 0;
            }
        }
        if ((addr & GEN_ADDR_OFFSET_MASK) == GEN_Z80_RESET_OFF && mem->z80_reset)
            *mem->z80_reset = val;
        return;
    }
    if (genesis_addr_in_psg(addr) && mem->psg)
    {
        gen_psg_write(mem->psg, val);
        return;
    }
    if (addr == GEN_IO_JOYPAD1_CTRL)
    {
        genesis_joypad_write_ctrl(mem, 0, val);
        return;
    }
    if (addr == GEN_IO_JOYPAD2_CTRL)
    {
        genesis_joypad_write_ctrl(mem, 1, val);
        return;
    }
    if (genesis_addr_in_tmss(addr))
    {
        int ti = (int)(addr - GEN_ADDR_TMSS_START);
        if (ti >= 0 && ti < 4)
        {
            mem->tmss[ti] = val;
            mem->tmss_unlocked = (memcmp(mem->tmss, GEN_HEADER_MAGIC, 4) == 0) ? 1 : 0;
        }
        return;
    }
    (void)addr;
    (void)val;
}

void genesis_mem_write16(genesis_mem_t *mem, uint32_t addr, uint16_t val)
{
    if (genesis_addr_in_ram(addr))
    {
        mem->ram[ram_addr(addr)] = (uint8_t)(val >> 8);
        mem->ram[ram_addr(addr) + 1] = (uint8_t)(val & 0xFF);
        return;
    }
    if (genesis_addr_in_psg(addr) && mem->psg)
    {
        gen_psg_write(mem->psg, (uint8_t)(val & 0xFF));
        return;
    }
    if (genesis_addr_in_vdp(addr) && mem->vdp)
    {
        if (addr == GEN_ADDR_VDP_DATA || addr == GEN_ADDR_VDP_DATA + 2)
            gen_vdp_write_data(mem->vdp, val);
        else if (addr == GEN_ADDR_VDP_CTRL || addr == GEN_ADDR_VDP_CTRL + 2)
            gen_vdp_write_ctrl(mem->vdp, val);
        return;
    }
    if (genesis_addr_in_ym(addr) && mem->ym2612)
    {
        int port = (int)(addr - GEN_ADDR_YM_START);
        gen_ym2612_write_port(mem->ym2612, port, (uint8_t)(val >> 8));
        gen_ym2612_write_port(mem->ym2612, port + 1, (uint8_t)(val & 0xFF));
        return;
    }
    if (genesis_addr_in_z80_ram(addr) && mem->z80_ram)
    {
        if (!gen_cpu_sync_m68k_may_access_z80_work_ram(mem->z80_bus_req, mem->z80_bus_ack_cycles))
            return;
        mem->z80_ram[addr & (GEN_Z80_RAM_SIZE - 1)] = (uint8_t)(val >> 8);
        mem->z80_ram[(addr + 1) & (GEN_Z80_RAM_SIZE - 1)] = (uint8_t)(val & 0xFF);
        return;
    }
    if (genesis_addr_in_z80_bus(addr))
    {
        if ((addr & GEN_ADDR_OFFSET_MASK) == GEN_Z80_BUSREQ_OFF && mem->z80_bus_req)
        {
            uint8_t b = (uint8_t)(val & 0xFF);
            *mem->z80_bus_req = b;
            if (mem->z80_bus_ack_cycles)
            {
                if ((b & 1) != 0)
                    *mem->z80_bus_ack_cycles = GEN_Z80_BUSACK_CYCLES_68K;
                else
                    *mem->z80_bus_ack_cycles = 0;
            }
        }
        if ((addr & GEN_ADDR_OFFSET_MASK) == GEN_Z80_RESET_OFF && mem->z80_reset)
            *mem->z80_reset = (uint8_t)(val & 0xFF);
        return;
    }
    /* TMSS: 16-bit write; no-op */
    if (genesis_addr_in_tmss(addr))
        return;
    genesis_mem_write8(mem, addr, (uint8_t)(val >> 8));
    genesis_mem_write8(mem, addr + 1, (uint8_t)(val & 0xFF));
}

void genesis_mem_write32(genesis_mem_t *mem, uint32_t addr, uint32_t val)
{
    genesis_mem_write16(mem, addr, (uint16_t)(val >> 16));
    genesis_mem_write16(mem, addr + 2, (uint16_t)(val & 0xFFFF));
}

/* Ruta ROM -> saves/base.sav (mismo esquema que GB) */
static void rom_path_to_sav(char *out, size_t out_size, const char *rom_path)
{
    const char *last_slash = strrchr(rom_path, '/');
#ifdef _WIN32
    {
        const char *bs = strrchr(rom_path, '\\');
        if (bs && (!last_slash || bs > last_slash))
            last_slash = bs;
    }
#endif
    const char *base = last_slash ? last_slash + 1 : rom_path;
    size_t dir_len = last_slash ? (size_t)(last_slash - rom_path) : 0;
    const char *dot = strrchr(base, '.');
    size_t base_len = dot ? (size_t)(dot - base) : strlen(base);
    if (dir_len > 0)
        snprintf(out, out_size, "%.*s/saves/%.*s.sav",
                 (int)dir_len, rom_path, (int)base_len, base);
    else
        snprintf(out, out_size, "saves/%.*s.sav", (int)base_len, base);
}

void genesis_mem_load_sav(genesis_mem_t *mem, const char *rom_path)
{
    if (!mem || !rom_path || !mem->sram_present)
        return;
    char path[1024];
    rom_path_to_sav(path, sizeof(path), rom_path);
    FILE *f = fopen(path, "rb");
    if (!f)
        return;
    uint32_t sz = mem->sram_bytes ? mem->sram_bytes : GEN_SRAM_SIZE;
    size_t read = fread(mem->sram, 1, sz, f);
    fclose(f);
    if (read > 0)
        log_info("Genesis save loaded: %s (%zu bytes)", path, read);
}

void genesis_mem_save_sav(const genesis_mem_t *mem, const char *rom_path)
{
    if (!mem || !rom_path || !mem->sram_present)
        return;
    char path[1024];
    rom_path_to_sav(path, sizeof(path), rom_path);
    char *slash = strrchr(path, '/');
#ifdef _WIN32
    {
        char *bs = strrchr(path, '\\');
        if (bs && (!slash || bs > slash))
            slash = bs;
    }
#endif
    if (slash)
    {
        *slash = '\0';
        mkdir(path, 0755);
        *slash = '/';
    }
    FILE *f = fopen(path, "wb");
    if (!f)
        return;
    uint32_t sz = mem->sram_bytes ? mem->sram_bytes : GEN_SRAM_SIZE;
    fwrite(mem->sram, 1, sz, f);
    fclose(f);
    log_info("Genesis save written: %s", path);
}
