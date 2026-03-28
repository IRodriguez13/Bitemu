/**
 * BE2 - SN76489 PSG
 * 3 canales de tono (square) + 1 ruido. Síntesis completa.
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#define PSG_NOISE_TAP_MASK 0x0009u  /* Genesis: bits LFSR 0 y 3 */

#include "psg.h"
#include "../genesis_constants.h"
#include <string.h>

/* Tabla de volumen: 2dB por paso, aprox lineal en amplitud */
static const uint16_t psg_vol_table[16] = {
    32767, 26028, 20675, 16422, 13045, 10363, 8232, 6541,
    5197, 4129, 3281, 2607, 2072, 1646, 1308, 0
};

static inline int parity16(uint16_t x)
{
    x ^= x >> 8;
    x ^= x >> 4;
    x ^= x >> 2;
    x ^= x >> 1;
    return (int)(x & 1);
}

void gen_psg_init(gen_psg_t *psg)
{
    memset(psg, 0, sizeof(*psg));
}

void gen_psg_reset(gen_psg_t *psg)
{
    memset(psg->regs, 0, sizeof(psg->regs));
    psg->latch = 0;
    psg->tone[0] = psg->tone[1] = psg->tone[2] = 0x3FF;
    psg->vol[0] = psg->vol[1] = psg->vol[2] = psg->vol[3] = 0x0F;
    psg->noise_type = 0;
    psg->noise_lfsr = 0x8000;
    psg->tone_counter[0] = psg->tone_counter[1] = psg->tone_counter[2] = 0;
    psg->tone_out[0] = psg->tone_out[1] = psg->tone_out[2] = 1;
    psg->noise_counter = 0x10;
    psg->noise_out = 0;
}

void gen_psg_write(gen_psg_t *psg, uint8_t val)
{
    if (val & 0x80)
    {
        psg->latch = (val >> 4) & 0x07;
        int r = psg->latch & 3;
        if (psg->latch & 4)
        {
            psg->vol[r] = val & 0x0F;
            return;
        }
        if (r < 3)
        {
            psg->tone[r] = (psg->tone[r] & 0x3F0) | (val & 0x0F);
            return;
        }
        psg->noise_type = val & 0x07;
        psg->noise_lfsr = 0x8000;  /* Reset LFSR al escribir noise */
        return;
    }

    int r = psg->latch & 3;
    if (!(psg->latch & 4) && r < 3)
    {
        psg->tone[r] = (psg->tone[r] & 0x00F) | ((val & 0x3F) << 4);
        if (psg->tone[r] == 0)
            psg->tone[r] = 0x400;
        return;
    }
    if (r != 3)
        return;
    psg->noise_type = (psg->noise_type & 4) | (val & 0x07);
    psg->noise_lfsr = 0x8000;
}

void gen_psg_step(gen_psg_t *psg, int cycles, void *audio_output)
{
    (void)psg;
    (void)cycles;
    (void)audio_output;
    /* La salida se genera en gen_psg_run_cycles cuando la consola emite muestra */
}

/* Obtiene periodo del contador de ruido (bits 0-1 del noise_type) */
static uint16_t noise_counter_reset(gen_psg_t *psg)
{
    switch (psg->noise_type & 3)
    {
        case 0: return 0x10;
        case 1: return 0x20;
        case 2: return 0x40;
        default: return psg->tone[2] ? psg->tone[2] : 0x400;  /* usa Tone2 */
    }
}

int gen_psg_run_cycles(gen_psg_t *psg, int cycles)
{
    int sum = 0;
    const int n = cycles;
    uint16_t period[3];
    for (int c = 0; c < 3; c++)
        period[c] = (psg->tone[c] == 0 || psg->tone[c] == 1) ? 1 : psg->tone[c];

    uint16_t noise_per = noise_counter_reset(psg);
    int white = (psg->noise_type >> 2) & 1;

    for (int i = 0; i < n; i++)
    {
        int sample = 0;

        /* Canales de tono */
        for (int c = 0; c < 3; c++)
        {
            if (psg->vol[c] >= 15)
                continue;
            if (psg->tone[c] == 0 || psg->tone[c] == 1)
            {
                sample += psg_vol_table[psg->vol[c]] >> 4;
                continue;
            }
            if (--psg->tone_counter[c] == 0)
            {
                psg->tone_counter[c] = period[c];
                psg->tone_out[c] ^= 1;
            }
            if (psg->tone_out[c])
                sample += psg_vol_table[psg->vol[c]];
        }

        /* Canal de ruido */
        if (psg->vol[3] < 15)
        {
            if (--psg->noise_counter == 0)
            {
                psg->noise_counter = noise_per;
                if (white)
                {
                    uint16_t fb = parity16(psg->noise_lfsr & PSG_NOISE_TAP_MASK);
                    psg->noise_lfsr = (psg->noise_lfsr >> 1) | (fb << 15);
                }
                else
                {
                    psg->noise_lfsr = (psg->noise_lfsr >> 1) | ((psg->noise_lfsr & 1) << 15);
                }
                psg->noise_out = psg->noise_lfsr & 1;
            }
            if (psg->noise_out)
                sample += psg_vol_table[psg->vol[3]];
        }

        sum += sample;
    }

    /* Escala 0-32767: 4 canales máx, promediamos y dividimos por 4 */
    if (n > 0)
        return (sum / n) / 4;
    return 0;
}
