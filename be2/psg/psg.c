/**
 * BE2 - SN76489 PSG
 * Acepta escrituras; síntesis de audio pendiente.
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "psg.h"
#include <string.h>

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
    psg->cycle_counter = 0;
}

void gen_psg_write(gen_psg_t *psg, uint8_t val)
{
    if (val & 0x80)
    {
        /* Latch: bits 5-4 = register, bit 3 = data type */
        psg->latch = (val >> 4) & 0x07;
        int r = psg->latch & 3;
        if (psg->latch & 4)
        {
            /* Volume register */
            psg->vol[r] = val & 0x0F;
        }
        else
        {
            /* Tone register: low 4 bits */
            if (r < 3)
            {
                psg->tone[r] = (psg->tone[r] & 0x3F0) | (val & 0x0F);
            }
            else
            {
                psg->noise_type = val & 0x07;
            }
        }
    }
    else
    {
        /* Data byte: high 6 bits of tone, or second byte of noise */
        int r = psg->latch & 3;
        if (psg->latch & 4)
        {
            /* Volume already set by latch */
            (void)r;
        }
        else if (r < 3)
        {
            psg->tone[r] = (psg->tone[r] & 0x00F) | ((val & 0x3F) << 4);
            if (psg->tone[r] == 0)
                psg->tone[r] = 0x400;  /* División por 0 = 0x400 según datasheet */
        }
    }
}

void gen_psg_step(gen_psg_t *psg, int cycles, void *audio_output)
{
    (void)audio_output;
    psg->cycle_counter += (uint32_t)cycles;
    /* TODO: generar muestras cuando cycle_counter >= ciclos_por_muestra */
}
