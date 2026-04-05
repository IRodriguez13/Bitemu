/**
 * BE2 - YM2612 FM synthesis
 *
 * Síntesis FM: fase, key on/off, TL, 8 algoritmos, envolventes ADSR.
 * DAC para samples PCM.
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#define YM_SIN_BITS 10
#define YM_SIN_SIZE (1 << (YM_SIN_BITS - 2))  /* 256 entradas */

#define YM_REG_DAC_DATA   0x2A
#define YM_REG_DAC_CTRL   0x2B
#define YM_DAC_ENABLE     0x80

/* Un tick de contador de timer YM ≈ cada N ciclos de bus 68k (FM clock no modelado al ciclo). */
#define YM_TIMER_SAMPLE_68K_CYCLES 16

enum {
    YM_ENV_ATTACK,
    YM_ENV_DECAY1,
    YM_ENV_DECAY2,
    YM_ENV_RELEASE,
    YM_ENV_OFF
};

#include "ym2612.h"
#include "core/simd/simd.h"
#include "../genesis_constants.h"
#include <string.h>
#include <math.h>

/* Tabla detune: [key_code][detune_mag] → delta fase (blog Part 2) */
static const uint8_t ym_detune_table[32][4] = {
    {0,0,1,2},{0,0,1,2},{0,0,1,2},{0,0,1,2},
    {0,1,2,2},{0,1,2,3},{0,1,2,3},{0,1,2,3},
    {0,1,2,4},{0,1,3,4},{0,1,3,4},{0,1,3,5},
    {0,2,4,5},{0,2,4,6},{0,2,4,6},{0,2,5,7},
    {0,2,5,8},{0,3,6,8},{0,3,6,9},{0,3,7,10},
    {0,4,8,11},{0,4,8,12},{0,4,9,13},{0,5,10,14},
    {0,5,11,16},{0,6,12,17},{0,6,13,19},{0,7,14,20},
    {0,8,16,22},{0,8,16,22},{0,8,16,22},{0,8,16,22},
};

/* Tabla de incremento por rate (0-31). Aproximación exponencial. */
static const uint16_t ym_rate_step[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 22, 24, 26, 28, 32, 36, 40, 48, 64, 96, 192
};

static const uint8_t ym_op_reg_base[] = { 0x30, 0x38, 0x34, 0x3C };
static const uint8_t ym_tl_reg_base[] = { 0x40, 0x48, 0x44, 0x4C };
static const uint8_t ym_env_ar_base[] = { 0x50, 0x58, 0x54, 0x5C };
static const uint8_t ym_env_dr_base[] = { 0x60, 0x68, 0x64, 0x6C };
static const uint8_t ym_env_sr_base[] = { 0x70, 0x78, 0x74, 0x7C };
static const uint8_t ym_env_rr_base[] = { 0x80, 0x88, 0x84, 0x8C };

static int16_t ym_sin_table[YM_SIN_SIZE + 1];

static void init_sin_table(void)
{
    static int inited;
    if (inited)
        return;
    inited = 1;
    for (int i = 0; i <= YM_SIN_SIZE; i++)
    {
        double x = (double)i / YM_SIN_SIZE * (3.14159265358979323846 / 2.0);
        ym_sin_table[i] = (int16_t)(sin(x) * 32767.0);
    }
}

static int ym_sin_lookup(uint16_t phase)
{
    int idx = phase & 0x1FF;
    if (phase & 0x200)
        idx = 0x1FF - idx;
    int s = (int)ym_sin_table[idx & 0xFF];
    if (phase & 0x400)
        s = -s;
    return s;
}

static uint8_t ym_key_code(uint16_t fnum, uint8_t block)
{
    uint8_t f11 = (fnum >> 10) & 1;
    uint8_t f10 = (fnum >> 9) & 1, f9 = (fnum >> 8) & 1, f8 = (fnum >> 7) & 1;
    uint8_t bit0 = (f11 & (f10 | f9 | f8)) | (uint8_t)((!f11) & f10 & f9 & f8);
    return (uint8_t)((block << 2) | (f11 << 1) | bit0);
}

void gen_ym2612_init(gen_ym2612_t *ym)
{
    memset(ym, 0, sizeof(*ym));
    init_sin_table();
}

void gen_ym2612_reset(gen_ym2612_t *ym)
{
    memset(ym->regs, 0, sizeof(ym->regs));
    memset(ym->phase, 0, sizeof(ym->phase));
    memset(ym->key, 0, sizeof(ym->key));
    memset(ym->env_state, 0, sizeof(ym->env_state));
    memset(ym->env_level, 0, sizeof(ym->env_level));
    ym->addr[0] = ym->addr[1] = 0;
    ym->cycle_counter = 0;
    ym->busy_cycles = 0;
    ym->timer_a_counter = ym->timer_b_counter = 0;
    ym->timer_tick_acc = 0;
    ym->timer_overflow = 0;
}

