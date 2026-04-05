/**
 * BE2 — línea de instrucción 68000 → etiqueta y ciclos referencia (gperf).
 *
 * La tabla por *línea* (nibble alto del opcode) no incluye ciclos por modo de direccionamiento.
 * Acople al core: tras cada `gen_cpu_step`, `gen_cpu_last_opcode_cycles_ref(&cpu)` usa el mismo
 * valor que `gen_cpu_cycles_ref_line_nibble(cpu.last_opcode)` para métricas o comparación con gentest.
 */

#include "cycle_sym.h"
#include <stddef.h>
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
#include "../gen/cycles_gperf.h"
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
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
