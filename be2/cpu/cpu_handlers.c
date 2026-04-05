/**
 * BE2 - Sega Genesis: CPU 68000 opcode handlers
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "cpu_internal.h"
#include "../genesis_constants.h"
#include <string.h>

static void gen_take_trap(gen_cpu_t *, genesis_mem_t *, int, uint32_t);
int gen_op_illegal(gen_cpu_t *, genesis_mem_t *);

static void gen_cpu_exception_jump(gen_cpu_t *cpu, genesis_mem_t *mem, int vector, uint32_t pc_val)
{
    cpu->a[7] -= 2;
    genesis_mem_write16(mem, cpu->a[7], cpu->sr);
    cpu->a[7] -= 4;
    genesis_mem_write32(mem, cpu->a[7], pc_val);
    cpu->sr |= 0x2000;
    cpu->sr &= ~0x8000;
    cpu->pc = genesis_mem_read32(mem, (unsigned)vector * 4u);
}

void gen_cpu_address_error(gen_cpu_t *cpu, genesis_mem_t *mem, uint32_t pc_stacked)
{
    gen_cpu_exception_jump(cpu, mem, GEN_VECTOR_ADDRESS_ERROR, pc_stacked);
    cpu->cycle_override = GEN_CYCLES_ADDRESS_ERROR;
}

static void gen_cpu_sp_require_even(gen_cpu_t *cpu, genesis_mem_t *mem)
{
    if (cpu->a[7] & 1u)
        gen_cpu_address_error(cpu, mem, cpu->inst_pc);
}

static void gen_mem_ea_align(gen_cpu_t *cpu, genesis_mem_t *mem, uint32_t addr, int size)
{
    if (size != 0 && (addr & 1u))
        gen_cpu_address_error(cpu, mem, cpu->inst_pc);
}

static uint16_t fetch16(gen_cpu_t *cpu, genesis_mem_t *mem)
{
    if (cpu->pc & 1u)
        gen_cpu_address_error(cpu, mem, cpu->inst_pc);
    uint16_t w = genesis_mem_read16(mem, cpu->pc);
    cpu->pc += 2;
    return w;
}

/* EA: modo 0-2 = Dn, An, (An); 3 = (An)+; 4 = -(An); 5 = (d16,An); 7.0 = (d16); 7.1 = (d32); 7.4 = #imm */
uint32_t gen_ea_read(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t ea, int size, uint32_t *out_addr)
{
    int mode = (ea >> 3) & 7;
    int reg = ea & 7;
    uint32_t addr;

    switch (mode)
    {
    case 0:  /* Dn */
        if (out_addr) *out_addr = 0;
        if (size == 0) return cpu->d[reg] & 0xFF;
        if (size == 1) return cpu->d[reg] & 0xFFFF;
        return cpu->d[reg];
    case 1:  /* An */
        if (out_addr) *out_addr = 0;
        return cpu->a[reg];
    case 2:  /* (An) */
        addr = cpu->a[reg];
        if (out_addr) *out_addr = addr;
        if (size) gen_mem_ea_align(cpu, mem, addr, size);
        if (size == 0) return genesis_mem_read8(mem, addr);
        if (size == 1) return genesis_mem_read16(mem, addr);
        return genesis_mem_read32(mem, addr);
    case 3:  /* (An)+ — alinear antes de tocar An; incremento solo si lectura ok */
        addr = cpu->a[reg];
        if (out_addr) *out_addr = addr;
        if (size) gen_mem_ea_align(cpu, mem, addr, size);
        {
            uint32_t v;
            if (size == 0) v = genesis_mem_read8(mem, addr);
            else if (size == 1) v = genesis_mem_read16(mem, addr);
            else v = genesis_mem_read32(mem, addr);
            cpu->a[reg] += (size == 0) ? 1 : (size == 1) ? 2 : 4;
            return v;
        }
    case 4:  /* -(An) */
        cpu->a[reg] -= (size == 0) ? 1 : (size == 1) ? 2 : 4;
        addr = cpu->a[reg];
        if (out_addr) *out_addr = addr;
        if (size) gen_mem_ea_align(cpu, mem, addr, size);
        if (size == 0) return genesis_mem_read8(mem, addr);
        if (size == 1) return genesis_mem_read16(mem, addr);
        return genesis_mem_read32(mem, addr);
    case 5:  /* (d16,An) */
    {
        int16_t d = (int16_t)fetch16(cpu, mem);
        addr = cpu->a[reg] + (int32_t)d;
        if (out_addr) *out_addr = addr;
        if (size) gen_mem_ea_align(cpu, mem, addr, size);
        if (size == 0) return genesis_mem_read8(mem, addr);
        if (size == 1) return genesis_mem_read16(mem, addr);
        return genesis_mem_read32(mem, addr);
    }
    case 6:  /* (d8,An,Xn) - indexed */
    {
        uint16_t ext = fetch16(cpu, mem);
        int8_t disp = (int8_t)(ext & 0xFF);
        int idx_reg = (ext >> 9) & 7;
        int idx_is_a = (ext >> 15) & 1;
        int idx_long = (ext >> 8) & 1;
        uint32_t xn = idx_is_a ? cpu->a[idx_reg] : cpu->d[idx_reg];
        if (!idx_long)
            xn = (uint32_t)(int32_t)(int16_t)(xn & 0xFFFF);
        addr = cpu->a[reg] + disp + xn;
        if (out_addr) *out_addr = addr;
        if (size) gen_mem_ea_align(cpu, mem, addr, size);
        if (size == 0) return genesis_mem_read8(mem, addr);
        if (size == 1) return genesis_mem_read16(mem, addr);
        return genesis_mem_read32(mem, addr);
    }
    case 7:
        if (reg == 0)  /* (d16) */
        {
            addr = (uint32_t)(int32_t)(int16_t)fetch16(cpu, mem);
            if (out_addr) *out_addr = addr;
            if (size) gen_mem_ea_align(cpu, mem, addr, size);
            if (size == 0) return genesis_mem_read8(mem, addr);
            if (size == 1) return genesis_mem_read16(mem, addr);
            return genesis_mem_read32(mem, addr);
        }
        if (reg == 1)  /* (d32) */
        {
            gen_mem_ea_align(cpu, mem, cpu->pc, 2);
            addr = genesis_mem_read32(mem, cpu->pc);
            cpu->pc += 4;
            if (out_addr) *out_addr = addr;
            if (size) gen_mem_ea_align(cpu, mem, addr, size);
            if (size == 0) return genesis_mem_read8(mem, addr);
            if (size == 1) return genesis_mem_read16(mem, addr);
            return genesis_mem_read32(mem, addr);
        }
        if (reg == 2)  /* (d16,PC) */
        {
            uint32_t base = cpu->pc;
            int16_t d = (int16_t)fetch16(cpu, mem);
            addr = base + (int32_t)d;
            if (out_addr) *out_addr = addr;
            if (size) gen_mem_ea_align(cpu, mem, addr, size);
            if (size == 0) return genesis_mem_read8(mem, addr);
            if (size == 1) return genesis_mem_read16(mem, addr);
            return genesis_mem_read32(mem, addr);
        }
        if (reg == 3)  /* (d8,PC,Xn) */
        {
            uint16_t ext = fetch16(cpu, mem);
            int8_t disp = (int8_t)(ext & 0xFF);
            int idx_reg = (ext >> 9) & 7;
            int idx_is_a = (ext >> 15) & 1;
            int idx_long = (ext >> 8) & 1;
            uint32_t xn = idx_is_a ? cpu->a[idx_reg] : cpu->d[idx_reg];
            if (!idx_long)
                xn = (uint32_t)(int32_t)(int16_t)(xn & 0xFFFF);
            uint32_t base = cpu->pc;
            addr = base + disp + xn;
            if (out_addr) *out_addr = addr;
            if (size) gen_mem_ea_align(cpu, mem, addr, size);
            if (size == 0) return genesis_mem_read8(mem, addr);
            if (size == 1) return genesis_mem_read16(mem, addr);
            return genesis_mem_read32(mem, addr);
        }
        if (reg == 4)  /* #imm */
        {
            if (size == 0) { uint8_t v = (uint8_t)fetch16(cpu, mem); return v; }
            if (size == 1) return fetch16(cpu, mem);
            return (fetch16(cpu, mem) << 16) | fetch16(cpu, mem);
        }
        break;
    default:
        break;
    }
    return 0;
}

uint32_t gen_ea_addr(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t ea, int size)
{
    int mode = (ea >> 3) & 7;
    int reg = ea & 7;
    switch (mode)
    {
    case 0: case 1: return 0;
    case 2: return cpu->a[reg];
    case 3:
    {
        uint32_t a = cpu->a[reg];
        if (size) gen_mem_ea_align(cpu, mem, a, size);
        cpu->a[reg] += (size == 0) ? 1 : (size == 1) ? 2 : 4;
        return a;
    }
    case 4: { cpu->a[reg] -= (size==0)?1:(size==1)?2:4; return cpu->a[reg]; }
    case 5: return cpu->a[reg] + (int32_t)(int16_t)fetch16(cpu, mem);
    case 6:
    {
        uint16_t ext = fetch16(cpu, mem);
        int8_t disp = (int8_t)(ext & 0xFF);
        int idx_reg = (ext >> 9) & 7;
        int idx_is_a = (ext >> 15) & 1;
        int idx_long = (ext >> 8) & 1;
        uint32_t xn = idx_is_a ? cpu->a[idx_reg] : cpu->d[idx_reg];
        if (!idx_long)
            xn = (uint32_t)(int32_t)(int16_t)(xn & 0xFFFF);
        return cpu->a[reg] + disp + xn;
    }
    case 7:
        if (reg == 0) return (uint32_t)(int32_t)(int16_t)fetch16(cpu, mem);
        if (reg == 1)
        {
            gen_mem_ea_align(cpu, mem, cpu->pc, 2);
            uint32_t a = genesis_mem_read32(mem, cpu->pc);
            cpu->pc += 4;
            return a;
        }
        if (reg == 2) return cpu->pc + (int32_t)(int16_t)fetch16(cpu, mem);
        if (reg == 3)
        {
            uint16_t ext = fetch16(cpu, mem);
            int8_t disp = (int8_t)(ext & 0xFF);
            int idx_reg = (ext >> 9) & 7;
            int idx_is_a = (ext >> 15) & 1;
            int idx_long = (ext >> 8) & 1;
            uint32_t xn = idx_is_a ? cpu->a[idx_reg] : cpu->d[idx_reg];
            if (!idx_long)
                xn = (uint32_t)(int32_t)(int16_t)(xn & 0xFFFF);
            return cpu->pc + disp + xn;
        }
        break;
    }
    return 0;
}

