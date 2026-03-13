/**
 * Bitemu - CPU internals (solo para cpu.c y cpu_handlers.c)
 *
 * Macros: HL, BC, DE (pares de registros como uint16_t), R(r) (puntero al registro r).
 * Helpers de flags: set_z/set_n/set_h/set_c, flag_z/flag_c.
 * Declaraciones: read_r8, write_r8, alu_* (implementadas en cpu.c).
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef BITEMU_CPU_INTERNAL_H
#define BITEMU_CPU_INTERNAL_H

#include "cpu.h"
#include "../memory.h"
#include "../gb_constants.h"

#define HL ((uint16_t)((cpu->h << 8) | cpu->l))
#define BC ((uint16_t)((cpu->b << 8) | cpu->c))
#define DE ((uint16_t)((cpu->d << 8) | cpu->e))
#define R(r) (*((uint8_t *[]){&cpu->b, &cpu->c, &cpu->d, &cpu->e, &cpu->h, &cpu->l, NULL, &cpu->a})[r])

static inline void set_z(gb_cpu_t *cpu, int v) { cpu->f = (cpu->f & ~F_Z) | (v ? F_Z : 0); }
static inline void set_n(gb_cpu_t *cpu, int v) { cpu->f = (cpu->f & ~F_N) | (v ? F_N : 0); }
static inline void set_h(gb_cpu_t *cpu, int v) { cpu->f = (cpu->f & ~F_H) | (v ? F_H : 0); }
static inline void set_c(gb_cpu_t *cpu, int v) { cpu->f = (cpu->f & ~F_C) | (v ? F_C : 0); }
static inline int flag_z(gb_cpu_t *cpu) { return (cpu->f & F_Z) != 0; }
static inline int flag_c(gb_cpu_t *cpu) { return (cpu->f & F_C) != 0; }

uint8_t read_r8(gb_cpu_t *cpu, gb_mem_t *mem, int r);
void write_r8(gb_cpu_t *cpu, gb_mem_t *mem, int r, uint8_t v);
void alu_add(gb_cpu_t *cpu, uint8_t a, uint8_t b, int carry);
void alu_sub(gb_cpu_t *cpu, uint8_t a, uint8_t b, int carry);
void alu_and(gb_cpu_t *cpu, uint8_t v);
void alu_xor(gb_cpu_t *cpu, uint8_t v);
void alu_or(gb_cpu_t *cpu, uint8_t v);
void alu_cp(gb_cpu_t *cpu, uint8_t a, uint8_t b);

#endif /* BITEMU_CPU_INTERNAL_H */
