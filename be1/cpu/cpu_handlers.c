/**
 * Bitemu - CPU opcode handlers (callbacks para gb_cpu_step)
 *
 * Las tablas gb_op_handlers[256] y gb_cb_handlers[32] se rellenan aquí.
 * Cada handler recibe (cpu, mem, opcode) y devuelve ciclos consumidos.
 * CB-prefix: índice = cb_op >> 3 (grupos RLC/RRC/RL/RR, BIT, RES, SET, etc.).
 * Main opcodes: un handler por opcode o por grupo (p. ej. LD r,r', ALU A,r).
 */
#include "cpu_internal.h"
#include "cpu_handlers.h"
#include "../gb_constants.h"
#include <stdint.h>

/* ============== CB-prefix handlers (index = cb_op >> 3) ============== */

static int cb_rlc(gb_cpu_t *cpu, gb_mem_t *m, uint8_t cb_op)
{
    int r = cb_op & 7;
    uint8_t v = read_r8(cpu, m, r);
    v = (v << 1) | (v >> 7);
    set_c(cpu, v & 1);
    set_z(cpu, (v &= GB_BYTE_LO) == 0);
    set_n(cpu, 0);
    set_h(cpu, 0);
    write_r8(cpu, m, r, v);
    return (r == 6) ? 16 : 8;
}

static int cb_rrc(gb_cpu_t *cpu, gb_mem_t *m, uint8_t cb_op)
{
    int r = cb_op & 7;
    uint8_t v = read_r8(cpu, m, r);
    int c = v & 1;
    v = (v >> 1) | (c << 7);
    set_c(cpu, c);
    set_z(cpu, (v &= GB_BYTE_LO) == 0);
    set_n(cpu, 0);
    set_h(cpu, 0);
    write_r8(cpu, m, r, v);
    return (r == 6) ? 16 : 8;
}

static int cb_rl(gb_cpu_t *cpu, gb_mem_t *m, uint8_t cb_op)
{
    int r = cb_op & 7;
    uint8_t v = read_r8(cpu, m, r);
    int c = flag_c(cpu);
    set_c(cpu, v >> 7);
    v = (v << 1) | c;
    set_z(cpu, (v &= GB_BYTE_LO) == 0);
    set_n(cpu, 0);
    set_h(cpu, 0);
    write_r8(cpu, m, r, v);
    return (r == 6) ? 16 : 8;
}

static int cb_rr(gb_cpu_t *cpu, gb_mem_t *m, uint8_t cb_op)
{
    int r = cb_op & 7;
    uint8_t v = read_r8(cpu, m, r);
    int c = flag_c(cpu);
    set_c(cpu, v & 1);
    v = (v >> 1) | (c << 7);
    set_z(cpu, (v &= GB_BYTE_LO) == 0);
    set_n(cpu, 0);
    set_h(cpu, 0);
    write_r8(cpu, m, r, v);
    return (r == 6) ? 16 : 8;
}

static int cb_sla(gb_cpu_t *cpu, gb_mem_t *m, uint8_t cb_op)
{
    int r = cb_op & 7;
    uint8_t v = read_r8(cpu, m, r);
    set_c(cpu, v >> 7);
    v <<= 1;
    set_z(cpu, (v &= GB_BYTE_LO) == 0);
    set_n(cpu, 0);
    set_h(cpu, 0);
    write_r8(cpu, m, r, v);
    return (r == 6) ? 16 : 8;
}

static int cb_sra(gb_cpu_t *cpu, gb_mem_t *m, uint8_t cb_op)
{
    int r = cb_op & 7;
    uint8_t v = read_r8(cpu, m, r);
    set_c(cpu, v & 1);
    v = (int8_t)v >> 1;
    set_z(cpu, (v &= GB_BYTE_LO) == 0);
    set_n(cpu, 0);
    set_h(cpu, 0);
    write_r8(cpu, m, r, v);
    return (r == 6) ? 16 : 8;
}

static int cb_swap(gb_cpu_t *cpu, gb_mem_t *m, uint8_t cb_op)
{
    int r = cb_op & 7;
    uint8_t v = read_r8(cpu, m, r);
    v = (v >> 4) | (v << 4);
    set_z(cpu, (v &= GB_BYTE_LO) == 0);
    set_n(cpu, 0);
    set_h(cpu, 0);
    set_c(cpu, 0);
    write_r8(cpu, m, r, v);
    return (r == 6) ? 16 : 8;
}

static int cb_srl(gb_cpu_t *cpu, gb_mem_t *m, uint8_t cb_op)
{
    int r = cb_op & 7;
    uint8_t v = read_r8(cpu, m, r);
    set_c(cpu, v & 1);
    v >>= 1;
    set_z(cpu, (v &= GB_BYTE_LO) == 0);
    set_n(cpu, 0);
    set_h(cpu, 0);
    write_r8(cpu, m, r, v);
    return (r == 6) ? 16 : 8;
}

static int cb_bit(gb_cpu_t *cpu, gb_mem_t *m, uint8_t cb_op)
{
    int r = cb_op & 7;
    int bit = (cb_op >> 3) & 7;
    uint8_t v = read_r8(cpu, m, r);
    set_z(cpu, !(v & (1 << bit)));
    set_n(cpu, 0);
    set_h(cpu, 1);
    return (r == 6) ? 16 : 8;
}

