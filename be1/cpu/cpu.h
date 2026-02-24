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
    uint8_t f;       /* Flags: Z(7) N(6) H(5) C(4) */
    int halted;
    int ime;         /* Interrupt Master Enable */
    int ime_delay;   /* EI: enable IME after next instruction */
} gb_cpu_t;

/* Flags */
#define F_Z 0x80
#define F_N 0x40
#define F_H 0x20
#define F_C 0x10

struct gb_mem;

void gb_cpu_init(gb_cpu_t *cpu);
void gb_cpu_reset(gb_cpu_t *cpu);

/* Ejecuta una instrucción, retorna ciclos consumidos (M-cycles × 4 = T-states) */
int gb_cpu_step(gb_cpu_t *cpu, struct gb_mem *mem);

#endif /* BITEMU_GB_CPU_H */