void gen_ea_write(gen_cpu_t *cpu, genesis_mem_t *mem, uint32_t addr, uint32_t val, int size)
{
    if (addr == 0) return;
    if (size) gen_mem_ea_align(cpu, mem, addr, size);
    if (size == 0) genesis_mem_write8(mem, addr, (uint8_t)val);
    else if (size == 1) genesis_mem_write16(mem, addr, (uint16_t)val);
    else genesis_mem_write32(mem, addr, val);
}

/* Branch condition: 0=RA, 1=RN, 2=HI, 3=LS, 4=CC, 5=CS, 6=NE, 7=EQ, 8=VC, 9=VS, 10=PL, 11=MI, 12=GE, 13=LT, 14=GT, 15=LE */
static int branch_taken(gen_cpu_t *cpu, int cond)
{
    int n = flag_n(cpu), z = flag_z(cpu), v = flag_v(cpu), c = flag_c(cpu);
    switch (cond)
    {
    case 0:  return 1;   /* RA */
    case 1:  return 0;   /* RN */
    case 2:  return !c && !z;  /* HI */
    case 3:  return c || z;    /* LS */
    case 4:  return !c;       /* CC */
    case 5:  return c;        /* CS */
    case 6:  return !z;      /* NE */
    case 7:  return z;       /* EQ */
    case 8:  return !v;      /* VC */
    case 9:  return v;       /* VS */
    case 10: return !n;      /* PL */
    case 11: return n;       /* MI */
    case 12: return (n && v) || (!n && !v);  /* GE */
    case 13: return (n && !v) || (!n && v);  /* LT */
    case 14: return (!z) && ((n && v) || (!n && !v));  /* GT */
    case 15: return z || (n && !v) || (!n && v);  /* LE */
    }
    return 0;
}

int gen_op_nop(gen_cpu_t *cpu, genesis_mem_t *mem) { (void)cpu;(void)mem; return 4; }
int gen_op_rts(gen_cpu_t *cpu, genesis_mem_t *mem)
{
    gen_cpu_sp_require_even(cpu, mem);
    cpu->pc = genesis_mem_read32(mem, cpu->a[7]);
    cpu->a[7] += 4;
    return 16;
}
int gen_op_rte(gen_cpu_t *cpu, genesis_mem_t *mem)
{
    /* Format 0: [PC][SR], a[7] points to SR. Pop SR, then PC. */
    gen_cpu_sp_require_even(cpu, mem);
    cpu->sr = genesis_mem_read16(mem, cpu->a[7]);
    cpu->a[7] += 2;
    gen_cpu_sp_require_even(cpu, mem);
    cpu->pc = genesis_mem_read32(mem, cpu->a[7]);
    cpu->a[7] += 4;
    return 20;
}

/* RTD: pop PC, SP += imm (16-bit) */
int gen_op_rtd(gen_cpu_t *cpu, genesis_mem_t *mem)
{
    int16_t imm = (int16_t)fetch16(cpu, mem);
    gen_cpu_sp_require_even(cpu, mem);
    cpu->pc = genesis_mem_read32(mem, cpu->a[7]);
    cpu->a[7] += 4;
    cpu->a[7] += (int32_t)imm;
    return 16;
}

/* TRAPV: trap #7 if V=1 */
int gen_op_trapv(gen_cpu_t *cpu, genesis_mem_t *mem)
{
    if (flag_v(cpu))
        gen_take_trap(cpu, mem, 39, cpu->pc);
    return 4;
}

int gen_op_moveq(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int reg = (op >> 9) & 7;
    cpu->d[reg] = (uint32_t)(int32_t)(int8_t)(op & 0xFF);
    set_n(cpu, (cpu->d[reg] & 0x80000000) != 0);
    set_z(cpu, cpu->d[reg] == 0);
    set_v(cpu, 0);
    set_c(cpu, 0);
    (void)mem;
    return 4;
}

int gen_op_bra(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int8_t d = (int8_t)(op & 0xFF);
    if (d == 0) { int16_t ext = (int16_t)fetch16(cpu, mem); cpu->pc += ext; }
    else cpu->pc += d;
    (void)mem;
    return 10;
}

int gen_op_bsr(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int8_t d = (int8_t)(op & 0xFF);
    if (d == 0)
    {
        int16_t ext = (int16_t)fetch16(cpu, mem);
        uint32_t link = cpu->pc;
        gen_cpu_sp_require_even(cpu, mem);
        cpu->a[7] -= 4;
        genesis_mem_write32(mem, cpu->a[7], link);
        cpu->pc = link + (uint32_t)(int32_t)ext;
    }
    else
    {
        uint32_t link = cpu->pc;
        gen_cpu_sp_require_even(cpu, mem);
        cpu->a[7] -= 4;
        genesis_mem_write32(mem, cpu->a[7], link);
        cpu->pc = link + (uint32_t)(int32_t)d;
    }
    return 18;
}

int gen_op_bcc(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int cond = (op >> 8) & 15;
    int8_t d = (int8_t)(op & 0xFF);
    int taken = branch_taken(cpu, cond);
    if (d == 0) { int16_t ext = (int16_t)fetch16(cpu, mem); if (taken) cpu->pc += ext; }
    else if (taken) cpu->pc += d;
    return taken ? 10 : 8;
}

int gen_op_move(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int sz = (op >> 12) & 3;  /* 01=B, 10=L, 11=W */
    int size = (sz == 1) ? 0 : (sz == 2) ? 2 : 1;

    uint32_t src = gen_ea_read(cpu, mem, op & 0x3F, size, NULL);
    if (cpu->cycle_override)
        return 4;
    uint16_t dst_ea = (op >> 6) & 0x3F;
    int dst_mode = (dst_ea >> 3) & 7;
    int dst_reg = dst_ea & 7;

    if (dst_mode == 0)
        cpu->d[dst_reg] = (size == 0) ? (cpu->d[dst_reg] & 0xFFFFFF00) | (src & 0xFF) : (size == 1) ? (cpu->d[dst_reg] & 0xFFFF0000) | (src & 0xFFFF) : src;
    else if (dst_mode == 1)
        cpu->a[dst_reg] = (size == 1) ? (uint32_t)(int32_t)(int16_t)(src & 0xFFFF) : src;
    else
    {
        uint32_t addr = gen_ea_addr(cpu, mem, dst_ea, size);
        gen_ea_write(cpu, mem, addr, src, size);
    }
    set_n(cpu, (size == 0 && (src & 0x80)) || (size == 1 && (src & 0x8000)) || (size == 2 && (src & 0x80000000)));
    set_z(cpu, (size == 0 && !(src & 0xFF)) || (size == 1 && !(src & 0xFFFF)) || (size == 2 && src == 0));
    set_v(cpu, 0);
    set_c(cpu, 0);
    return 4;
}

int gen_op_lea(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    uint32_t addr = gen_ea_addr(cpu, mem, op & 0x3F, 2);
    int reg = (op >> 9) & 7;
    cpu->a[reg] = addr;
    return 4;
}

int gen_op_jmp(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    cpu->pc = gen_ea_addr(cpu, mem, op & 0x3F, 2);
    return 8;
}

int gen_op_jsr(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    uint32_t addr = gen_ea_addr(cpu, mem, op & 0x3F, 2);
    gen_cpu_sp_require_even(cpu, mem);
    cpu->a[7] -= 4;
    genesis_mem_write32(mem, cpu->a[7], cpu->pc);
    cpu->pc = addr;
    return 16;
}

int gen_op_add(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int sz = (op >> 6) & 3;
    int size = (sz == 0) ? 0 : (sz == 1) ? 1 : 2;
    uint32_t src = gen_ea_read(cpu, mem, op & GEN_EA_MASK, size, NULL);
    int dst_reg = (op >> 9) & 7;
    uint32_t dst = (size == 0) ? (cpu->d[dst_reg] & 0xFF) : (size == 1) ? (cpu->d[dst_reg] & 0xFFFF) : cpu->d[dst_reg];
    uint32_t r = dst + src;
    uint32_t mask = (size == 0) ? 0xFFU : (size == 1) ? 0xFFFFU : 0xFFFFFFFFU;
    r &= mask;
    if (size == 0)
        cpu->d[dst_reg] = (cpu->d[dst_reg] & 0xFFFFFF00) | (r & 0xFF);
    else if (size == 1)
        cpu->d[dst_reg] = (cpu->d[dst_reg] & 0xFFFF0000) | (r & 0xFFFF);
    else
        cpu->d[dst_reg] = r;
    set_n(cpu, (r & (size == 0 ? 0x80U : size == 1 ? 0x8000U : 0x80000000U)) != 0);
    set_z(cpu, r == 0);
    set_v(cpu, ((dst ^ r) & (src ^ r)) & (size == 0 ? 0x80U : size == 1 ? 0x8000U : 0x80000000U));
    set_c(cpu, r < dst);
    return 4;
}

int gen_op_cmp(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int sz = (op >> 6) & 3;
    int size = (sz == 0) ? 0 : (sz == 1) ? 1 : 2;
    uint32_t src = gen_ea_read(cpu, mem, op & GEN_EA_MASK, size, NULL);
    int dst_reg = (op >> 9) & 7;
    uint32_t dst = (size == 0) ? (cpu->d[dst_reg] & 0xFF) : (size == 1) ? (cpu->d[dst_reg] & 0xFFFF) : cpu->d[dst_reg];
    uint32_t r = dst - src;
    uint32_t mask = (size == 0) ? 0xFFU : (size == 1) ? 0xFFFFU : 0xFFFFFFFFU;
    r &= mask;
    set_n(cpu, (r & (size == 0 ? 0x80U : size == 1 ? 0x8000U : 0x80000000U)) != 0);
    set_z(cpu, r == 0);
    set_v(cpu, ((dst ^ src) & (dst ^ r)) & (size == 0 ? 0x80U : size == 1 ? 0x8000U : 0x80000000U));
    set_c(cpu, src > dst);
    return 4;
}

int gen_op_dbcc(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int cond = (op >> 8) & 15;
    int reg = op & 7;
    uint16_t d = (uint16_t)cpu->d[reg];
    cpu->d[reg] = (cpu->d[reg] & 0xFFFF0000) | ((d - 1) & 0xFFFF);
    int taken = !branch_taken(cpu, cond) && (d != 0);
    int16_t disp = (int16_t)fetch16(cpu, mem);
    if (taken) cpu->pc += disp;
    return 10;
}

