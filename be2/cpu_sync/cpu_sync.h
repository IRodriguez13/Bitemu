/**
 * BE2 — Sincronización 68000 ↔ Z80 (Mega Drive)
 *
 * Centraliza:
 *   - proporción de ciclos Z80 por ciclos de bus 68k (NTSC/PAL);
 *   - cuándo el core avanza el Z80;
 *   - cuándo el 68k puede acceder a la work RAM del Z80 (0xA00000…).
 *
 * Ver Documentation/08-cpu-sync-genesis.md para el modelo y límites.
 *
 * Modelo **no** implementado aquí (fase bus/prefetch): cola de prefetch 68000 (palabra siguiente tras opcode),
 * contienda fina 68k↔Z80↔VDP en el mismo ciclo. Cuando exista, encajará bajo este módulo o bajo memory.c según acople.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef BITEMU_GEN_CPU_SYNC_H
#define BITEMU_GEN_CPU_SYNC_H

#include <stdint.h>

struct gen_cpu;
struct gen_vdp;

/* Enhanced CPU sync state for precise timing */
typedef struct {
    uint32_t z80_cycle_acc;        /* Accumulator for Z80 cycle timing */
    uint32_t m68k_cycle_acc;       /* Accumulator for 68k cycle timing */
    uint32_t last_sync_point;       /* Last cycle count where sync occurred */
    uint8_t bus_req_state;         /* Current BUSREQ state */
    uint8_t bus_ack_state;         /* Current BUSACK state */
    uint8_t z80_halted;            /* Z80 halted flag */
    uint32_t bus_request_cycles;    /* Cycles since last bus request */
} gen_cpu_sync_state_t;

/**
 * Ciclos Z80 a ejecutar en lockstep con `cycles_68k` ciclos de bus del 68000.
 * Reloj Z80 ≈ GEN_PSG_HZ (/ PAL); 68000 ≈ GEN_68000_HZ (/ PAL).
 */
int gen_cpu_sync_z80_cycles_from_68k(int cycles_68k, int is_pal);

/** 1 si el Z80 debe ejecutar instrucciones en este slice; 0 si está congelado. */
int gen_cpu_sync_z80_should_run(uint8_t z80_bus_req, uint8_t z80_reset);

/**
 * 1 si el 68k puede leer/escribir la RAM de trabajo del Z80 (mirror 0xA00000–0xA0FFFF)
 * en el modelo actual.
 *
 * Requiere BUS_REQ ≠ 0 y, si `bus_ack_rem` no es NULL, *bus_ack_rem == 0 (BUSACK modelado).
 * Mientras no se cumple, lecturas usan contención (`gen_cpu_sync_z80_ram_contention_read`).
 */
int gen_cpu_sync_m68k_may_access_z80_work_ram(const uint8_t *z80_bus_req, const uint32_t *bus_ack_rem);

/** Lectura 68k a RAM Z80 cuando el acceso está vetado (contención aproximada; no flat 0xFF). */
uint8_t gen_cpu_sync_z80_ram_contention_read(uint32_t addr);

/** Convención histórica en tests; preferir gen_cpu_sync_z80_ram_contention_read(addr). */
#define GEN_CPU_SYNC_Z80_RAM_DENIED_READ 0xFFu

/**
 * Ciclos 68k extra por modelo burdo: burbuja tras BRA/BSR/Bcc (msb 0x60–0x6F),
 * +1 si `gen_vdp_m68k_bus_slice_extra` (stall DMA restante o DMA activo sin fill).
 * Tope 3 por slice (auditable). cpu/vdp pueden ser NULL.
 */
int gen_cpu_sync_m68k_bus_extra_cycles(int cycles_68k_slice, const struct gen_cpu *cpu,
                                       const struct gen_vdp *vdp);

/* ===== Enhanced CPU sync functions ===== */

/* Initialize CPU sync state */
void gen_cpu_sync_init(gen_cpu_sync_state_t *sync);

/* Reset CPU sync state */
void gen_cpu_sync_reset(gen_cpu_sync_state_t *sync);

/* Update sync state for precise timing */
void gen_cpu_sync_update(gen_cpu_sync_state_t *sync, uint32_t m68k_cycles, uint32_t z80_cycles, 
                        uint8_t bus_req, uint8_t bus_ack);

/* Check if bus request is pending */
int gen_cpu_sync_is_bus_request_pending(const gen_cpu_sync_state_t *sync);

/* Get precise bus request timing */
uint32_t gen_cpu_sync_get_bus_request_cycles(const gen_cpu_sync_state_t *sync);

/* Validate bus timing for test ROMs */
int gen_cpu_sync_validate_bus_timing(const gen_cpu_sync_state_t *sync, uint32_t current_cycle, 
                                  uint32_t expected_min_cycles);

/* Calculate optimal sync point for both CPUs */
uint32_t gen_cpu_sync_calculate_sync_point(const gen_cpu_sync_state_t *sync, 
                                         uint32_t m68k_cycles, uint32_t z80_cycles);

#endif /* BITEMU_GEN_CPU_SYNC_H */