static int cb_res(gb_cpu_t *cpu, gb_mem_t *m, uint8_t cb_op)
{
    int r = cb_op & 7;
    int bit = (cb_op >> 3) & 7;
    uint8_t v = read_r8(cpu, m, r);
    v &= ~(1 << bit);
    write_r8(cpu, m, r, v);
    return (r == 6) ? 16 : 8;
}

static int cb_set(gb_cpu_t *cpu, gb_mem_t *m, uint8_t cb_op)
{
    int r = cb_op & 7;
    int bit = (cb_op >> 3) & 7;
    uint8_t v = read_r8(cpu, m, r);
    v |= (1 << bit);
    write_r8(cpu, m, r, v);
    return (r == 6) ? 16 : 8;
}

gb_cb_handler_t gb_cb_handlers[32] = {
    cb_rlc,
    cb_rrc,
    cb_rl,
    cb_rr,
    cb_sla,
    cb_sra,
    cb_swap,
    cb_srl,
    cb_bit,
    cb_bit,
    cb_bit,
    cb_bit,
    cb_bit,
    cb_bit,
    cb_bit,
    cb_bit,
    cb_res,
    cb_res,
    cb_res,
    cb_res,
    cb_res,
    cb_res,
    cb_res,
    cb_res,
    cb_set,
    cb_set,
    cb_set,
    cb_set,
    cb_set,
    cb_set,
    cb_set,
    cb_set,
};

/* ============== Main opcode handlers ============== */

static int op_nop(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)cpu;
    (void)m;
    (void)op;
    return 4;
}

static int op_ld_bc_nn(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->c = gb_mem_read(m, cpu->pc++);
    cpu->b = gb_mem_read(m, cpu->pc++);
    return 12;
}

/* LD r,r (0x40-0x7F) and HALT (0x76) */
static int op_ld_r_r(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    if (op == 0x76)
    {
        if (!cpu->ime && (m->io[GB_IO_IF] & m->ie & GB_IF_IE_MASK))
            cpu->halt_bug = 1;
        else
            cpu->halted = 1;
        return 4;
    }
    int dst = (op >> 3) & 7, src = op & 7;
    write_r8(cpu, m, dst, read_r8(cpu, m, src));
    return (dst == 6 || src == 6) ? 8 : 4;
}

/* ALU A,r (0x80-0xBF) */
static int op_alu_a_r(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    uint8_t v = read_r8(cpu, m, op & 7);
    switch ((op >> 3) & 7)
    {
    case 0:
        alu_add(cpu, cpu->a, v, 0);
        break;
    case 1:
        alu_add(cpu, cpu->a, v, flag_c(cpu));
        break;
    case 2:
        alu_sub(cpu, cpu->a, v, 0);
        break;
    case 3:
        alu_sub(cpu, cpu->a, v, flag_c(cpu));
        break;
    case 4:
        alu_and(cpu, v);
        break;
    case 5:
        alu_xor(cpu, v);
        break;
    case 6:
        alu_or(cpu, v);
        break;
    case 7:
        alu_cp(cpu, cpu->a, v);
        break;
    }
    return (op & 7) == 6 ? 8 : 4;
}

/* 0x02 - 0x0F */
static int op_ld_bc_a(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    gb_mem_write(m, BC, cpu->a);
    return 8;
}
static int op_inc_bc(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    uint16_t bc = BC + 1;
    cpu->b = bc >> 8;
    cpu->c = bc & GB_BYTE_LO;
    return 8;
}
static int op_inc_b(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    uint8_t b = cpu->b + 1;
    set_z(cpu, b == 0);
    set_n(cpu, 0);
    set_h(cpu, (cpu->b & 0xF) == 0xF);
    cpu->b = b;
    return 4;
}
static int op_dec_b(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    uint8_t b = cpu->b - 1;
    set_z(cpu, b == 0);
    set_n(cpu, 1);
    set_h(cpu, (cpu->b & 0xF) == 0);
    cpu->b = b;
    return 4;
}
static int op_ld_b_n(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->b = gb_mem_read(m, cpu->pc++);
    return 8;
}
static int op_rlca(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    uint8_t a = cpu->a;
    set_c(cpu, a >> 7);
    cpu->a = (a << 1) | (a >> 7);
    set_z(cpu, 0);
    set_n(cpu, 0);
    set_h(cpu, 0);
    return 4;
}
static int op_ld_nn_sp(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    uint16_t a = gb_mem_read(m, cpu->pc) | (gb_mem_read(m, cpu->pc + 1) << 8);
    cpu->pc += 2;
    gb_mem_write16(m, a, cpu->sp);
    return 20;
}
static int op_add_hl_bc(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    uint16_t hl = HL, bc = BC;
    uint32_t r = hl + bc;
    set_n(cpu, 0);
    set_h(cpu, ((hl & GB_HALFCARRY_12) + (bc & GB_HALFCARRY_12)) > GB_HALFCARRY_12);
    set_c(cpu, r > GB_WORD_LO);
    cpu->h = (r >> 8) & GB_BYTE_LO;
    cpu->l = r & GB_BYTE_LO;
    return 8;
}
static int op_ld_a_bc(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->a = gb_mem_read(m, BC);
    return 8;
}
static int op_dec_bc(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    uint16_t bc = BC - 1;
    cpu->b = bc >> 8;
    cpu->c = bc & GB_BYTE_LO;
    return 8;
}
static int op_inc_c(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    uint8_t c = cpu->c + 1;
    set_z(cpu, c == 0);
    set_n(cpu, 0);
    set_h(cpu, (cpu->c & 0xF) == 0xF);
    cpu->c = c;
    return 4;
}
static int op_dec_c(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    uint8_t c = cpu->c - 1;
    set_z(cpu, c == 0);
    set_n(cpu, 1);
    set_h(cpu, (cpu->c & 0xF) == 0);
    cpu->c = c;
    return 4;
}
static int op_ld_c_n(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->c = gb_mem_read(m, cpu->pc++);
    return 8;
}

