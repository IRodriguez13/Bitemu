/**
 * BE2 - YM2612 FM synthesis
 *
 * Síntesis FM: fase, key on/off, TL, 8 algoritmos, envolventes ADSR.
 * DAC para samples PCM.
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

/* Envelope states */
enum { YM_ENV_ATTACK, YM_ENV_DECAY1, YM_ENV_DECAY2, YM_ENV_RELEASE, YM_ENV_OFF };

#include "ym2612.h"
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

/* Seno 0..1023 → -32767..32767 (cuarto de onda, simetría) */
#define YM_SIN_BITS 10
#define YM_SIN_SIZE (1 << (YM_SIN_BITS - 2))  /* 256 entradas */
static int16_t ym_sin_table[YM_SIN_SIZE + 1];

static void init_sin_table(void)
{
    static int inited;
    if (inited) return;
    inited = 1;
    for (int i = 0; i <= YM_SIN_SIZE; i++)
    {
        double x = (double)i / YM_SIN_SIZE * (3.14159265358979323846 / 2.0);
        ym_sin_table[i] = (int16_t)(sin(x) * 32767.0);
    }
}

/* Fase 10-bit → amplitud signed (usa simetría) */
static int ym_sin_lookup(uint16_t phase)
{
    int idx = phase & 0x1FF;  /* 9 bits para 0..π/2 */
    if (phase & 0x200)       /* π/2..π */
        idx = 0x1FF - idx;
    int s = (int)ym_sin_table[idx & 0xFF];
    if (phase & 0x400)       /* π..3π/2 */
        s = -s;
    return s;
}

/* Key code para detune (F-number bits 8-11, block) */
static uint8_t ym_key_code(uint16_t fnum, uint8_t block)
{
    uint8_t f11 = (fnum >> 10) & 1;
    uint8_t f10 = (fnum >> 9) & 1, f9 = (fnum >> 8) & 1, f8 = (fnum >> 7) & 1;
    uint8_t bit0 = (f11 & (f10 | f9 | f8)) | ((!f11) & f10 & f9 & f8);
    return (block << 2) | (f11 << 1) | bit0;
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
}

void gen_ym2612_write_port(gen_ym2612_t *ym, int port, uint8_t val)
{
    int part = (port >> 1) & 1;
    int is_addr = (port & 1) == 0;
    if (is_addr)
    {
        ym->addr[part] = val;
        return;
    }
    uint8_t r = ym->addr[part];
    if (r < 0x30)
    {
        ym->regs[0][r] = val;
        if (r == 0x28)  /* Key on/off */
        {
            int ch = val & 7;
            if (ch < 6)
            {
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
                    ym->key[ch][op] = new_key;
                }
            }
        }
        return;
    }
    ym->regs[part][r] = val;
    /* Latch freq: $A0-$A2 write aplica $A4-$A6 */
    if (r >= 0xA0 && r <= 0xA2)
    {
        int ch = (r & 3) + part * 3;
        uint8_t hi = ym->regs[part][r + 4];  /* $A4,$A5,$A6 */
        ym->fnum[ch] = ((uint16_t)(hi & 7) << 8) | val;
        ym->block[ch] = (hi >> 3) & 7;
    }
}

void gen_ym2612_step(gen_ym2612_t *ym, int cycles, void *audio_output)
{
    (void)audio_output;
    ym->cycle_counter += cycles;
}

#define YM_REG_DAC_DATA   0x2A
#define YM_REG_DAC_CTRL   0x2B
#define YM_DAC_ENABLE     0x80

