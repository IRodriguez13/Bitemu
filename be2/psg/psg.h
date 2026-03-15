/**
 * BE2 - SN76489 PSG (Genesis)
 * 4 canales: 3 tono (square) + 1 ruido.
 * Stub: acepta escrituras; síntesis pendiente.
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef BITEMU_GENESIS_PSG_H
#define BITEMU_GENESIS_PSG_H

#include <stdint.h>

typedef struct gen_psg gen_psg_t;

struct gen_psg {
    uint8_t regs[8];      /* Latched register data */
    uint8_t latch;        /* Next write goes to this reg (bits 5-4 = reg, bit 3 = type) */
    uint16_t tone[3];     /* Frecuencias canales 0-2 (10 bits) */
    uint8_t vol[4];       /* Volumen 0-15 (canales 0-3) */
    uint8_t noise_type;   /* Tipo de ruido (channel 3): bits 0-1=shift, bit 2=white(1)/periodic(0) */
    uint16_t noise_lfsr;  /* LFSR 16-bit (Genesis: bits 0,3 tapped para white) */
    uint16_t tone_counter[3];  /* Contadores para tono */
    uint8_t tone_out[3];  /* Output bit 0/1 */
    uint16_t noise_counter;   /* Contador para ruido */
    uint8_t noise_out;     /* Output bit del LFSR */
};

void gen_psg_init(gen_psg_t *psg);
void gen_psg_reset(gen_psg_t *psg);
void gen_psg_write(gen_psg_t *psg, uint8_t val);
void gen_psg_step(gen_psg_t *psg, int cycles, void *audio_output);

/* Ejecuta N ciclos PSG y devuelve muestra mono (0-15*4 escala, mezclada) */
int gen_psg_run_cycles(gen_psg_t *psg, int cycles);

#endif /* BITEMU_GENESIS_PSG_H */