static int op_rrca(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    uint8_t a = cpu->a;
    set_c(cpu, a & 1);
    cpu->a = (a >> 1) | (a << 7);
    set_z(cpu, 0);
    set_n(cpu, 0);
    set_h(cpu, 0);
    return 4;
}

/* 0x10 - 0x1F */
static int op_stop(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    cpu->pc++;
    return 4;
}
static int op_ld_de_nn(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->e = gb_mem_read(m, cpu->pc++);
    cpu->d = gb_mem_read(m, cpu->pc++);
    return 12;
}
static int op_ld_de_a(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    gb_mem_write(m, DE, cpu->a);
    return 8;
}
static int op_inc_de(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    uint16_t de = DE + 1;
    cpu->d = de >> 8;
    cpu->e = de & GB_BYTE_LO;
    return 8;
}
static int op_inc_d(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    uint8_t d = cpu->d + 1;
    set_z(cpu, d == 0);
    set_n(cpu, 0);
    set_h(cpu, (cpu->d & 0xF) == 0xF);
    cpu->d = d;
    return 4;
}
static int op_dec_d(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    uint8_t d = cpu->d - 1;
    set_z(cpu, d == 0);
    set_n(cpu, 1);
    set_h(cpu, (cpu->d & 0xF) == 0);
    cpu->d = d;
    return 4;
}

static int op_ld_d_n(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->d = gb_mem_read(m, cpu->pc++);
    return 8;
}

static int op_rla(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    uint8_t a = cpu->a;
    int c = flag_c(cpu);
    set_c(cpu, a >> 7);
    cpu->a = (a << 1) | c;
    set_z(cpu, 0);
    set_n(cpu, 0);
    set_h(cpu, 0);
    return 4;
}
static int op_jr_n(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    int8_t e = (int8_t)gb_mem_read(m, cpu->pc++);
    cpu->pc += e;
    return 12;
}
static int op_add_hl_de(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    uint16_t hl = HL, de = DE;
    uint32_t r = hl + de;
    set_n(cpu, 0);
    set_h(cpu, ((hl & GB_HALFCARRY_12) + (de & GB_HALFCARRY_12)) > GB_HALFCARRY_12);
    set_c(cpu, r > GB_WORD_LO);
    cpu->h = (r >> 8) & GB_BYTE_LO;
    cpu->l = r & GB_BYTE_LO;
    return 8;
}
static int op_ld_a_de(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->a = gb_mem_read(m, DE);
    return 8;
}
static int op_dec_de(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    uint16_t de = DE - 1;
    cpu->d = de >> 8;
    cpu->e = de & GB_BYTE_LO;
    return 8;
}
static int op_inc_e(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    uint8_t e = cpu->e + 1;
    set_z(cpu, e == 0);
    set_n(cpu, 0);
    set_h(cpu, (cpu->e & 0xF) == 0xF);
    cpu->e = e;
    return 4;
}
static int op_dec_e(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    uint8_t e = cpu->e - 1;
    set_z(cpu, e == 0);
    set_n(cpu, 1);
    set_h(cpu, (cpu->e & 0xF) == 0);
    cpu->e = e;
    return 4;
}
static int op_ld_e_n(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->e = gb_mem_read(m, cpu->pc++);
    return 8;
}
static int op_rra(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    uint8_t a = cpu->a;
    int c = flag_c(cpu);
    set_c(cpu, a & 1);
    cpu->a = (a >> 1) | (c << 7);
    set_z(cpu, 0);
    set_n(cpu, 0);
    set_h(cpu, 0);
    return 4;
}

/* 0x20 - 0x2F */
static int op_jr_nz(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    int8_t e = (int8_t)gb_mem_read(m, cpu->pc++);
    if (!flag_z(cpu))
    {
        cpu->pc += e;
        return 12;
    }
    return 8;
}
static int op_ld_hl_nn(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->l = gb_mem_read(m, cpu->pc++);
    cpu->h = gb_mem_read(m, cpu->pc++);
    return 12;
}

static int op_ld_hlp_a(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    gb_mem_write(m, HL, cpu->a);
    cpu->l++;
    if (cpu->l == 0)
        cpu->h++;
    return 8;
}