int gen_op_sub(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int sz = (op >> 6) & 3;
    int size = (sz == 0) ? 0 : (sz == 1) ? 1 : 2;
    uint32_t src = gen_ea_read(cpu, mem, op & GEN_EA_MASK, size, NULL);
    int dst_reg = (op >> 9) & 7;
    uint32_t dst = (size == 0) ? (cpu->d[dst_reg] & 0xFF) : (size == 1) ? (cpu->d[dst_reg] & 0xFFFF) : cpu->d[dst_reg];
    uint32_t r = dst - src;
    uint32_t mask = (size == 0) ? 0xFFU : (size == 1) ? 0xFFFFU : 0xFFFFFFFFU;
    r &= mask;
    if (size == 0)
        cpu->d[dst_reg] = (cpu->d[dst_reg] & 0xFFFFFF00) | (r & 0xFF);
    else if (size == 1)
        cpu->d[dst_reg] = (cpu->d[dst_reg] & 0xFFFF0000) | (r & 0xFFFF);
    else
        cpu->d[dst_reg] = r;
    set_n(cpu, (r & (size == 0 ? 0x80U : size == 1 ? 0x8000U : 0x80000000U)) != 0);
    set_z(cpu, r == 0);
    set_v(cpu, ((dst ^ src) & (dst ^ r)) & (size == 0 ? 0x80U : size == 1 ? 0x8000U : 0x80000000U));
    set_c(cpu, src > dst);
    return 4;
}

/* ABCD: solo .B; Dy,Dx o -(Ay),-(Ax). Z se limpia si resultado != 0; si es 0, Z no cambia. */
int gen_op_abcd(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int rx = (op >> 9) & 7;
    int ry = op & 7;
    int memform = ((op >> 3) & 1) != 0;
    uint16_t corf = flag_x(cpu) ? 1u : 0u;
    uint8_t srcb, dstb;

    if (!memform)
    {
        srcb = (uint8_t)(cpu->d[ry] & 0xFFu);
        dstb = (uint8_t)(cpu->d[rx] & 0xFFu);
    }
    else
    {
        cpu->a[ry]--;
        cpu->a[rx]--;
        srcb = genesis_mem_read8(mem, cpu->a[ry]);
        dstb = genesis_mem_read8(mem, cpu->a[rx]);
    }

    uint16_t i = (uint16_t)((uint16_t)(srcb & 0xFu) + (uint16_t)(dstb & 0xFu) + corf);
    if (i > 9u)
        i += 6u;
    uint16_t j = (uint16_t)((uint16_t)(srcb & 0xF0u) + (uint16_t)(dstb & 0xF0u) + (i & 0xF0u));
    if (j > 0x90u)
        j += 0x60u;
    uint16_t res16 = (uint16_t)((j & 0xFFu) + (i & 0x0Fu));
    uint8_t resb = (uint8_t)(res16 & 0xFFu);
    int c = (j > 0xFFu);
    set_x(cpu, c);
    set_c(cpu, c);
    set_n(cpu, (resb & 0x80u) != 0);
    if (resb != 0)
        set_z(cpu, 0);
    set_v(cpu, 0);

    if (!memform)
        cpu->d[rx] = (cpu->d[rx] & 0xFFFFFF00u) | resb;
    else
        genesis_mem_write8(mem, cpu->a[rx], resb);

    return memform ? GEN_CYCLES_ABCD_MEM : GEN_CYCLES_ABCD_REG;
}

/* BCD empaquetado: dst - src - x_in; borrow_out = préstamo fuera del byte (C/X). */
static void bitemu_bcd_sub_byte(uint8_t dstb, uint8_t srcb, unsigned x_in,
                                uint8_t *res_out, int *borrow_out)
{
    int d0 = (dstb >> 0) & 0xF;
    int s0 = (srcb >> 0) & 0xF;
    int lo = d0 - s0 - (int)x_in;
    int bh = 0;
    if (lo < 0)
    {
        lo += 10;
        bh = 1;
    }
    int d1 = (dstb >> 4) & 0xF;
    int s1 = (srcb >> 4) & 0xF;
    int hi = d1 - s1 - bh;
    int byte_borrow = 0;
    if (hi < 0)
    {
        hi += 10;
        byte_borrow = 1;
    }
    *res_out = (uint8_t)(((hi & 0xF) << 4) | (lo & 0xF));
    *borrow_out = byte_borrow;
}

/* SBCD: solo .B; Dy,Dx o -(Ay),-(Ax). Z como ABCD. */
int gen_op_sbcd(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int rx = (op >> 9) & 7;
    int ry = op & 7;
    int memform = ((op >> 3) & 1) != 0;
    unsigned xin = flag_x(cpu) ? 1u : 0u;
    uint8_t srcb, dstb;
    uint8_t resb;
    int borrow;

    if (!memform)
    {
        srcb = (uint8_t)(cpu->d[ry] & 0xFFu);
        dstb = (uint8_t)(cpu->d[rx] & 0xFFu);
    }
    else
    {
        cpu->a[ry]--;
        cpu->a[rx]--;
        srcb = genesis_mem_read8(mem, cpu->a[ry]);
        dstb = genesis_mem_read8(mem, cpu->a[rx]);
    }

    bitemu_bcd_sub_byte(dstb, srcb, xin, &resb, &borrow);
    set_x(cpu, borrow);
    set_c(cpu, borrow);
    set_n(cpu, (resb & 0x80u) != 0);
    set_v(cpu, 0);
    if (resb != 0)
        set_z(cpu, 0);

    if (!memform)
        cpu->d[rx] = (cpu->d[rx] & 0xFFFFFF00u) | resb;
    else
        genesis_mem_write8(mem, cpu->a[rx], resb);

    return memform ? GEN_CYCLES_ABCD_MEM : GEN_CYCLES_ABCD_REG;
}

/* NBCD: 0 - <ea>.B - X en BCD. Solo byte. */
int gen_op_nbcd(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    uint16_t ea = op & GEN_EA_MASK;
    int mode = (ea >> GEN_EA_MODE_SHIFT) & GEN_EA_REG_MASK;
    int regv = ea & GEN_EA_REG_MASK;
    unsigned xin = flag_x(cpu) ? 1u : 0u;
    uint8_t srcb;
    uint32_t addr = 0;

    if (mode == 0)
        srcb = (uint8_t)(cpu->d[regv] & 0xFFu);
    else
        srcb = (uint8_t)(gen_ea_read(cpu, mem, ea, 0, &addr) & 0xFFu);

    uint8_t resb;
    int borrow;
    bitemu_bcd_sub_byte(0, srcb, xin, &resb, &borrow);

    set_x(cpu, borrow);
    set_c(cpu, borrow);
    set_n(cpu, (resb & 0x80u) != 0);
    set_v(cpu, 0);
    if (resb != 0)
        set_z(cpu, 0);

    if (mode == 0)
        cpu->d[regv] = (cpu->d[regv] & 0xFFFFFF00u) | resb;
    else if (addr)
        gen_ea_write(cpu, mem, addr, resb, 0);

    return (mode == 0) ? GEN_CYCLES_ABCD_REG : GEN_CYCLES_NEG_NOT;
}

int gen_op_and(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int sz = (op >> 6) & 3;
    int size = (sz == 0) ? 0 : (sz == 1) ? 1 : 2;
    int reg = (op >> 9) & 7;
    uint32_t dn = (size == 0) ? (cpu->d[reg] & 0xFF) : (size == 1) ? (cpu->d[reg] & 0xFFFF) : cpu->d[reg];
    uint32_t r;
    uint32_t addr = 0;
    if ((op & 0x0100) == 0)
    {
        uint32_t src = gen_ea_read(cpu, mem, op & GEN_EA_MASK, size, NULL);
        r = dn & src;
        if (size == 0)
            cpu->d[reg] = (cpu->d[reg] & 0xFFFFFF00) | (r & 0xFF);
        else if (size == 1)
            cpu->d[reg] = (cpu->d[reg] & 0xFFFF0000) | (r & 0xFFFF);
        else
            cpu->d[reg] = r;
    }
    else
    {
        uint16_t ea = op & GEN_EA_MASK;
        int mode = (ea >> GEN_EA_MODE_SHIFT) & GEN_EA_REG_MASK;
        uint32_t dst = gen_ea_read(cpu, mem, ea, size, &addr);
        r = dst & dn;
        if (addr)
            gen_ea_write(cpu, mem, addr, r, size);
        else if (mode == 0)
        {
            int dest_reg = ea & 7;
            if (size == 0) cpu->d[dest_reg] = (cpu->d[dest_reg] & 0xFFFFFF00) | (r & 0xFF);
            else if (size == 1) cpu->d[dest_reg] = (cpu->d[dest_reg] & 0xFFFF0000) | (r & 0xFFFF);
            else cpu->d[dest_reg] = r;
        }
    }
    set_n(cpu, (r & (size == 0 ? 0x80U : size == 1 ? 0x8000U : 0x80000000U)) != 0);
    set_z(cpu, r == 0);
    set_v(cpu, 0);
    set_c(cpu, 0);
    return 4;
}

int gen_op_or(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int sz = (op >> 6) & 3;
    int size = (sz == 0) ? 0 : (sz == 1) ? 1 : 2;
    int reg = (op >> 9) & 7;
    uint32_t dn = (size == 0) ? (cpu->d[reg] & 0xFF) : (size == 1) ? (cpu->d[reg] & 0xFFFF) : cpu->d[reg];
    uint32_t r;
    uint32_t addr = 0;
    if ((op & 0x0100) == 0)
    {
        uint32_t src = gen_ea_read(cpu, mem, op & GEN_EA_MASK, size, NULL);
        r = dn | src;
        if (size == 0)
            cpu->d[reg] = (cpu->d[reg] & 0xFFFFFF00) | (r & 0xFF);
        else if (size == 1)
            cpu->d[reg] = (cpu->d[reg] & 0xFFFF0000) | (r & 0xFFFF);
        else
            cpu->d[reg] = r;
    }
    else
    {
        uint16_t ea = op & GEN_EA_MASK;
        int mode = (ea >> GEN_EA_MODE_SHIFT) & GEN_EA_REG_MASK;
        uint32_t dst = gen_ea_read(cpu, mem, ea, size, &addr);
        r = dst | dn;
        if (addr)
            gen_ea_write(cpu, mem, addr, r, size);
        else if (mode == 0)
        {
            int dest_reg = ea & 7;
            if (size == 0) cpu->d[dest_reg] = (cpu->d[dest_reg] & 0xFFFFFF00) | (r & 0xFF);
            else if (size == 1) cpu->d[dest_reg] = (cpu->d[dest_reg] & 0xFFFF0000) | (r & 0xFFFF);
            else cpu->d[dest_reg] = r;
        }
    }
    set_n(cpu, (r & (size == 0 ? 0x80U : size == 1 ? 0x8000U : 0x80000000U)) != 0);
    set_z(cpu, r == 0);
    set_v(cpu, 0);
    set_c(cpu, 0);
    return 4;
}

