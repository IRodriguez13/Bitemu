/**
 * BE2 - Sega Genesis: estructura interna del impl
 *
 * Agrupa memoria, CPU, VDP. Sin magic numbers.
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef BITEMU_GENESIS_IMPL_H
#define BITEMU_GENESIS_IMPL_H

#include "genesis_constants.h"
#include "memory.h"
#include "cpu/cpu.h"
#include "vdp/vdp.h"
#include "ym2612/ym2612.h"
#include "psg/psg.h"
#include "z80/z80.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
    genesis_mem_t mem;
    gen_cpu_t cpu;
    gen_vdp_t vdp;
    gen_ym2612_t ym2612;
    gen_psg_t psg;
    gen_z80_t z80;
    uint8_t z80_ram[8192];  /* Z80 RAM 8KB */
    uint8_t z80_bus_req;    /* A11100: ver be2/cpu_sync y Documentation/08-cpu-sync-genesis.md */
    uint32_t z80_bus_ack_cycles; /* >0: 68k aún no puede usar RAM Z80 de forma segura (modelo BUSACK). */
    uint8_t z80_reset;      /* A11200: 0=reset Z80 activo, !=0 (p. ej. 0xFF) = Z80 corriendo */
    uint64_t stat_cpu_cyc;       /* acumulado en gen_step (herramientas) */
    uint64_t stat_z80_cyc;       /* ciclos Z80 ejecutados en gen_step */
    uint64_t stat_dma_stall_consumed; /* ciclos 68k consumidos en stall DMA */
    uint16_t joypad_state;  /* bits 0-11: mando 1; 1=presionado */
    uint16_t joypad2_state; /* bits 0-11: mando 2 (misma codificación) */
    uint8_t is_pal;         /* 1 = timing PAL (~50 Hz) desde región cartucho @ 0x1F0 */
    char rom_path[GEN_ROM_PATH_MAX];
    void *audio_output;
    int sample_clock;       /* ciclos 68k acumulados para sample rate */
} genesis_impl_t;

#endif /* BITEMU_GENESIS_IMPL_H */
