/**
 * BE2 - YM2612 stub
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "ym2612.h"
#include <string.h>

/* Ciclos YM2612 @ 7.67MHz / 144 = ~53kHz (clock interno) */
enum { GEN_YM_CYCLES_PER_SAMPLE = 144 };

void gen_ym2612_init(gen_ym2612_t *ym)
{
    memset(ym, 0, sizeof(*ym));
}

void gen_ym2612_reset(gen_ym2612_t *ym)
{
    memset(ym->regs, 0, sizeof(ym->regs));
    ym->addr[0] = 0;
    ym->addr[1] = 0;
    ym->cycle_counter = 0;
}

void gen_ym2612_write_port(gen_ym2612_t *ym, int port, uint8_t val)
{
    int part = (port >> 1) & 1;  /* 0,1 -> 0; 2,3 -> 1 */
    int is_addr = (port & 1) == 0;
    if (is_addr)
        ym->addr[part] = val;
    else
    {
        uint8_t r = ym->addr[part];
        if (r < 0x30)
            ym->regs[0][r] = val;
        else
            ym->regs[part][r] = val;
    }
}

void gen_ym2612_step(gen_ym2612_t *ym, int cycles, void *audio_output)
{
    (void)audio_output;
    ym->cycle_counter += cycles;
    /* TODO: generar muestras cuando cycle_counter >= GEN_YM_CYCLES_PER_SAMPLE */
}

void gen_ym2612_sample(gen_ym2612_t *ym, int16_t *left, int16_t *right)
{
    (void)ym;
    if (left)
        *left = 0;
    if (right)
        *right = 0;
}
