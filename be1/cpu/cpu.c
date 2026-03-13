/**
 * Bitemu - Game Boy CPU (LR35902 / SM83)
 *
 * Responsabilidades:
 * - Fetch-decode-execute: lee opcode en PC, despacha mediante tabla de handlers (cpu_handlers).
 * - Interrupciones: IME, retardo tras EI, despacho a vector y actualización de IF/IE.
 * - HALT: despertar cuando hay interrupción pendiente (IF & IE).
 * - Helpers compartidos: read_r8/write_r8 (registros y (HL)), alu_add/alu_sub/alu_and/alu_xor/alu_or/alu_cp.
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "cpu_internal.h"
#include "cpu_handlers.h"
#include <stddef.h>

/* ---------------------------------------------------------------------------
 * Acceso a registros de 8 bits: r 0..7 = B,C,D,E,H,L,(HL),A
 * --------------------------------------------------------------------------- */

uint8_t read_r8(gb_cpu_t *cpu, gb_mem_t *mem, int r)
{
    return (r == 6) ? gb_mem_read(mem, HL) : R(r);
}

void write_r8(gb_cpu_t *cpu, gb_mem_t *mem, int r, uint8_t v)
{
    if (r == 6)
        gb_mem_write(mem, HL, v);
    else
        R(r) = v;
}

void gb_cpu_init(gb_cpu_t *cpu)
{
    cpu->pc = 0;
    cpu->sp = 0;
    cpu->a = cpu->b = cpu->c = cpu->d = cpu->e = cpu->h = cpu->l = 0;
    cpu->f = 0;
    cpu->halted = 0;
    cpu->ime = 0;
    cpu->ime_delay = 0;
    cpu->halt_bug = 0;
}

void gb_cpu_reset(gb_cpu_t *cpu)
{
    cpu->pc = GB_CPU_ENTRY;
    cpu->sp = GB_SP_INIT;
    cpu->a = 0x01;
    cpu->f = GB_CPU_F_POSTBOOT;
    cpu->b = 0x00;
    cpu->c = 0x13;
    cpu->d = 0x00;
    cpu->e = 0xD8;
    cpu->h = 0x01;
    cpu->l = 0x4D;
    cpu->halted = 0;
    cpu->ime = 0;
    cpu->ime_delay = 0;
    cpu->halt_bug = 0;
}

/* ---------------------------------------------------------------------------
 * Despacho de interrupciones: prioridad VBLANK > LCDC > TIMER > SERIAL > JOYPAD
 * --------------------------------------------------------------------------- */
static int dispatch_interrupt(gb_cpu_t *cpu, gb_mem_t *m)
{
    uint8_t if_reg = m->io[GB_IO_IF] & GB_IF_IE_MASK;
    uint8_t ie = m->ie & GB_IF_IE_MASK;
    uint8_t pending = if_reg & ie;
    if (!pending)
        return 0;

    uint16_t vec;
    uint8_t mask;
    if (pending & GB_IF_VBLANK)
    {
        vec = GB_IV_VBLANK;
        mask = GB_IF_VBLANK;
    }
    else if (pending & GB_IF_LCDC)
    {
        vec = GB_IV_LCDC;
        mask = GB_IF_LCDC;
    }
    else if (pending & GB_IF_TIMER)
    {
        vec = GB_IV_TIMER;
        mask = GB_IF_TIMER;
    }
    else if (pending & GB_IF_SERIAL)
    {
        vec = GB_IV_SERIAL;
        mask = GB_IF_SERIAL;
    }
    else
    {
        vec = GB_IV_JOYPAD;
        mask = GB_IF_JOYPAD;
    }

    cpu->ime = 0;
    cpu->ime_delay = 0;
    m->io[GB_IO_IF] &= ~mask;
    cpu->sp -= 2;
    gb_mem_write16(m, cpu->sp, cpu->pc);
    cpu->pc = vec;
    return 20;  /* ciclos típicos de una interrupción */
}

/* ---------------------------------------------------------------------------
 * ALU: operaciones que actualizan A y flags Z,N,H,C
 * --------------------------------------------------------------------------- */
void alu_add(gb_cpu_t *cpu, uint8_t a, uint8_t b, int carry)
{
    uint16_t r = a + b + carry;
    uint8_t res = (uint8_t)r;
    set_z(cpu, res == 0);
    set_n(cpu, 0);
    set_h(cpu, ((a & 0xF) + (b & 0xF) + carry) > 0xF);
    set_c(cpu, r > GB_BYTE_LO);
    cpu->a = res;
}

void alu_sub(gb_cpu_t *cpu, uint8_t a, uint8_t b, int carry)
{
    uint16_t r = a - b - carry;
    uint8_t res = (uint8_t)r;
    set_z(cpu, res == 0);
    set_n(cpu, 1);
    set_h(cpu, (int)(a & 0xF) - (int)(b & 0xF) - carry < 0);
    set_c(cpu, r > GB_BYTE_LO);
    cpu->a = res;
}

void alu_and(gb_cpu_t *cpu, uint8_t v)
{
    cpu->a &= v;
    set_z(cpu, cpu->a == 0);
    set_n(cpu, 0);
    set_h(cpu, 1);
    set_c(cpu, 0);
}

void alu_xor(gb_cpu_t *cpu, uint8_t v)
{
    cpu->a ^= v;
    set_z(cpu, cpu->a == 0);
    set_n(cpu, 0);
    set_h(cpu, 0);
    set_c(cpu, 0);
}

void alu_or(gb_cpu_t *cpu, uint8_t v)
{
    cpu->a |= v;
    set_z(cpu, cpu->a == 0);
    set_n(cpu, 0);
    set_h(cpu, 0);
    set_c(cpu, 0);
}

void alu_cp(gb_cpu_t *cpu, uint8_t a, uint8_t b)
{
    set_z(cpu, a == b);
    set_n(cpu, 1);
    set_h(cpu, (int)(a & 0xF) - (int)(b & 0xF) < 0);
    set_c(cpu, a < b);
}

int gb_cpu_step(gb_cpu_t *cpu, struct gb_mem *mem)
{
    gb_mem_t *m = (gb_mem_t *)mem;

    /* EI: IME se activa después de la siguiente instrucción */
    if (cpu->ime_delay)
    {
        cpu->ime_delay = 0;
        cpu->ime = 1;
    }

    /* Interrupt check: si IME y hay pending, despachar */
    if (cpu->ime)
    {
        int cycles = dispatch_interrupt(cpu, m);

        if (cycles)
            return cycles;
    }

    /* HALT: si no hay interrupción pendiente, consumir 4 ciclos y no avanzar PC */
    if (cpu->halted)
    {
        if ((m->io[GB_IO_IF] & m->ie & GB_IF_IE_MASK))
            cpu->halted = 0;
        else
            return 4;
    }

    uint8_t op = gb_mem_read(m, cpu->pc++);
    if (cpu->halt_bug)
    {
        cpu->halt_bug = 0;
        cpu->pc--;
    }
    if (op == 0xCB) {
        uint8_t cb = gb_mem_read(m, cpu->pc++);
        return gb_cb_handlers[cb >> 3](cpu, m, cb);
    }
    return gb_op_handlers[op](cpu, m, op);
}