static int op_inc_hl(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    uint16_t hl = HL + 1;
    cpu->h = hl >> 8;
    cpu->l = hl & GB_BYTE_LO;
    return 8;
}

static int op_inc_h(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    uint8_t h = cpu->h + 1;
    set_z(cpu, h == 0);
    set_n(cpu, 0);
    set_h(cpu, (cpu->h & 0xF) == 0xF);
    cpu->h = h;
    return 4;
}

static int op_dec_h(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    uint8_t h = cpu->h - 1;
    set_z(cpu, h == 0);
    set_n(cpu, 1);
    set_h(cpu, (cpu->h & 0xF) == 0);
    cpu->h = h;
    return 4;
}

static int op_ld_h_n(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->h = gb_mem_read(m, cpu->pc++);
    return 8;
}

static int op_daa(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    uint8_t a = cpu->a;
    int c = flag_c(cpu), h = (cpu->f & F_H) != 0, n = (cpu->f & F_N) != 0;
    if (!n)
    {
        if (c || a > GB_BCD_HI)
        {
            a += GB_DAA_CARRY;
            set_c(cpu, 1);
        }
        if (h || (a & 0xF) > 9)
            a += GB_DAA_NIBBLE;
    }
    else
    {
        if (c)
            a -= GB_DAA_CARRY;
        if (h)
            a -= GB_DAA_NIBBLE;
    }
    cpu->a = a & GB_BYTE_LO;
    set_z(cpu, cpu->a == 0);
    set_h(cpu, 0);
    return 4;
}

static int op_jr_z(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    int8_t e = (int8_t)gb_mem_read(m, cpu->pc++);
    if (flag_z(cpu))
    {
        cpu->pc += e;
        return 12;
    }
    return 8;
}

static int op_add_hl_hl(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    uint16_t hl = HL;
    uint32_t r = hl + hl;
    set_n(cpu, 0);
    set_h(cpu, ((hl & GB_HALFCARRY_12) * 2) > GB_HALFCARRY_12);
    set_c(cpu, r > GB_WORD_LO);
    cpu->h = (r >> 8) & GB_BYTE_LO;
    cpu->l = r & GB_BYTE_LO;
    return 8;
}

static int op_ld_a_hlp(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->a = gb_mem_read(m, HL);
    cpu->l++;
    if (cpu->l == 0)
        cpu->h++;
    return 8;
}

static int op_dec_hl(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    uint16_t hl = HL - 1;
    cpu->h = hl >> 8;
    cpu->l = hl & GB_BYTE_LO;
    return 8;
}

static int op_inc_l(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    uint8_t l = cpu->l + 1;
    set_z(cpu, l == 0);
    set_n(cpu, 0);
    set_h(cpu, (cpu->l & 0xF) == 0xF);
    cpu->l = l;
    return 4;
}

static int op_dec_l(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    uint8_t l = cpu->l - 1;
    set_z(cpu, l == 0);
    set_n(cpu, 1);
    set_h(cpu, (cpu->l & 0xF) == 0);
    cpu->l = l;
    return 4;
}

static int op_ld_l_n(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->l = gb_mem_read(m, cpu->pc++);
    return 8;
}

static int op_cpl(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    cpu->a = ~cpu->a;
    set_n(cpu, 1);
    set_h(cpu, 1);
    return 4;
}

/* 0x30 - 0x3F */
static int op_jr_nc(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    int8_t e = (int8_t)gb_mem_read(m, cpu->pc++);
    if (!flag_c(cpu))
    {
        cpu->pc += e;
        return 12;
    }
    return 8;
}

static int op_ld_sp_nn(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->sp = gb_mem_read(m, cpu->pc) | (gb_mem_read(m, cpu->pc + 1) << 8);
    cpu->pc += 2;
    return 12;
}

static int op_ld_hlm_a(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    gb_mem_write(m, HL, cpu->a);
    cpu->l--;
    if (cpu->l == GB_BYTE_LO)
        cpu->h--;
    return 8;
}

static int op_inc_sp(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    cpu->sp++;
    return 8;
}

static int op_inc_hlp(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    uint8_t old = gb_mem_read(m, HL), v = old + 1;
    set_z(cpu, v == 0);
    set_n(cpu, 0);
    set_h(cpu, (old & 0xF) == 0xF);
    gb_mem_write(m, HL, v);
    return 12;
}

static int op_dec_hlp(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    uint8_t old = gb_mem_read(m, HL), v = old - 1;
    set_z(cpu, v == 0);
    set_n(cpu, 1);
    set_h(cpu, (old & 0xF) == 0);
    gb_mem_write(m, HL, v);
    return 12;
}

static int op_ld_hl_n(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    gb_mem_write(m, HL, gb_mem_read(m, cpu->pc++));
    return 12;
}

static int op_scf(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    set_n(cpu, 0);
    set_h(cpu, 0);
    set_c(cpu, 1);
    return 4;
}

static int op_jr_c(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    int8_t e = (int8_t)gb_mem_read(m, cpu->pc++);
    if (flag_c(cpu))
    {
        cpu->pc += e;
        return 12;
    }
    return 8;
}

