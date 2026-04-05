/**
 * BE2 - Z80 core (Genesis)
 *
 * Drivers de sonido Genesis: subset Z80 ampliable; ver z80.h.
 * LD, JP, JR, CALL, RET, IN, OUT, PUSH, POP, ADD, INC, DEC, NOP, HALT, EI, DI, etc.
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#define R8() (z80_mem_read(ctx, z80->pc++))
#define R16() R16_(z80, ctx)
#define W8(addr, v) z80_mem_write(ctx, addr, v)
#define W16(addr, v) do { W8(addr, (v)&0xFF); W8((addr)+1, (v)>>8); } while (0)

#define HI(x) (((x) >> 8) & 0xFF)
#define LO(x) ((x)&0xFF)

#define S 0x80
#define Z 0x40
#define H 0x10
#define P 0x04
#define N 0x02
#define C 0x01

#include "z80.h"
#include "../genesis_impl.h"
#include "../genesis_constants.h"
#include "../cpu_sync/cpu_sync.h"
#include "../memory.h"
#include "../ym2612/ym2612.h"
#include "../psg/psg.h"
#include <string.h>

static uint8_t z80_mem_read(void *ctx, uint16_t addr);
static void z80_mem_write(void *ctx, uint16_t addr, uint8_t val);

/* Z80 mem read: usa espacio Genesis Z80 */
static uint8_t z80_mem_read(void *ctx, uint16_t addr)
{
    genesis_impl_t *impl = (genesis_impl_t *)ctx;
    if (addr < 0x2000)
        return impl->z80_ram[addr & 0x1FFF];
    if (addr >= 0x4000 && addr <= 0x4003)
    {
        /* YM2612: 68k addr = 0xA04000 + (addr & 3) */
        uint32_t m68k = GEN_ADDR_YM_START + (addr & 3);
        return genesis_mem_read8(&impl->mem, m68k);
    }
    if (addr == 0x6000)
        return impl->z80.bank;
    if (addr == 0x7F11 || (addr >= 0x7F10 && addr <= 0x7F17))
    {
        /* PSG: write-only, read returns 0xFF */
        return 0xFF;
    }
    if (addr >= 0x8000 && impl->mem.rom)
    {
        uint32_t off = (uint32_t)impl->z80.bank << 15;
        off += (addr - 0x8000);
        if (off < impl->mem.rom_size)
            return impl->mem.rom[off];
    }
    return 0xFF;
}

static void z80_mem_write(void *ctx, uint16_t addr, uint8_t val)
{
    genesis_impl_t *impl = (genesis_impl_t *)ctx;
    if (addr < 0x2000)
    {
        impl->z80_ram[addr & 0x1FFF] = val;
        return;
    }
    if (addr >= 0x4000 && addr <= 0x4003)
    {
        uint32_t m68k = GEN_ADDR_YM_START + (addr & 3);
        genesis_mem_write8(&impl->mem, m68k, val);
        return;
    }
    if (addr == 0x6000)
    {
        impl->z80.bank = val;
        return;
    }
    if (addr == 0x7F11 || (addr >= 0x7F10 && addr <= 0x7F17))
    {
        if (impl->mem.psg)
            gen_psg_write(impl->mem.psg, val);
        return;
    }
    /* 0x8000-0xFFFF: ROM, no write */
}

static inline uint16_t R16_(gen_z80_t *z, void *c)
{
    uint16_t lo = z80_mem_read(c, z->pc++);
    uint16_t hi = z80_mem_read(c, z->pc++);
    return lo | (hi << 8);
}

static uint8_t z80_port_in(void *ctx, uint8_t port)
{
    genesis_impl_t *impl = (genesis_impl_t *)ctx;
    /* Drivers Genesis: YM2612 status vía IN en 0x40–0x43 (map a bus 68k A040xx). */
    if ((port & 0xFCu) == 0x40u && impl->mem.ym2612)
        return gen_ym2612_read_port(impl->mem.ym2612, (int)(port & 3u));
    return 0xFF;
}

static void z80_port_out(void *ctx, uint8_t port, uint8_t val)
{
    genesis_impl_t *impl = (genesis_impl_t *)ctx;
    uint16_t addr = (uint16_t)port | 0x7F00;  /* Puertos típicos en 0x7Fxx */
    if (addr == 0x7F11 && impl->mem.psg)
        gen_psg_write(impl->mem.psg, val);
    if (addr >= 0x4000 && addr <= 0x4003)
    {
        uint32_t m68k = GEN_ADDR_YM_START + (addr & 3);
        genesis_mem_write8(&impl->mem, m68k, val);
    }
}

static inline int parity(uint8_t x)
{
    x ^= x >> 4;
    x ^= x >> 2;
    x ^= x >> 1;
    return (~x) & 1;
}

static void z80_ed_adc_hl_rr(gen_z80_t *z80, uint8_t *af, uint16_t hlv, uint16_t rr)
{
    uint32_t c = (af[0] & C) ? 1u : 0u;
    uint32_t res = (uint32_t)hlv + (uint32_t)rr + c;
    uint16_t r = (uint16_t)res;
    z80->hl = r;
    uint8_t f = (r == 0 ? Z : 0) | ((r & 0x8000) ? S : 0);
    f |= (uint8_t)(((hlv ^ rr ^ r) >> 8) & H);
    if (!((hlv ^ rr) & 0x8000) && ((hlv ^ r) & 0x8000))
        f |= P;
    if (res & 0x10000u)
        f |= C;
    af[0] = f;
}

static void z80_ed_sbc_hl_rr(gen_z80_t *z80, uint8_t *af, uint16_t hlv, uint16_t rr)
{
    uint32_t c = (af[0] & C) ? 1u : 0u;
    uint32_t rhs = (uint32_t)rr + c;
    uint32_t res = (uint32_t)hlv - rhs;
    uint16_t r = (uint16_t)res;
    z80->hl = r;
    uint8_t f = N | (rhs > (uint32_t)hlv ? C : 0);
    if (r == 0)
        f |= Z;
    if (r & 0x8000)
        f |= S;
    f |= (uint8_t)(((hlv ^ rr ^ r) >> 8) & H);
    if (((hlv ^ rr) & 0x8000) && ((hlv ^ r) & 0x8000))
        f |= P;
    af[0] = f;
}

static void z80_daa(uint8_t *a, uint8_t *fo)
{
    uint8_t A = *a;
    uint8_t F = *fo;
    uint8_t n = F & N;
    uint8_t c = (uint8_t)(F & C);
    uint8_t h = (uint8_t)(F & H);
    uint8_t lo, new_c;
    if (!n)
    {
        if ((A & 0xF) > 9 || h)
            A += 6;
        if (A > 0x9F || c)
        {
            A += 0x60;
            new_c = C;
        }
        else
            new_c = 0;
    }
    else
    {
        if (h)
            A -= 6;
        if (c)
            A -= 0x60;
        new_c = c;
    }
    *a = A;
    lo = (uint8_t)(F & N);
    *fo = (uint8_t)(lo | new_c | (A & S) | (A ? 0 : Z) | parity(A));
}

/* 0x40–0x7F excepto 0x76 (HALT): LD dst,src; 4 ciclos, 7 si src o dst es (HL). */
static void z80_ld_r_r(gen_z80_t *z80, void *ctx, uint8_t op, uint8_t *bc, uint8_t *de, uint8_t *hl, uint8_t *af, int *executed)
{
    unsigned dst = ((unsigned)op >> 3) & 7u;
    unsigned src = (unsigned)op & 7u;
    uint8_t v;
    switch (src)
    {
    case 0: v = bc[1]; break;
    case 1: v = bc[0]; break;
    case 2: v = de[1]; break;
    case 3: v = de[0]; break;
    case 4: v = hl[1]; break;
    case 5: v = hl[0]; break;
    case 6: v = z80_mem_read(ctx, z80->hl); break;
    default: v = af[1]; break;
    }
    int cyc = (src == 6u || dst == 6u) ? 7 : 4;
    switch (dst)
    {
    case 0: bc[1] = v; break;
    case 1: bc[0] = v; break;
    case 2: de[1] = v; break;
    case 3: de[0] = v; break;
    case 4: hl[1] = v; break;
    case 5: hl[0] = v; break;
    case 6: W8(z80->hl, v); break;
    default: af[1] = v; break;
    }
    *executed += cyc;
}

static void z80_add_index_rr(uint8_t *af, uint16_t *ixy, uint16_t rr, int *executed)
{
    uint32_t r = (uint32_t)*ixy + rr;
    af[0] = (uint8_t)((af[0] & (S | Z | P | N)) | ((r >> 16) ? C : 0) |
                      (((*ixy & 0x0FFFu) + (rr & 0x0FFFu)) & 0x1000u ? H : 0));
    *ixy = (uint16_t)r;
    *executed += 15;
}

/* CB sobre memoria [addr]; BIT usa cyc_bit, rot/RES/SET usa cyc_rw. Si r≠6 en el opcode, copia a reg. */
static void z80_cb_mem(void *ctx, uint16_t addr, uint8_t cb, uint8_t *af, uint8_t *bc, uint8_t *de, uint8_t *hl, int *executed, int cyc_bit, int cyc_rw)
{
    int r = cb & 7;
    uint8_t *regs[] = { &bc[1], &bc[0], &de[1], &de[0], &hl[1], &hl[0], NULL, &af[1] };
    uint8_t v = z80_mem_read(ctx, addr);
    uint8_t res = v;
    int op = (cb >> 3) & 0x1F;
    if (op <= 6)
    {
        uint8_t old_c = (uint8_t)(af[0] & C);
        switch (op)
        {
        case 0: res = (uint8_t)((v << 1) | (v >> 7)); af[0] = (uint8_t)((af[0] & (S | Z | P | N)) | (v >> 7)); break;  /* RLC */
        case 1: res = (uint8_t)((v >> 1) | (v << 7)); af[0] = (uint8_t)((af[0] & (S | Z | P | N)) | (v & 1)); break;   /* RRC */
        case 2: res = (uint8_t)((v << 1) | old_c); af[0] = (uint8_t)((af[0] & (S | Z | P | N)) | (v >> 7)); break;     /* RL */
        case 3: res = (uint8_t)((v >> 1) | (uint8_t)(old_c << 7)); af[0] = (uint8_t)((af[0] & (S | Z | P | N)) | (v & 1)); break; /* RR */
        case 4: res = (uint8_t)(v << 1); af[0] = (uint8_t)((af[0] & (S | Z | P | N)) | (v >> 7)); break;             /* SLA */
        case 5: res = (uint8_t)((v >> 1) | (v & 0x80)); af[0] = (uint8_t)((af[0] & (S | Z | P | N)) | (v & 1)); break; /* SRA */
        default: res = (uint8_t)(v >> 1); af[0] = (uint8_t)((af[0] & (S | Z | P | N)) | (v & 1)); break;             /* SRL */
        }
        af[0] = (uint8_t)(af[0] | (uint8_t)((res ? 0 : Z) | parity(res) | ((res & 0x80) ? S : 0)));
        W8(addr, res);
        if (r != 6)
            *regs[r] = res;
        *executed += cyc_rw;
    }
    else if (op >= 8 && op <= 15)
    {
        int bit = op - 8;
        int zb = !((v >> bit) & 1);
        af[0] = (uint8_t)((af[0] & C) | (zb ? Z : 0) | H | parity(v));
        if (bit == 7)
            af[0] = (uint8_t)(af[0] | ((v & 0x80) ? S : 0));
        *executed += cyc_bit;
    }
    else if (op >= 16 && op <= 23)
    {
        int bit = op - 16;
        res = (uint8_t)(v & ~(1 << bit));
        W8(addr, res);
        if (r != 6)
            *regs[r] = res;
        *executed += cyc_rw;
    }
    else
    {
        int bit = op - 24;
        res = (uint8_t)(v | (1 << bit));
        W8(addr, res);
        if (r != 6)
            *regs[r] = res;
        *executed += cyc_rw;
    }
}

