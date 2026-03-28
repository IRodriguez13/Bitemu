/**
 * BE2 - YM2612 (chip de audio FM Genesis)
 *
 * FM operadores, algoritmos y envolventes (modelo en evolución).
 * Acceso: 0xA04000-0xA04001 (parte 0), 0xA04002-0xA04003 (parte 1).
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef BITEMU_YM2612_H
#define BITEMU_YM2612_H

#include "../genesis_constants.h"
#include <stdint.h>

typedef struct gen_ym2612 gen_ym2612_t;

struct gen_ym2612 {
    uint8_t regs[2][256];   /* parte 0 y 1, 256 registros cada una */
    uint8_t addr[2];        /* registro seleccionado por puerto */
    int cycle_counter;      /* para step/sample timing */
    uint32_t phase[6][4];   /* contador fase 20-bit por canal/operador */
    uint8_t key[6][4];     /* key on por canal/operador */
    uint16_t fnum[6];      /* F-number latched (11-bit) */
    uint8_t block[6];      /* Block latched (3-bit) */
    uint8_t env_state[6][4];   /* ADSR state por canal/operador */
    uint16_t env_level[6][4];   /* 0-1023, nivel de envolvente */
    int busy_cycles;            /* >0: bit busy (aprox. tras escritura de datos) */
    uint16_t timer_a_counter;
    uint16_t timer_b_counter;
    uint32_t timer_tick_acc;   /* acumulador ~ciclos 68k hasta tick de timer FM aproximado */
    uint8_t timer_overflow;    /* bits 0-1: desbordes A/B antes de leer status */
};

void gen_ym2612_init(gen_ym2612_t *ym);
void gen_ym2612_reset(gen_ym2612_t *ym);

/* port: 0=A04000, 1=A04001, 2=A04002, 3=A04003. val: byte a escribir */
void gen_ym2612_write_port(gen_ym2612_t *ym, int port, uint8_t val);

/* Lectura status: bit7 busy mientras busy_cycles > 0 tras escritura de datos. */
uint8_t gen_ym2612_read_port(gen_ym2612_t *ym, int port);

/* Avanza ciclos; genera muestras si audio != NULL */
void gen_ym2612_step(gen_ym2612_t *ym, int cycles, void *audio_output);

/* Ejecuta N ciclos YM y escribe muestra estéreo en left/right (escala int16) */
void gen_ym2612_run_cycles(gen_ym2612_t *ym, int cycles, int16_t *left, int16_t *right);

#endif /* BITEMU_YM2612_H */