/* Obtiene MUL, DT, TL para op en canal ch */
static void ym_get_op_params(gen_ym2612_t *ym, int ch, int op,
    int *mul_out, int *dt_out, int *tl_out)
{
    static const uint8_t op_reg_base[] = { 0x30, 0x38, 0x34, 0x3C };
    static const uint8_t tl_reg_base[] = { 0x40, 0x48, 0x44, 0x4C };
    int part = ch / 3;
    int rch = ch % 3;
    uint8_t r_mul = op_reg_base[op] + rch;
    uint8_t r_tl = tl_reg_base[op] + rch;
    uint8_t v = ym->regs[part][r_mul];
    uint8_t tl = ym->regs[part][r_tl];
    *mul_out = v & 0x0F;   /* 0 = 0.5x, 1-15 = 1x-15x */
    *dt_out = (v >> 4) & 7;
    *tl_out = tl & 0x7F;   /* 7 bits, 0.75 dB/step */
}

/* Obtiene AR, D1R, D2R, RR, SL para op en canal ch */
static void ym_get_env_params(gen_ym2612_t *ym, int ch, int op,
    int *ar, int *d1r, int *d2r, int *rr, int *sl)
{
    static const uint8_t ar_base[] = { 0x50, 0x58, 0x54, 0x5C };
    static const uint8_t dr_base[] = { 0x60, 0x68, 0x64, 0x6C };
    static const uint8_t sr_base[] = { 0x70, 0x78, 0x74, 0x7C };
    static const uint8_t rr_base[] = { 0x80, 0x88, 0x84, 0x8C };
    int part = ch / 3;
    int rch = ch % 3;
    *ar  = ym->regs[part][ar_base[op] + rch] & 0x1F;
    *d1r = (ym->regs[part][dr_base[op] + rch] >> 4) & 0x1F;
    *d2r = ym->regs[part][sr_base[op] + rch] & 0x1F;
    uint8_t rr_sl = ym->regs[part][rr_base[op] + rch];
    *rr = rr_sl & 0x0F;
    *sl = (rr_sl >> 4) & 0x0F;  /* 0=peak, 15=quiet */
}

/* Tabla de incremento por rate (0-31). Aproximación exponencial. */
static const uint16_t ym_rate_step[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 22, 24, 26, 28, 32, 36, 40, 48, 64, 96, 192
};

/* Avanza envolvente un paso. Retorna nivel 0-1023. */
static uint16_t ym_step_envelope(gen_ym2612_t *ym, int ch, int op)
{
    uint8_t st = ym->env_state[ch][op];
    uint16_t lv = ym->env_level[ch][op];
    int ar, d1r, d2r, rr, sl;
    ym_get_env_params(ym, ch, op, &ar, &d1r, &d2r, &rr, &sl);
    uint16_t sl_level = (uint16_t)((15 - sl) * 1023 / 15);  /* SL: 0=peak, 15=min */

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

/* Salida de operador: sin(phase) * atten. atten 0-255. */
static int ym_op_output(uint16_t phase, int atten)
{
    int s = ym_sin_lookup(phase);
    return (s * atten) >> 8;
}

/* Salida con modulación FM: sin(phase + mod_scale) * atten. mod de otro op. */
static int ym_op_output_mod(uint16_t phase, int mod, int atten)
{
    uint16_t p = (phase + (mod >> 5)) & 0x3FF;
    return (ym_sin_lookup(p) * atten) >> 8;
}

/* Avanza fase de op, retorna phase para lookup. inc en 20-bit. */
static uint16_t ym_advance_phase(gen_ym2612_t *ym, int ch, int op, uint32_t inc)
{
    ym->phase[ch][op] = (ym->phase[ch][op] + inc) & 0xFFFFF;
    return (uint16_t)(ym->phase[ch][op] >> 10);
}

/* Obtiene inc de fase para op (con MUL, DT, fnum, block). */
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
    inc = (mul == 0) ? (inc >> 1) : (inc * mul);
    return inc & 0xFFFFF;
}

