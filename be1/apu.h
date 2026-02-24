/**
 * Bitemu - Game Boy APU
 * 4 canales: Square1, Square2, Wave, Noise
 */

#ifndef BITEMU_GB_APU_H
#define BITEMU_GB_APU_H

#include <stdint.h>

struct gb_mem;

typedef struct gb_apu {
    int cycle_counter;
    uint8_t sq1_duty_pos;
    uint8_t sq2_duty_pos;
    uint16_t wave_pos;
    uint16_t lfsr;
} gb_apu_t;

void gb_apu_init(gb_apu_t *apu);
void gb_apu_step(gb_apu_t *apu, struct gb_mem *mem, int cycles);

#endif /* BITEMU_GB_APU_H */