static void z80_ei_post_instruction(gen_z80_t *z80)
{
    if (z80->ei_countdown > 0u)
    {
        z80->ei_countdown--;
        if (z80->ei_countdown == 0u)
        {
            z80->iff1 = 1u;
            z80->iff2 = 1u;
        }
    }
}

enum {
    Z80_IXALU_ADD = 0,
    Z80_IXALU_ADC,
    Z80_IXALU_SUB,
    Z80_IXALU_SBC,
    Z80_IXALU_AND,
    Z80_IXALU_XOR,
    Z80_IXALU_OR,
    Z80_IXALU_CP
};

/* DD/FD: SUB/ADC/… A,(IX+d) — 19 ciclos */
static void z80_alu_a_idisp(gen_z80_t *z80, void *ctx, uint16_t idx, uint8_t *af, int alu, int *executed)
{
    int8_t d = (int8_t)z80_mem_read(ctx, z80->pc++);
    uint8_t v = z80_mem_read(ctx, (uint16_t)(idx + d));
    uint8_t a = af[1];
    switch (alu)
    {
    case Z80_IXALU_ADD: {
        uint16_t r = (uint16_t)a + v;
        af[1] = (uint8_t)r;
        af[0] = (uint8_t)((r >> 8) | ((r & 0xFF) ? 0 : Z) | (((a & 0x0F) + (v & 0x0F)) & 0x10) | parity((uint8_t)r));
        break;
    }
    case Z80_IXALU_ADC: {
        uint8_t c = (af[0] & C) ? 1u : 0u;
        uint16_t r = (uint16_t)a + v + c;
        af[1] = (uint8_t)r;
        af[0] = (uint8_t)((r >> 8) | ((r & 0xFF) ? 0 : Z) | (((a & 0x0F) + (v & 0x0F) + c) & 0x10 ? H : 0) | parity((uint8_t)r));
        break;
    }
    case Z80_IXALU_SUB: {
        int res = (int)a - (int)v;
        uint8_t res8 = (uint8_t)res;
        af[1] = res8;
        int ov = ((a ^ v) & (a ^ res8)) & 0x80;
        af[0] = (uint8_t)(N | (res8 ? 0 : Z) | (res8 & S) | (((a & 0x0F) < (v & 0x0F)) ? H : 0) | (ov ? P : 0) | ((res & 0x100) ? C : 0));
        break;
    }
    case Z80_IXALU_SBC: {
        uint8_t c = (af[0] & C) ? 1u : 0u;
        int res = (int)a - (int)v - (int)c;
        uint8_t res8 = (uint8_t)res;
        af[1] = res8;
        int ov = ((a ^ v) & (a ^ res8)) & 0x80;
        af[0] = (uint8_t)(N | (res8 ? 0 : Z) | (res8 & S) | (((int)(a & 0xF) - (int)(v & 0xF) - (int)c) < 0 ? H : 0) | (ov ? P : 0) | ((res & 0x100) ? C : 0));
        break;
    }
    case Z80_IXALU_AND: {
        uint8_t r = (uint8_t)(a & v);
        af[1] = r;
        af[0] = (uint8_t)(H | (r & S) | (r ? 0 : Z) | parity(r));
        break;
    }
    case Z80_IXALU_XOR: {
        uint8_t r = (uint8_t)(a ^ v);
        af[1] = r;
        af[0] = (uint8_t)((r & S) | (r ? 0 : Z) | parity(r));
        break;
    }
    case Z80_IXALU_OR: {
        uint8_t r = (uint8_t)(a | v);
        af[1] = r;
        af[0] = (uint8_t)((r & S) | (r ? 0 : Z) | parity(r));
        break;
    }
    default: { /* CP */
        int res = (int)a - (int)v;
        uint8_t res8 = (uint8_t)res;
        int ov = ((a ^ v) & (a ^ res8)) & 0x80;
        af[0] = (uint8_t)(N | (res8 ? 0 : Z) | (res8 & S) | (((a & 0x0F) < (v & 0x0F)) ? H : 0) | (ov ? P : 0) | ((res & 0x100) ? C : 0));
        break;
    }
    }
    *executed += 19;
}

static void z80_ld_reg_idisp(gen_z80_t *z80, void *ctx, uint16_t idx, uint8_t *rp, int *executed)
{
    int8_t d = (int8_t)z80_mem_read(ctx, z80->pc++);
    *rp = z80_mem_read(ctx, (uint16_t)(idx + d));
    *executed += 19;
}

static void z80_ld_idisp_reg(gen_z80_t *z80, void *ctx, uint16_t idx, uint8_t v, int *executed)
{
    int8_t d = (int8_t)z80_mem_read(ctx, z80->pc++);
    W8((uint16_t)(idx + d), v);
    *executed += 19;
}

void gen_z80_init(gen_z80_t *z80)
{
    memset(z80, 0, sizeof(*z80));
}

void gen_z80_reset(gen_z80_t *z80)
{
    z80->pc = 0;
    z80->sp = 0;
    z80->af = z80->bc = z80->de = z80->hl = 0;
    z80->ix = z80->iy = 0;
    z80->iff1 = z80->iff2 = 0;
    z80->halted = 0;
    z80->bank = 0;
    z80->af_prime = 0;
    z80->ei_countdown = 0;
}

