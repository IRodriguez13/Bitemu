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
    uint8_t sram[GEN_SRAM_SIZE];  /* Backup RAM (batería) */
    uint32_t sram_bytes;          /* tamaño útil (header 0x1B4-0x1BB o GEN_SRAM_SIZE) */
    uint8_t sram_enabled;         /* 1 = SRAM mapeada en 0x200000 (A130F1 bit 0) */
    uint8_t sram_present;         /* 1 = header "RA", para load/save .sav */
    uint8_t lockon;               /* 1 = ROM lock-on (S&K + locked game) */
    uint8_t lockon_has_patch;     /* 1 = Sonic 2 & K (256KB patch en 0x300000-0x3FFFFF) */
    struct gen_vdp *vdp;
    struct gen_ym2612 *ym2612;
    struct gen_psg *psg;
    uint8_t *z80_ram;       /* 8KB, NULL si no init */
    uint8_t *z80_bus_req;   /* ref. cpu_sync: acceso 68k a RAM Z80 si *!=0 */
    uint32_t *z80_bus_ack_cycles; /* ref.: >0 bloquea acceso (BUSACK modelado) */
    uint8_t *z80_reset;
    uint16_t joypad[2];     /* puertos 1 y 2 (3-button compat) */
    uint16_t joypad_raw[2]; /* estado crudo: bits 0-11 = R,L,D,U,Start,A,B,C,X,Y,Z,Mode; 1=presionado */
    uint8_t joypad_ctrl[2]; /* TH/TR último escrito (0xA10003, 0xA10005) */
    uint8_t joypad_cycle[2];/* ciclo 0-8 para protocolo 6-button */
    uint8_t tmss[4];        /* 0xA14000–03: últimos bytes escritos (TMSS Model 1) */
    uint8_t tmss_unlocked; /* 1 si tmss[0..3] == SEGA (mismo orden que header cartucho) */
    uint8_t cart_requires_tmss; /* 1 si header 0x100 == "SEGA" (licencia TMSS consola model 1) */
    uint8_t ssf2_bank[GEN_SSF2_SLOT_COUNT]; /* páginas 512KB por slot (mapper SSF2) */
    uint8_t mapper_ssf2;   /* 1 si ROM > 4MB: ventana 0–0x3FFFFF bancada */
    uint8_t bus_read_latch; /* último byte leído en bus útil (open bus aprox.) */
};

void genesis_joypad_write_ctrl(genesis_mem_t *mem, int port, uint8_t val);
uint8_t genesis_joypad_read_byte(genesis_mem_t *mem, int port, int byte_sel);
void genesis_mem_init(genesis_mem_t *mem);
void genesis_mem_reset(genesis_mem_t *mem);

/* range_be32: 8 bytes (start BE32, end BE32 en espacio cartucho). NULL o !sram_present → sram_bytes=64K. */
void genesis_mem_apply_sram_header_ie32(genesis_mem_t *mem, const uint8_t *range_be32);

/* Battery save: load/save SRAM to .sav. path = ROM path. */
void genesis_mem_load_sav(genesis_mem_t *mem, const char *rom_path);
void genesis_mem_save_sav(const genesis_mem_t *mem, const char *rom_path);
void genesis_mem_set_vdp(genesis_mem_t *mem, struct gen_vdp *vdp);
void genesis_mem_set_ym(genesis_mem_t *mem, struct gen_ym2612 *ym);
void genesis_mem_set_psg(genesis_mem_t *mem, struct gen_psg *psg);
void genesis_mem_set_z80(genesis_mem_t *mem, uint8_t *z80_ram, uint8_t *bus_req, uint8_t *reset,
                         uint32_t *bus_ack_cycles);

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
static inline int genesis_addr_in_sram(uint32_t addr)
{
    return addr >= GEN_ADDR_SRAM_START && addr <= GEN_ADDR_SRAM_END;
}

#endif /* BITEMU_GENESIS_MEMORY_H */