int gen_op_eor(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int sz = (op >> 6) & 3;
    int size = (sz == 0) ? 0 : (sz == 1) ? 1 : 2;
    uint32_t src = gen_ea_read(cpu, mem, op & GEN_EA_MASK, size, NULL);
    int dst_reg = (op >> 9) & 7;
    uint32_t dst = (size == 0) ? (cpu->d[dst_reg] & 0xFF) : (size == 1) ? (cpu->d[dst_reg] & 0xFFFF) : cpu->d[dst_reg];
    uint32_t r = dst ^ src;
    if (size == 0)
        cpu->d[dst_reg] = (cpu->d[dst_reg] & 0xFFFFFF00) | (r & 0xFF);
    else if (size == 1)
        cpu->d[dst_reg] = (cpu->d[dst_reg] & 0xFFFF0000) | (r & 0xFFFF);
    else
        cpu->d[dst_reg] = r;
    set_n(cpu, (r & (size == 0 ? 0x80U : size == 1 ? 0x8000U : 0x80000000U)) != 0);
    set_z(cpu, r == 0);
    set_v(cpu, 0);
    set_c(cpu, 0);
    return 4;
}

int gen_op_clr(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int sz = (op >> 6) & 3;
    int size = (sz == 0) ? 0 : (sz == 1) ? 1 : 2;
    uint16_t ea = op & GEN_EA_MASK;
    int mode = (ea >> GEN_EA_MODE_SHIFT) & GEN_EA_REG_MASK;
    int reg = ea & GEN_EA_REG_MASK;
    if (mode == 0)
    {
        if (size == 0)
            cpu->d[reg] = (cpu->d[reg] & 0xFFFFFF00);
        else if (size == 1)
            cpu->d[reg] = (cpu->d[reg] & 0xFFFF0000);
        else
            cpu->d[reg] = 0;
    }
    else
    {
        uint32_t addr = gen_ea_addr(cpu, mem, ea, size);
        gen_ea_write(cpu, mem, addr, 0, size);
    }
    set_n(cpu, 0);
    set_z(cpu, 1);
    set_v(cpu, 0);
    set_c(cpu, 0);
    return 4;
}

/* TST: test operand, set N/Z, clear V/C. size: 0=B, 1=W, 2=L */
int gen_op_tst(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int sz = (op >> 6) & 3;
    int size = (sz == 0) ? 0 : (sz == 1) ? 1 : 2;
    uint32_t val = gen_ea_read(cpu, mem, op & 0x3F, size, NULL);
    set_n(cpu, (size == 0 && (val & 0x80)) || (size == 1 && (val & 0x8000)) || (size == 2 && (val & 0x80000000)));
    set_z(cpu, (size == 0 && !(val & 0xFF)) || (size == 1 && !(val & 0xFFFF)) || (size == 2 && val == 0));
    set_v(cpu, 0);
    set_c(cpu, 0);
    return GEN_CYCLES_TST;
}

/* ADDQ/SUBQ: imm 1-8 (0=8). Size .B/.W/.L (bits 7-6); 11 → ilegal. An: siempre .L al registro. */
int gen_op_addq_subq(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int imm = (op >> GEN_ADDQ_IMM_SHIFT) & GEN_ADDQ_IMM_MASK;
    if (imm == 0)
        imm = 8;
    int is_sub = (op & GEN_OP_ADDQ_SUBQ_MASK) == GEN_OP_SUBQ;
    int sz = (op >> 6) & 3;
    if (sz == 3)
    {
        gen_take_trap(cpu, mem, GEN_VECTOR_ILLEGAL, cpu->pc - 2);
        return GEN_CYCLES_ILLEGAL;
    }
    int size = (sz == 0) ? 0 : (sz == 1) ? GEN_TST_SIZE_W : GEN_TST_SIZE_L;
    int ea = op & GEN_EA_MASK;
    int mode = (ea >> GEN_EA_MODE_SHIFT) & GEN_EA_REG_MASK;
    int reg = ea & GEN_EA_REG_MASK;
    int to_addr_reg = (mode == 1);

    if (to_addr_reg)
    {
        uint32_t val = cpu->a[reg];
        cpu->a[reg] = is_sub ? val - imm : val + imm;
        return GEN_CYCLES_ADDQ_SUBQ;
    }

    if (mode == 0)
    {
        uint32_t val = (size == 0) ? (cpu->d[reg] & 0xFF)
                     : (size == GEN_TST_SIZE_W) ? (cpu->d[reg] & 0xFFFF) : cpu->d[reg];
        uint32_t r = is_sub ? val - (uint32_t)imm : val + (uint32_t)imm;
        uint32_t msb = (size == 0) ? 0x80U : (size == GEN_TST_SIZE_W) ? 0x8000U : 0x80000000U;
        uint32_t mask = (size == 0) ? 0xFFU : (size == GEN_TST_SIZE_W) ? 0xFFFFU : 0xFFFFFFFFU;
        r &= mask;
        if (size == 0)
            cpu->d[reg] = (cpu->d[reg] & 0xFFFFFF00) | (r & 0xFF);
        else if (size == GEN_TST_SIZE_W)
            cpu->d[reg] = (cpu->d[reg] & 0xFFFF0000) | (r & 0xFFFF);
        else
            cpu->d[reg] = r;
        set_n(cpu, (r & msb) != 0);
        set_z(cpu, r == 0);
        set_v(cpu, ((val ^ r) & ((is_sub ? (uint32_t)(-(int)imm) : (uint32_t)imm) ^ r)) & msb);
        set_c(cpu, is_sub ? (val < (uint32_t)imm) : (r < val));
        return GEN_CYCLES_ADDQ_SUBQ;
    }

    uint32_t addr;
    uint32_t val = gen_ea_read(cpu, mem, ea, size, &addr);
    uint32_t r = is_sub ? val - imm : val + imm;
    uint32_t msb = (size == 0) ? 0x80U : (size == GEN_TST_SIZE_W) ? 0x8000U : 0x80000000U;
    uint32_t mask = (size == 0) ? 0xFFU : (size == GEN_TST_SIZE_W) ? 0xFFFFU : 0xFFFFFFFFU;
    r &= mask;
    if (addr)
        gen_ea_write(cpu, mem, addr, r, size);
    set_n(cpu, (r & msb) != 0);
    set_z(cpu, r == 0);
    set_v(cpu, ((val ^ r) & ((is_sub ? (uint32_t)imm : (uint32_t)(-(int)imm)) ^ r)) & msb);
    set_c(cpu, is_sub ? (val < (uint32_t)imm) : (r < val));
    return GEN_CYCLES_ADDQ_SUBQ;
}

/* EXT.W Dn: byte→word. EXT.L Dn: word→long (sign extend) */
int gen_op_ext(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int reg = op & GEN_EA_REG_MASK;
    if ((op & GEN_OP_EXT_MASK) == GEN_OP_EXT_L)
    {
        int32_t w = (int32_t)(int16_t)(cpu->d[reg] & 0xFFFF);
        cpu->d[reg] = (uint32_t)w;
        set_n(cpu, (w & 0x80000000) != 0);
        set_z(cpu, w == 0);
        set_v(cpu, 0);
        set_c(cpu, 0);
    }
    else
    {
        int16_t w = (int16_t)(int8_t)(cpu->d[reg] & 0xFF);
        cpu->d[reg] = (cpu->d[reg] & 0xFFFF0000) | ((uint16_t)w & 0xFFFF);
        set_n(cpu, (w & 0x8000) != 0);
        set_z(cpu, w == 0);
        set_v(cpu, 0);
        set_c(cpu, 0);
    }
    (void)mem;
    return GEN_CYCLES_EXT;
}

/* SWAP Dn: intercambiar mitades de 32 bits */
int gen_op_swap(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int reg = op & GEN_EA_REG_MASK;
    uint32_t v = cpu->d[reg];
    cpu->d[reg] = (v >> 16) | (v << 16);
    set_n(cpu, (cpu->d[reg] & 0x80000000) != 0);
    set_z(cpu, cpu->d[reg] == 0);
    set_v(cpu, 0);
    set_c(cpu, 0);
    (void)mem;
    return GEN_CYCLES_SWAP;
}

/* NEG: negate (0 - x). size: 0=B, 1=W, 2=L */
int gen_op_neg(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int sz = (op >> 6) & 3;
    int size = (sz == 0) ? 0 : (sz == 1) ? GEN_TST_SIZE_W : GEN_TST_SIZE_L;
    uint16_t ea = op & GEN_EA_MASK;
    int mode = (ea >> GEN_EA_MODE_SHIFT) & GEN_EA_REG_MASK;
    int reg = ea & GEN_EA_REG_MASK;
    uint32_t mask = (size == 0) ? 0xFFU : (size == GEN_TST_SIZE_W) ? 0xFFFFU : 0xFFFFFFFFU;
    uint32_t val, r;
    uint32_t addr = 0;

    if (mode == 0)
    {
        val = (size == 0) ? (cpu->d[reg] & 0xFF) : (size == GEN_TST_SIZE_W) ? (cpu->d[reg] & 0xFFFF) : cpu->d[reg];
        r = (0 - val) & mask;
        if (size == 0)
            cpu->d[reg] = (cpu->d[reg] & 0xFFFFFF00) | (r & 0xFF);
        else if (size == GEN_TST_SIZE_W)
            cpu->d[reg] = (cpu->d[reg] & 0xFFFF0000) | (r & 0xFFFF);
        else
            cpu->d[reg] = r;
    }
    else
    {
        val = gen_ea_read(cpu, mem, ea, size, &addr);
        r = (0 - val) & mask;
        if (addr)
            gen_ea_write(cpu, mem, addr, r, size);
    }
    set_n(cpu, (r & (size == 0 ? 0x80U : size == GEN_TST_SIZE_W ? 0x8000U : 0x80000000U)) != 0);
    set_z(cpu, r == 0);
    set_v(cpu, (val & r & (size == 0 ? 0x80U : size == GEN_TST_SIZE_W ? 0x8000U : 0x80000000U)) != 0);
    set_c(cpu, val != 0);
    return GEN_CYCLES_NEG_NOT;
}