void gen_ym2612_run_cycles(gen_ym2612_t *ym, int cycles, int16_t *left, int16_t *right)
{
    (void)cycles;
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
        if (dac_en & YM_DAC_ENABLE && ch == 5)
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
        if (!pan_l && !pan_r) pan_l = pan_r = 1;

        /* Atenuación: TL (0.75 dB/step) × envolvente (0-1023) */
        int tl[4];
        uint16_t env[4];
        for (int op = 0; op < 4; op++)
        {
            int m, d, t;
            ym_get_op_params(ym, ch, op, &m, &d, &t);
            int tl_lin = 255 - (t * 3);
            if (tl_lin < 0) tl_lin = 0;
            env[op] = ym_step_envelope(ym, ch, op);
            tl[op] = (tl_lin * env[op]) >> 10;
        }

        int out = 0;
        uint32_t inc[4];
        for (int op = 0; op < 4; op++)
            inc[op] = ym_op_inc(ym, ch, op, fnum, block, kc);

        /* Avanzar fases una vez por muestra */
        uint16_t ph[4];
        for (int op = 0; op < 4; op++)
            ph[op] = ym_advance_phase(ym, ch, op, inc[op]);

        /* Algoritmos FM: M=modulator, C=carrier. Modulación: phase + (mod>>5). */
        switch (alg)
        {
        case 0:  /* M1→M2→M3→C */
            out = ym_op_output_mod(ph[3], ym_op_output_mod(ph[2], ym_op_output_mod(ph[1],
                ym_op_output(ph[0], tl[0]), tl[1]), tl[2]), tl[3]);
            break;
        case 1:  /* M1→M2→C, M3→C (ambos modulan C) */
        {
            int m2 = ym_op_output_mod(ph[1], ym_op_output(ph[0], tl[0]), tl[1]);
            int m3 = ym_op_output(ph[2], tl[2]);
            out = ym_op_output_mod(ph[3], m2 + m3, tl[3]);
            break;
        }
        case 2:  /* M1→M2→C, M3→C (M2+M3 suman a C) */
        {
            int m2 = ym_op_output_mod(ph[1], ym_op_output(ph[0], tl[0]), tl[1]);
            int m3 = ym_op_output(ph[2], tl[2]);
            out = ym_op_output_mod(ph[3], m2 + m3, tl[3]);
            break;
        }
        case 3:  /* M1→C, M2→C, M3→C (todos a C en paralelo) */
            out = ym_op_output(ph[3], tl[3]) + ym_op_output(ph[0], tl[0]) +
                  ym_op_output(ph[1], tl[1]) + ym_op_output(ph[2], tl[2]);
            break;
        case 4:
        case 5:  /* M1→M2→C, M3→C */
        {
            int m2 = ym_op_output_mod(ph[1], ym_op_output(ph[0], tl[0]), tl[1]);
            out = ym_op_output_mod(ph[3], m2, tl[3]) + ym_op_output(ph[2], tl[2]);
            break;
        }
        case 6:  /* M1+M2→M3→C */
        {
            int m12 = ym_op_output(ph[0], tl[0]) + ym_op_output(ph[1], tl[1]);
            int m3 = ym_op_output_mod(ph[2], m12, tl[2]);
            out = ym_op_output_mod(ph[3], m3, tl[3]);
            break;
        }
        case 7:  /* M1+M2+M3+C (todos carriers) */
        default:
            out = ym_op_output(ph[0], tl[0]) + ym_op_output(ph[1], tl[1]) +
                  ym_op_output(ph[2], tl[2]) + ym_op_output(ph[3], tl[3]);
            break;
        }

        /* Sumar si algún op está activo (key on o en release) */
        int any_active = 0;
        for (int op = 0; op < 4; op++)
            if (ym->key[ch][op] || ym->env_state[ch][op] != YM_ENV_OFF) { any_active = 1; break; }
        if (any_active)
        {
            sum_l += pan_l ? out : 0;
            sum_r += pan_r ? out : 0;
        }
    }

    if (sum_l > 32767)  sum_l = 32767;
    if (sum_l < -32768) sum_l = -32768;
    if (sum_r > 32767)  sum_r = 32767;
    if (sum_r < -32768) sum_r = -32768;
    if (left)  *left  = (int16_t)sum_l;
    if (right) *right = (int16_t)sum_r;
}
