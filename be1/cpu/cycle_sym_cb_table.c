/**
 * BE1 — tabla gperf T-ciclos prefijo CB.
 */

#include "cycle_sym.h"
#include <stddef.h>
#include <string.h>
#include "../gen/cycles_cb_gperf.h"
#include "../gen/cycle_mnemonic_tables.h"

static void be1_op_key(char *k2, uint8_t op)
{
    static const char hexd[] = "0123456789ABCDEF";
    k2[0] = hexd[op >> 4];
    k2[1] = hexd[op & 15];
}

const char *gb_cpu_cycle_symbol_cb(uint8_t cb_opcode)
{
    return gb_be1_mnem_cb[cb_opcode];
}

int gb_cpu_cycle_tdots_ref_cb(uint8_t cb_opcode)
{
    char k[3];
    be1_op_key(k, cb_opcode);
    k[2] = '\0';
    const struct gb_be1_cb_cycle_sym *e = gb_be1_cb_cycles_lookup(k, 2);
    return e ? e->tdots : 0;
}
