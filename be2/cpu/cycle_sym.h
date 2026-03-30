/**
 * BE2 — Símbolo y ciclos de referencia por línea de opcode 68000 (bits 15-12).
 * gperf: be2/gen/cycles_gperf.h; nombres: be2/gen/cycle_line_names.h
 */

#ifndef BITEMU_BE2_CYCLE_SYM_H
#define BITEMU_BE2_CYCLE_SYM_H

const char *gen_cpu_line_cycle_symbol(unsigned line_0_to_15);
int gen_cpu_line_cycles_ref(unsigned line_0_to_15);

/**
 * Ciclos de referencia agregados para la línea de decode 68000 (bits 15–12 de `op`).
 * Útil como ancla frente a gentest/BlastEm por familias de opcode; **no** incluye coste por modo de direccionamiento.
 */
static inline int gen_cpu_cycles_ref_line_nibble(uint16_t op)
{
    return gen_cpu_line_cycles_ref((unsigned)(op >> 12));
}

#endif
