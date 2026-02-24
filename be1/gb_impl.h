/**
 * BE1 - Game Boy: estructura interna del impl
 * Usado por main.c para alocar el estado.
 */

#ifndef BITEMU_GB_IMPL_H
#define BITEMU_GB_IMPL_H

#include "cpu.h"
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
} gb_impl_t;

#endif /* BITEMU_GB_IMPL_H */