/* NOT: logical complement. size: 0=B, 1=W, 2=L */
int gen_op_not(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int sz = (op >> 6) & 3;
    int size = (sz == 0) ? 0 : (sz == 1) ? GEN_TST_SIZE_W : GEN_TST_SIZE_L;
    uint16_t ea = op & GEN_EA_MASK;
    int mode = (ea >> GEN_EA_MODE_SHIFT) & GEN_EA_REG_MASK;
    int reg = ea & GEN_EA_REG_MASK;
    uint32_t r;

    if (mode == 0)
    {
        r = (size == 0) ? ~(cpu->d[reg] & 0xFF) : (size == GEN_TST_SIZE_W) ? ~(cpu->d[reg] & 0xFFFF) : ~cpu->d[reg];
        r &= (size == 0) ? 0xFFU : (size == GEN_TST_SIZE_W) ? 0xFFFFU : 0xFFFFFFFFU;
        if (size == 0)
            cpu->d[reg] = (cpu->d[reg] & 0xFFFFFF00) | (r & 0xFF);
        else if (size == GEN_TST_SIZE_W)
            cpu->d[reg] = (cpu->d[reg] & 0xFFFF0000) | (r & 0xFFFF);
        else
            cpu->d[reg] = r;
    }
    else
    {
        uint32_t addr;
        r = gen_ea_read(cpu, mem, ea, size, &addr);
        r = (size == 0) ? (~r & 0xFF) : (size == GEN_TST_SIZE_W) ? (~r & 0xFFFF) : ~r;
        if (addr)
            gen_ea_write(cpu, mem, addr, r, size);
    }
    set_n(cpu, (r & (size == 0 ? 0x80U : size == GEN_TST_SIZE_W ? 0x8000U : 0x80000000U)) != 0);
    set_z(cpu, r == 0);
    set_v(cpu, 0);
    set_c(cpu, 0);
    return GEN_CYCLES_NEG_NOT;
}

/* EXG Rx,Ry: exchange registers. 0xC108=D,D; 0xC148=A,A; 0xC188=D,A */
int gen_op_exg(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int rx = (op >> 9) & 7;
    int ry = op & 7;
    uint32_t tmp;
    if ((op & 0xF8) == 0x08)
    {
        tmp = cpu->d[rx];
        cpu->d[rx] = cpu->d[ry];
        cpu->d[ry] = tmp;
    }
    else if ((op & 0xF8) == 0x48)
    {
        tmp = cpu->a[rx];
        cpu->a[rx] = cpu->a[ry];
        cpu->a[ry] = tmp;
    }
    else
    {
        tmp = cpu->d[rx];
        cpu->d[rx] = cpu->a[ry];
        cpu->a[ry] = tmp;
    }
    (void)mem;
    return GEN_CYCLES_EXG;
}

/* PEA: push effective address. Excluye SWAP (bits 5-3=100). */
int gen_op_pea(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    uint32_t addr = gen_ea_addr(cpu, mem, op & GEN_EA_MASK, GEN_TST_SIZE_L);
    gen_cpu_sp_require_even(cpu, mem);
    cpu->a[7] -= 4;
    genesis_mem_write32(mem, cpu->a[7], addr);
    return GEN_CYCLES_PEA;
}

/* LINK An,#imm: push An, An=SP, SP+=imm */
int gen_op_link(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int reg = op & GEN_EA_REG_MASK;
    int16_t imm = (int16_t)fetch16(cpu, mem);
    gen_cpu_sp_require_even(cpu, mem);
    cpu->a[7] -= 4;
    genesis_mem_write32(mem, cpu->a[7], cpu->a[reg]);
    cpu->a[reg] = cpu->a[7];
    cpu->a[7] += imm;
    return GEN_CYCLES_LINK;
}

/* UNLK An: SP=An, pop An */
int gen_op_unlk(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int reg = op & GEN_EA_REG_MASK;
    cpu->a[7] = cpu->a[reg];
    gen_cpu_sp_require_even(cpu, mem);
    cpu->a[reg] = genesis_mem_read32(mem, cpu->a[7]);
    cpu->a[7] += 4;
    return GEN_CYCLES_UNLK;
}

/* Scc: set byte to 0xFF if cond, else 0 */
int gen_op_scc(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int cond = (op >> 8) & 15;
    uint16_t ea = op & GEN_EA_MASK;
    int mode = (ea >> GEN_EA_MODE_SHIFT) & GEN_EA_REG_MASK;
    int reg = ea & GEN_EA_REG_MASK;
    uint8_t val = branch_taken(cpu, cond) ? 0xFF : 0;
    if (mode == 0)
        cpu->d[reg] = (cpu->d[reg] & 0xFFFFFF00) | val;
    else
    {
        uint32_t addr = gen_ea_addr(cpu, mem, ea, 0);
        if (addr)
            genesis_mem_write8(mem, addr, val);
    }
    return GEN_CYCLES_SCC;
}

/* ADDA: An = An + <ea>. No flags. */
int gen_op_adda(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int is_long = ((op >> 7) & 1) != 0;
    int size = is_long ? GEN_TST_SIZE_L : GEN_TST_SIZE_W;
    int reg = (op >> 9) & 7;
    uint32_t src = gen_ea_read(cpu, mem, op & GEN_EA_MASK, size, NULL);
    if (!is_long)
        src = (uint32_t)(int32_t)(int16_t)(src & 0xFFFF);
    cpu->a[reg] += src;
    return GEN_CYCLES_ADDA_SUBA_CMPA;
}

/* SUBA: An = An - <ea>. No flags. */
int gen_op_suba(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int is_long = ((op >> 7) & 1) != 0;
    int size = is_long ? GEN_TST_SIZE_L : GEN_TST_SIZE_W;
    int reg = (op >> 9) & 7;
    uint32_t src = gen_ea_read(cpu, mem, op & GEN_EA_MASK, size, NULL);
    if (!is_long)
        src = (uint32_t)(int32_t)(int16_t)(src & 0xFFFF);
    cpu->a[reg] -= src;
    return GEN_CYCLES_ADDA_SUBA_CMPA;
}

/* CMPA: compare An with <ea>, set flags. */
int gen_op_cmpa(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int is_long = ((op >> 7) & 1) != 0;
    int size = is_long ? GEN_TST_SIZE_L : GEN_TST_SIZE_W;
    int reg = (op >> 9) & 7;
    uint32_t src = gen_ea_read(cpu, mem, op & GEN_EA_MASK, size, NULL);
    if (!is_long)
        src = (uint32_t)(int32_t)(int16_t)(src & 0xFFFF);
    uint32_t r = cpu->a[reg] - src;
    set_n(cpu, (r & 0x80000000) != 0);
    set_z(cpu, r == 0);
    set_v(cpu, ((cpu->a[reg] ^ src) & (cpu->a[reg] ^ r) & 0x80000000) != 0);
    set_c(cpu, src > cpu->a[reg]);
    return GEN_CYCLES_ADDA_SUBA_CMPA;
}

/* Immediate: ORI, ANDI, SUBI, ADDI, EORI, CMPI. #imm follows, size in bits 7-6. */
int gen_op_imm(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int base = (op >> 9) & 7;
    int sz = (op >> 6) & 3;

    /* Pseudo-EA 0x3C: ORI/ANDI/EORI/CMPI #imm, CCR (.B) o ORI/ANDI/EORI #imm, SR (.W). */
    if ((op & 0x3F) == 0x3C && (sz == 0 || sz == 1))
    {
        if (sz == 0)
        {
            if (base != 0 && base != 1 && base != 5 && base != 6)
            {
                gen_take_trap(cpu, mem, GEN_VECTOR_ILLEGAL, cpu->pc - 2);
                return GEN_CYCLES_ILLEGAL;
            }
            uint32_t imm = (uint32_t)fetch16(cpu, mem) & 0xFFu;
            uint32_t ccr = (uint32_t)(cpu->sr & 0xFFu);
            uint32_t r;
            if (base == 6)
            {
                r = (ccr - imm) & 0xFFu;
                set_n(cpu, (r & 0x80u) != 0);
                set_z(cpu, r == 0);
                set_v(cpu, ((ccr ^ imm) & (ccr ^ r) & 0x80u) != 0);
                set_c(cpu, imm > ccr);
                return GEN_CYCLES_IMM;
            }
            if (base == 0)
                r = ccr | imm;
            else if (base == 1)
                r = ccr & imm;
            else
                r = ccr ^ imm;
            cpu->sr = (cpu->sr & 0xFF00u) | (uint16_t)(r & 0xFFu);
            set_n(cpu, (r & 0x80u) != 0);
            set_z(cpu, (r & 0xFFu) == 0);
            set_v(cpu, 0);
            set_c(cpu, 0);
            return GEN_CYCLES_IMM;
        }
        /* .W a SR: sólo ORI/ANDI/EORI; privilegio supervisor */
        if (base != 0 && base != 1 && base != 5)
        {
            gen_take_trap(cpu, mem, GEN_VECTOR_ILLEGAL, cpu->pc - 2);
            return GEN_CYCLES_ILLEGAL;
        }
        if ((cpu->sr & 0x2000) == 0)
        {
            gen_take_trap(cpu, mem, GEN_VECTOR_PRIVILEGE_VIOLATION, cpu->pc - 2);
            return GEN_CYCLES_ILLEGAL;
        }
        uint32_t imm = (uint32_t)fetch16(cpu, mem);
        uint32_t sr = (uint32_t)cpu->sr;
        uint32_t r = (base == 0) ? (sr | imm) : (base == 1) ? (sr & imm) : (sr ^ imm);
        cpu->sr = (uint16_t)(r & 0xA71Fu);
        set_n(cpu, (cpu->sr & GEN_SR_N) != 0);
        set_z(cpu, (cpu->sr & GEN_SR_Z) != 0);
        set_v(cpu, (cpu->sr & GEN_SR_V) != 0);
        set_c(cpu, (cpu->sr & GEN_SR_C) != 0);
        return GEN_CYCLES_IMM;
    }

    if (sz == 3)
    {
        gen_take_trap(cpu, mem, GEN_VECTOR_ILLEGAL, cpu->pc - 2);
        return GEN_CYCLES_ILLEGAL;
    }

    int size = (sz == 0) ? 0 : (sz == 1) ? 1 : 2;
    uint32_t imm = (size == 0) ? (uint32_t)fetch16(cpu, mem) & 0xFF
                 : (size == 1) ? (uint32_t)fetch16(cpu, mem) : (uint32_t)(fetch16(cpu, mem) << 16) | fetch16(cpu, mem);
    uint16_t ea = op & GEN_EA_MASK;
    int mode = (ea >> GEN_EA_MODE_SHIFT) & GEN_EA_REG_MASK;
    int reg = ea & GEN_EA_REG_MASK;
    uint32_t val, r;
    uint32_t addr = 0;

    if (mode == 0)
        val = (size == 0) ? (cpu->d[reg] & 0xFF) : (size == 1) ? (cpu->d[reg] & 0xFFFF) : cpu->d[reg];
    else
        val = gen_ea_read(cpu, mem, ea, size, &addr);

    switch (base)
    {
    case 0: r = val | imm; break;
    case 1: r = val & imm; break;
    case 2: r = val - imm; break;
    case 3: r = val + imm; break;
    case 5: r = val ^ imm; break;
    case 6:
        r = val - imm;
        { uint32_t m = (size==0)?0xFFU:(size==1)?0xFFFFU:0xFFFFFFFFU; r &= m; }
        set_n(cpu, (r & (size==0?0x80U:size==1?0x8000U:0x80000000U))!=0);
        set_z(cpu, r==0);
        set_v(cpu, ((val^imm)&(val^r)&(size==0?0x80U:size==1?0x8000U:0x80000000U))!=0);
        set_c(cpu, imm>val);
        return GEN_CYCLES_IMM;
    default: return GEN_CYCLES_IMM;
    }

    if (base != 6)
    {
        uint32_t mask = (size == 0) ? 0xFFU : (size == 1) ? 0xFFFFU : 0xFFFFFFFFU;
        r &= mask;
        set_n(cpu, (r & (size==0?0x80U:size==1?0x8000U:0x80000000U)) != 0);
        set_z(cpu, r == 0);
        set_v(cpu, 0);
        set_c(cpu, 0);
    }

    if (mode == 0)
    {
        if (size == 0) cpu->d[reg] = (cpu->d[reg] & 0xFFFFFF00) | (r & 0xFF);
        else if (size == 1) cpu->d[reg] = (cpu->d[reg] & 0xFFFF0000) | (r & 0xFFFF);
        else cpu->d[reg] = r;
    }
    else if (addr && base != 6)
        gen_ea_write(cpu, mem, addr, r, size);
    return GEN_CYCLES_IMM;
}