static int op_add_hl_sp(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    uint16_t hl = HL;
    uint32_t r = hl + cpu->sp;
    set_n(cpu, 0);
    set_h(cpu, ((hl & GB_HALFCARRY_12) + (cpu->sp & GB_HALFCARRY_12)) > GB_HALFCARRY_12);
    set_c(cpu, r > GB_WORD_LO);
    cpu->h = (r >> 8) & GB_BYTE_LO;
    cpu->l = r & GB_BYTE_LO;
    return 8;
}

static int op_ld_a_hlm(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->a = gb_mem_read(m, HL);
    cpu->l--;
    if (cpu->l == GB_BYTE_LO)
        cpu->h--;
    return 8;
}

static int op_dec_sp(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    cpu->sp--;
    return 8;
}
static int op_inc_a(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    uint8_t a = cpu->a + 1;
    set_z(cpu, a == 0);
    set_n(cpu, 0);
    set_h(cpu, (cpu->a & 0xF) == 0xF);
    cpu->a = a;
    return 4;
}

static int op_dec_a(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    uint8_t a = cpu->a - 1;
    set_z(cpu, a == 0);
    set_n(cpu, 1);
    set_h(cpu, (cpu->a & 0xF) == 0);
    cpu->a = a;
    return 4;
}

static int op_ld_a_n(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->a = gb_mem_read(m, cpu->pc++);
    return 8;
}

static int op_ccf(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    set_n(cpu, 0);
    set_h(cpu, 0);
    set_c(cpu, !flag_c(cpu));
    return 4;
}

/* 0xC0-0xFF */
static int op_ret_nz(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    if (!flag_z(cpu))
    {
        cpu->pc = gb_mem_read(m, cpu->sp) | (gb_mem_read(m, cpu->sp + 1) << 8);
        cpu->sp += 2;
        return 20;
    }
    return 8;
}
static int op_pop_bc(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->c = gb_mem_read(m, cpu->sp++);
    cpu->b = gb_mem_read(m, cpu->sp++);
    return 12;
}
static int op_jp_nz(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    uint16_t a = gb_mem_read(m, cpu->pc) | (gb_mem_read(m, cpu->pc + 1) << 8);
    cpu->pc += 2;
    if (!flag_z(cpu))
    {
        cpu->pc = a;
        return 16;
    }
    return 12;
}
static int op_jp_nn(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    uint16_t a = gb_mem_read(m, cpu->pc) | (gb_mem_read(m, cpu->pc + 1) << 8);
    cpu->pc = a;
    return 16;
}

static int op_call_nz(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    uint16_t a = gb_mem_read(m, cpu->pc) | (gb_mem_read(m, cpu->pc + 1) << 8);
    cpu->pc += 2;
    if (!flag_z(cpu))
    {
        cpu->sp -= 2;
        gb_mem_write16(m, cpu->sp, cpu->pc);
        cpu->pc = a;
        return 24;
    }
    return 12;
}

static int op_push_bc(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->sp -= 2;
    gb_mem_write16(m, cpu->sp, BC);
    return 16;
}

static int op_add_a_n(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    alu_add(cpu, cpu->a, gb_mem_read(m, cpu->pc++), 0);
    return 8;
}

static int op_rst_0(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->sp -= 2;
    gb_mem_write16(m, cpu->sp, cpu->pc);
    cpu->pc = 0x00;
    return 16;
}

static int op_ret_z(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    if (flag_z(cpu))
    {
        cpu->pc = gb_mem_read(m, cpu->sp) | (gb_mem_read(m, cpu->sp + 1) << 8);
        cpu->sp += 2;
        return 20;
    }
    return 8;
}

static int op_ret(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->pc = gb_mem_read(m, cpu->sp) | (gb_mem_read(m, cpu->sp + 1) << 8);
    cpu->sp += 2;
    return 16;
}

static int op_jp_z(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    uint16_t a = gb_mem_read(m, cpu->pc) | (gb_mem_read(m, cpu->pc + 1) << 8);
    cpu->pc += 2;
    if (flag_z(cpu))
    {
        cpu->pc = a;
        return 16;
    }
    return 12;
}

