/**
 * Bitemu - CPU opcode handler tables
 *
 * Tipos de callback y tablas declaradas aquí; implementación en cpu_handlers.c.
 * gb_cpu_step usa gb_op_handlers[op] y, si op==0xCB, gb_cb_handlers[cb>>3].
 */
#ifndef BITEMU_CPU_HANDLERS_H
#define BITEMU_CPU_HANDLERS_H

#include "cpu.h"
#include "../memory.h"

typedef int (*gb_op_handler_t)(gb_cpu_t *cpu, gb_mem_t *mem, uint8_t op);
typedef int (*gb_cb_handler_t)(gb_cpu_t *cpu, gb_mem_t *mem, uint8_t cb_op);

extern gb_op_handler_t gb_op_handlers[256];
extern gb_cb_handler_t gb_cb_handlers[32];

#endif /* BITEMU_CPU_HANDLERS_H */