/* Bit ops: BTST, BCHG, BCLR, BSET. Bit in Dn or #imm. */
int gen_op_bit(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int sub = (op >> 6) & 3;
    int use_imm = (op & 0x0100) != 0;
    int bit;
    uint32_t val;
    uint32_t addr = 0;

    if (use_imm)
    {
        bit = fetch16(cpu, mem) & 0x1F;
        val = gen_ea_read(cpu, mem, op & GEN_EA_MASK, 0, &addr);
    }
    else
    {
        int rx = (op >> 9) & 7;
        bit = cpu->d[rx] & 0x1F;
        val = gen_ea_read(cpu, mem, op & GEN_EA_MASK, 0, &addr);
    }

    int z = !(val & (1U << bit));
    set_z(cpu, z);
    set_n(cpu, 0);
    set_v(cpu, 0);
    set_c(cpu, 0);

    if (sub == 1)
        val ^= (1U << bit);
    else if (sub == 2)
        val &= ~(1U << bit);
    else if (sub == 3)
        val |= (1U << bit);

    if (sub >= 1)
    {
        if (addr)
            genesis_mem_write8(mem, addr, (uint8_t)val);
        else
        {
            int r = op & 7;
            cpu->d[r] = (cpu->d[r] & 0xFFFFFF00) | (val & 0xFF);
        }
    }
    return GEN_CYCLES_BIT;
}

/* MULU: unsigned multiply. Dn = Dn * <ea> (low 16 bits). */
int gen_op_mulu(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int reg = (op >> 9) & 7;
    uint16_t src = (uint16_t)gen_ea_read(cpu, mem, op & GEN_EA_MASK, 1, NULL);
    uint32_t r = (uint32_t)(cpu->d[reg] & 0xFFFF) * (uint32_t)src;
    cpu->d[reg] = r;
    set_z(cpu, r == 0);
    set_n(cpu, (r & 0x80000000) != 0);
    set_v(cpu, 0);
    set_c(cpu, 0);
    return GEN_CYCLES_MULU;
}

/* MULS: signed multiply. */
int gen_op_muls(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int reg = (op >> 9) & 7;
    int32_t src = (int32_t)(int16_t)gen_ea_read(cpu, mem, op & GEN_EA_MASK, 1, NULL);
    int32_t r = (int32_t)(int16_t)(cpu->d[reg] & 0xFFFF) * src;
    cpu->d[reg] = (uint32_t)r;
    set_z(cpu, r == 0);
    set_n(cpu, (r & 0x80000000) != 0);
    set_v(cpu, 0);
    set_c(cpu, 0);
    return GEN_CYCLES_MULS;
}

/* DIVU: unsigned divide. Dn = Dn / <ea> (32b result: quotient low, remainder high). */
int gen_op_divu(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int reg = (op >> 9) & 7;
    uint32_t src = gen_ea_read(cpu, mem, op & GEN_EA_MASK, 1, NULL) & 0xFFFF;
    if (src == 0)
    {
        gen_take_trap(cpu, mem, GEN_VECTOR_ZERO_DIV, cpu->pc - 2);
        return GEN_CYCLES_DIVU;
    }
    uint32_t dividend = cpu->d[reg];
    uint32_t quot = dividend / src;
    uint32_t rem = dividend % src;
    if (quot > 0xFFFF) { set_v(cpu, 1); return GEN_CYCLES_DIVU; }
    cpu->d[reg] = (rem << 16) | (quot & 0xFFFF);
    set_z(cpu, quot == 0);
    set_n(cpu, (quot & 0x8000) != 0);
    set_v(cpu, 0);
    set_c(cpu, 0);
    return GEN_CYCLES_DIVU;
}

/* DIVS: signed divide. */
int gen_op_divs(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int reg = (op >> 9) & 7;
    int32_t src = (int32_t)(int16_t)(gen_ea_read(cpu, mem, op & GEN_EA_MASK, 1, NULL) & 0xFFFF);
    if (src == 0)
    {
        gen_take_trap(cpu, mem, GEN_VECTOR_ZERO_DIV, cpu->pc - 2);
        return GEN_CYCLES_DIVS;
    }
    int32_t dividend = (int32_t)cpu->d[reg];
    int32_t quot = dividend / src;
    int32_t rem = dividend % src;
    if (quot < -32768 || quot > 32767) { set_v(cpu, 1); return GEN_CYCLES_DIVS; }
    cpu->d[reg] = ((uint32_t)(rem & 0xFFFF) << 16) | ((uint32_t)quot & 0xFFFF);
    set_z(cpu, quot == 0);
    set_n(cpu, (quot & 0x8000) != 0);
    set_v(cpu, 0);
    set_c(cpu, 0);
    return GEN_CYCLES_DIVS;
}

/* ADDX/SUBX: Dy,Dx or -(Ay),-(Ax). With X flag. */
int gen_op_addx_subx(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int is_sub = (op & 0xF100) == GEN_OP_SUBX;
    int rx = (op >> 9) & 7;
    int ry = op & 7;
    int rm = (op >> 3) & 1;
    uint32_t x = flag_x(cpu) ? 1U : 0U;
    uint32_t src, dst, r;
    int size = ((op >> 6) & 1) ? GEN_TST_SIZE_L : GEN_TST_SIZE_W;

    if (rm == 0)
    {
        src = (size == GEN_TST_SIZE_W) ? (cpu->d[ry] & 0xFFFF) : cpu->d[ry];
        dst = (size == GEN_TST_SIZE_W) ? (cpu->d[rx] & 0xFFFF) : cpu->d[rx];
    }
    else
    {
        cpu->a[ry] -= (size == GEN_TST_SIZE_W) ? 2 : 4;
        cpu->a[rx] -= (size == GEN_TST_SIZE_W) ? 2 : 4;
        src = (size == GEN_TST_SIZE_W) ? genesis_mem_read16(mem, cpu->a[ry]) : genesis_mem_read32(mem, cpu->a[ry]);
        dst = (size == GEN_TST_SIZE_W) ? genesis_mem_read16(mem, cpu->a[rx]) : genesis_mem_read32(mem, cpu->a[rx]);
    }

    r = is_sub ? (dst - src - x) : (dst + src + x);
    uint32_t mask = (size == GEN_TST_SIZE_W) ? 0xFFFFU : 0xFFFFFFFFU;
    r &= mask;

    if (rm == 0)
    {
        if (size == GEN_TST_SIZE_W)
            cpu->d[rx] = (cpu->d[rx] & 0xFFFF0000) | (r & 0xFFFF);
        else
            cpu->d[rx] = r;
    }
    else
    {
        if (size == GEN_TST_SIZE_W)
            genesis_mem_write16(mem, cpu->a[rx], (uint16_t)r);
        else
            genesis_mem_write32(mem, cpu->a[rx], r);
    }

    set_x(cpu, is_sub ? (src + x > dst) : (r < dst || (r == dst && x)));
    set_c(cpu, is_sub ? (src + x > dst) : (r < dst || (r == dst && x)));
    set_n(cpu, (r & (size==GEN_TST_SIZE_W?0x8000U:0x80000000U)) != 0);
    set_z(cpu, r == 0);
    set_v(cpu, 0);
    return GEN_CYCLES_ADDX_SUBX;
}

/* NEGX: negate with extend. */
int gen_op_negx(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int sz = (op >> 6) & 3;
    int size = (sz == 0) ? 0 : (sz == 1) ? 1 : 2;
    uint16_t ea = op & GEN_EA_MASK;
    int mode = (ea >> GEN_EA_MODE_SHIFT) & GEN_EA_REG_MASK;
    int reg = ea & GEN_EA_REG_MASK;
    uint32_t x = flag_x(cpu) ? 1U : 0U;
    uint32_t val, r;
    uint32_t addr = 0;
    uint32_t mask = (size == 0) ? 0xFFU : (size == 1) ? 0xFFFFU : 0xFFFFFFFFU;

    if (mode == 0)
        val = (size == 0) ? (cpu->d[reg] & 0xFF) : (size == 1) ? (cpu->d[reg] & 0xFFFF) : cpu->d[reg];
    else
        val = gen_ea_read(cpu, mem, ea, size, &addr);

    r = (0 - val - x) & mask;

    if (mode == 0)
    {
        if (size == 0) cpu->d[reg] = (cpu->d[reg] & 0xFFFFFF00) | (r & 0xFF);
        else if (size == 1) cpu->d[reg] = (cpu->d[reg] & 0xFFFF0000) | (r & 0xFFFF);
        else cpu->d[reg] = r;
    }
    else if (addr)
        gen_ea_write(cpu, mem, addr, r, size);

    set_x(cpu, val != 0 || x != 0);
    set_c(cpu, val != 0 || x != 0);
    set_n(cpu, (r & (size==0?0x80U:size==1?0x8000U:0x80000000U)) != 0);
    set_z(cpu, r == 0);
    set_v(cpu, val == (size==0?0x80U:size==1?0x8000U:0x80000000U) && x != 0);
    return GEN_CYCLES_NEGX;
}

/* TAS: test and set. Read, set N/Z, set bit 7 of byte. */
int gen_op_tas(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    uint16_t ea = op & GEN_EA_MASK;
    uint32_t addr = gen_ea_addr(cpu, mem, ea, 0);
    uint8_t val = addr ? genesis_mem_read8(mem, addr) : (uint8_t)(cpu->d[ea & 7] & 0xFF);
    set_n(cpu, (val & 0x80) != 0);
    set_z(cpu, val == 0);
    set_v(cpu, 0);
    set_c(cpu, 0);
    val |= 0x80;
    if (addr)
        genesis_mem_write8(mem, addr, val);
    else
        cpu->d[ea & 7] = (cpu->d[ea & 7] & 0xFFFFFF00) | val;
    return GEN_CYCLES_TAS;
}

