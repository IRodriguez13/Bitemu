/**
 * BE2 - Sega Genesis: implementación del bus de memoria
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "memory.h"
#include "vdp/vdp.h"
#include "ym2612/ym2612.h"
#include "psg/psg.h"
#include <string.h>

void genesis_mem_init(genesis_mem_t *mem)
{
    memset(mem, 0, sizeof(*mem));
}

void genesis_mem_reset(genesis_mem_t *mem)
{
    memset(mem->ram, 0, GEN_RAM_SIZE);
    mem->joypad[0] = 0x3F;
    mem->joypad[1] = 0x3F;
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

void genesis_mem_set_z80(genesis_mem_t *mem, uint8_t *z80_ram, uint8_t *bus_req, uint8_t *reset)
{
    mem->z80_ram = z80_ram;
    mem->z80_bus_req = bus_req;
    mem->z80_reset = reset;
}

static uint32_t rom_addr(uint32_t addr)
{
    return addr & GEN_ROM_MASK;
}

static uint32_t ram_addr(uint32_t addr)
{
    return addr & GEN_RAM_MASK;
}

uint8_t genesis_mem_read8(genesis_mem_t *mem, uint32_t addr)
{
    if (genesis_addr_in_rom(addr) && mem->rom)
    {
        uint32_t off = rom_addr(addr);
        if (off < mem->rom_size)
            return mem->rom[off];
        return 0xFF;
    }
    if (genesis_addr_in_ram(addr))
        return mem->ram[ram_addr(addr)];
    if (genesis_addr_in_io(addr))
    {
        if (addr == GEN_IO_JOYPAD1)
            return (uint8_t)(mem->joypad[0] & 0xFF);
        if (addr == GEN_IO_JOYPAD1 + 1)
            return (uint8_t)(mem->joypad[0] >> 8);
        if (addr == GEN_IO_JOYPAD2)
            return (uint8_t)(mem->joypad[1] & 0xFF);
        if (addr == GEN_IO_VERSION)
            return 0x00;  /* Version register */
        return 0xFF;
    }
    if (genesis_addr_in_ym(addr) && mem->ym2612)
        return 0x00;
    if (genesis_addr_in_z80_ram(addr) && mem->z80_ram)
        return mem->z80_ram[addr & (GEN_Z80_RAM_SIZE - 1)];
    if (genesis_addr_in_z80_bus(addr) && mem->z80_bus_req && mem->z80_reset)
    {
        if ((addr & 0xFFFF) == 0x1100)
            return *mem->z80_bus_req;
        if ((addr & 0xFFFF) == 0x1200)
            return *mem->z80_reset;
    }
    if (genesis_addr_in_psg(addr))
        return 0xFF;  /* PSG es write-only */
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
        return 0x00;
    }
    return 0xFF;
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
        mem->z80_ram[addr & (GEN_Z80_RAM_SIZE - 1)] = val;
        return;
    }
    if (genesis_addr_in_z80_bus(addr))
    {
        if ((addr & 0xFFFF) == 0x1100 && mem->z80_bus_req)
            *mem->z80_bus_req = val;
        if ((addr & 0xFFFF) == 0x1200 && mem->z80_reset)
            *mem->z80_reset = val;
        return;
    }
    if (genesis_addr_in_psg(addr) && mem->psg)
    {
        gen_psg_write(mem->psg, val);
        return;
    }
    /* TMSS (0xA14000): no-op en emulación */
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
        mem->z80_ram[addr & (GEN_Z80_RAM_SIZE - 1)] = (uint8_t)(val >> 8);
        mem->z80_ram[(addr + 1) & (GEN_Z80_RAM_SIZE - 1)] = (uint8_t)(val & 0xFF);
        return;
    }
    if (genesis_addr_in_z80_bus(addr))
    {
        if ((addr & 0xFFFF) == 0x1100 && mem->z80_bus_req)
            *mem->z80_bus_req = (uint8_t)(val & 0xFF);
        if ((addr & 0xFFFF) == 0x1200 && mem->z80_reset)
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