static int op_call_z(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    uint16_t a = gb_mem_read(m, cpu->pc) | (gb_mem_read(m, cpu->pc + 1) << 8);
    cpu->pc += 2;
    if (flag_z(cpu))
    {
        cpu->sp -= 2;
        gb_mem_write16(m, cpu->sp, cpu->pc);
        cpu->pc = a;
        return 24;
    }
    return 12;
}
static int op_call_nn(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    uint16_t a = gb_mem_read(m, cpu->pc) | (gb_mem_read(m, cpu->pc + 1) << 8);
    cpu->pc += 2;
    cpu->sp -= 2;
    gb_mem_write16(m, cpu->sp, cpu->pc);
    cpu->pc = a;
    return 24;
}
static int op_adc_a_n(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    alu_add(cpu, cpu->a, gb_mem_read(m, cpu->pc++), flag_c(cpu));
    return 8;
}
static int op_rst_8(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->sp -= 2;
    gb_mem_write16(m, cpu->sp, cpu->pc);
    cpu->pc = 0x08;
    return 16;
}
static int op_ret_nc(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    if (!flag_c(cpu))
    {
        cpu->pc = gb_mem_read(m, cpu->sp) | (gb_mem_read(m, cpu->sp + 1) << 8);
        cpu->sp += 2;
        return 20;
    }
    return 8;
}
static int op_pop_de(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->e = gb_mem_read(m, cpu->sp++);
    cpu->d = gb_mem_read(m, cpu->sp++);
    return 12;
}
static int op_jp_nc(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    uint16_t a = gb_mem_read(m, cpu->pc) | (gb_mem_read(m, cpu->pc + 1) << 8);
    cpu->pc += 2;
    if (!flag_c(cpu))
    {
        cpu->pc = a;
        return 16;
    }
    return 12;
}
static int op_call_nc(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    uint16_t a = gb_mem_read(m, cpu->pc) | (gb_mem_read(m, cpu->pc + 1) << 8);
    cpu->pc += 2;
    if (!flag_c(cpu))
    {
        cpu->sp -= 2;
        gb_mem_write16(m, cpu->sp, cpu->pc);
        cpu->pc = a;
        return 24;
    }
    return 12;
}
static int op_push_de(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->sp -= 2;
    gb_mem_write16(m, cpu->sp, DE);
    return 16;
}
static int op_sub_n(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    alu_sub(cpu, cpu->a, gb_mem_read(m, cpu->pc++), 0);
    return 8;
}
static int op_rst_10(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->sp -= 2;
    gb_mem_write16(m, cpu->sp, cpu->pc);
    cpu->pc = 0x10;
    return 16;
}
static int op_ret_c(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    if (flag_c(cpu))
    {
        cpu->pc = gb_mem_read(m, cpu->sp) | (gb_mem_read(m, cpu->sp + 1) << 8);
        cpu->sp += 2;
        return 20;
    }
    return 8;
}
static int op_reti(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->pc = gb_mem_read(m, cpu->sp) | (gb_mem_read(m, cpu->sp + 1) << 8);
    cpu->sp += 2;
    cpu->ime = 1;
    return 16;
}
static int op_jp_c(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    uint16_t a = gb_mem_read(m, cpu->pc) | (gb_mem_read(m, cpu->pc + 1) << 8);
    cpu->pc += 2;
    if (flag_c(cpu))
    {
        cpu->pc = a;
        return 16;
    }
    return 12;
}
static int op_call_c(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    uint16_t a = gb_mem_read(m, cpu->pc) | (gb_mem_read(m, cpu->pc + 1) << 8);
    cpu->pc += 2;
    if (flag_c(cpu))
    {
        cpu->sp -= 2;
        gb_mem_write16(m, cpu->sp, cpu->pc);
        cpu->pc = a;
        return 24;
    }
    return 12;
}
static int op_sbc_a_n(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    alu_sub(cpu, cpu->a, gb_mem_read(m, cpu->pc++), flag_c(cpu));
    return 8;
}
static int op_rst_18(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->sp -= 2;
    gb_mem_write16(m, cpu->sp, cpu->pc);
    cpu->pc = 0x18;
    return 16;
}
static int op_ldh_n_a(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    gb_mem_write(m, GB_IO_BASE | gb_mem_read(m, cpu->pc++), cpu->a);
    return 12;
}
static int op_pop_hl(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->l = gb_mem_read(m, cpu->sp++);
    cpu->h = gb_mem_read(m, cpu->sp++);
    return 12;
}
static int op_ld_c_a(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    gb_mem_write(m, GB_IO_BASE | cpu->c, cpu->a);
    return 8;
}
static int op_push_hl(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->sp -= 2;
    gb_mem_write16(m, cpu->sp, HL);
    return 16;
}
static int op_and_n(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    alu_and(cpu, gb_mem_read(m, cpu->pc++));
    return 8;
}
static int op_rst_20(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->sp -= 2;
    gb_mem_write16(m, cpu->sp, cpu->pc);
    cpu->pc = 0x20;
    return 16;
}
static int op_add_sp_e(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    int8_t e = (int8_t)gb_mem_read(m, cpu->pc++);
    uint32_t r = cpu->sp + e;
    set_z(cpu, 0);
    set_n(cpu, 0);
    set_h(cpu, ((cpu->sp & 0xF) + (e & 0xF)) > 0xF);
    set_c(cpu, ((cpu->sp & GB_BYTE_LO) + (e & GB_BYTE_LO)) > GB_BYTE_LO);
    cpu->sp = (uint16_t)r;
    return 16;
}
static int op_jp_hl(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    cpu->pc = HL;
    return 4;
}
static int op_ld_nn_a(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    uint16_t a = gb_mem_read(m, cpu->pc) | (gb_mem_read(m, cpu->pc + 1) << 8);
    cpu->pc += 2;
    gb_mem_write(m, a, cpu->a);
    return 16;
}
static int op_xor_n(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    alu_xor(cpu, gb_mem_read(m, cpu->pc++));
    return 8;
}
static int op_rst_28(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->sp -= 2;
    gb_mem_write16(m, cpu->sp, cpu->pc);
    cpu->pc = 0x28;
    return 16;
}
static int op_ldh_a_n(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->a = gb_mem_read(m, GB_IO_BASE | gb_mem_read(m, cpu->pc++));
    return 12;
}
static int op_pop_af(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    uint8_t f = gb_mem_read(m, cpu->sp++) & GB_F_UPPER;
    cpu->a = gb_mem_read(m, cpu->sp++);
    cpu->f = f;
    return 12;
}
static int op_ld_a_c(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->a = gb_mem_read(m, 0xFF00u | cpu->c);
    return 8;
}
static int op_di(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    cpu->ime = 0;
    cpu->ime_delay = 0;
    return 4;
}
static int op_push_af(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->sp -= 2;
    gb_mem_write16(m, cpu->sp, (cpu->a << 8) | (cpu->f & GB_F_UPPER));
    return 16;
}
static int op_or_n(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    alu_or(cpu, gb_mem_read(m, cpu->pc++));
    return 8;
}
static int op_rst_30(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->sp -= 2;
    gb_mem_write16(m, cpu->sp, cpu->pc);
    cpu->pc = 0x30;
    return 16;
}
static int op_ld_hl_sp_e(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    int8_t e = (int8_t)gb_mem_read(m, cpu->pc++);
    uint32_t r = cpu->sp + e;
    set_z(cpu, 0);
    set_n(cpu, 0);
    set_h(cpu, ((cpu->sp & 0xF) + (e & 0xF)) > 0xF);
    set_c(cpu, ((cpu->sp & GB_BYTE_LO) + (e & GB_BYTE_LO)) > GB_BYTE_LO);
    cpu->h = (r >> 8) & GB_BYTE_LO;
    cpu->l = r & GB_BYTE_LO;
    return 12;
}
static int op_ld_sp_hl(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    cpu->sp = HL;
    return 8;
}
static int op_ld_a_nn(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    uint16_t a = gb_mem_read(m, cpu->pc) | (gb_mem_read(m, cpu->pc + 1) << 8);
    cpu->pc += 2;
    cpu->a = gb_mem_read(m, a);
    return 16;
}
static int op_ei(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)m;
    (void)op;
    cpu->ime_delay = 1;
    return 4;
}
static int op_cp_n(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    alu_cp(cpu, cpu->a, gb_mem_read(m, cpu->pc++));
    return 8;
}
static int op_rst_38(gb_cpu_t *cpu, gb_mem_t *m, uint8_t op)
{
    (void)op;
    cpu->sp -= 2;
    gb_mem_write16(m, cpu->sp, cpu->pc);
    cpu->pc = 0x38;
    return 16;
}