/* MOVE from/to SR, CCR. */
int gen_op_move_sr_ccr(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    if ((op & 0xFFC0) == GEN_OP_MOVE_FROM_SR)
    {
        uint16_t ea = op & GEN_EA_MASK;
        uint32_t addr = gen_ea_addr(cpu, mem, ea, 1);
        if (addr)
            genesis_mem_write16(mem, addr, cpu->sr);
        else
            cpu->d[ea & 7] = (cpu->d[ea & 7] & 0xFFFF0000) | cpu->sr;
    }
    else if ((op & 0xFFC0) == GEN_OP_MOVE_TO_CCR)
    {
        uint32_t val = gen_ea_read(cpu, mem, op & GEN_EA_MASK, 1, NULL) & 0xFF;
        cpu->sr = (cpu->sr & 0xFF00) | val;
    }
    else
    {
        uint32_t val = gen_ea_read(cpu, mem, op & GEN_EA_MASK, 1, NULL);
        cpu->sr = val & 0xA71F;
    }
    return 12;
}

/* Exception: push PC, SR; set S=1, T=0; jump to vector. 68000 format 0: [SR][PC], a[7]->SR. */
static void gen_take_trap(gen_cpu_t *cpu, genesis_mem_t *mem, int vector, uint32_t pc_val)
{
    gen_cpu_exception_jump(cpu, mem, vector, pc_val);
}

/* CHK Dn,<ea>: trap #6 if Dn < 0 or Dn > bound. */
int gen_op_chk(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int reg = (op >> 9) & 7;
    int32_t dn = (int32_t)(int16_t)(cpu->d[reg] & 0xFFFF);
    int32_t bound = (int32_t)(int16_t)gen_ea_read(cpu, mem, op & GEN_EA_MASK, 1, NULL);
    if (dn < 0 || dn > bound)
        gen_take_trap(cpu, mem, GEN_VECTOR_CHK, cpu->inst_pc);
    return GEN_CYCLES_CHK;
}

/* RESET: assert reset line. NOP (no external bus model). */
int gen_op_reset(gen_cpu_t *cpu, genesis_mem_t *mem)
{
    (void)cpu;
    (void)mem;
    return GEN_CYCLES_RESET;
}

/* STOP #imm: load SR, halt until interrupt. */
int gen_op_stop(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    uint16_t imm = fetch16(cpu, mem);
    cpu->sr = (cpu->sr & 0xFF00) | (imm & 0xA71F);
    cpu->stopped = 1;
    (void)op;
    return GEN_CYCLES_STOP;
}

