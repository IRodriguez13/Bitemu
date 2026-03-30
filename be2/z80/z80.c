/**
 * BE2 - Z80 core (Genesis)
 *
 * Implementación mínima: opcodes más usados por drivers de sonido.
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
        case 0x76: /* HALT */
            z80->halted = 1;
            executed += 4;
            return executed;
        case 0xF3: /* DI */
            z80->iff1 = z80->iff2 = 0;
            executed += 4;
            break;
        case 0xFB: /* EI */
            z80->iff1 = z80->iff2 = 1;
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
        case 0xC6: { uint8_t n = R8(); uint16_t r = HI(z80->af) + n; af[0] = (r >> 8) | ((r & 0xFF) ? 0 : Z) | (((HI(z80->af) & 0x0F) + (n & 0x0F)) & 0x10) | parity((uint8_t)r); af[1] = (uint8_t)r; } executed += 7; break;  /* ADD A,n */
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
                uint8_t v, res;
                int is_hl = (r == 6);
                if (is_hl)
                    v = z80_mem_read(ctx, z80->hl);
                else
                    v = *regs[r];
                int op = (cb >> 3) & 0x1F;
                if (op <= 6) {  /* RLC,RRC,RL,RR,SLA,SRA,SRL */
                    uint8_t old_c = (af[0] & C);
                    switch (op) {
                        case 0: res = (v << 1) | (v >> 7); af[0] = (af[0] & (S|Z|P|N)) | (v >> 7); break;  /* RLC */
                        case 1: res = (v >> 1) | (v << 7); af[0] = (af[0] & (S|Z|P|N)) | (v & 1); break;   /* RRC */
                        case 2: res = (v << 1) | old_c; af[0] = (af[0] & (S|Z|P|N)) | (v >> 7); break;     /* RL */
                        case 3: res = (v >> 1) | (old_c << 7); af[0] = (af[0] & (S|Z|P|N)) | (v & 1); break; /* RR */
                        case 4: res = v << 1; af[0] = (af[0] & (S|Z|P|N)) | (v >> 7); break;             /* SLA */
                        case 5: res = (v >> 1) | (v & 0x80); af[0] = (af[0] & (S|Z|P|N)) | (v & 1); break; /* SRA */
                        default: res = v >> 1; af[0] = (af[0] & (S|Z|P|N)) | (v & 1); break;             /* SRL */
                    }
                    af[0] |= (res ? 0 : Z) | parity(res) | ((res & 0x80) ? S : 0);
                    if (is_hl) W8(z80->hl, res); else *regs[r] = res;
                } else if (op >= 8 && op <= 15) {  /* BIT 0-7 */
                    int bit = op - 8;
                    int z = !((v >> bit) & 1);
                    af[0] = (af[0] & C) | (z ? Z : 0) | H | parity(v);
                    if (bit == 7) af[0] |= (v & 0x80) ? S : 0;
                } else if (op >= 16 && op <= 23) {  /* RES 0-7 */
                    int bit = op - 16;
                    res = v & ~(1 << bit);
                    if (is_hl) W8(z80->hl, res); else *regs[r] = res;
                } else if (op >= 24 && op <= 31) {  /* SET 0-7 */
                    int bit = op - 24;
                    res = v | (1 << bit);
                    if (is_hl) W8(z80->hl, res); else *regs[r] = res;
                }
                executed += is_hl ? 15 : 8;
            }
            break;
        case 0xDD: /* DD prefix: IX */
            {
                uint8_t dd = R8();
                if (dd == 0x21) { z80->ix = R16(); executed += 14; break; }  /* LD IX,nn */
                if (dd == 0x36) { int8_t d = R8(); uint8_t n = R8(); W8(z80->ix + d, n); executed += 19; break; }  /* LD (IX+d),n */
                if (dd == 0x7E) { int8_t d = R8(); af[1] = z80_mem_read(ctx, z80->ix + d); executed += 19; break; }  /* LD A,(IX+d) */
                if (dd == 0x77) { int8_t d = R8(); W8(z80->ix + d, af[1]); executed += 19; break; }  /* LD (IX+d),A */
                if (dd == 0x86) { int8_t d = R8(); uint8_t v = z80_mem_read(ctx, z80->ix + d); uint16_t r = af[1] + v; af[0] = (r >> 8) | ((r & 0xFF) ? 0 : Z) | (((af[1] & 0x0F) + (v & 0x0F)) & 0x10) | parity((uint8_t)r); af[1] = (uint8_t)r; executed += 19; break; }  /* ADD A,(IX+d) */
                if (dd == 0x23) { z80->ix++; executed += 10; break; }  /* INC IX */
                if (dd == 0x2B) { z80->ix--; executed += 10; break; }  /* DEC IX */
                executed += 8;  /* DD desconocido */
            }
            break;
        case 0xFD: /* FD prefix: IY */
            {
                uint8_t fd = R8();
                if (fd == 0x21) { z80->iy = R16(); executed += 14; break; }  /* LD IY,nn */
                if (fd == 0x36) { int8_t d = R8(); uint8_t n = R8(); W8(z80->iy + d, n); executed += 19; break; }
                if (fd == 0x7E) { int8_t d = R8(); af[1] = z80_mem_read(ctx, z80->iy + d); executed += 19; break; }
                if (fd == 0x77) { int8_t d = R8(); W8(z80->iy + d, af[1]); executed += 19; break; }
                if (fd == 0x86) { int8_t d = R8(); uint8_t v = z80_mem_read(ctx, z80->iy + d); uint16_t r = af[1] + v; af[0] = (r >> 8) | ((r & 0xFF) ? 0 : Z) | (((af[1] & 0x0F) + (v & 0x0F)) & 0x10) | parity((uint8_t)r); af[1] = (uint8_t)r; executed += 19; break; }
                if (fd == 0x23) { z80->iy++; executed += 10; break; }
                if (fd == 0x2B) { z80->iy--; executed += 10; break; }
                executed += 8;
            }
            break;
        case 0xED: /* ED prefix */
            {
                uint8_t ed = R8();
                if (ed == 0x56) { z80->iff1 = 0; executed += 8; break; }  /* IM 1 */
                if (ed == 0x5E) { z80->iff1 = 0; executed += 8; break; }  /* IM 2 */
                if (ed == 0xB0) {  /* LDIR */
                    uint8_t v = z80_mem_read(ctx, z80->hl);
                    W8(z80->de, v);
                    z80->de++; z80->hl++; z80->bc--;
                    af[1] &= ~(H|N|P);
                    if (z80->bc) { z80->pc -= 2; af[1] |= P; }
                    executed += 16;
                    break;
                }
                /* ED desconocido: tratar como NOP */
                executed += 8;
            }
            break;
        default:
            /* Opcode no implementado: NOP para evitar loop infinito */
            executed += 4;
            break;
        }
    }
    return executed;
}
