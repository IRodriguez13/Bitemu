/**
 * Bitemu - Game Boy APU
 * 4 canales: Square1, Square2, Wave, Noise
 */

#ifndef BITEMU_GB_APU_H
#define BITEMU_GB_APU_H

#include <stdint.h>

struct gb_mem;
struct bitemu_audio;

typedef struct gb_apu {
    int cycle_counter;
    int sample_clock;  /* acumulador fraccionario para muestreo; persistente entre frames */
    uint8_t sq1_duty_pos;
    uint8_t sq2_duty_pos;
    uint16_t wave_pos;
    uint16_t lfsr;
} gb_apu_t;

void gb_apu_init(gb_apu_t *apu);
void gb_apu_step(gb_apu_t *apu, struct gb_mem *mem, int cycles);
void apu_mix_sample(gb_apu_t *apu, struct gb_mem *mem, struct bitemu_audio *audio);

#endif /* BITEMU_GB_APU_H */