static void ym2612_timer_tick(gen_ym2612_t *ym)
{
    uint8_t mode = ym->regs[0][0x27];
    if (mode & 0x04u)
    {
        uint16_t na = ((uint16_t)(ym->regs[0][0x25] & 3) << 8) | ym->regs[0][0x24];
        uint16_t period = (uint16_t)(1024u - (na & 0x3FFu));
        if (period == 0)
            period = 1024;
        if (ym->timer_a_counter == 0 || ym->timer_a_counter > period)
            ym->timer_a_counter = period;
        ym->timer_a_counter--;
        if (ym->timer_a_counter == 0)
        {
            ym->timer_overflow = (uint8_t)(ym->timer_overflow | 1u);
            ym->timer_a_counter = period;
        }
    }
    else
        ym->timer_a_counter = 0;

    if (mode & 0x08u)
    {
        uint16_t period = (uint16_t)(256u - ym->regs[0][0x26]);
        if (period == 0)
            period = 256;
        if (ym->timer_b_counter == 0 || ym->timer_b_counter > period)
            ym->timer_b_counter = period;
        ym->timer_b_counter--;
        if (ym->timer_b_counter == 0)
        {
            ym->timer_overflow = (uint8_t)(ym->timer_overflow | 2u);
            ym->timer_b_counter = period;
        }
    }
    else
        ym->timer_b_counter = 0;
}

uint8_t gen_ym2612_read_port(gen_ym2612_t *ym, int port)
{
    (void)port;
    if (ym->busy_cycles > 0)
        return 0x80u;
    uint8_t st = (uint8_t)(ym->timer_overflow & 3u);
    ym->timer_overflow = (uint8_t)(ym->timer_overflow & (uint8_t)~3u);
    return st;
}

void gen_ym2612_write_port(gen_ym2612_t *ym, int port, uint8_t val)
{
    int part = (port >> 1) & 1;

    if ((port & 1) == 0)
    {
        ym->addr[part] = val;
        if (ym->busy_cycles < GEN_YM2612_ADDR_BUSY_CYCLES_68K)
            ym->busy_cycles = GEN_YM2612_ADDR_BUSY_CYCLES_68K;
        return;
    }

    ym->busy_cycles = GEN_YM2612_WRITE_BUSY_CYCLES_68K;

    uint8_t r = ym->addr[part];

    if (r >= 0x30)
    {
        ym->regs[part][r] = val;
        if (r >= 0xA0 && r <= 0xA2)
        {
            int ch = (r & 3) + part * 3;
            uint8_t hi = ym->regs[part][r + 4];
            ym->fnum[ch] = ((uint16_t)(hi & 7) << 8) | val;
            ym->block[ch] = (hi >> 3) & 7;
        }
        return;
    }

    if (r == 0x27)
    {
        if (val & 0x10u)
            ym->timer_overflow = (uint8_t)(ym->timer_overflow & (uint8_t)~1u);
        if (val & 0x20u)
            ym->timer_overflow = (uint8_t)(ym->timer_overflow & (uint8_t)~2u);
    }

    ym->regs[0][r] = val;
    if (r != 0x28)
        return;

    int ch = val & 7;
    if (ch > 6)
        return;

    for (int op = 0; op < 4; op++)
    {
        int new_key = (val >> (4 + op)) & 1;
        if (new_key && !ym->key[ch][op])
        {
            ym->phase[ch][op] = 0;
            ym->env_state[ch][op] = YM_ENV_ATTACK;
            ym->env_level[ch][op] = 0;
        }
        else if (!new_key && ym->key[ch][op])
            ym->env_state[ch][op] = YM_ENV_RELEASE;
        ym->key[ch][op] = (uint8_t)new_key;
    }
}

void gen_ym2612_step(gen_ym2612_t *ym, int cycles, void *audio_output)
{
    (void)audio_output;
    if (ym->busy_cycles > 0)
    {
        ym->busy_cycles -= cycles;
        if (ym->busy_cycles < 0)
            ym->busy_cycles = 0;
    }
    if (cycles > 0)
    {
        ym->timer_tick_acc += (uint32_t)cycles;
        while (ym->timer_tick_acc >= (uint32_t)YM_TIMER_SAMPLE_68K_CYCLES)
        {
            ym->timer_tick_acc -= (uint32_t)YM_TIMER_SAMPLE_68K_CYCLES;
            ym2612_timer_tick(ym);
        }
    }
    ym->cycle_counter += cycles;
}

