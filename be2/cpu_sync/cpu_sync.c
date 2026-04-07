/**
 * BE2 — implementación cpu_sync
 *
 * Próximo refinamiento (fase bus): granularidad BUSACK por ciclo, contienda 68k↔VDP DMA,
 * lecturas Z80 con último valor de bus donde aplique (distinto del open bus 68k en memory.c).
 * Hecho en CPU: excepción vector 3 por word/long en dirección impar (EA, fetch, pila);
 * STOP con ciclos y fetch alineados; marco Address Error largo del 68000 no simulado (solo stack tipo trap).
 *
 * Profiling del **host** (gprof): conviene cuando el core ya es estable para la ROM medida; ver Documentation/07-gprof.md.
 * Ciclos de referencia por **línea** de opcode: `gen_cpu_line_cycles_ref`, `gen_cpu_cycles_ref_line_nibble`,
 * `gen_cpu_last_opcode_cycles_ref` tras `gen_cpu_step` (sin EA).
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "cpu_sync.h"
#include "../cpu/cpu.h"
#include "../genesis_constants.h"
#include "../vdp/vdp.h"
#include <string.h>

/* ===== Enhanced CPU sync implementation ===== */

void gen_cpu_sync_init(gen_cpu_sync_state_t *sync)
{
    if (!sync) return;
    memset(sync, 0, sizeof(*sync));
}

void gen_cpu_sync_reset(gen_cpu_sync_state_t *sync)
{
    if (!sync) return;
    sync->z80_cycle_acc = 0;
    sync->m68k_cycle_acc = 0;
    sync->last_sync_point = 0;
    sync->bus_req_state = 0;
    sync->bus_ack_state = 0;
    sync->z80_halted = 0;
    sync->bus_request_cycles = 0;
}

void gen_cpu_sync_update(gen_cpu_sync_state_t *sync, uint32_t m68k_cycles, uint32_t z80_cycles, 
                        uint8_t bus_req, uint8_t bus_ack)
{
    if (!sync) return;
    
    sync->m68k_cycle_acc += m68k_cycles;
    sync->z80_cycle_acc += z80_cycles;
    
    /* Track bus request state changes */
    if (bus_req != sync->bus_req_state) {
        sync->bus_req_state = bus_req;
        sync->bus_request_cycles = 0;
    }
    
    sync->bus_ack_state = bus_ack;
    sync->bus_request_cycles += m68k_cycles;
}

int gen_cpu_sync_is_bus_request_pending(const gen_cpu_sync_state_t *sync)
{
    return sync ? (sync->bus_req_state != 0) : 0;
}

uint32_t gen_cpu_sync_get_bus_request_cycles(const gen_cpu_sync_state_t *sync)
{
    return sync ? sync->bus_request_cycles : 0;
}

int gen_cpu_sync_validate_bus_timing(const gen_cpu_sync_state_t *sync, uint32_t current_cycle, 
                                  uint32_t expected_min_cycles)
{
    if (!sync) return 0;
    
    uint32_t elapsed = current_cycle - sync->last_sync_point;
    return (elapsed >= expected_min_cycles) ? 1 : 0;
}

uint32_t gen_cpu_sync_calculate_sync_point(const gen_cpu_sync_state_t *sync, 
                                         uint32_t m68k_cycles, uint32_t z80_cycles)
{
    if (!sync) return 0;
    
    /* Calculate when both CPUs will be at a natural sync point */
    uint32_t m68k_target = sync->m68k_cycle_acc + m68k_cycles;
    uint32_t z80_target = sync->z80_cycle_acc + z80_cycles;
    
    /* Find the least common multiple for optimal sync */
    uint32_t gcd = m68k_target;
    uint32_t temp = z80_target;
    while (temp != 0) {
        uint32_t remainder = gcd % temp;
        gcd = temp;
        temp = remainder;
    }
    
    return (m68k_target / gcd) * z80_target;
}

int gen_cpu_sync_z80_cycles_from_68k(int cycles_68k, int is_pal)
{
    if (cycles_68k <= 0)
        return 0;
    uint32_t zclk = is_pal ? GEN_PSG_HZ_PAL : GEN_PSG_HZ;
    uint32_t mclk = is_pal ? GEN_68000_HZ_PAL : GEN_68000_HZ;
    return (int)(((uint64_t)(unsigned)cycles_68k * zclk) / mclk);
}

int gen_cpu_sync_z80_should_run(uint8_t z80_bus_req, uint8_t z80_reset)
{
    /* A11200: reset activo bajo en hardware; en la emu z80_reset==0 detiene el Z80. */
    return (z80_bus_req == 0 && z80_reset != 0) ? 1 : 0;
}

int gen_cpu_sync_m68k_may_access_z80_work_ram(const uint8_t *z80_bus_req, const uint32_t *bus_ack_rem)
{
    if (!z80_bus_req)
        return 0;
    if (*z80_bus_req == 0)
        return 0;
    if (bus_ack_rem && *bus_ack_rem > 0)
        return 0;
    return 1;
}

uint8_t gen_cpu_sync_z80_ram_contention_read(uint32_t addr)
{
    uint32_t z = addr & (uint32_t)(GEN_Z80_RAM_SIZE - 1);
    uint32_t mix = z ^ (addr >> 9) ^ (addr >> 1);
    return (uint8_t)(0xFFu ^ (mix & 0x7Fu) ^ ((mix >> 7) & 0x7Fu));
}

int gen_cpu_sync_m68k_bus_extra_cycles(int cycles_68k_slice, const struct gen_cpu *cpu,
                                       const struct gen_vdp *vdp)
{
    int extra = 0;

    if (cpu)
    {
        /* Ramas: más refill de prefetch (aprox. un ciclo útil de bus). */
        uint8_t msb = (uint8_t)(cpu->last_opcode >> 8);
        if (msb >= 0x60u && msb <= 0x6Fu)
            extra += 1;
    }

    if (vdp && gen_vdp_m68k_bus_slice_extra(vdp))
        extra += 1;

    (void)cycles_68k_slice;
    if (extra > 3)
        extra = 3;
    return extra;
}
