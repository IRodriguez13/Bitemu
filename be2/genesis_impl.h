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
#include <stddef.h>
#include <stdint.h>

typedef struct {
    genesis_mem_t mem;
    gen_cpu_t cpu;
    gen_vdp_t vdp;
    gen_ym2612_t ym2612;
    gen_psg_t psg;
    uint8_t z80_ram[8192];  /* Z80 RAM 8KB */
    uint8_t z80_bus_req;    /* 0=68k tiene bus, 1=Z80 tiene bus */
    uint8_t z80_reset;      /* 0=Z80 en reset, 1=running */
    uint8_t joypad_state;   /* 1=presionado, mapeado desde bitemu_set_input */
    char rom_path[GEN_ROM_PATH_MAX];
    void *audio_output;
} genesis_impl_t;

#endif /* BITEMU_GENESIS_IMPL_H */
