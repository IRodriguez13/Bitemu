/**
 * Bitemu - Game Boy CPU
 * Emulación del Sharp LR35902 (Z80-like)
 */

#ifndef BITEMU_GB_CPU_H
#define BITEMU_GB_CPU_H

#include <stdint.h>

typedef struct gb_cpu {
    uint16_t pc, sp;
    uint8_t a, b, c, d, e, h, l;
    int halted;
} gb_cpu_t;

void gb_cpu_init(gb_cpu_t *cpu);
void gb_cpu_step(gb_cpu_t *cpu);

#endif /* BITEMU_GB_CPU_H */