static uint8_t ym_tl_linear_atten(uint8_t tl)
{
    double db = (double)(tl & 0x7F) * -0.75;
    double lin = pow(10.0, db / 20.0);
    int a = (int)(lin * 255.0 + 0.5);
    if (a < 0)
        return 0;
    if (a > 255)
        return 255;
    return (uint8_t)a;
}

static void ym_get_op_params(gen_ym2612_t *ym, int ch, int op,
    int *mul_out, int *dt_out, int *tl_out)
{
    int part = ch / 3;
    int rch = ch % 3;
    uint8_t r_mul = (uint8_t)(ym_op_reg_base[op] + rch);
    uint8_t r_tl = (uint8_t)(ym_tl_reg_base[op] + rch);
    uint8_t v = ym->regs[part][r_mul];
    uint8_t tl = ym->regs[part][r_tl];
    *mul_out = v & 0x0F;
    *dt_out = (v >> 4) & 7;
    *tl_out = tl & 0x7F;
}

static void ym_get_env_params(gen_ym2612_t *ym, int ch, int op,
    int *ar, int *d1r, int *d2r, int *rr, int *sl)
{
    int part = ch / 3;
    int rch = ch % 3;
    *ar  = ym->regs[part][ym_env_ar_base[op] + rch] & 0x1F;
    *d1r = (ym->regs[part][ym_env_dr_base[op] + rch] >> 4) & 0x1F;
    *d2r = ym->regs[part][ym_env_sr_base[op] + rch] & 0x1F;
    uint8_t rr_sl = ym->regs[part][ym_env_rr_base[op] + rch];
    *rr = rr_sl & 0x0F;
    *sl = (rr_sl >> 4) & 0x0F;
}

static uint16_t ym_step_envelope(gen_ym2612_t *ym, int ch, int op)
{
    uint8_t st = ym->env_state[ch][op];
    uint16_t lv = ym->env_level[ch][op];
    int ar, d1r, d2r, rr, sl;
    ym_get_env_params(ym, ch, op, &ar, &d1r, &d2r, &rr, &sl);
    uint16_t sl_level = (uint16_t)((15 - sl) * 1023 / 15);

    switch (st)
    {
    case YM_ENV_ATTACK:
        lv += ym_rate_step[ar];
        if (lv >= 1023) { lv = 1023; ym->env_state[ch][op] = YM_ENV_DECAY1; }
        break;
    case YM_ENV_DECAY1:
        lv -= ym_rate_step[d1r];
        if (lv <= sl_level || lv > 1023) { lv = sl_level; ym->env_state[ch][op] = YM_ENV_DECAY2; }
        break;
    case YM_ENV_DECAY2:
        lv -= ym_rate_step[d2r];
        if (lv <= sl_level || lv > 1023) lv = sl_level;
        break;
    case YM_ENV_RELEASE:
        lv -= ym_rate_step[rr];
        if (lv > 1023 || lv == 0) { lv = 0; ym->env_state[ch][op] = YM_ENV_OFF; }
        break;
    default:
        break;
    }
    ym->env_level[ch][op] = lv;
    return lv;
}

static int ym_op_output(uint16_t phase, int atten)
{
    int s = ym_sin_lookup(phase);
    return (s * atten) >> 8;
}

static int ym_op_output_mod(uint16_t phase, int mod, int atten)
{
    uint16_t p = (uint16_t)((phase + (unsigned)(mod >> 5)) & 0x3FF);
    return (ym_sin_lookup(p) * atten) >> 8;
}

static uint16_t ym_advance_phase(gen_ym2612_t *ym, int ch, int op, uint32_t inc)
{
    ym->phase[ch][op] = (ym->phase[ch][op] + inc) & 0xFFFFF;
    return (uint16_t)(ym->phase[ch][op] >> 10);
}

static uint32_t ym_op_inc(gen_ym2612_t *ym, int ch, int op, uint16_t fnum, uint8_t block, uint8_t kc)
{
    int mul, dt, tl;
    ym_get_op_params(ym, ch, op, &mul, &dt, &tl);
    (void)tl;
    uint32_t inc = (uint32_t)(fnum << block) >> 1;
    if (dt & 3)
    {
        uint32_t d = ym_detune_table[kc][dt & 3];
        inc = (dt & 4) ? (inc - d) : (inc + d);
        inc &= 0x1FFFF;
    }
    inc = (mul == 0) ? (inc >> 1) : (inc * (uint32_t)mul);
    return inc & 0xFFFFF;
}

