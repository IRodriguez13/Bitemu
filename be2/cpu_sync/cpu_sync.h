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
 * +1 si VDP marca DMA activo sin fill pendiente. Tope 3 por slice (auditable).
 * cpu/vdp pueden ser NULL (trata como sin extra salvo step).
 */
int gen_cpu_sync_m68k_bus_extra_cycles(int cycles_68k_slice, const struct gen_cpu *cpu,
                                       const struct gen_vdp *vdp);

#endif /* BITEMU_GEN_CPU_SYNC_H */
