/**
 * BE2 — línea de instrucción 68000 → etiqueta y ciclos referencia (gperf).
 */

#include "cycle_sym.h"
#include <stddef.h>
#include "../gen/cycles_gperf.h"
#include "../gen/cycle_line_names.h"

static void be2_line_key(char *k3, unsigned line)
{
    k3[0] = 'L';
    if (line < 10u)
        k3[1] = (char)('0' + line);
    else
        k3[1] = (char)('A' + (int)(line - 10u));
    k3[2] = '\0';
}

const char *gen_cpu_line_cycle_symbol(unsigned line_0_to_15)
{
    if (line_0_to_15 > 15u)
        return "?";
    return gen_be2_line_sym[line_0_to_15];
}

int gen_cpu_line_cycles_ref(unsigned line_0_to_15)
{
    if (line_0_to_15 > 15u)
        return 0;
    char k[3];
    be2_line_key(k, line_0_to_15);
    const struct gen_be2_line_sym *e = gen_be2_line_cycles_lookup(k, 2);
    return e ? e->cycles_ref : 0;
}
