/**
 * BE1 - Game Boy: estructura interna del impl
 * Usado por main.c para alocar el estado.
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef BITEMU_GB_IMPL_H
#define BITEMU_GB_IMPL_H

#include "cpu/cpu.h"
#include "ppu.h"
#include "apu.h"
#include "memory.h"

#define GB_ROM_PATH_MAX 1024

typedef struct {
    gb_mem_t mem;
    gb_cpu_t cpu;
    gb_ppu_t ppu;
    gb_apu_t apu;
    char rom_path[GB_ROM_PATH_MAX]; /* for battery save .sav path */
    void *audio_output; /* bitemu_audio_t* cuando audio está inicializado; NULL si no */
} gb_impl_t;

#endif /* BITEMU_GB_IMPL_H */
