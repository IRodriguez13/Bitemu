/**
 * BE1 — tabla gperf T-ciclos opcodes principales (un .c por tabla gperf).
 */

#include "cycle_sym.h"
#include <stddef.h>
#include <string.h>
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
#include "../gen/cycles_gperf.h"
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
#include "../gen/cycle_mnemonic_tables.h"

static void be1_op_key(char *k2, uint8_t op)
{
    static const char hexd[] = "0123456789ABCDEF";
    k2[0] = hexd[op >> 4];
    k2[1] = hexd[op & 15];
}

const char *gb_cpu_cycle_symbol_main(uint8_t opcode)
{
    return gb_be1_mnem_main[opcode];
}

int gb_cpu_cycle_tdots_ref_main(uint8_t opcode)
{
    char k[3];
    be1_op_key(k, opcode);
    k[2] = '\0';
    const struct gb_be1_cycle_sym *e = gb_be1_cycles_lookup(k, 2);
    return e ? e->tdots : 0;
}