void gen_ym2612_run_cycles(gen_ym2612_t *ym, int cycles, int16_t *left, int16_t *right)
{
    gen_ym2612_step(ym, cycles, NULL);
    int32_t sum_l = 0, sum_r = 0;

    uint8_t dac_en = ym->regs[0][YM_REG_DAC_CTRL];
    if (dac_en & YM_DAC_ENABLE)
    {
        uint8_t dac_val = ym->regs[0][YM_REG_DAC_DATA];
        int16_t dac_out = (int16_t)((int)(dac_val - 128) * 256);
        sum_l += dac_out;
        sum_r += dac_out;
    }

    for (int ch = 0; ch < 6; ch++)
    {
        if ((dac_en & YM_DAC_ENABLE) && ch == 5)
            continue;
        uint16_t fnum = ym->fnum[ch];
        uint8_t block = ym->block[ch];
        uint8_t kc = ym_key_code(fnum, block);
        int part = ch / 3;
        uint8_t ctrl = ym->regs[part][0xB0 + (ch % 3)];
        int alg = ctrl & 7;
        uint8_t b4 = ym->regs[part][0xB4 + (ch % 3)];
        int pan_l = (b4 >> 7) & 1;
        int pan_r = (b4 >> 6) & 1;
        if (!pan_l && !pan_r)
            pan_l = pan_r = 1;

        int tl[4];
        uint16_t env[4];
        for (int op = 0; op < 4; op++)
        {
            int m, d, t;
            ym_get_op_params(ym, ch, op, &m, &d, &t);
            int tl_lin = ym_tl_linear_atten((uint8_t)t);
            env[op] = ym_step_envelope(ym, ch, op);
            tl[op] = (tl_lin * (int)env[op]) >> 10;
        }

        int out = 0;
        uint32_t inc[4];
        for (int op = 0; op < 4; op++)
            inc[op] = ym_op_inc(ym, ch, op, fnum, block, kc);

        uint16_t ph[4];
        for (int op = 0; op < 4; op++)
            ph[op] = ym_advance_phase(ym, ch, op, inc[op]);

        switch (alg)
        {
        case 0:
            out = ym_op_output_mod(ph[3], ym_op_output_mod(ph[2], ym_op_output_mod(ph[1],
                ym_op_output(ph[0], tl[0]), tl[1]), tl[2]), tl[3]);
            break;
        case 1:
        {
            int m2 = ym_op_output_mod(ph[1], ym_op_output(ph[0], tl[0]), tl[1]);
            int m3 = ym_op_output(ph[2], tl[2]);
            out = ym_op_output_mod(ph[3], m2 + m3, tl[3]);
            break;
        }
        case 2:
        {
            int m2 = ym_op_output_mod(ph[1], ym_op_output(ph[0], tl[0]), tl[1]);
            int m3 = ym_op_output(ph[2], tl[2]);
            out = ym_op_output_mod(ph[3], m2 + m3, tl[3]);
            break;
        }
        case 3:
            out = ym_op_output(ph[3], tl[3]) + ym_op_output(ph[0], tl[0]) +
                  ym_op_output(ph[1], tl[1]) + ym_op_output(ph[2], tl[2]);
            break;
        case 4:
        case 5:
        {
            int m2 = ym_op_output_mod(ph[1], ym_op_output(ph[0], tl[0]), tl[1]);
            out = ym_op_output_mod(ph[3], m2, tl[3]) + ym_op_output(ph[2], tl[2]);
            break;
        }
        case 6:
        {
            int m12 = ym_op_output(ph[0], tl[0]) + ym_op_output(ph[1], tl[1]);
            int m3 = ym_op_output_mod(ph[2], m12, tl[2]);
            out = ym_op_output_mod(ph[3], m3, tl[3]);
            break;
        }
        case 7:
        default:
            out = ym_op_output(ph[0], tl[0]) + ym_op_output(ph[1], tl[1]) +
                  ym_op_output(ph[2], tl[2]) + ym_op_output(ph[3], tl[3]);
            break;
        }

        int any_active = 0;
        for (int op = 0; op < 4; op++)
        {
            if (ym->key[ch][op] || ym->env_state[ch][op] != YM_ENV_OFF)
            {
                any_active = 1;
                break;
            }
        }
        if (!any_active)
            continue;
        sum_l += pan_l ? out : 0;
        sum_r += pan_r ? out : 0;
    }

    if (left)
        *left = bitemu_sat_i16_i32(sum_l);
    if (right)
        *right = bitemu_sat_i16_i32(sum_r);
}