int gen_z80_step(gen_z80_t *z80, void *ctx, int cycles)
{
    genesis_impl_t *impl = (genesis_impl_t *)ctx;
    if (!gen_cpu_sync_z80_should_run(impl->z80_bus_req, impl->z80_reset))
        return 0;
    if (z80->halted)
        return cycles;

    int executed = 0;
    /* AF: little-endian storage; af[0]=F, af[1]=A */
    uint8_t *af = (uint8_t *)&z80->af;
    uint8_t *bc = (uint8_t *)&z80->bc;
    uint8_t *de = (uint8_t *)&z80->de;
    uint8_t *hl = (uint8_t *)&z80->hl;

    while (executed < cycles)
    {
        uint8_t op = R8();

        switch (op)
        {
        case 0x00: /* NOP */
            executed += 4;
            break;
        case 0x08: /* EX AF,AF' */
            {
                uint16_t t = z80->af;
                z80->af = z80->af_prime;
                z80->af_prime = t;
            }
            executed += 4;
            break;
        case 0x07: /* RLCA */
            {
                uint8_t a = af[1];
                af[0] = (uint8_t)((af[0] & ~(S | Z | P | H | N | C)) | ((a >> 7) & C));
                af[1] = (uint8_t)((a << 1) | (a >> 7));
            }
            executed += 4;
            break;
        case 0x0F: /* RRCA */
            {
                uint8_t a = af[1];
                af[0] = (uint8_t)((af[0] & ~(S | Z | P | H | N | C)) | (a & C));
                af[1] = (uint8_t)((a >> 1) | (a << 7));
            }
            executed += 4;
            break;
        case 0x17: /* RLA */
            {
                uint8_t a = af[1];
                uint8_t new_c = (uint8_t)((a >> 7) & C);
                af[1] = (uint8_t)((a << 1) | ((af[0] & C) ? 1u : 0u));
                af[0] = (uint8_t)((af[0] & ~(S | Z | P | H | N | C)) | new_c);
            }
            executed += 4;
            break;
        case 0x1F: /* RRA */
            {
                uint8_t a = af[1];
                uint8_t new_c = (uint8_t)(a & C);
                af[1] = (uint8_t)((a >> 1) | ((af[0] & C) ? (uint8_t)0x80 : 0u));
                af[0] = (uint8_t)((af[0] & ~(S | Z | P | H | N | C)) | new_c);
            }
            executed += 4;
            break;
        case 0x27: /* DAA */
            z80_daa(&af[1], &af[0]);
            executed += 4;
            break;
        case 0x2F: /* CPL */
            af[1] = (uint8_t)~af[1];
            af[0] = (uint8_t)((af[0] & C) | N | H | (af[1] & S) | (af[1] ? 0 : Z) | parity(af[1]));
            executed += 4;
            break;
        case 0x37: /* SCF */
            af[0] = (uint8_t)((af[0] & (S | Z | P)) | C);
            executed += 4;
            break;
        case 0x3F: /* CCF */
            {
                uint8_t oldc = (uint8_t)(af[0] & C);
                af[0] = (uint8_t)((af[0] & (S | Z | P)) | (oldc ? H : 0) | (oldc ? 0 : C));
            }
            executed += 4;
            break;
        case 0x3C: /* INC A */
            af[1]++;
            af[0] = (af[0] & C) | (af[1] ? 0 : Z) | ((af[1] & 0x0F) ? 0 : H) | (af[1] & S) | parity(af[1]);
            executed += 4;
            break;
        case 0x3D: /* DEC A */
            af[1]--;
            af[0] = (uint8_t)((af[0] & C) | N | (af[1] ? 0 : Z) | ((af[1] & 0x0F) == 0x0F ? H : 0) | (af[1] & S) | parity(af[1]));
            executed += 4;
            break;
        case 0x33: /* INC SP */
            z80->sp++;
            executed += 6;
            break;
        case 0x3B: /* DEC SP */
            z80->sp--;
            executed += 6;
            break;
        case 0x76: /* HALT */
            z80->halted = 1;
            executed += 4;
            z80_ei_post_instruction(z80);
            return executed;
        case 0xF3: /* DI */
            z80->iff1 = z80->iff2 = 0;
            z80->ei_countdown = 0;
            executed += 4;
            break;
        case 0xFB: /* EI: IFF tras ejecutar la instrucción siguiente */
            z80->ei_countdown = 2;
            executed += 4;
            break;
        case 0xC3: /* JP nn */
            z80->pc = R16();
            executed += 10;
            break;
        case 0xC9: /* RET */
            z80->pc = z80_mem_read(ctx, z80->sp) | (z80_mem_read(ctx, z80->sp + 1) << 8);
            z80->sp += 2;
            executed += 10;
            break;
        case 0xCD: /* CALL nn */
            {
                uint16_t nn = R16();
                z80->sp -= 2;
                W16(z80->sp, z80->pc);
                z80->pc = nn;
            }
            executed += 17;
            break;
        case 0xDB: /* IN A,(n) */
            af[1] = z80_port_in(ctx, R8());
            executed += 11;
            break;
        case 0x32: /* LD (nn),A */
            {
                uint16_t nn = R16();
                W8(nn, af[1]);
            }
            executed += 13;
            break;
        case 0x3A: /* LD A,(nn) */
            {
                uint16_t nn = R16();
                af[1] = z80_mem_read(ctx, nn);
            }
            executed += 13;
            break;
        case 0xD3: /* OUT (n),A */
            z80_port_out(ctx, R8(), HI(z80->af));
            executed += 11;
            break;
        case 0x06: bc[1] = R8(); executed += 7; break;  /* LD B,n */
        case 0x0E: bc[0] = R8(); executed += 7; break; /* LD C,n */
        case 0x16: de[1] = R8(); executed += 7; break;
        case 0x1E: de[0] = R8(); executed += 7; break;
        case 0x26: hl[1] = R8(); executed += 7; break;
        case 0x2E: hl[0] = R8(); executed += 7; break;
        case 0x36: /* LD (HL),n */
            W8(z80->hl, R8());
            executed += 10;
            break;
        case 0x7E: af[1] = z80_mem_read(ctx, z80->hl); executed += 7; break;  /* LD A,(HL) */
        case 0x77: W8(z80->hl, HI(z80->af)); executed += 7; break;  /* LD (HL),A */
        case 0x0A: af[1] = z80_mem_read(ctx, z80->bc); executed += 7; break;  /* LD A,(BC) */
        case 0x02: W8(z80->bc, HI(z80->af)); executed += 7; break;  /* LD (BC),A */
        case 0x1A: af[1] = z80_mem_read(ctx, z80->de); executed += 7; break;
        case 0x12: W8(z80->de, HI(z80->af)); executed += 7; break;
        case 0x78: af[1] = bc[1]; executed += 4; break;  /* LD A,B */
        case 0x79: af[1] = bc[0]; executed += 4; break;
        case 0x7A: af[1] = de[1]; executed += 4; break;
        case 0x7B: af[1] = de[0]; executed += 4; break;
        case 0x7C: af[1] = hl[1]; executed += 4; break;
        case 0x7D: af[1] = hl[0]; executed += 4; break;
        case 0x3E: af[1] = R8(); executed += 7; break;  /* LD A,n */
        case 0x47: bc[1] = HI(z80->af); executed += 4; break;  /* LD B,A */
        case 0x4F: bc[0] = HI(z80->af); executed += 4; break;
        case 0x57: de[1] = HI(z80->af); executed += 4; break;
        case 0x5F: de[0] = HI(z80->af); executed += 4; break;
        case 0x67: hl[1] = HI(z80->af); executed += 4; break;
        case 0x6F: hl[0] = HI(z80->af); executed += 4; break;
        case 0x01: z80->bc = R16(); executed += 10; break;  /* LD BC,nn */
        case 0x11: z80->de = R16(); executed += 10; break;
        case 0x21: z80->hl = R16(); executed += 10; break;
        case 0x31: z80->sp = R16(); executed += 10; break;
        case 0xE9: z80->pc = z80->hl; executed += 4; break;  /* JP (HL) */
        case 0xEB: { uint16_t t = z80->de; z80->de = z80->hl; z80->hl = t; } executed += 4; break;  /* EX DE,HL */
        case 0xF9: z80->sp = z80->hl; executed += 6; break;  /* LD SP,HL */
        case 0x2A: { uint16_t nn = R16(); z80->hl = z80_mem_read(ctx, nn) | (z80_mem_read(ctx, nn + 1) << 8); } executed += 16; break;  /* LD HL,(nn) */
        case 0x22: { uint16_t nn = R16(); W16(nn, z80->hl); } executed += 16; break;  /* LD (nn),HL */
        case 0xF5: z80->sp -= 2; W16(z80->sp, z80->af); executed += 11; break;  /* PUSH AF */
        case 0xC5: z80->sp -= 2; W16(z80->sp, z80->bc); executed += 11; break;
        case 0xD5: z80->sp -= 2; W16(z80->sp, z80->de); executed += 11; break;
        case 0xE5: z80->sp -= 2; W16(z80->sp, z80->hl); executed += 11; break;
        case 0xF1: z80->af = z80_mem_read(ctx, z80->sp) | (z80_mem_read(ctx, z80->sp + 1) << 8); z80->sp += 2; executed += 10; break;  /* POP AF */
        case 0xC1: z80->bc = z80_mem_read(ctx, z80->sp) | (z80_mem_read(ctx, z80->sp + 1) << 8); z80->sp += 2; executed += 10; break;
        case 0xD1: z80->de = z80_mem_read(ctx, z80->sp) | (z80_mem_read(ctx, z80->sp + 1) << 8); z80->sp += 2; executed += 10; break;
        case 0xE1: z80->hl = z80_mem_read(ctx, z80->sp) | (z80_mem_read(ctx, z80->sp + 1) << 8); z80->sp += 2; executed += 10; break;
        case 0xE3: { /* EX (SP),HL */
            uint8_t l = z80_mem_read(ctx, z80->sp);
            uint8_t h = z80_mem_read(ctx, z80->sp + 1);
            W8(z80->sp, hl[0]);
            W8(z80->sp + 1, hl[1]);
            hl[0] = l;
            hl[1] = h;
            executed += 19;
            break;
        }
        case 0x04: bc[1]++; af[0] = (af[0] & C) | (bc[1] ? 0 : Z) | ((bc[1] & 0x0F) ? 0 : H); executed += 4; break;  /* INC B */
        case 0x0C: bc[0]++; af[0] = (af[0] & C) | (bc[0] ? 0 : Z) | ((bc[0] & 0x0F) ? 0 : H); executed += 4; break;
        case 0x14: de[1]++; af[0] = (af[0] & C) | (de[1] ? 0 : Z) | ((de[1] & 0x0F) ? 0 : H); executed += 4; break;
        case 0x1C: de[0]++; af[0] = (af[0] & C) | (de[0] ? 0 : Z) | ((de[0] & 0x0F) ? 0 : H); executed += 4; break;
        case 0x24: hl[1]++; af[0] = (af[0] & C) | (hl[1] ? 0 : Z) | ((hl[1] & 0x0F) ? 0 : H); executed += 4; break;
        case 0x2C: hl[0]++; af[0] = (af[0] & C) | (hl[0] ? 0 : Z) | ((hl[0] & 0x0F) ? 0 : H); executed += 4; break;
        case 0x34: { uint8_t v = z80_mem_read(ctx, z80->hl) + 1; W8(z80->hl, v); af[0] = (af[0] & C) | (v ? 0 : Z) | ((v & 0x0F) ? 0 : H); } executed += 11; break;
        case 0x05: bc[1]--; af[0] = (af[0] & C) | N | (bc[1] ? 0 : Z) | ((bc[1] & 0x0F) == 0x0F ? H : 0); executed += 4; break;  /* DEC B */
        case 0x0D: bc[0]--; af[0] = (af[0] & C) | N | (bc[0] ? 0 : Z) | ((bc[0] & 0x0F) == 0x0F ? H : 0); executed += 4; break;
        case 0x15: de[1]--; af[0] = (af[0] & C) | N | (de[1] ? 0 : Z) | ((de[1] & 0x0F) == 0x0F ? H : 0); executed += 4; break;
        case 0x1D: de[0]--; af[0] = (af[0] & C) | N | (de[0] ? 0 : Z) | ((de[0] & 0x0F) == 0x0F ? H : 0); executed += 4; break;
        case 0x25: hl[1]--; af[0] = (af[0] & C) | N | (hl[1] ? 0 : Z) | ((hl[1] & 0x0F) == 0x0F ? H : 0); executed += 4; break;
        case 0x2D: hl[0]--; af[0] = (af[0] & C) | N | (hl[0] ? 0 : Z) | ((hl[0] & 0x0F) == 0x0F ? H : 0); executed += 4; break;
        case 0x35: { uint8_t v = z80_mem_read(ctx, z80->hl) - 1; W8(z80->hl, v); af[0] = (af[0] & C) | N | (v ? 0 : Z) | ((v & 0x0F) == 0x0F ? H : 0); } executed += 11; break;
        case 0xA6: /* AND (HL) */
            {
                uint8_t v = z80_mem_read(ctx, z80->hl);
                af[1] &= v;
                uint8_t r = af[1];
                af[0] = H | (r & S) | (r ? 0 : Z) | parity(r);
                executed += 7;
            }
            break;
        case 0xB6: /* OR (HL) */
            {
                uint8_t v = z80_mem_read(ctx, z80->hl);
                af[1] |= v;
                uint8_t r = af[1];
                af[0] = (r & S) | (r ? 0 : Z) | parity(r);
                executed += 7;
            }
            break;
        case 0xAE: /* XOR (HL) */
            {
                uint8_t v = z80_mem_read(ctx, z80->hl);
                af[1] ^= v;
                uint8_t r = af[1];
                af[0] = (r & S) | (r ? 0 : Z) | parity(r);
                executed += 7;
            }
            break;
        case 0x96: /* SUB (HL) */
            {
                uint8_t v = z80_mem_read(ctx, z80->hl);
                uint8_t a = af[1];
                int res = (int)a - (int)v;
                uint8_t res8 = (uint8_t)res;
                af[1] = res8;
                int ov = ((a ^ v) & (a ^ res8)) & 0x80;
                af[0] = N | (res8 ? 0 : Z) | (res8 & S) | (((a & 0x0F) < (v & 0x0F)) ? H : 0) |
                    (ov ? P : 0) | ((res & 0x100) ? C : 0);
                executed += 7;
            }
            break;
        case 0xBE: /* CP (HL) */
            {
                uint8_t v = z80_mem_read(ctx, z80->hl);
                uint8_t a = af[1];
                int res = (int)a - (int)v;
                uint8_t res8 = (uint8_t)res;
                int ov = ((a ^ v) & (a ^ res8)) & 0x80;
                af[0] = N | (res8 ? 0 : Z) | (res8 & S) | (((a & 0x0F) < (v & 0x0F)) ? H : 0) |
                    (ov ? P : 0) | ((res & 0x100) ? C : 0);
                executed += 7;
            }
            break;
        case 0x86: { uint8_t v = z80_mem_read(ctx, z80->hl); uint16_t r = HI(z80->af) + v; af[0] = (r >> 8) | ((r & 0xFF) ? 0 : Z) | (((HI(z80->af) & 0x0F) + (v & 0x0F)) & 0x10) | parity((uint8_t)r); af[1] = (uint8_t)r; } executed += 7; break;  /* ADD A,(HL) */
        case 0x80: { uint16_t r = HI(z80->af) + bc[1]; af[0] = (r >> 8) | ((r & 0xFF) ? 0 : Z) | (((HI(z80->af) & 0x0F) + (bc[1] & 0x0F)) & 0x10) | parity((uint8_t)r); af[1] = (uint8_t)r; } executed += 4; break;  /* ADD A,B */
        case 0x81: { uint16_t r = HI(z80->af) + bc[0]; af[0] = (r >> 8) | ((r & 0xFF) ? 0 : Z) | (((HI(z80->af) & 0x0F) + (bc[0] & 0x0F)) & 0x10) | parity((uint8_t)r); af[1] = (uint8_t)r; } executed += 4; break;
        case 0x82: { uint16_t r = HI(z80->af) + de[1]; af[0] = (r >> 8) | ((r & 0xFF) ? 0 : Z) | (((HI(z80->af) & 0x0F) + (de[1] & 0x0F)) & 0x10) | parity((uint8_t)r); af[1] = (uint8_t)r; } executed += 4; break;
        case 0x83: { uint16_t r = HI(z80->af) + de[0]; af[0] = (r >> 8) | ((r & 0xFF) ? 0 : Z) | (((HI(z80->af) & 0x0F) + (de[0] & 0x0F)) & 0x10) | parity((uint8_t)r); af[1] = (uint8_t)r; } executed += 4; break;
        case 0x84: { uint16_t r = HI(z80->af) + hl[1]; af[0] = (r >> 8) | ((r & 0xFF) ? 0 : Z) | (((HI(z80->af) & 0x0F) + (hl[1] & 0x0F)) & 0x10) | parity((uint8_t)r); af[1] = (uint8_t)r; } executed += 4; break;
        case 0x85: { uint16_t r = HI(z80->af) + hl[0]; af[0] = (r >> 8) | ((r & 0xFF) ? 0 : Z) | (((HI(z80->af) & 0x0F) + (hl[0] & 0x0F)) & 0x10) | parity((uint8_t)r); af[1] = (uint8_t)r; } executed += 4; break;
        case 0x87: { uint16_t r = (uint16_t)(af[1] + af[1]); af[0] = (r >> 8) | ((r & 0xFF) ? 0 : Z) | (((af[1] & 0x0F) + (af[1] & 0x0F)) & 0x10) | parity((uint8_t)r); af[1] = (uint8_t)r; } executed += 4; break;  /* ADD A,A */
        case 0x88: { uint8_t v = bc[1], c = (af[0] & C) ? 1u : 0u; uint16_t r = (uint16_t)af[1] + v + c; af[0] = (r >> 8) | ((r & 0xFF) ? 0 : Z) | (((af[1] & 0x0F) + (v & 0x0F) + c) & 0x10 ? H : 0) | parity((uint8_t)r); af[1] = (uint8_t)r; } executed += 4; break;
        case 0x89: { uint8_t v = bc[0], c = (af[0] & C) ? 1u : 0u; uint16_t r = (uint16_t)af[1] + v + c; af[0] = (r >> 8) | ((r & 0xFF) ? 0 : Z) | (((af[1] & 0x0F) + (v & 0x0F) + c) & 0x10 ? H : 0) | parity((uint8_t)r); af[1] = (uint8_t)r; } executed += 4; break;
        case 0x8A: { uint8_t v = de[1], c = (af[0] & C) ? 1u : 0u; uint16_t r = (uint16_t)af[1] + v + c; af[0] = (r >> 8) | ((r & 0xFF) ? 0 : Z) | (((af[1] & 0x0F) + (v & 0x0F) + c) & 0x10 ? H : 0) | parity((uint8_t)r); af[1] = (uint8_t)r; } executed += 4; break;
        case 0x8B: { uint8_t v = de[0], c = (af[0] & C) ? 1u : 0u; uint16_t r = (uint16_t)af[1] + v + c; af[0] = (r >> 8) | ((r & 0xFF) ? 0 : Z) | (((af[1] & 0x0F) + (v & 0x0F) + c) & 0x10 ? H : 0) | parity((uint8_t)r); af[1] = (uint8_t)r; } executed += 4; break;
        case 0x8C: { uint8_t v = hl[1], c = (af[0] & C) ? 1u : 0u; uint16_t r = (uint16_t)af[1] + v + c; af[0] = (r >> 8) | ((r & 0xFF) ? 0 : Z) | (((af[1] & 0x0F) + (v & 0x0F) + c) & 0x10 ? H : 0) | parity((uint8_t)r); af[1] = (uint8_t)r; } executed += 4; break;
        case 0x8D: { uint8_t v = hl[0], c = (af[0] & C) ? 1u : 0u; uint16_t r = (uint16_t)af[1] + v + c; af[0] = (r >> 8) | ((r & 0xFF) ? 0 : Z) | (((af[1] & 0x0F) + (v & 0x0F) + c) & 0x10 ? H : 0) | parity((uint8_t)r); af[1] = (uint8_t)r; } executed += 4; break;
        case 0x8E: { uint8_t v = z80_mem_read(ctx, z80->hl), co = (af[0] & C) ? 1u : 0u; uint16_t r = (uint16_t)af[1] + v + co; af[0] = (r >> 8) | ((r & 0xFF) ? 0 : Z) | (((af[1] & 0x0F) + (v & 0x0F) + co) & 0x10 ? H : 0) | parity((uint8_t)r); af[1] = (uint8_t)r; } executed += 7; break;
        case 0x8F: { uint8_t v = af[1], co = (af[0] & C) ? 1u : 0u; uint16_t r = (uint16_t)af[1] + v + co; af[0] = (r >> 8) | ((r & 0xFF) ? 0 : Z) | (((af[1] & 0x0F) + (v & 0x0F) + co) & 0x10 ? H : 0) | parity((uint8_t)r); af[1] = (uint8_t)r; } executed += 4; break;
        case 0x90: { uint8_t a = af[1], v = bc[1]; int res = (int)a - (int)v; uint8_t r8 = (uint8_t)res; int ov = ((a ^ v) & (a ^ r8)) & 0x80; af[1] = r8; af[0] = (uint8_t)(N | (r8 ? 0 : Z) | (r8 & S) | (((a & 0x0F) < (v & 0x0F)) ? H : 0) | (ov ? P : 0) | ((res & 0x100) ? C : 0)); } executed += 4; break;
        case 0x91: { uint8_t a = af[1], v = bc[0]; int res = (int)a - (int)v; uint8_t r8 = (uint8_t)res; int ov = ((a ^ v) & (a ^ r8)) & 0x80; af[1] = r8; af[0] = (uint8_t)(N | (r8 ? 0 : Z) | (r8 & S) | (((a & 0x0F) < (v & 0x0F)) ? H : 0) | (ov ? P : 0) | ((res & 0x100) ? C : 0)); } executed += 4; break;
        case 0x92: { uint8_t a = af[1], v = de[1]; int res = (int)a - (int)v; uint8_t r8 = (uint8_t)res; int ov = ((a ^ v) & (a ^ r8)) & 0x80; af[1] = r8; af[0] = (uint8_t)(N | (r8 ? 0 : Z) | (r8 & S) | (((a & 0x0F) < (v & 0x0F)) ? H : 0) | (ov ? P : 0) | ((res & 0x100) ? C : 0)); } executed += 4; break;
        case 0x93: { uint8_t a = af[1], v = de[0]; int res = (int)a - (int)v; uint8_t r8 = (uint8_t)res; int ov = ((a ^ v) & (a ^ r8)) & 0x80; af[1] = r8; af[0] = (uint8_t)(N | (r8 ? 0 : Z) | (r8 & S) | (((a & 0x0F) < (v & 0x0F)) ? H : 0) | (ov ? P : 0) | ((res & 0x100) ? C : 0)); } executed += 4; break;
        case 0x94: { uint8_t a = af[1], v = hl[1]; int res = (int)a - (int)v; uint8_t r8 = (uint8_t)res; int ov = ((a ^ v) & (a ^ r8)) & 0x80; af[1] = r8; af[0] = (uint8_t)(N | (r8 ? 0 : Z) | (r8 & S) | (((a & 0x0F) < (v & 0x0F)) ? H : 0) | (ov ? P : 0) | ((res & 0x100) ? C : 0)); } executed += 4; break;
        case 0x95: { uint8_t a = af[1], v = hl[0]; int res = (int)a - (int)v; uint8_t r8 = (uint8_t)res; int ov = ((a ^ v) & (a ^ r8)) & 0x80; af[1] = r8; af[0] = (uint8_t)(N | (r8 ? 0 : Z) | (r8 & S) | (((a & 0x0F) < (v & 0x0F)) ? H : 0) | (ov ? P : 0) | ((res & 0x100) ? C : 0)); } executed += 4; break;
        case 0x97: { uint8_t a = af[1]; int res = 0; af[1] = 0; af[0] = (uint8_t)(N | Z | parity(0)); (void)a; (void)res; } executed += 4; break;  /* SUB A,A */
        case 0x98: { uint8_t a = af[1], v = bc[1], c = (af[0] & C) ? 1u : 0u; int res = (int)a - (int)v - (int)c; uint8_t r8 = (uint8_t)res; int ov = ((a ^ v) & (a ^ r8)) & 0x80; af[1] = r8; af[0] = (uint8_t)(N | (r8 ? 0 : Z) | (r8 & S) | (((int)(a & 0xF) - (int)(v & 0xF) - (int)c) < 0 ? H : 0) | (ov ? P : 0) | ((res & 0x100) ? C : 0)); } executed += 4; break;
        case 0x99: { uint8_t a = af[1], v = bc[0], c = (af[0] & C) ? 1u : 0u; int res = (int)a - (int)v - (int)c; uint8_t r8 = (uint8_t)res; int ov = ((a ^ v) & (a ^ r8)) & 0x80; af[1] = r8; af[0] = (uint8_t)(N | (r8 ? 0 : Z) | (r8 & S) | (((int)(a & 0xF) - (int)(v & 0xF) - (int)c) < 0 ? H : 0) | (ov ? P : 0) | ((res & 0x100) ? C : 0)); } executed += 4; break;
        case 0x9A: { uint8_t a = af[1], v = de[1], c = (af[0] & C) ? 1u : 0u; int res = (int)a - (int)v - (int)c; uint8_t r8 = (uint8_t)res; int ov = ((a ^ v) & (a ^ r8)) & 0x80; af[1] = r8; af[0] = (uint8_t)(N | (r8 ? 0 : Z) | (r8 & S) | (((int)(a & 0xF) - (int)(v & 0xF) - (int)c) < 0 ? H : 0) | (ov ? P : 0) | ((res & 0x100) ? C : 0)); } executed += 4; break;
        case 0x9B: { uint8_t a = af[1], v = de[0], c = (af[0] & C) ? 1u : 0u; int res = (int)a - (int)v - (int)c; uint8_t r8 = (uint8_t)res; int ov = ((a ^ v) & (a ^ r8)) & 0x80; af[1] = r8; af[0] = (uint8_t)(N | (r8 ? 0 : Z) | (r8 & S) | (((int)(a & 0xF) - (int)(v & 0xF) - (int)c) < 0 ? H : 0) | (ov ? P : 0) | ((res & 0x100) ? C : 0)); } executed += 4; break;
        case 0x9C: { uint8_t a = af[1], v = hl[1], c = (af[0] & C) ? 1u : 0u; int res = (int)a - (int)v - (int)c; uint8_t r8 = (uint8_t)res; int ov = ((a ^ v) & (a ^ r8)) & 0x80; af[1] = r8; af[0] = (uint8_t)(N | (r8 ? 0 : Z) | (r8 & S) | (((int)(a & 0xF) - (int)(v & 0xF) - (int)c) < 0 ? H : 0) | (ov ? P : 0) | ((res & 0x100) ? C : 0)); } executed += 4; break;
        case 0x9D: { uint8_t a = af[1], v = hl[0], c = (af[0] & C) ? 1u : 0u; int res = (int)a - (int)v - (int)c; uint8_t r8 = (uint8_t)res; int ov = ((a ^ v) & (a ^ r8)) & 0x80; af[1] = r8; af[0] = (uint8_t)(N | (r8 ? 0 : Z) | (r8 & S) | (((int)(a & 0xF) - (int)(v & 0xF) - (int)c) < 0 ? H : 0) | (ov ? P : 0) | ((res & 0x100) ? C : 0)); } executed += 4; break;
        case 0x9E: { uint8_t a = af[1], v = z80_mem_read(ctx, z80->hl), c = (af[0] & C) ? 1u : 0u; int res = (int)a - (int)v - (int)c; uint8_t r8 = (uint8_t)res; int ov = ((a ^ v) & (a ^ r8)) & 0x80; af[1] = r8; af[0] = (uint8_t)(N | (r8 ? 0 : Z) | (r8 & S) | (((int)(a & 0xF) - (int)(v & 0xF) - (int)c) < 0 ? H : 0) | (ov ? P : 0) | ((res & 0x100) ? C : 0)); } executed += 7; break;
        case 0x9F: { uint8_t a = af[1], c = (af[0] & C) ? 1u : 0u; int res = (int)a - (int)a - (int)c; uint8_t r8 = (uint8_t)res; int ov = ((a ^ a) & (a ^ r8)) & 0x80; af[1] = r8; af[0] = (uint8_t)(N | (r8 ? 0 : Z) | (r8 & S) | (((int)(a & 0xF) - (int)(a & 0xF) - (int)c) < 0 ? H : 0) | (ov ? P : 0) | ((res & 0x100) ? C : 0)); } executed += 4; break;
        case 0xA0: { uint8_t r = (uint8_t)(af[1] & bc[1]); af[1] = r; af[0] = (uint8_t)(H | (r & S) | (r ? 0 : Z) | parity(r)); } executed += 4; break;
        case 0xA1: { uint8_t r = (uint8_t)(af[1] & bc[0]); af[1] = r; af[0] = (uint8_t)(H | (r & S) | (r ? 0 : Z) | parity(r)); } executed += 4; break;
        case 0xA2: { uint8_t r = (uint8_t)(af[1] & de[1]); af[1] = r; af[0] = (uint8_t)(H | (r & S) | (r ? 0 : Z) | parity(r)); } executed += 4; break;
        case 0xA3: { uint8_t r = (uint8_t)(af[1] & de[0]); af[1] = r; af[0] = (uint8_t)(H | (r & S) | (r ? 0 : Z) | parity(r)); } executed += 4; break;
        case 0xA4: { uint8_t r = (uint8_t)(af[1] & hl[1]); af[1] = r; af[0] = (uint8_t)(H | (r & S) | (r ? 0 : Z) | parity(r)); } executed += 4; break;
        case 0xA5: { uint8_t r = (uint8_t)(af[1] & hl[0]); af[1] = r; af[0] = (uint8_t)(H | (r & S) | (r ? 0 : Z) | parity(r)); } executed += 4; break;
        case 0xA7: { uint8_t r = af[1]; af[0] = (uint8_t)(H | (r & S) | (r ? 0 : Z) | parity(r)); executed += 4; break; }
        case 0xA8: { uint8_t r = (uint8_t)(af[1] ^ bc[1]); af[1] = r; af[0] = (uint8_t)((r & S) | (r ? 0 : Z) | parity(r)); } executed += 4; break;
        case 0xA9: { uint8_t r = (uint8_t)(af[1] ^ bc[0]); af[1] = r; af[0] = (uint8_t)((r & S) | (r ? 0 : Z) | parity(r)); } executed += 4; break;
        case 0xAA: { uint8_t r = (uint8_t)(af[1] ^ de[1]); af[1] = r; af[0] = (uint8_t)((r & S) | (r ? 0 : Z) | parity(r)); } executed += 4; break;
        case 0xAB: { uint8_t r = (uint8_t)(af[1] ^ de[0]); af[1] = r; af[0] = (uint8_t)((r & S) | (r ? 0 : Z) | parity(r)); } executed += 4; break;
        case 0xAC: { uint8_t r = (uint8_t)(af[1] ^ hl[1]); af[1] = r; af[0] = (uint8_t)((r & S) | (r ? 0 : Z) | parity(r)); } executed += 4; break;
        case 0xAD: { uint8_t r = (uint8_t)(af[1] ^ hl[0]); af[1] = r; af[0] = (uint8_t)((r & S) | (r ? 0 : Z) | parity(r)); } executed += 4; break;
        case 0xAF: { af[1] = 0; af[0] = (uint8_t)(Z | parity(0)); } executed += 4; break;
        case 0xB0: { uint8_t r = (uint8_t)(af[1] | bc[1]); af[1] = r; af[0] = (uint8_t)((r & S) | (r ? 0 : Z) | parity(r)); } executed += 4; break;
        case 0xB1: { uint8_t r = (uint8_t)(af[1] | bc[0]); af[1] = r; af[0] = (uint8_t)((r & S) | (r ? 0 : Z) | parity(r)); } executed += 4; break;
        case 0xB2: { uint8_t r = (uint8_t)(af[1] | de[1]); af[1] = r; af[0] = (uint8_t)((r & S) | (r ? 0 : Z) | parity(r)); } executed += 4; break;
        case 0xB3: { uint8_t r = (uint8_t)(af[1] | de[0]); af[1] = r; af[0] = (uint8_t)((r & S) | (r ? 0 : Z) | parity(r)); } executed += 4; break;
        case 0xB4: { uint8_t r = (uint8_t)(af[1] | hl[1]); af[1] = r; af[0] = (uint8_t)((r & S) | (r ? 0 : Z) | parity(r)); } executed += 4; break;
        case 0xB5: { uint8_t r = (uint8_t)(af[1] | hl[0]); af[1] = r; af[0] = (uint8_t)((r & S) | (r ? 0 : Z) | parity(r)); } executed += 4; break;
        case 0xB7: { /* OR A,A */ uint8_t r = af[1]; af[0] = (uint8_t)((r & S) | (r ? 0 : Z) | parity(r)); } executed += 4; break;
        case 0xB8: { uint8_t v = bc[1], a = af[1]; int res = (int)a - (int)v; uint8_t res8 = (uint8_t)res; int ov = ((a ^ v) & (a ^ res8)) & 0x80; af[0] = (uint8_t)(N | (res8 ? 0 : Z) | (res8 & S) | (((a & 0x0F) < (v & 0x0F)) ? H : 0) | (ov ? P : 0) | ((res & 0x100) ? C : 0)); } executed += 4; break;
        case 0xB9: { uint8_t v = bc[0], a = af[1]; int res = (int)a - (int)v; uint8_t res8 = (uint8_t)res; int ov = ((a ^ v) & (a ^ res8)) & 0x80; af[0] = (uint8_t)(N | (res8 ? 0 : Z) | (res8 & S) | (((a & 0x0F) < (v & 0x0F)) ? H : 0) | (ov ? P : 0) | ((res & 0x100) ? C : 0)); } executed += 4; break;
        case 0xBA: { uint8_t v = de[1], a = af[1]; int res = (int)a - (int)v; uint8_t res8 = (uint8_t)res; int ov = ((a ^ v) & (a ^ res8)) & 0x80; af[0] = (uint8_t)(N | (res8 ? 0 : Z) | (res8 & S) | (((a & 0x0F) < (v & 0x0F)) ? H : 0) | (ov ? P : 0) | ((res & 0x100) ? C : 0)); } executed += 4; break;
        case 0xBB: { uint8_t v = de[0], a = af[1]; int res = (int)a - (int)v; uint8_t res8 = (uint8_t)res; int ov = ((a ^ v) & (a ^ res8)) & 0x80; af[0] = (uint8_t)(N | (res8 ? 0 : Z) | (res8 & S) | (((a & 0x0F) < (v & 0x0F)) ? H : 0) | (ov ? P : 0) | ((res & 0x100) ? C : 0)); } executed += 4; break;
        case 0xBC: { uint8_t v = hl[1], a = af[1]; int res = (int)a - (int)v; uint8_t res8 = (uint8_t)res; int ov = ((a ^ v) & (a ^ res8)) & 0x80; af[0] = (uint8_t)(N | (res8 ? 0 : Z) | (res8 & S) | (((a & 0x0F) < (v & 0x0F)) ? H : 0) | (ov ? P : 0) | ((res & 0x100) ? C : 0)); } executed += 4; break;
        case 0xBD: { uint8_t v = hl[0], a = af[1]; int res = (int)a - (int)v; uint8_t res8 = (uint8_t)res; int ov = ((a ^ v) & (a ^ res8)) & 0x80; af[0] = (uint8_t)(N | (res8 ? 0 : Z) | (res8 & S) | (((a & 0x0F) < (v & 0x0F)) ? H : 0) | (ov ? P : 0) | ((res & 0x100) ? C : 0)); } executed += 4; break;
        case 0xBF: { /* CP A */ af[0] = (uint8_t)(N | Z | parity(0)); } executed += 4; break;
        case 0xC6: { uint8_t n = R8(); uint16_t r = HI(z80->af) + n; af[0] = (r >> 8) | ((r & 0xFF) ? 0 : Z) | (((HI(z80->af) & 0x0F) + (n & 0x0F)) & 0x10) | parity((uint8_t)r); af[1] = (uint8_t)r; } executed += 7; break;
        case 0xD6: { uint8_t v = R8(), a = af[1]; int res = (int)a - (int)v; uint8_t r8 = (uint8_t)res; int ov = ((a ^ v) & (a ^ r8)) & 0x80; af[1] = r8; af[0] = (uint8_t)(N | (r8 ? 0 : Z) | (r8 & S) | (((a & 0x0F) < (v & 0x0F)) ? H : 0) | (ov ? P : 0) | ((res & 0x100) ? C : 0)); } executed += 7; break;
        case 0xFE: { uint8_t v = R8(), a = af[1]; int res = (int)a - (int)v; uint8_t res8 = (uint8_t)res; int ov = ((a ^ v) & (a ^ res8)) & 0x80; af[0] = (uint8_t)(N | (res8 ? 0 : Z) | (res8 & S) | (((a & 0x0F) < (v & 0x0F)) ? H : 0) | (ov ? P : 0) | ((res & 0x100) ? C : 0)); } executed += 7; break;

        case 0x18: { int8_t e = R8(); z80->pc += e; executed += 12; } break;  /* JR e */
        case 0x20: { int8_t e = R8(); if (!(af[1] & Z)) z80->pc += e; executed += 7; } break;  /* JR NZ,e */
        case 0x28: { int8_t e = R8(); if (af[1] & Z) z80->pc += e; executed += 7; } break;   /* JR Z,e */
        case 0x30: { int8_t e = R8(); if (!(af[1] & C)) z80->pc += e; executed += 7; } break; /* JR NC,e */
        case 0x38: { int8_t e = R8(); if (af[1] & C) z80->pc += e; executed += 7; } break;   /* JR C,e */
        case 0x10: { int8_t e = R8(); bc[1]--; if (bc[1]) z80->pc += e; executed += 8; } break;  /* DJNZ e */
        case 0xC7: z80->sp -= 2; W16(z80->sp, z80->pc); z80->pc = 0x00; executed += 11; break;  /* RST 0 */
        case 0xCF: z80->sp -= 2; W16(z80->sp, z80->pc); z80->pc = 0x08; executed += 11; break;  /* RST 8 */
        case 0xD7: z80->sp -= 2; W16(z80->sp, z80->pc); z80->pc = 0x10; executed += 11; break;
        case 0xDF: z80->sp -= 2; W16(z80->sp, z80->pc); z80->pc = 0x18; executed += 11; break;
        case 0xE7: z80->sp -= 2; W16(z80->sp, z80->pc); z80->pc = 0x20; executed += 11; break;
        case 0xEF: z80->sp -= 2; W16(z80->sp, z80->pc); z80->pc = 0x28; executed += 11; break;
        case 0xF7: z80->sp -= 2; W16(z80->sp, z80->pc); z80->pc = 0x30; executed += 11; break;
        case 0xFF: z80->sp -= 2; W16(z80->sp, z80->pc); z80->pc = 0x38; executed += 11; break;
        case 0xC2: { uint16_t nn = R16(); if (!(af[1] & Z)) z80->pc = nn; executed += 10; } break;  /* JP NZ,nn */
        case 0xCA: { uint16_t nn = R16(); if (af[1] & Z) z80->pc = nn; executed += 10; } break;
        case 0xD2: { uint16_t nn = R16(); if (!(af[1] & C)) z80->pc = nn; executed += 10; } break;
        case 0xDA: { uint16_t nn = R16(); if (af[1] & C) z80->pc = nn; executed += 10; } break;
        case 0xC4: { uint16_t nn = R16(); if (!(af[1] & Z)) { z80->sp -= 2; W16(z80->sp, z80->pc); z80->pc = nn; } executed += 10; } break;  /* CALL NZ,nn */
        case 0xCC: { uint16_t nn = R16(); if (af[1] & Z) { z80->sp -= 2; W16(z80->sp, z80->pc); z80->pc = nn; } executed += 10; } break;
        case 0xD4: { uint16_t nn = R16(); if (!(af[1] & C)) { z80->sp -= 2; W16(z80->sp, z80->pc); z80->pc = nn; } executed += 10; } break;
        case 0xDC: { uint16_t nn = R16(); if (af[1] & C) { z80->sp -= 2; W16(z80->sp, z80->pc); z80->pc = nn; } executed += 10; } break;
        case 0xC0: if (!(af[1] & Z)) { z80->pc = z80_mem_read(ctx, z80->sp) | (z80_mem_read(ctx, z80->sp + 1) << 8); z80->sp += 2; } executed += 5; break;  /* RET NZ */
        case 0xC8: if (af[1] & Z) { z80->pc = z80_mem_read(ctx, z80->sp) | (z80_mem_read(ctx, z80->sp + 1) << 8); z80->sp += 2; } executed += 5; break;
        case 0xD0: if (!(af[1] & C)) { z80->pc = z80_mem_read(ctx, z80->sp) | (z80_mem_read(ctx, z80->sp + 1) << 8); z80->sp += 2; } executed += 5; break;
        case 0xD8: if (af[1] & C) { z80->pc = z80_mem_read(ctx, z80->sp) | (z80_mem_read(ctx, z80->sp + 1) << 8); z80->sp += 2; } executed += 5; break;
        case 0x13: z80->de++; executed += 6; break;  /* INC DE */
        case 0x23: z80->hl++; executed += 6; break;  /* INC HL */
        case 0x1B: z80->de--; executed += 6; break;
        case 0x2B: z80->hl--; executed += 6; break;
        case 0x03: z80->bc++; executed += 6; break;
        case 0x09: { uint32_t r = z80->hl + z80->bc; af[0] = (af[0] & (S|Z|P|N)) | ((r >> 16) ? C : 0) | (((z80->hl & 0x0FFF) + (z80->bc & 0x0FFF)) & 0x1000 ? H : 0); z80->hl = (uint16_t)r; } executed += 11; break;  /* ADD HL,BC */
        case 0x19: { uint32_t r = z80->hl + z80->de; af[0] = (af[0] & (S|Z|P|N)) | ((r >> 16) ? C : 0) | (((z80->hl & 0x0FFF) + (z80->de & 0x0FFF)) & 0x1000 ? H : 0); z80->hl = (uint16_t)r; } executed += 11; break;
        case 0x29: { uint32_t r = z80->hl + z80->hl; af[0] = (af[0] & (S|Z|P|N)) | ((r >> 16) ? C : 0) | (((z80->hl & 0x0FFF) + (z80->hl & 0x0FFF)) & 0x1000 ? H : 0); z80->hl = (uint16_t)r; } executed += 11; break;
        case 0x39: { uint32_t r = z80->hl + z80->sp; af[0] = (af[0] & (S|Z|P|N)) | ((r >> 16) ? C : 0) | (((z80->hl & 0x0FFF) + (z80->sp & 0x0FFF)) & 0x1000 ? H : 0); z80->hl = (uint16_t)r; } executed += 11; break;
        case 0xCB: /* CB prefix: RLC,RRC,RL,RR,SLA,SRA,SRL,BIT,SET,RES */
            {
                uint8_t cb = R8();
                int r = cb & 7;
                uint8_t *regs[] = { &bc[1], &bc[0], &de[1], &de[0], &hl[1], &hl[0], NULL, &af[1] };
                if (r == 6)
                    z80_cb_mem(ctx, z80->hl, cb, af, bc, de, hl, &executed, 15, 15);
                else
                {
                    uint8_t v = *regs[r];
                    uint8_t res = v;
                    int op = (cb >> 3) & 0x1F;
                    if (op <= 6)
                    {  /* RLC,RRC,RL,RR,SLA,SRA,SRL */
                        uint8_t old_c = (af[0] & C);
                        switch (op)
                        {
                        case 0: res = (uint8_t)((v << 1) | (v >> 7)); af[0] = (af[0] & (S | Z | P | N)) | (v >> 7); break;
                        case 1: res = (uint8_t)((v >> 1) | (v << 7)); af[0] = (af[0] & (S | Z | P | N)) | (v & 1); break;
                        case 2: res = (uint8_t)((v << 1) | old_c); af[0] = (af[0] & (S | Z | P | N)) | (v >> 7); break;
                        case 3: res = (uint8_t)((v >> 1) | (old_c << 7)); af[0] = (af[0] & (S | Z | P | N)) | (v & 1); break;
                        case 4: res = (uint8_t)(v << 1); af[0] = (af[0] & (S | Z | P | N)) | (v >> 7); break;
                        case 5: res = (uint8_t)((v >> 1) | (v & 0x80)); af[0] = (af[0] & (S | Z | P | N)) | (v & 1); break;
                        default: res = (uint8_t)(v >> 1); af[0] = (af[0] & (S | Z | P | N)) | (v & 1); break;
                        }
                        af[0] = (uint8_t)(af[0] | (uint8_t)((res ? 0 : Z) | parity(res) | ((res & 0x80) ? S : 0)));
                        *regs[r] = res;
                    }
                    else if (op >= 8 && op <= 15)
                    {  /* BIT 0-7 */
                        int bit = op - 8;
                        int z = !((v >> bit) & 1);
                        af[0] = (uint8_t)((af[0] & C) | (z ? Z : 0) | H | parity(v));
                        if (bit == 7)
                            af[0] = (uint8_t)(af[0] | ((v & 0x80) ? S : 0));
                    }
                    else if (op >= 16 && op <= 23)
                    {  /* RES 0-7 */
                        int bit = op - 16;
                        res = (uint8_t)(v & ~(1 << bit));
                        *regs[r] = res;
                    }
                    else if (op >= 24 && op <= 31)
                    {  /* SET 0-7 */
                        int bit = op - 24;
                        res = (uint8_t)(v | (1 << bit));
                        *regs[r] = res;
                    }
                    executed += 8;
                }
            }
            break;
        case 0xDD: /* DD prefix: IX */
            {
                uint8_t dd = R8();
                if (dd == 0xCB)
                {
                    int8_t d = (int8_t)R8();
                    uint8_t cb = R8();
                    z80_cb_mem(ctx, (uint16_t)(z80->ix + d), cb, af, bc, de, hl, &executed, 20, 23);
                    break;
                }
                if (dd == 0x09) { z80_add_index_rr(af, &z80->ix, z80->bc, &executed); break; }   /* ADD IX,BC */
                if (dd == 0x19) { z80_add_index_rr(af, &z80->ix, z80->de, &executed); break; }   /* ADD IX,DE */
                if (dd == 0x29) { z80_add_index_rr(af, &z80->ix, z80->ix, &executed); break; }   /* ADD IX,IX */
                if (dd == 0x39) { z80_add_index_rr(af, &z80->ix, z80->sp, &executed); break; }   /* ADD IX,SP */
                if (dd == 0x22) { uint16_t nn = R16(); W8(nn, z80->ix & 0xFFu); W8(nn + 1, z80->ix >> 8); executed += 20; break; }  /* LD (nn),IX */
                if (dd == 0x2A)
                {
                    uint16_t nn = R16();
                    z80->ix = z80_mem_read(ctx, nn) | (uint16_t)(z80_mem_read(ctx, nn + 1u) << 8);
                    executed += 20;
                    break;
                }  /* LD IX,(nn) */
                if (dd == 0xF9) { z80->sp = z80->ix; executed += 10; break; }  /* LD SP,IX */
                if (dd == 0xE3)
                {
                    uint16_t sp = z80->sp;
                    uint8_t lo = z80_mem_read(ctx, sp);
                    uint8_t hi = z80_mem_read(ctx, (uint16_t)(sp + 1u));
                    uint8_t *p = (uint8_t *)&z80->ix;
                    W8(sp, p[0]);
                    W8((uint16_t)(sp + 1u), p[1]);
                    p[0] = lo;
                    p[1] = hi;
                    executed += 23;
                    break;
                }  /* EX (SP),IX */
                if (dd == 0x34)
                {
                    int8_t d = (int8_t)R8();
                    uint16_t a = (uint16_t)(z80->ix + d);
                    uint8_t v = (uint8_t)(z80_mem_read(ctx, a) + 1u);
                    W8(a, v);
                    af[0] = (uint8_t)((af[0] & C) | (v ? 0 : Z) | ((v & 0x0Fu) ? 0 : H));
                    executed += 23;
                    break;
                }  /* INC (IX+d) */
                if (dd == 0x35)
                {
                    int8_t d = (int8_t)R8();
                    uint16_t a = (uint16_t)(z80->ix + d);
                    uint8_t v = (uint8_t)(z80_mem_read(ctx, a) - 1u);
                    W8(a, v);
                    af[0] = (uint8_t)((af[0] & C) | N | (v ? 0 : Z) | ((v & 0x0Fu) == 0x0Fu ? H : 0));
                    executed += 23;
                    break;
                }  /* DEC (IX+d) */
                if (dd == 0x21) { z80->ix = R16(); executed += 14; break; }  /* LD IX,nn */
                if (dd == 0x46) { z80_ld_reg_idisp(z80, ctx, z80->ix, &bc[1], &executed); break; }
                if (dd == 0x4E) { z80_ld_reg_idisp(z80, ctx, z80->ix, &bc[0], &executed); break; }
                if (dd == 0x56) { z80_ld_reg_idisp(z80, ctx, z80->ix, &de[1], &executed); break; }
                if (dd == 0x5E) { z80_ld_reg_idisp(z80, ctx, z80->ix, &de[0], &executed); break; }
                if (dd == 0x66) { z80_ld_reg_idisp(z80, ctx, z80->ix, &hl[1], &executed); break; }
                if (dd == 0x6E) { z80_ld_reg_idisp(z80, ctx, z80->ix, &hl[0], &executed); break; }
                if (dd == 0x36) { int8_t d = R8(); uint8_t n = R8(); W8(z80->ix + d, n); executed += 19; break; }  /* LD (IX+d),n */
                if (dd == 0x7E) { int8_t d = R8(); af[1] = z80_mem_read(ctx, z80->ix + d); executed += 19; break; }  /* LD A,(IX+d) */
                if (dd == 0x70) { z80_ld_idisp_reg(z80, ctx, z80->ix, bc[1], &executed); break; }
                if (dd == 0x71) { z80_ld_idisp_reg(z80, ctx, z80->ix, bc[0], &executed); break; }
                if (dd == 0x72) { z80_ld_idisp_reg(z80, ctx, z80->ix, de[1], &executed); break; }
                if (dd == 0x73) { z80_ld_idisp_reg(z80, ctx, z80->ix, de[0], &executed); break; }
                if (dd == 0x74) { z80_ld_idisp_reg(z80, ctx, z80->ix, hl[1], &executed); break; }
                if (dd == 0x75) { z80_ld_idisp_reg(z80, ctx, z80->ix, hl[0], &executed); break; }
                if (dd == 0x77) { int8_t d = R8(); W8(z80->ix + d, af[1]); executed += 19; break; }  /* LD (IX+d),A */
                if (dd == 0x86) { z80_alu_a_idisp(z80, ctx, z80->ix, af, Z80_IXALU_ADD, &executed); break; }
                if (dd == 0x8E) { z80_alu_a_idisp(z80, ctx, z80->ix, af, Z80_IXALU_ADC, &executed); break; }
                if (dd == 0x96) { z80_alu_a_idisp(z80, ctx, z80->ix, af, Z80_IXALU_SUB, &executed); break; }
                if (dd == 0x9E) { z80_alu_a_idisp(z80, ctx, z80->ix, af, Z80_IXALU_SBC, &executed); break; }
                if (dd == 0xA6) { z80_alu_a_idisp(z80, ctx, z80->ix, af, Z80_IXALU_AND, &executed); break; }
                if (dd == 0xAE) { z80_alu_a_idisp(z80, ctx, z80->ix, af, Z80_IXALU_XOR, &executed); break; }
                if (dd == 0xB6) { z80_alu_a_idisp(z80, ctx, z80->ix, af, Z80_IXALU_OR, &executed); break; }
                if (dd == 0xBE) { z80_alu_a_idisp(z80, ctx, z80->ix, af, Z80_IXALU_CP, &executed); break; }
                if (dd == 0x23) { z80->ix++; executed += 10; break; }  /* INC IX */
                if (dd == 0x2B) { z80->ix--; executed += 10; break; }  /* DEC IX */
                if (dd == 0xE1) { z80->ix = z80_mem_read(ctx, z80->sp) | (z80_mem_read(ctx, z80->sp + 1) << 8); z80->sp += 2; executed += 14; break; }  /* POP IX */
                if (dd == 0xE5) { z80->sp -= 2; W16(z80->sp, z80->ix); executed += 15; break; }  /* PUSH IX */
                if (dd == 0xE9) { z80->pc = z80->ix; executed += 8; break; }  /* JP (IX) */
                executed += 8;  /* DD desconocido */
            }
            break;
        case 0xFD: /* FD prefix: IY */
            {
                uint8_t fd = R8();
                if (fd == 0xCB)
                {
                    int8_t d = (int8_t)R8();
                    uint8_t cb = R8();
                    z80_cb_mem(ctx, (uint16_t)(z80->iy + d), cb, af, bc, de, hl, &executed, 20, 23);
                    break;
                }
                if (fd == 0x09) { z80_add_index_rr(af, &z80->iy, z80->bc, &executed); break; }
                if (fd == 0x19) { z80_add_index_rr(af, &z80->iy, z80->de, &executed); break; }
                if (fd == 0x29) { z80_add_index_rr(af, &z80->iy, z80->iy, &executed); break; }
                if (fd == 0x39) { z80_add_index_rr(af, &z80->iy, z80->sp, &executed); break; }
                if (fd == 0x22) { uint16_t nn = R16(); W8(nn, z80->iy & 0xFFu); W8(nn + 1, z80->iy >> 8); executed += 20; break; }
                if (fd == 0x2A)
                {
                    uint16_t nn = R16();
                    z80->iy = z80_mem_read(ctx, nn) | (uint16_t)(z80_mem_read(ctx, nn + 1u) << 8);
                    executed += 20;
                    break;
                }
                if (fd == 0xF9) { z80->sp = z80->iy; executed += 10; break; }
                if (fd == 0xE3)
                {
                    uint16_t sp = z80->sp;
                    uint8_t lo = z80_mem_read(ctx, sp);
                    uint8_t hi = z80_mem_read(ctx, (uint16_t)(sp + 1u));
                    uint8_t *p = (uint8_t *)&z80->iy;
                    W8(sp, p[0]);
                    W8((uint16_t)(sp + 1u), p[1]);
                    p[0] = lo;
                    p[1] = hi;
                    executed += 23;
                    break;
                }
                if (fd == 0x34)
                {
                    int8_t d = (int8_t)R8();
                    uint16_t a = (uint16_t)(z80->iy + d);
                    uint8_t v = (uint8_t)(z80_mem_read(ctx, a) + 1u);
                    W8(a, v);
                    af[0] = (uint8_t)((af[0] & C) | (v ? 0 : Z) | ((v & 0x0Fu) ? 0 : H));
                    executed += 23;
                    break;
                }
                if (fd == 0x35)
                {
                    int8_t d = (int8_t)R8();
                    uint16_t a = (uint16_t)(z80->iy + d);
                    uint8_t v = (uint8_t)(z80_mem_read(ctx, a) - 1u);
                    W8(a, v);
                    af[0] = (uint8_t)((af[0] & C) | N | (v ? 0 : Z) | ((v & 0x0Fu) == 0x0Fu ? H : 0));
                    executed += 23;
                    break;
                }
                if (fd == 0x21) { z80->iy = R16(); executed += 14; break; }  /* LD IY,nn */
                if (fd == 0x46) { z80_ld_reg_idisp(z80, ctx, z80->iy, &bc[1], &executed); break; }
                if (fd == 0x4E) { z80_ld_reg_idisp(z80, ctx, z80->iy, &bc[0], &executed); break; }
                if (fd == 0x56) { z80_ld_reg_idisp(z80, ctx, z80->iy, &de[1], &executed); break; }
                if (fd == 0x5E) { z80_ld_reg_idisp(z80, ctx, z80->iy, &de[0], &executed); break; }
                if (fd == 0x66) { z80_ld_reg_idisp(z80, ctx, z80->iy, &hl[1], &executed); break; }
                if (fd == 0x6E) { z80_ld_reg_idisp(z80, ctx, z80->iy, &hl[0], &executed); break; }
                if (fd == 0x36) { int8_t d = R8(); uint8_t n = R8(); W8(z80->iy + d, n); executed += 19; break; }
                if (fd == 0x7E) { int8_t d = R8(); af[1] = z80_mem_read(ctx, z80->iy + d); executed += 19; break; }
                if (fd == 0x70) { z80_ld_idisp_reg(z80, ctx, z80->iy, bc[1], &executed); break; }
                if (fd == 0x71) { z80_ld_idisp_reg(z80, ctx, z80->iy, bc[0], &executed); break; }
                if (fd == 0x72) { z80_ld_idisp_reg(z80, ctx, z80->iy, de[1], &executed); break; }
                if (fd == 0x73) { z80_ld_idisp_reg(z80, ctx, z80->iy, de[0], &executed); break; }
                if (fd == 0x74) { z80_ld_idisp_reg(z80, ctx, z80->iy, hl[1], &executed); break; }
                if (fd == 0x75) { z80_ld_idisp_reg(z80, ctx, z80->iy, hl[0], &executed); break; }
                if (fd == 0x77) { int8_t d = R8(); W8(z80->iy + d, af[1]); executed += 19; break; }
                if (fd == 0x86) { z80_alu_a_idisp(z80, ctx, z80->iy, af, Z80_IXALU_ADD, &executed); break; }
                if (fd == 0x8E) { z80_alu_a_idisp(z80, ctx, z80->iy, af, Z80_IXALU_ADC, &executed); break; }
                if (fd == 0x96) { z80_alu_a_idisp(z80, ctx, z80->iy, af, Z80_IXALU_SUB, &executed); break; }
                if (fd == 0x9E) { z80_alu_a_idisp(z80, ctx, z80->iy, af, Z80_IXALU_SBC, &executed); break; }
                if (fd == 0xA6) { z80_alu_a_idisp(z80, ctx, z80->iy, af, Z80_IXALU_AND, &executed); break; }
                if (fd == 0xAE) { z80_alu_a_idisp(z80, ctx, z80->iy, af, Z80_IXALU_XOR, &executed); break; }
                if (fd == 0xB6) { z80_alu_a_idisp(z80, ctx, z80->iy, af, Z80_IXALU_OR, &executed); break; }
                if (fd == 0xBE) { z80_alu_a_idisp(z80, ctx, z80->iy, af, Z80_IXALU_CP, &executed); break; }
                if (fd == 0x23) { z80->iy++; executed += 10; break; }
                if (fd == 0x2B) { z80->iy--; executed += 10; break; }
                if (fd == 0xE1) { z80->iy = z80_mem_read(ctx, z80->sp) | (z80_mem_read(ctx, z80->sp + 1) << 8); z80->sp += 2; executed += 14; break; }
                if (fd == 0xE5) { z80->sp -= 2; W16(z80->sp, z80->iy); executed += 15; break; }
                if (fd == 0xE9) { z80->pc = z80->iy; executed += 8; break; }
                executed += 8;
            }
            break;
        case 0xED: /* ED prefix */
            {
                uint8_t ed = R8();
                if (ed == 0x46) { executed += 8; break; }  /* IM 0 */
                if (ed == 0x44) {  /* NEG */
                    uint8_t v = af[1];
                    uint8_t r = (uint8_t)(0u - v);
                    af[1] = r;
                    int ov = (v == 0x80);
                    af[0] = (uint8_t)(N | (r & S) | (r ? 0 : Z) | parity(r) | ((v & 0xF) ? H : 0) | (v ? C : 0) |
                        (ov ? P : 0));
                    executed += 8;
                    break;
                }
                if (ed == 0x6F)
                {  /* RLD */
                    uint8_t m = z80_mem_read(ctx, z80->hl);
                    uint8_t a = af[1];
                    uint8_t new_m = (uint8_t)((m << 4) | (a & 0x0Fu));
                    af[1] = (uint8_t)((a & 0xF0u) | ((m >> 4) & 0x0Fu));
                    W8(z80->hl, new_m);
                    {
                        uint8_t na = af[1];
                        af[0] = (uint8_t)((af[0] & C) | (na ? 0 : Z) | (na & S) | parity(na));
                    }
                    executed += 18;
                    break;
                }
                if (ed == 0x67)
                {  /* RRD */
                    uint8_t m = z80_mem_read(ctx, z80->hl);
                    uint8_t a = af[1];
                    uint8_t new_m = (uint8_t)((m >> 4) | ((a & 0x0Fu) << 4));
                    af[1] = (uint8_t)((a & 0xF0u) | (m & 0x0Fu));
                    W8(z80->hl, new_m);
                    {
                        uint8_t na = af[1];
                        af[0] = (uint8_t)((af[0] & C) | (na ? 0 : Z) | (na & S) | parity(na));
                    }
                    executed += 18;
                    break;
                }
                if (ed == 0x56) { z80->iff1 = 0; executed += 8; break; }  /* IM 1 */
                if (ed == 0x5E) { z80->iff1 = 0; executed += 8; break; }  /* IM 2 */
                if (ed == 0x47) { z80->i = af[1]; executed += 9; break; }  /* LD I,A */
                if (ed == 0x4F) { z80->r = af[1]; executed += 9; break; }  /* LD R,A */
                if (ed == 0x57) {  /* LD A,I */
                    af[1] = z80->i;
                    af[0] = (uint8_t)((af[0] & C) | (z80->iff2 ? P : 0) | (af[1] & S) | (af[1] ? 0 : Z));
                    executed += 9;
                    break;
                }
                if (ed == 0x5F) {  /* LD A,R */
                    af[1] = z80->r;
                    af[0] = (uint8_t)((af[0] & C) | (z80->iff2 ? P : 0) | (af[1] & S) | (af[1] ? 0 : Z));
                    executed += 9;
                    break;
                }
                if (ed == 0x43) { uint16_t nn = R16(); W8(nn, bc[0]); W8(nn + 1, bc[1]); executed += 20; break; }  /* LD (nn),BC */
                if (ed == 0x53) { uint16_t nn = R16(); W8(nn, de[0]); W8(nn + 1, de[1]); executed += 20; break; }  /* LD (nn),DE */
                if (ed == 0x73) { uint16_t nn = R16(); W8(nn, z80->sp & 0xFF); W8(nn + 1, z80->sp >> 8); executed += 20; break; }  /* LD (nn),SP */
                if (ed == 0x4B) { uint16_t nn = R16(); z80->bc = z80_mem_read(ctx, nn) | ((uint16_t)z80_mem_read(ctx, nn + 1) << 8); executed += 20; break; }  /* LD BC,(nn) */
                if (ed == 0x5B) { uint16_t nn = R16(); z80->de = z80_mem_read(ctx, nn) | ((uint16_t)z80_mem_read(ctx, nn + 1) << 8); executed += 20; break; }  /* LD DE,(nn) */
                if (ed == 0x7B) { uint16_t nn = R16(); z80->sp = z80_mem_read(ctx, nn) | ((uint16_t)z80_mem_read(ctx, nn + 1) << 8); executed += 20; break; }  /* LD SP,(nn) */
                if (ed == 0x4A) { z80_ed_adc_hl_rr(z80, af, z80->hl, z80->bc); executed += 15; break; }  /* ADC HL,BC */
                if (ed == 0x5A) { z80_ed_adc_hl_rr(z80, af, z80->hl, z80->de); executed += 15; break; }  /* ADC HL,DE */
                if (ed == 0x6A) { z80_ed_adc_hl_rr(z80, af, z80->hl, z80->hl); executed += 15; break; }  /* ADC HL,HL */
                if (ed == 0x7A) { z80_ed_adc_hl_rr(z80, af, z80->hl, z80->sp); executed += 15; break; }  /* ADC HL,SP */
                if (ed == 0x42) { z80_ed_sbc_hl_rr(z80, af, z80->hl, z80->bc); executed += 15; break; }  /* SBC HL,BC */
                if (ed == 0x52) { z80_ed_sbc_hl_rr(z80, af, z80->hl, z80->de); executed += 15; break; }  /* SBC HL,DE */
                if (ed == 0x62) { z80_ed_sbc_hl_rr(z80, af, z80->hl, z80->hl); executed += 15; break; }  /* SBC HL,HL */
                if (ed == 0x72) { z80_ed_sbc_hl_rr(z80, af, z80->hl, z80->sp); executed += 15; break; }  /* SBC HL,SP */
                if (ed == 0x45 || ed == 0x4D) {  /* RETN / RETI: pop PC; IFF1 ← IFF2 */
                    uint8_t lo = z80_mem_read(ctx, z80->sp);
                    uint8_t hi = z80_mem_read(ctx, z80->sp + 1);
                    z80->sp += 2;
                    z80->pc = (uint16_t)lo | ((uint16_t)hi << 8);
                    z80->iff1 = z80->iff2;
                    executed += 14;
                    break;
                }
                if (ed == 0x40) {  /* IN B,(C) */
                    uint8_t v = z80_port_in(ctx, bc[0]);
                    bc[1] = v;
                    af[0] = (uint8_t)((af[0] & C) | (v & S) | (v ? 0 : Z) | parity(v));
                    executed += 12;
                    break;
                }
                if (ed == 0x48) {  /* IN C,(C) */
                    uint8_t p = bc[0];
                    uint8_t v = z80_port_in(ctx, p);
                    bc[0] = v;
                    af[0] = (uint8_t)((af[0] & C) | (v & S) | (v ? 0 : Z) | parity(v));
                    executed += 12;
                    break;
                }
                if (ed == 0x50) {  /* IN D,(C) */
                    uint8_t v = z80_port_in(ctx, bc[0]);
                    de[1] = v;
                    af[0] = (uint8_t)((af[0] & C) | (v & S) | (v ? 0 : Z) | parity(v));
                    executed += 12;
                    break;
                }
                if (ed == 0x58) {  /* IN E,(C) */
                    uint8_t v = z80_port_in(ctx, bc[0]);
                    de[0] = v;
                    af[0] = (uint8_t)((af[0] & C) | (v & S) | (v ? 0 : Z) | parity(v));
                    executed += 12;
                    break;
                }
                if (ed == 0x60) {  /* IN H,(C) */
                    uint8_t v = z80_port_in(ctx, bc[0]);
                    hl[1] = v;
                    af[0] = (uint8_t)((af[0] & C) | (v & S) | (v ? 0 : Z) | parity(v));
                    executed += 12;
                    break;
                }
                if (ed == 0x68) {  /* IN L,(C) */
                    uint8_t v = z80_port_in(ctx, bc[0]);
                    hl[0] = v;
                    af[0] = (uint8_t)((af[0] & C) | (v & S) | (v ? 0 : Z) | parity(v));
                    executed += 12;
                    break;
                }
                if (ed == 0x78) {  /* IN A,(C) */
                    uint8_t v = z80_port_in(ctx, bc[0]);
                    af[1] = v;
                    af[0] = (uint8_t)((af[0] & C) | (v & S) | (v ? 0 : Z) | parity(v));
                    executed += 12;
                    break;
                }
                if (ed == 0x41) {  /* OUT (C),B */
                    z80_port_out(ctx, bc[0], bc[1]);
                    af[0] = (uint8_t)((af[0] & C) | (bc[1] & S) | (bc[1] ? 0 : Z) | parity(bc[1]));
                    executed += 12;
                    break;
                }
                if (ed == 0x49) {  /* OUT (C),C */
                    z80_port_out(ctx, bc[0], bc[0]);
                    af[0] = (uint8_t)((af[0] & C) | (bc[0] & S) | (bc[0] ? 0 : Z) | parity(bc[0]));
                    executed += 12;
                    break;
                }
                if (ed == 0x51) {  /* OUT (C),D */
                    z80_port_out(ctx, bc[0], de[1]);
                    af[0] = (uint8_t)((af[0] & C) | (de[1] & S) | (de[1] ? 0 : Z) | parity(de[1]));
                    executed += 12;
                    break;
                }
                if (ed == 0x59) {  /* OUT (C),E */
                    z80_port_out(ctx, bc[0], de[0]);
                    af[0] = (uint8_t)((af[0] & C) | (de[0] & S) | (de[0] ? 0 : Z) | parity(de[0]));
                    executed += 12;
                    break;
                }
                if (ed == 0x61) {  /* OUT (C),H */
                    z80_port_out(ctx, bc[0], hl[1]);
                    af[0] = (uint8_t)((af[0] & C) | (hl[1] & S) | (hl[1] ? 0 : Z) | parity(hl[1]));
                    executed += 12;
                    break;
                }
                if (ed == 0x69) {  /* OUT (C),L */
                    z80_port_out(ctx, bc[0], hl[0]);
                    af[0] = (uint8_t)((af[0] & C) | (hl[0] & S) | (hl[0] ? 0 : Z) | parity(hl[0]));
                    executed += 12;
                    break;
                }
                if (ed == 0x79) {  /* OUT (C),A */
                    z80_port_out(ctx, bc[0], af[1]);
                    af[0] = (uint8_t)((af[0] & C) | (af[1] & S) | (af[1] ? 0 : Z) | parity(af[1]));
                    executed += 12;
                    break;
                }
                if (ed == 0x71) {  /* OUT (C),0 */
                    z80_port_out(ctx, bc[0], 0);
                    af[0] = (uint8_t)((af[0] & C) | Z | parity(0));
                    executed += 12;
                    break;
                }
                if (ed == 0xA0) {  /* LDI */
                    uint8_t v = z80_mem_read(ctx, z80->hl);
                    W8(z80->de, v);
                    z80->hl++;
                    z80->de++;
                    z80->bc--;
                    af[0] = (uint8_t)((af[0] & C) | (z80->bc ? P : 0));
                    executed += 16;
                    break;
                }
                if (ed == 0xA8) {  /* LDD */
                    uint8_t v = z80_mem_read(ctx, z80->hl);
                    W8(z80->de, v);
                    z80->hl--;
                    z80->de--;
                    z80->bc--;
                    af[0] = (uint8_t)((af[0] & C) | (z80->bc ? P : 0));
                    executed += 16;
                    break;
                }
                if (ed == 0xA1) {  /* CPI */
                    uint8_t v = z80_mem_read(ctx, z80->hl);
                    uint8_t a = af[1];
                    uint8_t res = (uint8_t)(a - v);
                    z80->hl++;
                    z80->bc--;
                    uint8_t f = (uint8_t)((af[0] & C) | N);
                    if (res == 0)
                        f |= Z;
                    if (res & 0x80u)
                        f |= S;
                    if (((a ^ v ^ res) & 0x10u) != 0)
                        f |= H;
                    if (z80->bc != 0)
                        f |= P;
                    af[0] = f;
                    executed += 16;
                    break;
                }
                if (ed == 0xA9) {  /* CPD */
                    uint8_t v = z80_mem_read(ctx, z80->hl);
                    uint8_t a = af[1];
                    uint8_t res = (uint8_t)(a - v);
                    z80->hl--;
                    z80->bc--;
                    uint8_t f = (uint8_t)((af[0] & C) | N);
                    if (res == 0)
                        f |= Z;
                    if (res & 0x80u)
                        f |= S;
                    if (((a ^ v ^ res) & 0x10u) != 0)
                        f |= H;
                    if (z80->bc != 0)
                        f |= P;
                    af[0] = f;
                    executed += 16;
                    break;
                }
                if (ed == 0xB0) {  /* LDIR */
                    uint8_t v = z80_mem_read(ctx, z80->hl);
                    W8(z80->de, v);
                    z80->de++; z80->hl++; z80->bc--;
                    af[1] &= ~(H|N|P);
                    if (z80->bc) { z80->pc -= 2; af[1] |= P; }
                    executed += (z80->bc != 0) ? 21 : 16;
                    break;
                }
                if (ed == 0xB8) {  /* LDDR */
                    uint8_t v = z80_mem_read(ctx, z80->hl);
                    W8(z80->de, v);
                    z80->de--; z80->hl--; z80->bc--;
                    af[1] &= ~(H|N|P);
                    if (z80->bc) { z80->pc -= 2; af[1] |= P; }
                    executed += (z80->bc != 0) ? 21 : 16;
                    break;
                }
                if (ed == 0xA2) {  /* INI: IN (C),(HL) → (HL); HL++; B-- */
                    uint8_t v = z80_port_in(ctx, bc[0]);
                    W8(z80->hl, v);
                    z80->hl++;
                    bc[1] = (uint8_t)(bc[1] - 1u);
                    uint8_t b = bc[1];
                    af[0] &= (uint8_t)~(S | Z | H | P | N | C);
                    af[0] |= N;
                    if (b == 0)
                        af[0] |= Z;
                    executed += 12;
                    break;
                }
                if (ed == 0xA3) {  /* OUTI */
                    uint8_t v = z80_mem_read(ctx, z80->hl);
                    z80_port_out(ctx, bc[0], v);
                    z80->hl++;
                    bc[1] = (uint8_t)(bc[1] - 1u);
                    uint8_t b = bc[1];
                    af[0] &= (uint8_t)~(S | Z | H | P | N | C);
                    af[0] |= N;
                    if (b == 0)
                        af[0] |= Z;
                    executed += 16;
                    break;
                }
                if (ed == 0xB2) {  /* INIR */
                    uint8_t v = z80_port_in(ctx, bc[0]);
                    W8(z80->hl, v);
                    z80->hl++;
                    bc[1] = (uint8_t)(bc[1] - 1u);
                    uint8_t b = bc[1];
                    af[0] &= (uint8_t)~(S | Z | H | P | N | C);
                    af[0] |= N;
                    if (b == 0)
                        af[0] |= Z;
                    if (b != 0)
                        z80->pc -= 2;
                    executed += (b != 0) ? 21 : 16;
                    break;
                }
                if (ed == 0xBA) {  /* INDR */
                    uint8_t v = z80_port_in(ctx, bc[0]);
                    W8(z80->hl, v);
                    z80->hl--;
                    bc[1] = (uint8_t)(bc[1] - 1u);
                    uint8_t b = bc[1];
                    af[0] &= (uint8_t)~(S | Z | H | P | N | C);
                    af[0] |= N;
                    if (b == 0)
                        af[0] |= Z;
                    if (b != 0)
                        z80->pc -= 2;
                    executed += (b != 0) ? 21 : 16;
                    break;
                }
                if (ed == 0xAB) {  /* OUTD */
                    uint8_t v = z80_mem_read(ctx, z80->hl);
                    z80_port_out(ctx, bc[0], v);
                    z80->hl--;
                    bc[1] = (uint8_t)(bc[1] - 1u);
                    uint8_t b = bc[1];
                    af[0] &= (uint8_t)~(S | Z | H | P | N | C);
                    af[0] |= N;
                    if (b == 0)
                        af[0] |= Z;
                    executed += 16;
                    break;
                }
                if (ed == 0xB3) {  /* OTIR: OUT (C),(HL); HL++; B-- */
                    uint8_t v = z80_mem_read(ctx, z80->hl);
                    z80_port_out(ctx, bc[0], v);
                    z80->hl++;
                    bc[1] = (uint8_t)(bc[1] - 1u);
                    uint8_t b = bc[1];
                    af[0] &= (uint8_t)~(S | Z | H | P | N | C);
                    af[0] |= N;
                    if (b == 0)
                        af[0] |= Z;
                    if (b != 0)
                        z80->pc -= 2;
                    executed += (b != 0) ? 21 : 16;
                    break;
                }
                if (ed == 0xBB) {  /* OTDR: OUT (C),(HL); HL--; B-- */
                    uint8_t v = z80_mem_read(ctx, z80->hl);
                    z80_port_out(ctx, bc[0], v);
                    z80->hl--;
                    bc[1] = (uint8_t)(bc[1] - 1u);
                    uint8_t b = bc[1];
                    af[0] &= (uint8_t)~(S | Z | H | P | N | C);
                    af[0] |= N;
                    if (b == 0)
                        af[0] |= Z;
                    if (b != 0)
                        z80->pc -= 2;
                    executed += (b != 0) ? 21 : 16;
                    break;
                }
                /* ED desconocido: tratar como NOP */
                executed += 8;
            }
            break;
        default:
            if (op >= 0x40u && op <= 0x7Fu && op != 0x76u)
            {
                z80_ld_r_r(z80, ctx, op, bc, de, hl, af, &executed);
                break;
            }
            /* Opcode no implementado: NOP para evitar loop infinito */
            executed += 4;
            break;
        }
        z80_ei_post_instruction(z80);
    }
    return executed;
}
