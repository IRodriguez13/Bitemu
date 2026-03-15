/**
 * BE2 - Sega Genesis: mapa de memoria y bus 68K
 *
 * Encapsula lecturas/escrituras según rangos de dirección.
 * Sin magic numbers: usa genesis_constants.h
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef BITEMU_GENESIS_MEMORY_H
#define BITEMU_GENESIS_MEMORY_H

#include "genesis_constants.h"
#include <stddef.h>
#include <stdint.h>

struct gen_vdp;
struct gen_ym2612;
struct gen_psg;
typedef struct genesis_mem genesis_mem_t;

struct genesis_mem {
    uint8_t *rom;
    size_t rom_size;
    uint8_t ram[GEN_RAM_SIZE];
    struct gen_vdp *vdp;
    struct gen_ym2612 *ym2612;
    struct gen_psg *psg;
    uint8_t *z80_ram;       /* 8KB, NULL si no init */
    uint8_t *z80_bus_req;   /* 0=68k tiene bus */
    uint8_t *z80_reset;
    uint16_t joypad[2];     /* puertos 1 y 2 */
};

void genesis_mem_init(genesis_mem_t *mem);
void genesis_mem_reset(genesis_mem_t *mem);
void genesis_mem_set_vdp(genesis_mem_t *mem, struct gen_vdp *vdp);
void genesis_mem_set_ym(genesis_mem_t *mem, struct gen_ym2612 *ym);
void genesis_mem_set_psg(genesis_mem_t *mem, struct gen_psg *psg);
void genesis_mem_set_z80(genesis_mem_t *mem, uint8_t *z80_ram, uint8_t *bus_req, uint8_t *reset);

/* Lectura/escritura byte, word, long (big-endian). Retornan valor leído o 0. */
uint8_t genesis_mem_read8(genesis_mem_t *mem, uint32_t addr);
uint16_t genesis_mem_read16(genesis_mem_t *mem, uint32_t addr);
uint32_t genesis_mem_read32(genesis_mem_t *mem, uint32_t addr);
void genesis_mem_write8(genesis_mem_t *mem, uint32_t addr, uint8_t val);
void genesis_mem_write16(genesis_mem_t *mem, uint32_t addr, uint16_t val);
void genesis_mem_write32(genesis_mem_t *mem, uint32_t addr, uint32_t val);

/* Helpers: ¿la dirección cae en ROM/RAM/IO? */
static inline int genesis_addr_in_rom(uint32_t addr)
{
    return addr <= GEN_ADDR_ROM_END;  /* ROM: 0x000000-0x3FFFFF */
}
static inline int genesis_addr_in_ram(uint32_t addr)
{
    return addr >= GEN_ADDR_RAM_START && addr <= GEN_ADDR_RAM_END;
}
static inline int genesis_addr_in_io(uint32_t addr)
{
    return addr >= GEN_ADDR_IO_START && addr <= GEN_ADDR_IO_END;
}
static inline int genesis_addr_in_vdp(uint32_t addr)
{
    return addr >= GEN_ADDR_VDP_DATA && addr <= GEN_ADDR_VDP_HV + 1;
}
static inline int genesis_addr_in_psg(uint32_t addr)
{
    return addr == GEN_ADDR_PSG;
}
static inline int genesis_addr_in_tmss(uint32_t addr)
{
    return addr >= GEN_ADDR_TMSS_START && addr <= GEN_ADDR_TMSS_END;
}
static inline int genesis_addr_in_ym(uint32_t addr)
{
    return addr >= GEN_ADDR_YM_START && addr <= GEN_ADDR_YM_END;
}
static inline int genesis_addr_in_z80_ram(uint32_t addr)
{
    return addr >= GEN_ADDR_Z80_START && addr < GEN_ADDR_Z80_START + GEN_Z80_RAM_SIZE;
}
static inline int genesis_addr_in_z80_bus(uint32_t addr)
{
    return (addr & 0x00FFFFFE) == GEN_ADDR_Z80_BUSREQ || (addr & 0x00FFFFFE) == GEN_ADDR_Z80_RESET;
}

#endif /* BITEMU_GENESIS_MEMORY_H */
