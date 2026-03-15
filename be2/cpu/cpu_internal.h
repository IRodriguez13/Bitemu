/**
 * BE2 - Sega Genesis: CPU 68000 internals
 *
 * Effective address, SR flags, helpers.
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef BITEMU_GENESIS_CPU_INTERNAL_H
#define BITEMU_GENESIS_CPU_INTERNAL_H

#include "cpu.h"
#include "../memory.h"
#include <stdint.h>

/* SR bits: X=4, N=3, Z=2, V=1, C=0 */
#define GEN_SR_X  0x0010
#define GEN_SR_N  0x0008
#define GEN_SR_Z  0x0004
#define GEN_SR_V  0x0002
#define GEN_SR_C  0x0001

#define flag_x(cpu)  (((cpu)->sr & GEN_SR_X) != 0)
#define flag_n(cpu)  (((cpu)->sr & GEN_SR_N) != 0)
#define flag_z(cpu)  (((cpu)->sr & GEN_SR_Z) != 0)
#define flag_v(cpu)  (((cpu)->sr & GEN_SR_V) != 0)
#define flag_c(cpu)  (((cpu)->sr & GEN_SR_C) != 0)

#define set_x(cpu, v)  do { (cpu)->sr = ((cpu)->sr & ~GEN_SR_X) | ((v) ? GEN_SR_X : 0); } while(0)
#define set_n(cpu, v)  do { (cpu)->sr = ((cpu)->sr & ~GEN_SR_N) | ((v) ? GEN_SR_N : 0); } while(0)
#define set_z(cpu, v)  do { (cpu)->sr = ((cpu)->sr & ~GEN_SR_Z) | ((v) ? GEN_SR_Z : 0); } while(0)
#define set_v(cpu, v)  do { (cpu)->sr = ((cpu)->sr & ~GEN_SR_V) | ((v) ? GEN_SR_V : 0); } while(0)
#define set_c(cpu, v)  do { (cpu)->sr = ((cpu)->sr & ~GEN_SR_C) | ((v) ? GEN_SR_C : 0); } while(0)

/* ea = 6 bits: mode (3) | reg (3). Lee EA. size: 0=B, 1=W, 2=L. out_addr para escritura. */
uint32_t gen_ea_read(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t ea, int size, uint32_t *out_addr);
void gen_ea_write(genesis_mem_t *mem, uint32_t addr, uint32_t val, int size);
/* Obtiene solo la dirección (para destino de MOVE), avanza An si (An)+/-(An) */
uint32_t gen_ea_addr(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t ea, int size);

#endif /* BITEMU_GENESIS_CPU_INTERNAL_H */