gb_op_handler_t gb_op_handlers[256];

/* Main opcode table */
static void init_op_handlers(void)
{
    int i;
    for (i = 0; i < 256; i++)
        gb_op_handlers[i] = op_nop;

    gb_op_handlers[0x00] = op_nop;
    gb_op_handlers[0x01] = op_ld_bc_nn;
    gb_op_handlers[0x02] = op_ld_bc_a;
    gb_op_handlers[0x03] = op_inc_bc;
    gb_op_handlers[0x04] = op_inc_b;
    gb_op_handlers[0x05] = op_dec_b;
    gb_op_handlers[0x06] = op_ld_b_n;
    gb_op_handlers[0x07] = op_rlca;
    gb_op_handlers[0x08] = op_ld_nn_sp;
    gb_op_handlers[0x09] = op_add_hl_bc;
    gb_op_handlers[0x0A] = op_ld_a_bc;
    gb_op_handlers[0x0B] = op_dec_bc;
    gb_op_handlers[0x0C] = op_inc_c;
    gb_op_handlers[0x0D] = op_dec_c;
    gb_op_handlers[0x0E] = op_ld_c_n;
    gb_op_handlers[0x0F] = op_rrca;
    gb_op_handlers[0x10] = op_stop;
    gb_op_handlers[0x11] = op_ld_de_nn;
    gb_op_handlers[0x12] = op_ld_de_a;
    gb_op_handlers[0x13] = op_inc_de;
    gb_op_handlers[0x14] = op_inc_d;
    gb_op_handlers[0x15] = op_dec_d;
    gb_op_handlers[0x16] = op_ld_d_n;
    gb_op_handlers[0x17] = op_rla;
    gb_op_handlers[0x18] = op_jr_n;
    gb_op_handlers[0x19] = op_add_hl_de;
    gb_op_handlers[0x1A] = op_ld_a_de;
    gb_op_handlers[0x1B] = op_dec_de;
    gb_op_handlers[0x1C] = op_inc_e;
    gb_op_handlers[0x1D] = op_dec_e;
    gb_op_handlers[0x1E] = op_ld_e_n;
    gb_op_handlers[0x1F] = op_rra;
    gb_op_handlers[0x20] = op_jr_nz;
    gb_op_handlers[0x21] = op_ld_hl_nn;
    gb_op_handlers[0x22] = op_ld_hlp_a;
    gb_op_handlers[0x23] = op_inc_hl;
    gb_op_handlers[0x24] = op_inc_h;
    gb_op_handlers[0x25] = op_dec_h;
    gb_op_handlers[0x26] = op_ld_h_n;
    gb_op_handlers[0x27] = op_daa;
    gb_op_handlers[0x28] = op_jr_z;
    gb_op_handlers[0x29] = op_add_hl_hl;
    gb_op_handlers[0x2A] = op_ld_a_hlp;
    gb_op_handlers[0x2B] = op_dec_hl;
    gb_op_handlers[0x2C] = op_inc_l;
    gb_op_handlers[0x2D] = op_dec_l;
    gb_op_handlers[0x2E] = op_ld_l_n;
    gb_op_handlers[0x2F] = op_cpl;
    gb_op_handlers[0x30] = op_jr_nc;
    gb_op_handlers[0x31] = op_ld_sp_nn;
    gb_op_handlers[0x32] = op_ld_hlm_a;
    gb_op_handlers[0x33] = op_inc_sp;
    gb_op_handlers[0x34] = op_inc_hlp;
    gb_op_handlers[0x35] = op_dec_hlp;
    gb_op_handlers[0x36] = op_ld_hl_n;
    gb_op_handlers[0x37] = op_scf;
    gb_op_handlers[0x38] = op_jr_c;
    gb_op_handlers[0x39] = op_add_hl_sp;
    gb_op_handlers[0x3A] = op_ld_a_hlm;
    gb_op_handlers[0x3B] = op_dec_sp;
    gb_op_handlers[0x3C] = op_inc_a;
    gb_op_handlers[0x3D] = op_dec_a;
    gb_op_handlers[0x3E] = op_ld_a_n;
    gb_op_handlers[0x3F] = op_ccf;

    for (i = 0x40; i <= 0x7F; i++)
        gb_op_handlers[i] = op_ld_r_r;
    for (i = 0x80; i <= 0xBF; i++)
        gb_op_handlers[i] = op_alu_a_r;

    gb_op_handlers[0xC0] = op_ret_nz;
    gb_op_handlers[0xC1] = op_pop_bc;
    gb_op_handlers[0xC2] = op_jp_nz;
    gb_op_handlers[0xC3] = op_jp_nn;
    gb_op_handlers[0xC4] = op_call_nz;
    gb_op_handlers[0xC5] = op_push_bc;
    gb_op_handlers[0xC6] = op_add_a_n;
    gb_op_handlers[0xC7] = op_rst_0;
    gb_op_handlers[0xC8] = op_ret_z;
    gb_op_handlers[0xC9] = op_ret;
    gb_op_handlers[0xCA] = op_jp_z;
    gb_op_handlers[0xCC] = op_call_z;
    gb_op_handlers[0xCD] = op_call_nn;
    gb_op_handlers[0xCE] = op_adc_a_n;
    gb_op_handlers[0xCF] = op_rst_8;
    gb_op_handlers[0xD0] = op_ret_nc;
    gb_op_handlers[0xD1] = op_pop_de;
    gb_op_handlers[0xD2] = op_jp_nc;
    gb_op_handlers[0xD4] = op_call_nc;
    gb_op_handlers[0xD5] = op_push_de;
    gb_op_handlers[0xD6] = op_sub_n;
    gb_op_handlers[0xD7] = op_rst_10;
    gb_op_handlers[0xD8] = op_ret_c;
    gb_op_handlers[0xD9] = op_reti;
    gb_op_handlers[0xDA] = op_jp_c;
    gb_op_handlers[0xDC] = op_call_c;
    gb_op_handlers[0xDE] = op_sbc_a_n;
    gb_op_handlers[0xDF] = op_rst_18;
    gb_op_handlers[0xE0] = op_ldh_n_a;
    gb_op_handlers[0xE1] = op_pop_hl;
    gb_op_handlers[0xE2] = op_ld_c_a;
    gb_op_handlers[0xE5] = op_push_hl;
    gb_op_handlers[0xE6] = op_and_n;
    gb_op_handlers[0xE7] = op_rst_20;
    gb_op_handlers[0xE8] = op_add_sp_e;
    gb_op_handlers[0xE9] = op_jp_hl;
    gb_op_handlers[0xEA] = op_ld_nn_a;
    gb_op_handlers[0xEE] = op_xor_n;
    gb_op_handlers[0xEF] = op_rst_28;
    gb_op_handlers[0xF0] = op_ldh_a_n;
    gb_op_handlers[0xF1] = op_pop_af;
    gb_op_handlers[0xF2] = op_ld_a_c;
    gb_op_handlers[0xF3] = op_di;
    gb_op_handlers[0xF5] = op_push_af;
    gb_op_handlers[0xF6] = op_or_n;
    gb_op_handlers[0xF7] = op_rst_30;
    gb_op_handlers[0xF8] = op_ld_hl_sp_e;
    gb_op_handlers[0xF9] = op_ld_sp_hl;
    gb_op_handlers[0xFA] = op_ld_a_nn;
    gb_op_handlers[0xFB] = op_ei;
    gb_op_handlers[0xFE] = op_cp_n;
    gb_op_handlers[0xFF] = op_rst_38;
}

#if defined(__GNUC__) || defined(__clang__)
static void __attribute__((constructor)) cpu_handlers_ctor(void) { init_op_handlers(); }
#endif
