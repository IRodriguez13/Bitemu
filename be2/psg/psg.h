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
    uint8_t noise_type;   /* Tipo de ruido (channel 3) */
    uint16_t noise_lfsr;  /* LFSR para ruido */
    uint32_t cycle_counter;
};

void gen_psg_init(gen_psg_t *psg);
void gen_psg_reset(gen_psg_t *psg);
void gen_psg_write(gen_psg_t *psg, uint8_t val);
void gen_psg_step(gen_psg_t *psg, int cycles, void *audio_output);

#endif /* BITEMU_GENESIS_PSG_H */
