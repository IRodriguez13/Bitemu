/**
 * BE2 - Sega Genesis: CPU 68000
 *
 * Registros y step (68000: fetch/decode/ejecutar; IRQ desde VDP).
 * Sin magic numbers: usa genesis_constants.h
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef BITEMU_GENESIS_CPU_H
#define BITEMU_GENESIS_CPU_H

#include "../genesis_constants.h"
#include "../memory.h"
#include <stdint.h>

typedef struct gen_cpu gen_cpu_t;

struct gen_cpu {
    uint32_t d[8];   /* D0-D7 data */
    uint32_t a[8];   /* A0-A7 address, A7=SP */
    uint32_t pc;
    uint32_t inst_pc; /* Dirección de la 1ª palabra de la instrucción en curso (excepciones). */
    uint16_t sr;     /* status register */
    int cycles;      /* ciclos consumidos en último step */
    int cycle_override; /* Tras Address Error dentro de EA: ciclos finales del step (sustituye ret del handler). */
    unsigned stopped : 1;  /* STOP: CPU halted until interrupt */
};

void gen_cpu_init(gen_cpu_t *cpu);
void gen_cpu_reset(gen_cpu_t *cpu, genesis_mem_t *mem);

/* Ejecuta hasta consumir 'max_cycles'. Retorna ciclos consumidos (0 si stopped). */
int gen_cpu_step(gen_cpu_t *cpu, genesis_mem_t *mem, int max_cycles);

/* 1 si STOP ejecutado; se limpia en reset o al tomar interrupción. */
int gen_cpu_stopped(const gen_cpu_t *cpu);

#endif /* BITEMU_GENESIS_CPU_H */