/* MOVEP: solo (d16,An); bytes en direcciones pares espaciadas (+2). */
int gen_op_movep(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    if (((op >> 3) & 7) != 5)
        return gen_op_illegal(cpu, mem);
    int an = op & 7;
    int dn = (op >> 9) & 7;
    int reg_to_mem = (op >> 7) & 1;
    int is_long = (op >> 6) & 1;
    int16_t d16 = (int16_t)genesis_mem_read16(mem, cpu->pc);
    cpu->pc += 2;
    uint32_t addr = cpu->a[an] + (int32_t)d16;

    if (reg_to_mem)
    {
        uint32_t v = cpu->d[dn];
        if (!is_long)
        {
            genesis_mem_write8(mem, addr, (uint8_t)(v >> 8));
            genesis_mem_write8(mem, addr + 2, (uint8_t)(v & 0xFF));
            return 16;
        }
        genesis_mem_write8(mem, addr, (uint8_t)(v >> 24));
        genesis_mem_write8(mem, addr + 2, (uint8_t)(v >> 16));
        genesis_mem_write8(mem, addr + 4, (uint8_t)(v >> 8));
        genesis_mem_write8(mem, addr + 6, (uint8_t)v);
        return 24;
    }
    if (!is_long)
    {
        uint32_t hi = genesis_mem_read8(mem, addr);
        uint32_t lo = genesis_mem_read8(mem, addr + 2);
        cpu->d[dn] = (cpu->d[dn] & ~0xFFFFu) | (hi << 8) | lo;
        return 16;
    }
    uint32_t b0 = genesis_mem_read8(mem, addr);
    uint32_t b1 = genesis_mem_read8(mem, addr + 2);
    uint32_t b2 = genesis_mem_read8(mem, addr + 4);
    uint32_t b3 = genesis_mem_read8(mem, addr + 6);
    cpu->d[dn] = (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
    return 24;
}

/* ILLEGAL: trap #4. */
int gen_op_illegal(gen_cpu_t *cpu, genesis_mem_t *mem)
{
    gen_take_trap(cpu, mem, GEN_VECTOR_ILLEGAL, cpu->pc - 2);
    return GEN_CYCLES_ILLEGAL;
}

int gen_op_line_a(gen_cpu_t *cpu, genesis_mem_t *mem)
{
    gen_take_trap(cpu, mem, GEN_VECTOR_LINE_A, cpu->pc - 2);
    return GEN_CYCLES_ILLEGAL;
}

int gen_op_line_f(gen_cpu_t *cpu, genesis_mem_t *mem)
{
    gen_take_trap(cpu, mem, GEN_VECTOR_LINE_F, cpu->pc - 2);
    return GEN_CYCLES_ILLEGAL;
}

/* TRAP #n: vector 32+n. Push PC (next instr), SR; jump to vector. */
int gen_op_trap(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int n = op & 0x000F;
    int vector = 32 + n;
    gen_take_trap(cpu, mem, vector, cpu->pc);
    return GEN_CYCLES_TRAP;
}

/* RTR: pop CCR, pop PC (return and restore) */
int gen_op_rtr(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    gen_cpu_sp_require_even(cpu, mem);
    uint16_t ccr = genesis_mem_read16(mem, cpu->a[7]);
    cpu->a[7] += 2;
    gen_cpu_sp_require_even(cpu, mem);
    cpu->pc = genesis_mem_read32(mem, cpu->a[7]);
    cpu->a[7] += 4;
    cpu->sr = (cpu->sr & 0xFF00) | (ccr & 0xFF);
    (void)op;
    return GEN_CYCLES_RTR;
}

/* Shift count: bit 8=0 → bits 11-9 inmediato (0=8); bit 8=1 → Dn mod 64 (0→8). */
static int shift_count(gen_cpu_t *cpu, uint16_t op)
{
    if ((op & 0x0100) == 0)
    {
        int c = (op >> 9) & 7;
        return c ? c : 8;
    }
    unsigned c = (unsigned)cpu->d[(op >> 9) & 7] & 63u;
    return c ? (int)c : 8;
}

/* LSL #n,Dn: logical shift left. size: 0=B, 1=W, 2=L */
int gen_op_lsl(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int count = shift_count(cpu, op);
    int sz = (op >> 6) & 3;
    int size = (sz == 0) ? 0 : (sz == 1) ? 1 : 2;
    int reg = (op >> 3) & 7;
    uint32_t val = (size == 0) ? (cpu->d[reg] & 0xFF) : (size == 1) ? (cpu->d[reg] & 0xFFFF) : cpu->d[reg];
    uint32_t mask = (size == 0) ? 0xFF : (size == 1) ? 0xFFFF : 0xFFFFFFFF;
    uint32_t last_out = 0;

    if (count > 0)
    {
        for (int i = 0; i < count; i++)
        {
            last_out = (val >> ((size == 0 ? 7 : size == 1 ? 15 : 31))) & 1;
            val = (val << 1) & mask;
        }
        cpu->d[reg] = (size == 0) ? (cpu->d[reg] & 0xFFFFFF00) | (val & 0xFF) : (size == 1) ? (cpu->d[reg] & 0xFFFF0000) | (val & 0xFFFF) : val;
    }
    set_x(cpu, last_out);
    set_n(cpu, (val & (size == 0 ? 0x80 : size == 1 ? 0x8000 : 0x80000000)) != 0);
    set_z(cpu, val == 0);
    set_v(cpu, 0);
    set_c(cpu, last_out);
    (void)mem;
    return GEN_CYCLES_SHIFT + count * 2;
}

/* LSR #n,Dn: logical shift right */
int gen_op_lsr(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int count = shift_count(cpu, op);
    int sz = (op >> 6) & 3;
    int size = (sz == 0) ? 0 : (sz == 1) ? 1 : 2;
    int reg = (op >> 3) & 7;
    uint32_t val = (size == 0) ? (cpu->d[reg] & 0xFF) : (size == 1) ? (cpu->d[reg] & 0xFFFF) : cpu->d[reg];
    uint32_t mask = (size == 0) ? 0xFF : (size == 1) ? 0xFFFF : 0xFFFFFFFF;
    uint32_t last_out = 0;

    if (count > 0)
    {
        for (int i = 0; i < count; i++)
        {
            last_out = val & 1;
            val = (val >> 1) & mask;
        }
        cpu->d[reg] = (size == 0) ? (cpu->d[reg] & 0xFFFFFF00) | (val & 0xFF) : (size == 1) ? (cpu->d[reg] & 0xFFFF0000) | (val & 0xFFFF) : val;
    }
    set_x(cpu, last_out);
    set_n(cpu, 0);
    set_z(cpu, val == 0);
    set_v(cpu, 0);
    set_c(cpu, last_out);
    (void)mem;
    return GEN_CYCLES_SHIFT + count * 2;
}

/* ASL = LSL para nuestro propósito (mismo resultado, flags idénticos) */
int gen_op_asl(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    return gen_op_lsl(cpu, mem, op);
}

/* ROR: rotate right (bit 0 → C → bit 31) */
int gen_op_ror(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int count = shift_count(cpu, op);
    int sz = (op >> 6) & 3;
    int size = (sz == 0) ? 0 : (sz == 1) ? 1 : 2;
    int reg = (op >> 3) & 7;
    uint32_t val = (size == 0) ? (cpu->d[reg] & 0xFF) : (size == 1) ? (cpu->d[reg] & 0xFFFF) : cpu->d[reg];
    uint32_t mask = (size == 0) ? 0xFFU : (size == 1) ? 0xFFFFU : 0xFFFFFFFFU;
    int shift = (size == 0) ? 8 : (size == 1) ? 16 : 32;
    uint32_t last_out = 0;
    if (count > 0)
    {
        for (int i = 0; i < count; i++)
        {
            last_out = val & 1;
            val = ((val >> 1) | (last_out << (shift - 1))) & mask;
        }
        cpu->d[reg] = (size == 0) ? (cpu->d[reg] & 0xFFFFFF00) | (val & 0xFF) : (size == 1) ? (cpu->d[reg] & 0xFFFF0000) | (val & 0xFFFF) : val;
    }
    set_x(cpu, last_out);
    set_n(cpu, (val & (size == 0 ? 0x80U : size == 1 ? 0x8000U : 0x80000000U)) != 0);
    set_z(cpu, val == 0);
    set_v(cpu, 0);
    set_c(cpu, last_out);
    (void)mem;
    return GEN_CYCLES_SHIFT + count * 2;
}

/* ROL: rotate left (bit 31 → C → bit 0) */
int gen_op_rol(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int count = shift_count(cpu, op);
    int sz = (op >> 6) & 3;
    int size = (sz == 0) ? 0 : (sz == 1) ? 1 : 2;
    int reg = (op >> 3) & 7;
    uint32_t val = (size == 0) ? (cpu->d[reg] & 0xFF) : (size == 1) ? (cpu->d[reg] & 0xFFFF) : cpu->d[reg];
    uint32_t mask = (size == 0) ? 0xFFU : (size == 1) ? 0xFFFFU : 0xFFFFFFFFU;
    int top = (size == 0) ? 7 : (size == 1) ? 15 : 31;
    uint32_t last_out = 0;
    if (count > 0)
    {
        for (int i = 0; i < count; i++)
        {
            last_out = (val >> top) & 1;
            val = ((val << 1) | last_out) & mask;
        }
        cpu->d[reg] = (size == 0) ? (cpu->d[reg] & 0xFFFFFF00) | (val & 0xFF) : (size == 1) ? (cpu->d[reg] & 0xFFFF0000) | (val & 0xFFFF) : val;
    }
    set_x(cpu, last_out);
    set_n(cpu, (val & (size == 0 ? 0x80U : size == 1 ? 0x8000U : 0x80000000U)) != 0);
    set_z(cpu, val == 0);
    set_v(cpu, 0);
    set_c(cpu, last_out);
    (void)mem;
    return GEN_CYCLES_SHIFT + count * 2;
}

/* ROXR: rotate right through X */
int gen_op_roxr(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int count = shift_count(cpu, op);
    int sz = (op >> 6) & 3;
    int size = (sz == 0) ? 0 : (sz == 1) ? 1 : 2;
    int reg = (op >> 3) & 7;
    uint32_t val = (size == 0) ? (cpu->d[reg] & 0xFF) : (size == 1) ? (cpu->d[reg] & 0xFFFF) : cpu->d[reg];
    uint32_t mask = (size == 0) ? 0xFFU : (size == 1) ? 0xFFFFU : 0xFFFFFFFFU;
    int shift = (size == 0) ? 8 : (size == 1) ? 16 : 32;
    uint32_t x = flag_x(cpu) ? 1U : 0U;
    uint32_t last_out = 0;
    if (count > 0)
    {
        for (int i = 0; i < count; i++)
        {
            last_out = val & 1;
            val = ((val >> 1) | (x << (shift - 1))) & mask;
            x = last_out;
        }
        cpu->d[reg] = (size == 0) ? (cpu->d[reg] & 0xFFFFFF00) | (val & 0xFF) : (size == 1) ? (cpu->d[reg] & 0xFFFF0000) | (val & 0xFFFF) : val;
    }
    set_x(cpu, last_out);
    set_n(cpu, (val & (size == 0 ? 0x80U : size == 1 ? 0x8000U : 0x80000000U)) != 0);
    set_z(cpu, val == 0);
    set_v(cpu, 0);
    set_c(cpu, last_out);
    (void)mem;
    return GEN_CYCLES_SHIFT + count * 2;
}

/* ROXL: rotate left through X */
int gen_op_roxl(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int count = shift_count(cpu, op);
    int sz = (op >> 6) & 3;
    int size = (sz == 0) ? 0 : (sz == 1) ? 1 : 2;
    int reg = (op >> 3) & 7;
    uint32_t val = (size == 0) ? (cpu->d[reg] & 0xFF) : (size == 1) ? (cpu->d[reg] & 0xFFFF) : cpu->d[reg];
    uint32_t mask = (size == 0) ? 0xFFU : (size == 1) ? 0xFFFFU : 0xFFFFFFFFU;
    int top = (size == 0) ? 7 : (size == 1) ? 15 : 31;
    uint32_t x = flag_x(cpu) ? 1U : 0U;
    uint32_t last_out = 0;
    if (count > 0)
    {
        for (int i = 0; i < count; i++)
        {
            last_out = (val >> top) & 1;
            val = ((val << 1) | x) & mask;
            x = last_out;
        }
        cpu->d[reg] = (size == 0) ? (cpu->d[reg] & 0xFFFFFF00) | (val & 0xFF) : (size == 1) ? (cpu->d[reg] & 0xFFFF0000) | (val & 0xFFFF) : val;
    }
    set_x(cpu, last_out);
    set_n(cpu, (val & (size == 0 ? 0x80U : size == 1 ? 0x8000U : 0x80000000U)) != 0);
    set_z(cpu, val == 0);
    set_v(cpu, 0);
    set_c(cpu, last_out);
    (void)mem;
    return GEN_CYCLES_SHIFT + count * 2;
}

/* MOVEM: move multiple registers. Lista en palabra siguiente. */
int gen_op_movem(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    uint16_t list = fetch16(cpu, mem);
    int is_store = (op & GEN_OP_MOVEM_MASK) == GEN_OP_MOVEM_STORE;
    int is_long = (op >> GEN_MOVEM_SIZE_BIT) & 1;
    int size = is_long ? GEN_TST_SIZE_L : GEN_TST_SIZE_W;
    int ea = op & GEN_EA_MASK;
    int mode = (ea >> GEN_EA_MODE_SHIFT) & GEN_EA_REG_MASK;
    int reg = ea & GEN_EA_REG_MASK;
    uint32_t addr;
    int count = 0;

    for (int i = 0; i < GEN_MOVEM_REGS; i++)
        if (list & (1u << i))
            count++;

    if (count == 0)
        return GEN_CYCLES_MOVEM_PER_REG;

    if (is_store)
    {
        if (mode == GEN_EA_MODE_PREDEC)
        {
            addr = cpu->a[reg];
            for (int i = GEN_MOVEM_REGS - 1; i >= 0; i--)
            {
                if (!(list & (1u << i)))
                    continue;
                addr -= (size == GEN_TST_SIZE_W) ? GEN_MOVEM_STEP_WORD : GEN_MOVEM_STEP_LONG;
                if (i < 8)
                {
                    if (size == GEN_TST_SIZE_W)
                        genesis_mem_write16(mem, addr, (uint16_t)(cpu->d[i] & 0xFFFF));
                    else
                        genesis_mem_write32(mem, addr, cpu->d[i]);
                }
                else
                {
                    if (size == GEN_TST_SIZE_W)
                        genesis_mem_write16(mem, addr, (uint16_t)(cpu->a[i - 8] & 0xFFFF));
                    else
                        genesis_mem_write32(mem, addr, cpu->a[i - 8]);
                }
            }
            cpu->a[reg] = addr;
        }
        else
        {
            addr = gen_ea_addr(cpu, mem, ea, GEN_TST_SIZE_L);
            for (int i = 0; i < GEN_MOVEM_REGS; i++)
            {
                if (!(list & (1u << i)))
                    continue;
                if (i < 8)
                    gen_ea_write(cpu, mem, addr, cpu->d[i], size);
                else
                    gen_ea_write(cpu, mem, addr, cpu->a[i - 8], size);
                addr += (size == GEN_TST_SIZE_W) ? GEN_MOVEM_STEP_WORD : GEN_MOVEM_STEP_LONG;
            }
        }
    }
    else
    {
        if (mode == GEN_EA_MODE_POSTINC)
        {
            addr = cpu->a[reg];
            for (int i = 0; i < GEN_MOVEM_REGS; i++)
            {
                if (!(list & (1u << i)))
                    continue;
                uint32_t val = (size == GEN_TST_SIZE_W) ? (uint32_t)(int32_t)(int16_t)genesis_mem_read16(mem, addr) : genesis_mem_read32(mem, addr);
                if (i < 8)
                    cpu->d[i] = val;
                else
                    cpu->a[i - 8] = val;
                addr += (size == GEN_TST_SIZE_W) ? GEN_MOVEM_STEP_WORD : GEN_MOVEM_STEP_LONG;
            }
            cpu->a[reg] = addr;
        }
        else
        {
            addr = gen_ea_addr(cpu, mem, ea, GEN_TST_SIZE_L);
            for (int i = 0; i < GEN_MOVEM_REGS; i++)
            {
                if (!(list & (1u << i)))
                    continue;
                uint32_t val = (size == GEN_TST_SIZE_W) ? (uint32_t)(int32_t)(int16_t)genesis_mem_read16(mem, addr) : genesis_mem_read32(mem, addr);
                if (i < 8)
                    cpu->d[i] = val;
                else
                    cpu->a[i - 8] = val;
                addr += (size == GEN_TST_SIZE_W) ? GEN_MOVEM_STEP_WORD : GEN_MOVEM_STEP_LONG;
            }
        }
    }

    return GEN_CYCLES_MOVEM_PER_REG * count;
}

/* ASR #n,Dn: arithmetic shift right (sign-extend) */
int gen_op_asr(gen_cpu_t *cpu, genesis_mem_t *mem, uint16_t op)
{
    int count = shift_count(cpu, op);
    int sz = (op >> 6) & 3;
    int size = (sz == 0) ? 0 : (sz == 1) ? 1 : 2;
    int reg = (op >> 3) & 7;
    uint32_t val = (size == 0) ? (cpu->d[reg] & 0xFF) : (size == 1) ? (cpu->d[reg] & 0xFFFF) : cpu->d[reg];
    uint32_t mask = (size == 0) ? 0xFF : (size == 1) ? 0xFFFF : 0xFFFFFFFF;
    uint32_t last_out = 0;

    if (count > 0)
    {
        for (int i = 0; i < count; i++)
        {
            last_out = val & 1;
            val = ((uint32_t)((int32_t)(val & mask) >> 1)) & mask;
        }
        cpu->d[reg] = (size == 0) ? (cpu->d[reg] & 0xFFFFFF00) | (val & 0xFF) : (size == 1) ? (cpu->d[reg] & 0xFFFF0000) | (val & 0xFFFF) : val;
    }
    set_x(cpu, last_out);
    set_n(cpu, (val & (size == 0 ? 0x80 : size == 1 ? 0x8000 : 0x80000000)) != 0);
    set_z(cpu, val == 0);
    set_v(cpu, 0);
    set_c(cpu, last_out);
    (void)mem;
    return GEN_CYCLES_SHIFT + count * 2;
}
