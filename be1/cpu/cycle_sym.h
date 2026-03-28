/**
 * BE1 — Símbolo y T-ciclos de referencia por opcode (tablas gperf en be1/gen/).
 * Útil para perf, asserts y contraste con el retorno real de handlers.
 */

#ifndef BITEMU_BE1_CYCLE_SYM_H
#define BITEMU_BE1_CYCLE_SYM_H

#include <stdint.h>

const char *gb_cpu_cycle_symbol_main(uint8_t opcode);
int gb_cpu_cycle_tdots_ref_main(uint8_t opcode);

const char *gb_cpu_cycle_symbol_cb(uint8_t cb_opcode);
int gb_cpu_cycle_tdots_ref_cb(uint8_t cb_opcode);

#endif
