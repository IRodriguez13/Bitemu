/**
 * Bitemu - Game Boy APU
 * Audio Processing Unit
 */

#ifndef BITEMU_GB_APU_H
#define BITEMU_GB_APU_H

#include <stdint.h>

typedef struct gb_apu {
    uint8_t nr50, nr51, nr52;
} gb_apu_t;

void gb_apu_init(gb_apu_t *apu);
void gb_apu_step(gb_apu_t *apu, int cycles);

#endif /* BITEMU_GB_APU_H */
