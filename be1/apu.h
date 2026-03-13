/**
 * Bitemu - Game Boy APU
 * 4 canales: Square1, Square2, Wave, Noise
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef BITEMU_GB_APU_H
#define BITEMU_GB_APU_H

#include <stdint.h>

struct gb_mem;
struct bitemu_audio;

typedef struct gb_apu 
{
    int sample_clock;       /* acumulador fraccionario para muestreo */
    int frame_seq_cycles;   /* ciclos hasta próximo tick del frame sequencer */
    int frame_seq_step;     /* paso 0-7 del frame sequencer (512 Hz) */

    uint8_t sq1_duty_pos;
    uint8_t sq2_duty_pos;
    uint16_t wave_pos;
    uint16_t lfsr;

    int sq1_timer;          /* countdown timers de frecuencia */
    int sq2_timer;
    int wave_timer;
    int noise_timer;

    uint8_t sq1_env_vol;    /* volumen actual del envelope (0-15) */
    uint8_t sq2_env_vol;
    uint8_t noise_env_vol;
    uint8_t sq1_env_timer;  /* countdown del periodo del envelope */
    uint8_t sq2_env_timer;
    uint8_t noise_env_timer;

    int sq1_length;         /* length counters (canal off cuando llega a 0) */
    int sq2_length;
    int wave_length;
    int noise_length;

    /* Frequency sweep (channel 1 only) */
    int sweep_timer;
    int sweep_freq;         /* shadow frequency register */
    uint8_t sweep_enabled;
} gb_apu_t;

void gb_apu_init(gb_apu_t *apu);
void gb_apu_step(gb_apu_t *apu, struct gb_mem *mem, int cycles);
void gb_apu_trigger(gb_apu_t *apu, struct gb_mem *mem, int channel);
void apu_mix_sample(gb_apu_t *apu, struct gb_mem *mem, struct bitemu_audio *audio);

#endif /* BITEMU_GB_APU_H */
