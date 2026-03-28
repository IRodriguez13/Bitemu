/**
 * BE2 - Sega Genesis: implementación CPU 68000
 *
 * Fetch-decode-execute. Reset vector desde ROM.
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "cpu.h"
#include "cpu_internal.h"
#include "../genesis_constants.h"
#include "../vdp/vdp.h"
#include <string.h>

/* Handlers externos */
int gen_op_nop(gen_cpu_t *, genesis_mem_t *);
int gen_op_rts(gen_cpu_t *, genesis_mem_t *);
int gen_op_rte(gen_cpu_t *, genesis_mem_t *);
int gen_op_rtd(gen_cpu_t *, genesis_mem_t *);
int gen_op_trapv(gen_cpu_t *, genesis_mem_t *);
int gen_op_moveq(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_bra(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_bsr(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_bcc(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_move(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_lea(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_jmp(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_jsr(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_add(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_cmp(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_dbcc(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_sub(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_and(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_or(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_eor(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_clr(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_tst(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_trap(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_rtr(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_lsl(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_lsr(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_asl(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_asr(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_ror(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_rol(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_roxr(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_roxl(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_movem(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_addq_subq(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_ext(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_swap(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_neg(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_not(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_exg(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_pea(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_link(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_unlk(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_scc(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_adda(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_suba(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_cmpa(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_imm(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_bit(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_mulu(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_muls(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_divu(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_divs(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_addx_subx(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_negx(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_tas(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_move_sr_ccr(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_chk(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_reset(gen_cpu_t *, genesis_mem_t *);
int gen_op_stop(gen_cpu_t *, genesis_mem_t *, uint16_t);
int gen_op_illegal(gen_cpu_t *, genesis_mem_t *);

void gen_cpu_init(gen_cpu_t *cpu)
{
    memset(cpu, 0, sizeof(*cpu));
}

void gen_cpu_reset(gen_cpu_t *cpu, genesis_mem_t *mem)
{
    if (mem && mem->rom && mem->rom_size >= GEN_VECTOR_PC + 4)
    {
        cpu->a[7] = genesis_mem_read32(mem, GEN_VECTOR_SSP);
        cpu->pc = genesis_mem_read32(mem, GEN_VECTOR_PC);
    }
    else
    {
        cpu->pc = GEN_CPU_PC_RESET;
        cpu->a[7] = GEN_CPU_SP_INIT;
    }
    cpu->sr = GEN_CPU_SR_INIT;
    cpu->cycles = 0;
    cpu->stopped = 0;
}

int gen_cpu_stopped(const gen_cpu_t *cpu)
{
    return cpu ? cpu->stopped : 0;
}

/* Toma interrupción: retorna ciclos consumidos (0 si no hubo) */
static int gen_cpu_take_interrupt(gen_cpu_t *cpu, genesis_mem_t *mem)
{
    if (!mem->vdp)
        return 0;
    int level = gen_vdp_pending_irq_level(mem->vdp);
    if (level == 0)
        return 0;
    int mask = (cpu->sr & GEN_SR_IMASK_MASK) >> GEN_SR_IMASK_SHIFT;
    if (level <= mask)
        return 0;

    uint16_t vector;
    if (level == GEN_IRQ_LEVEL_VBLANK)
        vector = GEN_VECTOR_VBLANK;
    else
        vector = GEN_VECTOR_HBLANK;

    uint32_t vec_addr = (uint32_t)vector * 4;
    uint32_t new_pc = genesis_mem_read32(mem, vec_addr);

    cpu->a[7] -= 4;
    genesis_mem_write32(mem, cpu->a[7], cpu->pc);
    cpu->a[7] -= 2;
    genesis_mem_write16(mem, cpu->a[7], cpu->sr);

    cpu->pc = new_pc;
    cpu->sr = (cpu->sr & ~GEN_SR_IMASK_MASK) | (level << GEN_SR_IMASK_SHIFT);
    cpu->sr |= 0x2000;  /* Supervisor mode */
    cpu->stopped = 0;   /* STOP despertado por IRQ */

    return 44;  /* ~44 ciclos para ack + stack + fetch */
}

int gen_cpu_step(gen_cpu_t *cpu, genesis_mem_t *mem, int max_cycles)
{
    if (!mem || max_cycles <= 0)
        return 0;
    if (cpu->stopped)
        return 0;

    int irq_cycles = gen_cpu_take_interrupt(cpu, mem);
    if (irq_cycles > 0)
    {
        cpu->cycles = irq_cycles;
        return irq_cycles;
    }

    uint16_t op = genesis_mem_read16(mem, cpu->pc);
    cpu->pc += 2;
    int cycles = 4;

    switch (op >> 12)
    {
    case 0:
        if ((op & 0xF0C0) == GEN_OP_DBCC && (op & 0x00C0) == 0x00C0)
            cycles = gen_op_dbcc(cpu, mem, op);
        else if ((op & GEN_OP_SCC_MASK) == GEN_OP_SCC && (op & 0x00C0) != 0x00C0)
            cycles = gen_op_scc(cpu, mem, op);
        else if ((op & 0xFFC0) == 0x42C0)
            cycles = gen_op_clr(cpu, mem, op);
        else if ((op & GEN_OP_IMM_MASK) == GEN_OP_ORI || (op & GEN_OP_IMM_MASK) == GEN_OP_ANDI
              || (op & GEN_OP_IMM_MASK) == GEN_OP_SUBI || (op & GEN_OP_IMM_MASK) == GEN_OP_ADDI
              || (op & GEN_OP_IMM_MASK) == GEN_OP_EORI || (op & GEN_OP_IMM_MASK) == GEN_OP_CMPI)
            cycles = gen_op_imm(cpu, mem, op);
        else if ((op & 0xFF00) == 0x0800 || (op & 0xFF00) == 0x0900)
            cycles = gen_op_bit(cpu, mem, op);
        else
            cycles = GEN_CYCLES_NOP;
        break;
    case 1:  /* MOVE.B */
        cycles = gen_op_move(cpu, mem, op);
        break;
    case 2:  /* MOVE.L */
        cycles = gen_op_move(cpu, mem, op);
        break;
    case 3:  /* MOVE.W */
        cycles = gen_op_move(cpu, mem, op);
        break;
    case 4:  /* LEA, JMP, JSR, TST, TRAP, NOP, RTS, RTE, RTR, EXT, SWAP, NEG, NOT, MOVEM */
        if ((op & GEN_OP_EXT_MASK) == GEN_OP_EXT_W || (op & GEN_OP_EXT_MASK) == GEN_OP_EXT_L)
            cycles = gen_op_ext(cpu, mem, op);
        else if ((op & GEN_OP_SWAP_MASK) == GEN_OP_SWAP)
            cycles = gen_op_swap(cpu, mem, op);
        else if ((op & GEN_OP_NEG_MASK) == GEN_OP_NEG)
            cycles = gen_op_neg(cpu, mem, op);
        else if ((op & GEN_OP_NOT_MASK) == GEN_OP_NOT)
            cycles = gen_op_not(cpu, mem, op);
        else if ((op & GEN_OP_MOVEA_MASK) == GEN_OP_LEA)
            cycles = gen_op_lea(cpu, mem, op);
        else if ((op & GEN_OP_MOVEA_MASK) == GEN_OP_JMP)
            cycles = gen_op_jmp(cpu, mem, op);
        else if ((op & GEN_OP_MOVEA_MASK) == GEN_OP_JSR)
            cycles = gen_op_jsr(cpu, mem, op);
        else if ((op & GEN_OP_TST_MASK) == GEN_OP_TST)
            cycles = gen_op_tst(cpu, mem, op);
        else if ((op & GEN_OP_TRAP_MASK) == GEN_OP_TRAP)
            cycles = gen_op_trap(cpu, mem, op);
        else if (op == GEN_OP_NOP)
            cycles = gen_op_nop(cpu, mem);
        else if (op == GEN_OP_RESET)
            cycles = gen_op_reset(cpu, mem);
        else if (op == GEN_OP_STOP)
            cycles = gen_op_stop(cpu, mem, op);
        else if (op == GEN_OP_RTS)
            cycles = gen_op_rts(cpu, mem);
        else if (op == GEN_OP_RTE)
            cycles = gen_op_rte(cpu, mem);
        else if (op == GEN_OP_RTD)
            cycles = gen_op_rtd(cpu, mem);
        else if (op == GEN_OP_TRAPV)
            cycles = gen_op_trapv(cpu, mem);
        else if (op == GEN_OP_RTR)
            cycles = gen_op_rtr(cpu, mem, op);
        else if ((op & GEN_OP_MOVEM_MASK) == GEN_OP_MOVEM_STORE || (op & GEN_OP_MOVEM_MASK) == GEN_OP_MOVEM_LOAD)
            cycles = gen_op_movem(cpu, mem, op);
        else if ((op & GEN_OP_PEA_MASK) == GEN_OP_PEA && (op & 0x0038) != 0x0020)
            cycles = gen_op_pea(cpu, mem, op);
        else if ((op & GEN_OP_LINK_MASK) == GEN_OP_LINK)
            cycles = gen_op_link(cpu, mem, op);
        else if ((op & GEN_OP_UNLK_MASK) == GEN_OP_UNLK)
            cycles = gen_op_unlk(cpu, mem, op);
        else if ((op & GEN_OP_TAS_MASK) == GEN_OP_TAS)
            cycles = gen_op_tas(cpu, mem, op);
        else if ((op & 0xFFC0) == GEN_OP_MOVE_FROM_SR || (op & 0xFFC0) == GEN_OP_MOVE_TO_CCR
              || (op & 0xFFC0) == GEN_OP_MOVE_TO_SR)
            cycles = gen_op_move_sr_ccr(cpu, mem, op);
        else if ((op & GEN_OP_CHK_MASK) == GEN_OP_CHK)
            cycles = gen_op_chk(cpu, mem, op);
        else if (op == GEN_OP_ILLEGAL)
            cycles = gen_op_illegal(cpu, mem);
        else if ((op & GEN_OP_NEGX_MASK) == GEN_OP_NEGX)
            cycles = gen_op_negx(cpu, mem, op);
        else
            cycles = GEN_CYCLES_NOP;
        break;
    case 5:  /* DBcc (0101 .... 11001nnn) comparte prefijo 0101 con SUBQ; probar DBcc antes */
        if ((op & 0xF0C0) == GEN_OP_DBCC && (op & 0x00F8) == 0x00C8)
            cycles = gen_op_dbcc(cpu, mem, op);
        else if ((op & GEN_OP_ADDQ_SUBQ_MASK) == GEN_OP_ADDQ || (op & GEN_OP_ADDQ_SUBQ_MASK) == GEN_OP_SUBQ)
            cycles = gen_op_addq_subq(cpu, mem, op);
        else
            cycles = GEN_CYCLES_NOP;
        break;
    case 6:  /* BRA, BSR, Bcc */
        if ((op & 0xFF00) == GEN_OP_BRA)
            cycles = gen_op_bra(cpu, mem, op);
        else if ((op & 0xFF00) == 0x6100)
            cycles = gen_op_bsr(cpu, mem, op);
        else if ((op & 0xF000) == 0x6000)
            cycles = gen_op_bcc(cpu, mem, op);
        else
            cycles = GEN_CYCLES_NOP;
        break;
    case 7:  /* MOVEQ */
        if ((op & GEN_OP_MOVEQ_MASK) == GEN_OP_MOVEQ)
            cycles = gen_op_moveq(cpu, mem, op);
        else
            cycles = GEN_CYCLES_NOP;
        break;
    case 8:  /* OR, DIVU, DIVS */
        if ((op & GEN_OP_DIV_MASK) == GEN_OP_DIVU)
            cycles = gen_op_divu(cpu, mem, op);
        else if ((op & GEN_OP_DIV_MASK) == GEN_OP_DIVS)
            cycles = gen_op_divs(cpu, mem, op);
        else if ((op & 0xF000) == 0x8000)
            cycles = gen_op_or(cpu, mem, op);
        else
            cycles = GEN_CYCLES_NOP;
        break;
    case 9:  /* SUB, SUBA, SUBX */
        if (((op >> 6) & 0x07) == 3 || ((op >> 6) & 0x07) == 7)
            cycles = gen_op_suba(cpu, mem, op);
        else if ((op & 0xF100) == GEN_OP_SUBX)
            cycles = gen_op_addx_subx(cpu, mem, op);
        else if ((op & 0xF100) == 0x9100)
            cycles = gen_op_sub(cpu, mem, op);
        else
            cycles = GEN_CYCLES_NOP;
        break;
    case 11:  /* CMP, CMPA, EOR */
        if (((op >> 6) & 0x07) == 3 || ((op >> 6) & 0x07) == 7)
            cycles = gen_op_cmpa(cpu, mem, op);
        else if ((op & 0xF100) == 0xB000)
            cycles = gen_op_cmp(cpu, mem, op);
        else if ((op & 0xF100) == 0xB100)
            cycles = gen_op_eor(cpu, mem, op);
        else
            cycles = GEN_CYCLES_NOP;
        break;
    case 13:  /* ADD, ADDA, ADDX */
        if (((op >> 6) & 0x07) == 3 || ((op >> 6) & 0x07) == 7)
            cycles = gen_op_adda(cpu, mem, op);
        else if ((op & 0xF100) == GEN_OP_ADDX)
            cycles = gen_op_addx_subx(cpu, mem, op);
        else if ((op & 0xF100) == 0xD000)
            cycles = gen_op_add(cpu, mem, op);
        else
            cycles = GEN_CYCLES_NOP;
        break;
    case 12:  /* AND, EXG, MULU, MULS */
        if ((op & GEN_OP_EXG_MASK) == GEN_OP_EXG
              && ((op & GEN_OP_EXG_OPS) == 0x08 || (op & GEN_OP_EXG_OPS) == 0x48 || (op & GEN_OP_EXG_OPS) == 0x88))
            cycles = gen_op_exg(cpu, mem, op);
        else if ((op & GEN_OP_MUL_MASK) == GEN_OP_MULU)
            cycles = gen_op_mulu(cpu, mem, op);
        else if ((op & GEN_OP_MUL_MASK) == GEN_OP_MULS)
            cycles = gen_op_muls(cpu, mem, op);
        else if ((op & 0xF000) == 0xC000)
            cycles = gen_op_and(cpu, mem, op);
        else
            cycles = GEN_CYCLES_NOP;
        break;
    case 14:  /* 0xE0xx: ASR, LSR, ASL, LSL, ROR, ROL, ROXR, ROXL */
        if ((op & GEN_OP_SHIFT_MASK) == GEN_OP_LSL)
            cycles = gen_op_lsl(cpu, mem, op);
        else if ((op & GEN_OP_SHIFT_MASK) == GEN_OP_LSR)
            cycles = gen_op_lsr(cpu, mem, op);
        else if ((op & GEN_OP_SHIFT_MASK) == GEN_OP_ASL)
            cycles = gen_op_asl(cpu, mem, op);
        else if ((op & GEN_OP_SHIFT_MASK) == GEN_OP_ASR)
            cycles = gen_op_asr(cpu, mem, op);
        else if ((op & GEN_OP_SHIFT_MASK) == GEN_OP_ROR)
            cycles = gen_op_ror(cpu, mem, op);
        else if ((op & GEN_OP_SHIFT_MASK) == GEN_OP_ROL)
            cycles = gen_op_rol(cpu, mem, op);
        else if ((op & GEN_OP_SHIFT_MASK) == GEN_OP_ROXR)
            cycles = gen_op_roxr(cpu, mem, op);
        else if ((op & GEN_OP_SHIFT_MASK) == GEN_OP_ROXL)
            cycles = gen_op_roxl(cpu, mem, op);
        else
            cycles = GEN_CYCLES_NOP;
        break;
    case 10:  /* LINE A (0xAxxx): no instrucción válida en 68000 base → excepción */
        cycles = gen_op_illegal(cpu, mem);
        break;
    case 15:  /* LINE F (0xFxxx): reservado / coprocesador → ilegal en 68000 */
        cycles = gen_op_illegal(cpu, mem);
        break;
    default:
        cycles = GEN_CYCLES_NOP;
        break;
    }

    cpu->cycles = cycles;
    return cycles;
}
