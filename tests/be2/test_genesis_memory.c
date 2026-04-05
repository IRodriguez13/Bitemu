/**
 * BE2 - Genesis: tests del subsistema de memoria
 *
 * RAM, ROM, joypad, VDP (status/HV), PSG, Z80 RAM, Z80 bus.
 * Usa genesis_impl_t para tener todos los subsistemas conectados.
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "test_harness.h"
#include "be2/genesis_impl.h"
#include "be2/genesis_constants.h"
#include "be2/cpu/cpu.h"
#include "be2/cpu_sync/cpu_sync.h"
#include "be2/memory.h"
#include "be2/vdp/vdp.h"
#include "be2/ym2612/ym2612.h"
#include "be2/psg/psg.h"
#include "be2/z80/z80.h"
#include <string.h>
#include <stdlib.h>

static genesis_impl_t impl;

static void setup(void)
{
    memset(&impl, 0, sizeof(impl));
    genesis_mem_init(&impl.mem);
    genesis_mem_set_vdp(&impl.mem, &impl.vdp);
    genesis_mem_set_ym(&impl.mem, &impl.ym2612);
    genesis_mem_set_psg(&impl.mem, &impl.psg);
    genesis_mem_set_z80(&impl.mem, impl.z80_ram, &impl.z80_bus_req, &impl.z80_reset,
                        &impl.z80_bus_ack_cycles);
    gen_vdp_init(&impl.vdp);
    gen_ym2612_init(&impl.ym2612);
    gen_psg_init(&impl.psg);
    genesis_mem_reset(&impl.mem);
}

/* --- RAM read/write --- */

TEST(gen_mem_ram_readwrite)
{
    setup();
    genesis_mem_write8(&impl.mem, GEN_ADDR_RAM_START, 0x42);
    ASSERT_EQ(genesis_mem_read8(&impl.mem, GEN_ADDR_RAM_START), 0x42);

    genesis_mem_write8(&impl.mem, GEN_ADDR_RAM_END - 1, 0xAB);
    ASSERT_EQ(genesis_mem_read8(&impl.mem, GEN_ADDR_RAM_END - 1), 0xAB);
}

TEST(gen_mem_ram_word)
{
    setup();
    genesis_mem_write16(&impl.mem, GEN_ADDR_RAM_START, 0x1234);
    ASSERT_EQ(genesis_mem_read16(&impl.mem, GEN_ADDR_RAM_START), 0x1234);
}

TEST(gen_mem_ram_long)
{
    setup();
    genesis_mem_write32(&impl.mem, GEN_ADDR_RAM_START, 0x12345678);
    ASSERT_EQ(genesis_mem_read32(&impl.mem, GEN_ADDR_RAM_START), 0x12345678);
}

/* --- ROM read (sin ROM = 0xFF) --- */

TEST(gen_mem_rom_no_rom_returns_ff)
{
    setup();
    ASSERT_EQ(genesis_mem_read8(&impl.mem, 0x000000), 0xFF);
    ASSERT_EQ(genesis_mem_read16(&impl.mem, 0x100000), 0xFFFF);
}

/* Hueco cartucho (p. ej. 0x500000): open bus aprox. (patrón + latch del último byte). */
TEST(gen_mem_unmapped_open_bus)
{
    setup();
    uint8_t b = genesis_mem_read8(&impl.mem, 0x500000);
    ASSERT_EQ((unsigned)(b & 0xC0u), 0x40u);
    uint16_t w = genesis_mem_read16(&impl.mem, 0x500002);
    ASSERT_EQ((unsigned)((w >> 8) & 0xC0u), 0x40u);
    ASSERT_EQ((unsigned)(w & 0xC0u), 0x40u);
}

TEST(gen_mem_rom_with_rom)
{
    setup();
    uint8_t rom[0x200];
    memset(rom, 0xAA, sizeof(rom));
    memcpy(rom + GEN_HEADER_OFFSET, GEN_HEADER_MAGIC, GEN_HEADER_MAGIC_LEN);
    impl.mem.rom = rom;
    impl.mem.rom_size = sizeof(rom);

    ASSERT_EQ(genesis_mem_read8(&impl.mem, 0x000000), 0xAA);
    ASSERT_EQ(genesis_mem_read8(&impl.mem, GEN_HEADER_OFFSET), 'S');
    ASSERT_EQ(genesis_mem_read8(&impl.mem, GEN_HEADER_OFFSET + 1), 'E');
    ASSERT_EQ(genesis_mem_read16(&impl.mem, GEN_HEADER_OFFSET), 0x5345);  /* "SE" big-endian */
}

/* --- Joypad (reset = 0x3F = no buttons) --- */

TEST(gen_mem_joypad_default)
{
    setup();
    uint8_t lo = genesis_mem_read8(&impl.mem, GEN_IO_JOYPAD1);
    uint8_t hi = genesis_mem_read8(&impl.mem, GEN_IO_JOYPAD1 + 1);
    ASSERT_EQ(lo, 0x3F);
    ASSERT_EQ(hi, 0x3F);
}

TEST(gen_mem_joypad_custom)
{
    setup();
    impl.mem.joypad_raw[0] = 0x0F;  /* algunos botones presionados */
    genesis_mem_write8(&impl.mem, GEN_IO_JOYPAD1_CTRL, GEN_JOYPAD_TH);
    uint8_t lo = genesis_mem_read8(&impl.mem, GEN_IO_JOYPAD1_DATA);
    ASSERT_EQ(lo, 0xF0);
}

/* 6-button: ciclos 7/9 leen X,Y,Z,Mode (raw bits 8-11) */
TEST(gen_mem_joypad_6button_xyz_mode)
{
    setup();
    impl.mem.joypad_raw[0] = (1u << 8) | (1u << 10);  /* X y Z presionados */
    /* Avanzar a ciclo 7: TH toggle 7 veces (0->1, 1->0, ... hasta cycle=7, TH=1) */
    for (int i = 0; i < 7; i++)
        genesis_mem_write8(&impl.mem, GEN_IO_JOYPAD1_CTRL, (i & 1) ? 0 : GEN_JOYPAD_TH);
    uint8_t lo = genesis_mem_read8(&impl.mem, GEN_IO_JOYPAD1_DATA);
    /* Ciclo 7 TH high: bits 0=X, 1=Y, 2=Z, 3=Mode. X y Z → 0x0F & ~0x05 = 0x0A */
    ASSERT_EQ(lo, 0x0A);
}

/* --- Version register --- */

TEST(gen_mem_version_register)
{
    setup();
    uint8_t v = genesis_mem_read8(&impl.mem, GEN_IO_VERSION);
    ASSERT_EQ(v, 0x00);
}

/* --- VDP: status y HV counter (no crash) --- */

TEST(gen_mem_vdp_status_read)
{
    setup();
    uint8_t lo = genesis_mem_read8(&impl.mem, GEN_ADDR_VDP_CTRL);
    uint8_t hi = genesis_mem_read8(&impl.mem, GEN_ADDR_VDP_CTRL + 1);
    (void)lo;
    (void)hi;
    ASSERT_TRUE(1);
}

TEST(gen_mem_vdp_hv_read)
{
    setup();
    uint16_t hv = genesis_mem_read16(&impl.mem, GEN_ADDR_VDP_HV);
    ASSERT_RANGE(hv, 0, 0xFFFF);
}

/* --- VDP: write data/ctrl (VRAM via comando 32-bit) --- */

TEST(gen_mem_vdp_write_ctrl)
{
    setup();
    genesis_mem_write16(&impl.mem, GEN_ADDR_VDP_CTRL, 0x8140);  /* reg 1 = 0x40 (display on) */
    ASSERT_EQ(impl.vdp.regs[1], 0x40);
}

TEST(gen_mem_vdp_write_data)
{
    setup();
    gen_vdp_reset(&impl.vdp);
    /* Smoke: comando VRAM write (0x40000000) + escritura datos. El bus VDP acepta sin crash. */
    genesis_mem_write16(&impl.mem, GEN_ADDR_VDP_CTRL, 0x4000);
    genesis_mem_write16(&impl.mem, GEN_ADDR_VDP_CTRL, 0x0000);
    genesis_mem_write16(&impl.mem, GEN_ADDR_VDP_DATA, 0x1234);
    ASSERT_EQ(impl.vdp.vram[0], 0x1234u);
    ASSERT_EQ(impl.vdp.code_reg, (unsigned)GEN_VDP_CODE_VRAM_WRITE);
    ASSERT_EQ((unsigned)impl.vdp.addr_reg, 2u); /* auto-increment por defecto */
}

/* SOVR: más de 20 sprites en una línea tras gen_vdp_render */
TEST(gen_mem_vdp_sprite_overflow)
{
    setup();
    gen_vdp_reset(&impl.vdp);
    impl.vdp.regs[1] = 0x40;
    impl.vdp.regs[5] = 0;
    for (int s = 0; s <= GEN_VDP_MAX_SPRITES_PER_LINE; s++)
    {
        int w = s * (int)GEN_VDP_SAT_ENTRY_WORDS;
        impl.vdp.vram[w + 0] = 40;
        int link = (s < GEN_VDP_MAX_SPRITES_PER_LINE) ? (s + 1) : 0;
        impl.vdp.vram[w + 1] = (uint16_t)((GEN_VDP_SPR_SIZE_8x8 << GEN_VDP_SPR_SIZE_SHIFT) | link);
        impl.vdp.vram[w + 2] = 0;
        impl.vdp.vram[w + 3] = (uint16_t)(s * 4);
    }
    gen_vdp_render(&impl.vdp);
    ASSERT_TRUE((impl.vdp.status_reg & GEN_VDP_STATUS_SOVR) != 0);
}

/* --- PSG write-only (read returns 0xFF) --- */

TEST(gen_mem_psg_write)
{
    setup();
    genesis_mem_write8(&impl.mem, GEN_ADDR_PSG, 0x80);  /* latch tone 0 */
    ASSERT_EQ(genesis_mem_read8(&impl.mem, GEN_ADDR_PSG), 0xFF);
}

/* --- Z80 RAM --- */

TEST(gen_mem_z80_ram_readwrite)
{
    setup();
    impl.z80_bus_req = 1;
    genesis_mem_write8(&impl.mem, GEN_ADDR_Z80_START, 0x55);
    ASSERT_EQ(genesis_mem_read8(&impl.mem, GEN_ADDR_Z80_START), 0x55);

    genesis_mem_write16(&impl.mem, GEN_ADDR_Z80_START + 100, 0xABCD);
    ASSERT_EQ(genesis_mem_read16(&impl.mem, GEN_ADDR_Z80_START + 100), 0xABCD);
}

TEST(gen_mem_z80_ram_denied_without_busreq)
{
    setup();
    impl.z80_bus_req = 0;
    impl.z80_ram[0] = 0x42;
    ASSERT_EQ(genesis_mem_read8(&impl.mem, GEN_ADDR_Z80_START),
              gen_cpu_sync_z80_ram_contention_read(GEN_ADDR_Z80_START));
    genesis_mem_write8(&impl.mem, GEN_ADDR_Z80_START, 0x99);
    ASSERT_EQ(impl.z80_ram[0], 0x42);
}

TEST(gen_mem_z80_ram_denied_while_busack_pending)
{
    setup();
    impl.z80_bus_req = 1;
    impl.z80_bus_ack_cycles = 64;
    impl.z80_ram[0] = 0x42;
    ASSERT_EQ(genesis_mem_read8(&impl.mem, GEN_ADDR_Z80_START),
              gen_cpu_sync_z80_ram_contention_read(GEN_ADDR_Z80_START));
    impl.z80_bus_ack_cycles = 0;
    ASSERT_EQ(genesis_mem_read8(&impl.mem, GEN_ADDR_Z80_START), 0x42);
}

TEST(gen_cpu_sync_z80_cycles_from_68k_sane)
{
    int n = gen_cpu_sync_z80_cycles_from_68k(100000, 0);
    ASSERT_TRUE(n > 40000 && n < 50000);
    int p = gen_cpu_sync_z80_cycles_from_68k(100000, 1);
    ASSERT_TRUE(p > 0 && p <= n);
}

TEST(gen_cpu_sync_m68k_bus_extra_zero)
{
    ASSERT_EQ(gen_cpu_sync_m68k_bus_extra_cycles(128, NULL, NULL), 0);
}

TEST(gen_cpu_sync_m68k_bus_extra_branch_hint)
{
    gen_cpu_t cpu;
    gen_cpu_init(&cpu);
    gen_cpu_reset(&cpu, NULL);
    cpu.last_opcode = 0x6000;
    ASSERT_EQ(gen_cpu_sync_m68k_bus_extra_cycles(4, &cpu, NULL), 1);
    cpu.last_opcode = 0x2000;
    ASSERT_EQ(gen_cpu_sync_m68k_bus_extra_cycles(4, &cpu, NULL), 0);
}

TEST(gen_cpu_sync_m68k_bus_extra_vdp_dma)
{
    gen_cpu_t cpu;
    gen_vdp_t vdp;
    gen_cpu_init(&cpu);
    gen_cpu_reset(&cpu, NULL);
    gen_vdp_init(&vdp);
    gen_vdp_reset(&vdp);
    cpu.last_opcode = 0x4E71;
    vdp.status_reg |= GEN_VDP_STATUS_DMA;
    ASSERT_EQ(gen_cpu_sync_m68k_bus_extra_cycles(4, &cpu, &vdp), 1);
    vdp.dma_fill_pending = 1;
    ASSERT_EQ(gen_cpu_sync_m68k_bus_extra_cycles(4, &cpu, &vdp), 0);
    vdp.dma_fill_pending = 0;
    cpu.last_opcode = 0x60FF;
    ASSERT_EQ(gen_cpu_sync_m68k_bus_extra_cycles(4, &cpu, &vdp), 2);
    /* Stall restante sin bit DMA en status (p. ej. tras transferencia ya modelada). */
    vdp.status_reg &= (uint8_t)~GEN_VDP_STATUS_DMA;
    vdp.dma_stall_68k = 4u;
    cpu.last_opcode = 0x4E71;
    ASSERT_EQ(gen_cpu_sync_m68k_bus_extra_cycles(4, &cpu, &vdp), 1);
}

TEST(gen_vdp_m68k_bus_slice_extra_matches_dma_stall)
{
    gen_vdp_t vdp;
    gen_vdp_init(&vdp);
    gen_vdp_reset(&vdp);
    ASSERT_EQ(gen_vdp_m68k_bus_slice_extra(&vdp), 0);
    vdp.dma_stall_68k = 1u;
    ASSERT_EQ(gen_vdp_m68k_bus_slice_extra(&vdp), 1);
    vdp.dma_stall_68k = 0;
    vdp.status_reg |= GEN_VDP_STATUS_DMA;
    ASSERT_EQ(gen_vdp_m68k_bus_slice_extra(&vdp), 1);
    vdp.dma_fill_pending = 1;
    ASSERT_EQ(gen_vdp_m68k_bus_slice_extra(&vdp), 0);
}

TEST(gen_z80_jp_hl_ex_de_ld_sp)
{
    setup();
    impl.z80_reset = 0xFF;
    gen_z80_t z;
    gen_z80_init(&z);
    z.pc = 0x0100;
    z.hl = 0x0200;
    z.de = 0x0300;
    z.sp = 0x0100;
    impl.z80_ram[0x100] = 0xE9;
    impl.z80_ram[0x200] = 0xEB;
    impl.z80_ram[0x201] = 0xF9;
    impl.z80_ram[0x202] = 0x00;
    gen_z80_step(&z, &impl, 4);
    ASSERT_EQ(z.pc, 0x0200);
    gen_z80_step(&z, &impl, 4);
    ASSERT_EQ(z.hl, 0x0300);
    ASSERT_EQ(z.de, 0x0200);
    ASSERT_EQ(z.pc, 0x0201);
    gen_z80_step(&z, &impl, 6);
    ASSERT_EQ(z.sp, 0x0300);
    ASSERT_EQ(z.pc, 0x0202);
    gen_z80_step(&z, &impl, 4);
    ASSERT_EQ(z.pc, 0x0203);
}

TEST(gen_z80_ex_sp_hl_ld_a_i)
{
    setup();
    impl.z80_reset = 0xFF;
    gen_z80_t z;
    gen_z80_init(&z);
    z.pc = 0x0100;
    z.sp = 0x0200;
    z.hl = 0x3344;
    impl.z80_ram[0x200] = 0x99;
    impl.z80_ram[0x201] = 0x10;
    impl.z80_ram[0x100] = 0xE3;
    gen_z80_step(&z, &impl, 19);
    ASSERT_EQ(z.hl, 0x1099u);
    ASSERT_EQ((unsigned)impl.z80_ram[0x200], 0x44u);
    ASSERT_EQ((unsigned)impl.z80_ram[0x201], 0x33u);
    ASSERT_EQ(z.pc, 0x0101u);

    z.pc = 0x0300;
    z.i = 0xAB;
    z.iff2 = 1;
    z.af = 0x0040; /* flags basura; LD A,I repone A y P/V ← IFF2 */
    impl.z80_ram[0x300] = 0xED;
    impl.z80_ram[0x301] = 0x57;
    gen_z80_step(&z, &impl, 9);
    ASSERT_EQ((unsigned)(z.af >> 8), 0xABu);
    ASSERT_EQ(z.pc, 0x0302u);
    ASSERT_TRUE((z.af & 0xFFu & 0x04u) != 0); /* P/V = IFF2 */
}

TEST(gen_z80_rlca_rrca_rla_rra)
{
    setup();
    impl.z80_reset = 0xFF;
    gen_z80_t z;
    gen_z80_init(&z);
    z.pc = 0x0100;
    z.af = (uint16_t)(0x80u << 8); /* A alto, F bajo */
    impl.z80_ram[0x100] = 0x07; /* RLCA */
    gen_z80_step(&z, &impl, 4);
    ASSERT_EQ((unsigned)(z.af >> 8), 0x01u);
    ASSERT_EQ(z.af & 0xFFu, 0x01u); /* C */

    z.pc = 0x0200;
    z.af = (uint16_t)(0x01u << 8);
    impl.z80_ram[0x200] = 0x0F; /* RRCA */
    gen_z80_step(&z, &impl, 4);
    ASSERT_EQ((unsigned)(z.af >> 8), 0x80u);
    ASSERT_EQ(z.af & 0xFFu, 0x01u);

    z.pc = 0x0300;
    z.af = (uint16_t)(0x80u << 8);
    impl.z80_ram[0x300] = 0x17; /* RLA */
    gen_z80_step(&z, &impl, 4);
    ASSERT_EQ((unsigned)(z.af >> 8), 0u);
    ASSERT_EQ(z.af & 0xFFu, 0x01u);

    z.pc = 0x0400;
    z.af = (uint16_t)((0x01u << 8) | 1u); /* A=0x01, C */
    impl.z80_ram[0x400] = 0x1F; /* RRA */
    gen_z80_step(&z, &impl, 4);
    ASSERT_EQ((unsigned)(z.af >> 8), 0x80u);
    ASSERT_EQ(z.af & 0xFFu, 0x01u);
}

/* DAA tras suma BCD “99+1”, PUSH/POP IX, JP (IX), NEG */
TEST(gen_z80_batch_daa_ix_neg)
{
    setup();
    impl.z80_reset = 0xFF;
    gen_z80_t z;
    gen_z80_init(&z);
    z.pc = 0x0100;
    impl.z80_ram[0x100] = 0x3E;
    impl.z80_ram[0x101] = 0x99;
    impl.z80_ram[0x102] = 0xC6;
    impl.z80_ram[0x103] = 0x01;
    impl.z80_ram[0x104] = 0x27;
    gen_z80_step(&z, &impl, 7 + 7 + 4);
    ASSERT_EQ((unsigned)(z.af >> 8), 0u);
    ASSERT_TRUE((z.af & 0xFFu & 0x01u) != 0);

    z.pc = 0x0200;
    z.sp = 0x01F0;
    z.ix = 0xBEEF;
    impl.z80_ram[0x200] = 0xDD;
    impl.z80_ram[0x201] = 0xE5;
    gen_z80_step(&z, &impl, 15);
    ASSERT_EQ(z.sp, 0x01EEu);
    ASSERT_EQ((unsigned)impl.z80_ram[0x1EE], 0xEFu);
    ASSERT_EQ((unsigned)impl.z80_ram[0x1EF], 0xBEu);
    impl.z80_ram[0x202] = 0xDD;
    impl.z80_ram[0x203] = 0xE9;
    z.ix = 0x0304;
    gen_z80_step(&z, &impl, 8);
    ASSERT_EQ(z.pc, 0x0304u);

    z.pc = 0x0400;
    z.sp = 0x01EE;
    impl.z80_ram[0x400] = 0xDD;
    impl.z80_ram[0x401] = 0xE1;
    gen_z80_step(&z, &impl, 14);
    ASSERT_EQ(z.ix, 0xBEEFu);

    z.pc = 0x0500;
    z.af = (uint16_t)((0x05u << 8) | 0x00);
    impl.z80_ram[0x500] = 0xED;
    impl.z80_ram[0x501] = 0x44;
    gen_z80_step(&z, &impl, 8);
    ASSERT_EQ((unsigned)(z.af >> 8), 0xFBu);
    ASSERT_TRUE((z.af & 0xFFu & (1u << 1)) != 0); /* N */
}

/* LD B,(HL) / LD (HL),D / LD C,L vía bloque 0x40–0x7F (default path) */
TEST(gen_z80_ld_r_r_block)
{
    setup();
    impl.z80_reset = 0xFF;
    gen_z80_t z;
    gen_z80_init(&z);
    z.pc = 0x0100;
    z.hl = 0x00AA;
    impl.z80_ram[0xAA] = 0x37;
    impl.z80_ram[0x100] = 0x46; /* LD B,(HL) */
    gen_z80_step(&z, &impl, 7);
    ASSERT_EQ((unsigned)(z.bc >> 8), 0x37u);
    ASSERT_EQ(z.pc, 0x0101u);

    z.pc = 0x0200;
    z.hl = 0x00BB;
    z.de = 0x5500; /* D=0x55 */
    impl.z80_ram[0x200] = 0x72; /* LD (HL),D */
    gen_z80_step(&z, &impl, 7);
    ASSERT_EQ((unsigned)impl.z80_ram[0xBB], 0x55u);

    z.pc = 0x0300;
    z.bc = 0x1200; /* B=0x12 */
    z.hl = 0x5600; /* H=0x56 */
    impl.z80_ram[0x300] = 0x44; /* LD B,H */
    gen_z80_step(&z, &impl, 4);
    ASSERT_EQ((unsigned)(z.bc >> 8), 0x56u);

    z.pc = 0x0400;
    z.bc = 0x00EE; /* C=0xEE */
    z.hl = 0x0033; /* L=0x33 */
    impl.z80_ram[0x400] = 0x4D; /* LD C,L */
    gen_z80_step(&z, &impl, 4);
    ASSERT_EQ(z.bc & 0xFFu, 0x33u);
}

/* EX AF,AF'; ADD/LD IX,IY; EX (SP),IX; INC/DEC (IX+d); DD CB / FD CB */
TEST(gen_z80_batch_ixiy_exaf_cb)
{
    setup();
    impl.z80_reset = 0xFF;
    gen_z80_t z;
    gen_z80_init(&z);
    z.af = 0x1234;
    z.af_prime = 0xABCD;
    z.pc = 0x0100;
    impl.z80_ram[0x100] = 0x08;
    gen_z80_step(&z, &impl, 4);
    ASSERT_EQ(z.af, 0xABCDu);
    ASSERT_EQ(z.af_prime, 0x1234u);

    z.pc = 0x0200;
    z.ix = 0x1000u;
    z.bc = 0x0200u;
    impl.z80_ram[0x200] = 0xDD;
    impl.z80_ram[0x201] = 0x09;
    gen_z80_step(&z, &impl, 15);
    ASSERT_EQ(z.ix, 0x1200u);

    z.pc = 0x0300;
    impl.z80_ram[0x050] = 0x78;
    impl.z80_ram[0x051] = 0x56;
    impl.z80_ram[0x300] = 0xDD;
    impl.z80_ram[0x301] = 0x2A;
    impl.z80_ram[0x302] = 0x50;
    impl.z80_ram[0x303] = 0x00;
    gen_z80_step(&z, &impl, 20);
    ASSERT_EQ(z.ix, 0x5678u);

    z.pc = 0x0400;
    z.ix = 0xBEEFu;
    impl.z80_ram[0x400] = 0xDD;
    impl.z80_ram[0x401] = 0x22;
    impl.z80_ram[0x402] = 0x60;
    impl.z80_ram[0x403] = 0x01;
    gen_z80_step(&z, &impl, 20);
    ASSERT_EQ((unsigned)impl.z80_ram[0x160], 0xEFu);
    ASSERT_EQ((unsigned)impl.z80_ram[0x161], 0xBEu);

    z.pc = 0x0500;
    z.ix = 0x2000u;
    z.sp = 0x01F8u;
    impl.z80_ram[0x1F8] = 0x11;
    impl.z80_ram[0x1F9] = 0x22;
    impl.z80_ram[0x500] = 0xDD;
    impl.z80_ram[0x501] = 0xE3;
    gen_z80_step(&z, &impl, 23);
    ASSERT_EQ(z.ix, 0x2211u);
    ASSERT_EQ((unsigned)impl.z80_ram[0x1F8], 0x00u);
    ASSERT_EQ((unsigned)impl.z80_ram[0x1F9], 0x20u);

    z.pc = 0x0600;
    z.ix = 0x0105u;
    impl.z80_ram[0x10A] = 0x3Fu;
    impl.z80_ram[0x600] = 0xDD;
    impl.z80_ram[0x601] = 0x34;
    impl.z80_ram[0x602] = 0x05;
    gen_z80_step(&z, &impl, 23);
    ASSERT_EQ((unsigned)impl.z80_ram[0x10A], 0x40u);

    z.pc = 0x0700;
    z.ix = 0x0200u;
    impl.z80_ram[0x203] = 0x80;
    impl.z80_ram[0x700]      = 0xDD;
    impl.z80_ram[0x701]      = 0xCB;
    impl.z80_ram[0x702]      = 0x03;
    impl.z80_ram[0x703]      = 0x06; /* RLC (IX+3) */
    gen_z80_step(&z, &impl, 23);
    ASSERT_EQ((unsigned)impl.z80_ram[0x203], 0x01u);
    ASSERT_TRUE((z.af & 0xFFu & 0x01u) != 0);

    z.pc = 0x0800;
    z.iy = 0x0300u;
    impl.z80_ram[0x302] = 0x04;
    impl.z80_ram[0x800] = 0xFD;
    impl.z80_ram[0x801] = 0xCB;
    impl.z80_ram[0x802] = 0x02;
    impl.z80_ram[0x803] = 0x46; /* BIT 0,(IY+2) */
    gen_z80_step(&z, &impl, 20);
    ASSERT_TRUE((z.af & 0xFFu & 0x40u) != 0);

    z.pc = 0x0900;
    z.iy = 0x1000u;
    impl.z80_ram[0x900] = 0xFD;
    impl.z80_ram[0x901] = 0xF9;
    gen_z80_step(&z, &impl, 10);
    ASSERT_EQ(z.sp, 0x1000u);
}

/* ALU A,(IX+d)/(IY+d); EI habilita IFF tras la siguiente instrucción */
TEST(gen_z80_ixiy_alu_ei)
{
    setup();
    impl.z80_reset = 0xFF;
    gen_z80_t z;
    gen_z80_init(&z);
    z.pc = 0x0100;
    z.ix = 0x0200u;
    z.af = (uint16_t)(0x10u << 8);
    impl.z80_ram[0x202] = 0x03;
    impl.z80_ram[0x100] = 0xDD;
    impl.z80_ram[0x101] = 0x96;
    impl.z80_ram[0x102] = 0x02;
    gen_z80_step(&z, &impl, 19);
    ASSERT_EQ((unsigned)(z.af >> 8), 0x0Du);

    z.pc = 0x0200;
    z.iy = 0x0400u;
    z.af = (uint16_t)(0x03u << 8) | 0x01u; /* A=3, C */
    impl.z80_ram[0x401] = 0x05;
    impl.z80_ram[0x200] = 0xFD;
    impl.z80_ram[0x201] = 0x8E;
    impl.z80_ram[0x202] = 0x01;
    gen_z80_step(&z, &impl, 19);
    ASSERT_EQ((unsigned)(z.af >> 8), 0x09u); /* 3+5+1 */

    z.pc = 0x0300;
    z.iff1 = z.iff2 = 0;
    z.ei_countdown = 0;
    impl.z80_ram[0x300] = 0xFB;
    impl.z80_ram[0x301] = 0x00;
    gen_z80_step(&z, &impl, 4);
    ASSERT_EQ(z.iff1, 0u);
    gen_z80_step(&z, &impl, 4);
    ASSERT_EQ(z.iff1, 1u);
    ASSERT_EQ(z.iff2, 1u);

    z.pc = 0x0400;
    z.iff1 = 0;
    z.ei_countdown = 0;
    impl.z80_ram[0x400] = 0xFB;
    impl.z80_ram[0x401] = 0xF3;
    gen_z80_step(&z, &impl, 8);
    ASSERT_EQ(z.iff1, 0u);
    ASSERT_EQ(z.ei_countdown, 0u);
}

/* LD B,(IX+d); LD (IY+d),E; ED RLD / RRD */
TEST(gen_z80_ld_indexed_rld_rrd)
{
    setup();
    impl.z80_reset = 0xFF;
    gen_z80_t z;
    gen_z80_init(&z);
    z.pc = 0x0100;
    z.ix = 0x0200u;
    impl.z80_ram[0x203] = 0xAB;
    impl.z80_ram[0x100] = 0xDD;
    impl.z80_ram[0x101] = 0x46;
    impl.z80_ram[0x102] = 0x03;
    gen_z80_step(&z, &impl, 19);
    ASSERT_EQ((unsigned)(z.bc >> 8), 0xABu);

    z.pc = 0x0200;
    z.iy = 0x0300u;
    z.de = 0x004Eu;
    impl.z80_ram[0x302] = 0x00;
    impl.z80_ram[0x200] = 0xFD;
    impl.z80_ram[0x201] = 0x73;
    impl.z80_ram[0x202] = 0x02;
    gen_z80_step(&z, &impl, 19);
    ASSERT_EQ((unsigned)impl.z80_ram[0x302], 0x4Eu);

    z.pc = 0x0300;
    z.hl = 0x00AAu;
    z.af = (uint16_t)(0x12u << 8);
    impl.z80_ram[0xAA] = 0x34u;
    impl.z80_ram[0x300] = 0xED;
    impl.z80_ram[0x301] = 0x6F;
    gen_z80_step(&z, &impl, 18);
    ASSERT_EQ((unsigned)(z.af >> 8), 0x13u);
    ASSERT_EQ((unsigned)impl.z80_ram[0xAA], 0x42u);

    z.pc = 0x0400;
    z.hl = 0x00BBu;
    z.af = (uint16_t)(0x20u << 8);
    impl.z80_ram[0xBB] = 0xC5u;
    impl.z80_ram[0x400] = 0xED;
    impl.z80_ram[0x401] = 0x67;
    gen_z80_step(&z, &impl, 18);
    ASSERT_EQ((unsigned)(z.af >> 8), 0x25u);
    ASSERT_EQ((unsigned)impl.z80_ram[0xBB], 0x0Cu);
}

TEST(gen_z80_ed_ld_nn_pairs_adc_sbc_hl)
{
    setup();
    impl.z80_reset = 0xFF;
    gen_z80_t z;
    gen_z80_init(&z);
    /* LD BC,(0x0150): RAM 0x50,0x12 → BC=0x1250 little-endian C,L B,H */
    z.pc = 0x0100;
    impl.z80_ram[0x150] = 0x50;
    impl.z80_ram[0x151] = 0x12;
    impl.z80_ram[0x100] = 0xED;
    impl.z80_ram[0x101] = 0x4B;
    impl.z80_ram[0x102] = 0x50;
    impl.z80_ram[0x103] = 0x01;
    gen_z80_step(&z, &impl, 20);
    ASSERT_EQ(z.bc, 0x1250);
    ASSERT_EQ(z.pc, 0x0104);
    /* LD (0x0160),BC */
    z.pc = 0x0200;
    z.bc = 0xABCD;
    impl.z80_ram[0x200] = 0xED;
    impl.z80_ram[0x201] = 0x43;
    impl.z80_ram[0x202] = 0x60;
    impl.z80_ram[0x203] = 0x01;
    gen_z80_step(&z, &impl, 20);
    ASSERT_EQ((unsigned)impl.z80_ram[0x160], 0xCDu);
    ASSERT_EQ((unsigned)impl.z80_ram[0x161], 0xABu);
    /* ADC HL,BC: HL=0x0FFF BC=0x0001 C=1 → 0x1001 */
    z.pc = 0x0300;
    z.hl = 0x0FFF;
    z.bc = 0x0001;
    z.af = 0x0001; /* F=C */
    impl.z80_ram[0x300] = 0xED;
    impl.z80_ram[0x301] = 0x4A;
    impl.z80_ram[0x302] = 0x00;
    gen_z80_step(&z, &impl, 15);
    ASSERT_EQ(z.hl, 0x1001);
    /* SBC HL,DE: HL=0x1000 DE=0x0001 C=1 → 0x0FFE */
    z.pc = 0x0400;
    z.hl = 0x1000;
    z.de = 0x0001;
    z.af = 0x0001;
    impl.z80_ram[0x400] = 0xED;
    impl.z80_ram[0x401] = 0x52;
    impl.z80_ram[0x402] = 0x00;
    gen_z80_step(&z, &impl, 15);
    ASSERT_EQ(z.hl, 0x0FFE);
}

TEST(gen_z80_ldi_ldd)
{
    setup();
    impl.z80_reset = 0xFF;
    gen_z80_t z;
    gen_z80_init(&z);
    z.pc = 0x0100;
    z.hl = 0x0150;
    z.de = 0x0160;
    z.bc = 3;
    impl.z80_ram[0x150] = 0x42;
    impl.z80_ram[0x100] = 0xED;
    impl.z80_ram[0x101] = 0xA0;
    gen_z80_step(&z, &impl, 16);
    ASSERT_EQ((unsigned)impl.z80_ram[0x160], 0x42u);
    ASSERT_EQ(z.hl, 0x0151);
    ASSERT_EQ(z.de, 0x0161);
    ASSERT_EQ(z.bc, 2);
    z.pc = 0x0200;
    z.hl = 0x0152;
    z.de = 0x0162;
    z.bc = 2;
    impl.z80_ram[0x152] = 0x99;
    impl.z80_ram[0x200] = 0xED;
    impl.z80_ram[0x201] = 0xA8;
    gen_z80_step(&z, &impl, 16);
    ASSERT_EQ((unsigned)impl.z80_ram[0x162], 0x99u);
    ASSERT_EQ(z.hl, 0x0151);
    ASSERT_EQ(z.de, 0x0161);
    ASSERT_EQ(z.bc, 1);
}

TEST(gen_z80_cpi_cpd_outi_outd)
{
    setup();
    impl.z80_reset = 0xFF;
    gen_z80_t z;
    gen_z80_init(&z);
    uint8_t *af = (uint8_t *)&z.af;
    z.pc = 0x0100;
    z.hl = 0x0200;
    z.bc = 2;
    af[1] = 0x42;
    af[0] = 0;
    impl.z80_ram[0x200] = 0x42;
    impl.z80_ram[0x100] = 0xED;
    impl.z80_ram[0x101] = 0xA1;
    gen_z80_step(&z, &impl, 16);
    ASSERT_EQ(z.hl, 0x0201);
    ASSERT_EQ(z.bc, 1);
    ASSERT_TRUE((af[0] & 0x40u) != 0);

    z.pc = 0x0103;
    z.hl = 0x0205;
    z.bc = 3;
    af[1] = 0x10;
    impl.z80_ram[0x205] = 0x99;
    impl.z80_ram[0x103] = 0xED;
    impl.z80_ram[0x104] = 0xA9;
    gen_z80_step(&z, &impl, 16);
    ASSERT_EQ(z.hl, 0x0204);
    ASSERT_EQ(z.bc, 2);

    z.pc = 0x0300;
    z.hl = 0x0400;
    z.bc = 0x0211;
    impl.z80_ram[0x400] = 0x55;
    impl.z80_ram[0x300] = 0xED;
    impl.z80_ram[0x301] = 0xA3;
    gen_z80_step(&z, &impl, 16);
    ASSERT_EQ(z.hl, 0x0401);
    ASSERT_EQ((z.bc >> 8) & 0xFFu, 1u);

    z.pc = 0x0303;
    z.hl = 0x0502;
    z.bc = 0x0211;
    impl.z80_ram[0x502] = 0x66;
    impl.z80_ram[0x303] = 0xED;
    impl.z80_ram[0x304] = 0xAB;
    gen_z80_step(&z, &impl, 16);
    ASSERT_EQ(z.hl, 0x0501u);
    ASSERT_EQ((z.bc >> 8) & 0xFFu, 1u);
}

TEST(gen_z80_in_out_c_retn)
{
    setup();
    impl.z80_reset = 0xFF;
    gen_z80_t z;
    gen_z80_init(&z);
    z.pc = 0x0100;
    z.bc = 0x0302;
    z.af = 0x0000;
    impl.z80_ram[0x100] = 0xED;
    impl.z80_ram[0x101] = 0x40;
    gen_z80_step(&z, &impl, 12);
    ASSERT_EQ((z.bc >> 8) & 0xFFu, 0xFFu);
    ASSERT_EQ(z.pc, 0x0102u);
    z.bc = 0x117F;
    impl.z80_ram[0x102] = 0xED;
    impl.z80_ram[0x103] = 0x71;
    gen_z80_step(&z, &impl, 12);
    ASSERT_EQ(z.pc, 0x0104u);
    z.pc = 0x0200;
    z.sp = 0x01F0;
    z.iff1 = 0;
    z.iff2 = 1;
    impl.z80_ram[0x1F0] = 0x34;
    impl.z80_ram[0x1F1] = 0x12;
    impl.z80_ram[0x200] = 0xED;
    impl.z80_ram[0x201] = 0x45;
    /* RETN = 14 T; no sobrar ciclos o se ejecuta RAM tras el return (p. ej. NOP en 0x1234). */
    gen_z80_step(&z, &impl, 14);
    ASSERT_EQ(z.pc, 0x1234u);
    ASSERT_EQ((int)z.iff1, 1);
    ASSERT_EQ(z.sp, 0x01F2u);
}

/* --- Z80 bus request / reset --- */

TEST(gen_mem_z80_bus_req)
{
    setup();
    impl.z80_bus_req = 0;
    impl.z80_bus_ack_cycles = 0;
    ASSERT_EQ(genesis_mem_read8(&impl.mem, 0xA11100), 0);
    genesis_mem_write8(&impl.mem, 0xA11100, 1);
    ASSERT_EQ(impl.z80_bus_req, 1);
    ASSERT_EQ(impl.z80_bus_ack_cycles, (uint32_t)GEN_Z80_BUSACK_CYCLES_68K);
}

TEST(gen_mem_z80_reset)
{
    setup();
    impl.z80_reset = 0xFF;
    ASSERT_EQ(genesis_mem_read8(&impl.mem, 0xA11200), 0xFF);
    genesis_mem_write8(&impl.mem, 0xA11200, 0);
    ASSERT_EQ(impl.z80_reset, 0);
}

/* --- TMSS: write a 0xA14000 no debe fallar --- */

/* TMSS + lectura VRAM: con cart_requires_tmss bloquea hasta "SEGA"; VDP read word en bus. */
TEST(gen_mem_tmss_vdp_vram_read_gated)
{
    setup();
    impl.mem.cart_requires_tmss = 1;
    gen_vdp_reset(&impl.vdp);
    impl.vdp.vram[0] = 0xCAFE;
    gen_vdp_write_ctrl(&impl.vdp, 0x0000);
    gen_vdp_write_ctrl(&impl.vdp, 0x0000);
    ASSERT_EQ(genesis_mem_read16(&impl.mem, GEN_ADDR_VDP_DATA), 0xFFFF);
    genesis_mem_write8(&impl.mem, GEN_ADDR_TMSS, 0x53);
    genesis_mem_write8(&impl.mem, GEN_ADDR_TMSS + 1, 0x45);
    genesis_mem_write8(&impl.mem, GEN_ADDR_TMSS + 2, 0x47);
    genesis_mem_write8(&impl.mem, GEN_ADDR_TMSS + 3, 0x41);
    ASSERT_EQ(impl.mem.tmss_unlocked, 1u);
    gen_vdp_write_ctrl(&impl.vdp, 0x0000);
    gen_vdp_write_ctrl(&impl.vdp, 0x0000);
    ASSERT_EQ(genesis_mem_read16(&impl.mem, GEN_ADDR_VDP_DATA), 0xCAFE);
}

TEST(gen_mem_vdp_vram_read_word)
{
    setup();
    gen_vdp_reset(&impl.vdp);
    impl.vdp.vram[0] = 0x1234;
    gen_vdp_write_ctrl(&impl.vdp, 0x0000);
    gen_vdp_write_ctrl(&impl.vdp, 0x0000);
    ASSERT_EQ(genesis_mem_read16(&impl.mem, GEN_ADDR_VDP_DATA), 0x1234);
}

/* Comando 32-bit CD=8 (CRAM read): cmd = 0x01000020 → addr byte 1 → word índice 0. */
TEST(gen_mem_vdp_cram_read_word)
{
    setup();
    gen_vdp_reset(&impl.vdp);
    impl.vdp.cram[0] = (uint16_t)(0x0555u & GEN_VDP_CRAM_COLOR);
    /* cmd=0x00010020: CD=8 CRAM read, addr byte 0. */
    gen_vdp_write_ctrl(&impl.vdp, 0x0001);
    gen_vdp_write_ctrl(&impl.vdp, 0x0020);
    ASSERT_EQ(gen_vdp_is_vram_read_mode(&impl.vdp), 0);
    ASSERT_EQ(genesis_mem_read16(&impl.mem, GEN_ADDR_VDP_DATA), impl.vdp.cram[0]);
}

TEST(gen_mem_vdp_vsram_read_word)
{
    setup();
    gen_vdp_reset(&impl.vdp);
    impl.vdp.vsram[0] = (uint16_t)(0x01EFu & GEN_VDP_VSRAM_MASK);
    /* cmd=0x00010010: CD=4 VSRAM read, índice 0 (no 0x0100 → offset fuera de 40 words). */
    gen_vdp_write_ctrl(&impl.vdp, 0x0001);
    gen_vdp_write_ctrl(&impl.vdp, 0x0010);
    ASSERT_EQ(genesis_mem_read16(&impl.mem, GEN_ADDR_VDP_DATA), impl.vdp.vsram[0]);
}

TEST(gen_z80_lddr_copies_down)
{
    setup();
    impl.z80_reset = 0xFF;
    gen_z80_t z;
    gen_z80_init(&z);
    z.pc = 0x0100;
    z.hl = 0x0105;
    z.de = 0x0200;
    z.bc = 2;
    impl.z80_ram[0x100] = 0xED;
    impl.z80_ram[0x101] = 0xB8;
    impl.z80_ram[0x102] = 0x00;
    impl.z80_ram[0x105] = 0x11;
    impl.z80_ram[0x104] = 0x22;
    gen_z80_step(&z, &impl, 41);
    ASSERT_EQ(impl.z80_ram[0x0200], 0x11);
    ASSERT_EQ(impl.z80_ram[0x01FF], 0x22);
    ASSERT_EQ(z.hl, 0x0103);
    ASSERT_EQ(z.pc, 0x0103);
}

TEST(gen_z80_otir_outputs)
{
    setup();
    impl.z80_reset = 0xFF;
    gen_z80_t z;
    gen_z80_init(&z);
    z.pc = 0x0100;
    z.hl = 0x0108;
    z.bc = 0x0311;
    impl.z80_ram[0x100] = 0xED;
    impl.z80_ram[0x101] = 0xB3;
    impl.z80_ram[0x102] = 0x00;
    impl.z80_ram[0x108] = 0x80;
    impl.z80_ram[0x109] = 0x81;
    impl.z80_ram[0x10A] = 0x82;
    gen_z80_step(&z, &impl, 62);
    ASSERT_EQ(z.hl, 0x010Bu);
    ASSERT_EQ((z.bc >> 8) & 0xFFu, 0u);
    ASSERT_EQ(z.bc & 0xFFu, 0x11u);
    ASSERT_EQ(z.pc, 0x0103);
}

TEST(gen_z80_inir_reads_port)
{
    setup();
    impl.z80_reset = 0xFF;
    gen_z80_t z;
    gen_z80_init(&z);
    z.pc = 0x0100;
    z.hl = 0x0200;
    z.bc = 0x0240;
    impl.z80_ram[0x100] = 0xED;
    impl.z80_ram[0x101] = 0xB2;
    impl.z80_ram[0x102] = 0x00;
    gen_z80_step(&z, &impl, 41);
    ASSERT_EQ(impl.z80_ram[0x0200], 0);
    ASSERT_EQ(impl.z80_ram[0x0201], 0);
    ASSERT_EQ(z.hl, 0x0202);
    ASSERT_EQ((z.bc >> 8) & 0xFFu, 0u);
    ASSERT_EQ(z.pc, 0x0103);
}

TEST(gen_z80_otdr_outputs_down)
{
    setup();
    impl.z80_reset = 0xFF;
    gen_z80_t z;
    gen_z80_init(&z);
    z.pc = 0x0100;
    z.hl = 0x010A;
    z.bc = 0x0311;
    impl.z80_ram[0x100] = 0xED;
    impl.z80_ram[0x101] = 0xBB;
    impl.z80_ram[0x102] = 0x00;
    impl.z80_ram[0x10A] = 0x80;
    impl.z80_ram[0x109] = 0x81;
    impl.z80_ram[0x108] = 0x82;
    gen_z80_step(&z, &impl, 62);
    ASSERT_EQ(z.hl, 0x0107u);
    ASSERT_EQ((z.bc >> 8) & 0xFFu, 0u);
    ASSERT_EQ(z.bc & 0xFFu, 0x11u);
    ASSERT_EQ(z.pc, 0x0103);
}

TEST(gen_mem_tmss_write)
{
    setup();
    genesis_mem_write8(&impl.mem, GEN_ADDR_TMSS, 0x53);       /* 'S' */
    genesis_mem_write8(&impl.mem, GEN_ADDR_TMSS + 1, 0x45);   /* 'E' */
    genesis_mem_write8(&impl.mem, GEN_ADDR_TMSS + 2, 0x47);   /* 'G' */
    genesis_mem_write8(&impl.mem, GEN_ADDR_TMSS + 3, 0x41);   /* 'A' */
    ASSERT_EQ(genesis_mem_read8(&impl.mem, GEN_ADDR_TMSS), 0x53);
    ASSERT_EQ(genesis_mem_read8(&impl.mem, GEN_ADDR_TMSS + 3), 0x41);
    ASSERT_EQ(impl.mem.tmss_unlocked, 1u);
}

/* --- SRAM: A130F1 habilita mapeo, header "RA" → sram_present --- */

TEST(gen_mem_sram_a130f1)
{
    setup();
    uint8_t rom[0x300];
    memset(rom, 0xFF, sizeof(rom));
    memcpy(rom + GEN_HEADER_OFFSET, GEN_HEADER_MAGIC, GEN_HEADER_MAGIC_LEN);
    rom[GEN_HEADER_OFFSET + GEN_HEADER_SRAM_OFF] = 0x52;     /* 'R' */
    rom[GEN_HEADER_OFFSET + GEN_HEADER_SRAM_OFF + 1] = 0x41; /* 'A' */
    impl.mem.rom = rom;
    impl.mem.rom_size = sizeof(rom);
    impl.mem.sram_present = 1;
    impl.mem.sram_enabled = 0;

    ASSERT_EQ(genesis_mem_read8(&impl.mem, GEN_ADDR_SRAM_START), 0xFF);
    genesis_mem_write8(&impl.mem, GEN_ADDR_SRAM_ENABLE, 1);
    ASSERT_EQ(impl.mem.sram_enabled, 1);
    genesis_mem_write8(&impl.mem, GEN_ADDR_SRAM_START, 0xAB);
    ASSERT_EQ(genesis_mem_read8(&impl.mem, GEN_ADDR_SRAM_START), 0xAB);
}

/* --- SRAM: 0x1B4-0x1BB BE32 start/end → sram_bytes y espejo --- */

TEST(gen_mem_sram_header_size_mirror)
{
    setup();
    uint8_t rom[0x400];
    memset(rom, 0, sizeof(rom));
    memcpy(rom + GEN_HEADER_OFFSET, GEN_HEADER_MAGIC, GEN_HEADER_MAGIC_LEN);
    rom[GEN_HEADER_OFFSET + GEN_HEADER_SRAM_OFF] = 0x52;
    rom[GEN_HEADER_OFFSET + GEN_HEADER_SRAM_OFF + 1] = 0x41;
    rom[GEN_HEADER_SRAM_START_OFF + 0] = 0x00;
    rom[GEN_HEADER_SRAM_START_OFF + 1] = 0x20;
    rom[GEN_HEADER_SRAM_START_OFF + 2] = 0x00;
    rom[GEN_HEADER_SRAM_START_OFF + 3] = 0x00;
    rom[GEN_HEADER_SRAM_END_OFF + 0] = 0x00;
    rom[GEN_HEADER_SRAM_END_OFF + 1] = 0x20;
    rom[GEN_HEADER_SRAM_END_OFF + 2] = 0x0F;
    rom[GEN_HEADER_SRAM_END_OFF + 3] = 0xFF;
    impl.mem.rom = rom;
    impl.mem.rom_size = sizeof(rom);
    impl.mem.sram_present = 1;
    impl.mem.sram_enabled = 1;
    genesis_mem_apply_sram_header_ie32(&impl.mem, rom + GEN_HEADER_SRAM_START_OFF);
    ASSERT_EQ(impl.mem.sram_bytes, 4096u);
    genesis_mem_write8(&impl.mem, GEN_ADDR_SRAM_START, 0xAA);
    ASSERT_EQ(genesis_mem_read8(&impl.mem, GEN_ADDR_SRAM_START + 4096), 0xAA);
}

/* --- Lock-on: ROM ≥4MB, 0x200000 lee del bloqueado --- */

TEST(gen_mem_lockon_detection)
{
    setup();
    uint8_t *rom = calloc(1, GEN_LOCKON_SIZE + 0x100);
    ASSERT_TRUE(rom != NULL);
    memcpy(rom, GEN_HEADER_MAGIC, GEN_HEADER_MAGIC_LEN);
    memcpy(rom + GEN_HEADER_OFFSET, GEN_HEADER_MAGIC, GEN_HEADER_MAGIC_LEN);
    rom[0x200000] = 0xBB;
    rom[0x200001] = 0xCC;
    impl.mem.rom = rom;
    impl.mem.rom_size = GEN_LOCKON_SIZE + 0x100;
    impl.mem.lockon = 1;
    impl.mem.lockon_has_patch = 0;
    impl.mem.sram_enabled = 0;

    ASSERT_EQ(genesis_mem_read8(&impl.mem, 0x000000), 'S');
    ASSERT_EQ(genesis_mem_read8(&impl.mem, 0x200000), 0xBB);
    ASSERT_EQ(genesis_mem_read16(&impl.mem, 0x200000), 0xBBCC);

    free(rom);
}

/* --- Lock-on + A130F1: Sonic 3 & K, sram_enabled mapea SRAM en 0x200000 --- */

TEST(gen_mem_lockon_sram)
{
    setup();
    uint8_t *rom = calloc(1, GEN_LOCKON_SIZE);
    ASSERT_TRUE(rom != NULL);
    memcpy(rom + GEN_HEADER_OFFSET, GEN_HEADER_MAGIC, GEN_HEADER_MAGIC_LEN);
    impl.mem.rom = rom;
    impl.mem.rom_size = GEN_LOCKON_SIZE;
    impl.mem.lockon = 1;
    impl.mem.lockon_has_patch = 0;
    impl.mem.sram_present = 1;
    impl.mem.sram_enabled = 1;

    genesis_mem_write8(&impl.mem, GEN_ADDR_SRAM_START, 0x77);
    ASSERT_EQ(genesis_mem_read8(&impl.mem, GEN_ADDR_SRAM_START), 0x77);

    free(rom);
}

/* --- Lock-on + patch: Sonic 2 & K, 0x300000 lee patch cuando sram_enabled --- */

TEST(gen_mem_lockon_patch)
{
    setup();
    size_t sz = GEN_LOCKON_SIZE + GEN_LOCKON_PATCH;
    uint8_t *rom = calloc(1, sz);
    ASSERT_TRUE(rom != NULL);
    memcpy(rom + GEN_HEADER_OFFSET, GEN_HEADER_MAGIC, GEN_HEADER_MAGIC_LEN);
    for (size_t i = 0; i < GEN_LOCKON_PATCH; i++)
        rom[GEN_LOCKON_SIZE + i] = (uint8_t)(0xD0 + (i & 0x0F));
    impl.mem.rom = rom;
    impl.mem.rom_size = sz;
    impl.mem.lockon = 1;
    impl.mem.lockon_has_patch = 1;
    impl.mem.sram_enabled = 1;

    uint8_t v = genesis_mem_read8(&impl.mem, 0x300000);
    ASSERT_EQ(v, 0xD0);
    v = genesis_mem_read8(&impl.mem, 0x300001);
    ASSERT_EQ(v, 0xD1);

    free(rom);
}

/* --- ROM tamaño no múltiplo de 2: lectura con máscara, fuera de tamaño → 0xFF --- */

/* ROM > 4MB: mapper SSF2 (8×512KB), registro impar A130F3+2*slot */

TEST(gen_mem_ssf2_bank_switch)
{
    setup();
    const size_t sz = 0x400000u + 0x80000u; /* 4.5 MiB */
    uint8_t *rom = calloc(1, sz);
    ASSERT_TRUE(rom != NULL);
    rom[0] = 0x11;
    rom[0x80000] = 0x22;
    impl.mem.rom = rom;
    impl.mem.rom_size = sz;
    impl.mem.mapper_ssf2 = 1;
    impl.mem.lockon = 0;
    for (int i = 0; i < GEN_SSF2_SLOT_COUNT; i++)
        impl.mem.ssf2_bank[i] = (uint8_t)i;

    ASSERT_EQ(genesis_mem_read8(&impl.mem, 0x000000), 0x11);
    genesis_mem_write8(&impl.mem, GEN_ADDR_SSF2_SLOT0, 1);
    ASSERT_EQ(genesis_mem_read8(&impl.mem, 0x000000), 0x22);

    free(rom);
}

TEST(gen_mem_rom_odd_size_bounds)
{
    setup();
    uint8_t rom[17];
    memset(rom, 0xCD, sizeof(rom));
    rom[0] = 0x11;
    rom[16] = 0xEE;
    impl.mem.rom = rom;
    impl.mem.rom_size = 17;
    ASSERT_EQ(genesis_mem_read8(&impl.mem, 0), 0x11);
    ASSERT_EQ(genesis_mem_read8(&impl.mem, 16), 0xEE);
    ASSERT_EQ(genesis_mem_read8(&impl.mem, 17), GEN_IO_UNMAPPED_READ);
}

/* --- YM2612 lectura: MVP retorna 0 (no busy) --- */

TEST(gen_mem_ym_read)
{
    setup();
    ASSERT_EQ(genesis_mem_read8(&impl.mem, GEN_ADDR_YM_START), 0);
    ASSERT_EQ(genesis_mem_read8(&impl.mem, GEN_ADDR_YM_END), 0);
}

TEST(gen_mem_ym2612_timer_overflow_and_clear)
{
    setup();
    genesis_mem_write8(&impl.mem, GEN_ADDR_YM_START, 0x24);
    genesis_mem_write8(&impl.mem, GEN_ADDR_YM_START + 1, 0xFF);
    genesis_mem_write8(&impl.mem, GEN_ADDR_YM_START, 0x25);
    genesis_mem_write8(&impl.mem, GEN_ADDR_YM_START + 1, 0x03);
    genesis_mem_write8(&impl.mem, GEN_ADDR_YM_START, 0x27);
    genesis_mem_write8(&impl.mem, GEN_ADDR_YM_START + 1, 0x04);
    gen_ym2612_step(&impl.ym2612, 800000, NULL);
    uint8_t st = genesis_mem_read8(&impl.mem, GEN_ADDR_YM_START);
    ASSERT_TRUE((st & 1u) != 0);
    st = genesis_mem_read8(&impl.mem, GEN_ADDR_YM_START);
    ASSERT_EQ(st & 3u, 0u);
}

void run_genesis_memory_tests(void)
{
    SUITE("Genesis Memory");
    RUN(gen_mem_ram_readwrite);
    RUN(gen_mem_ram_word);
    RUN(gen_mem_ram_long);
    RUN(gen_mem_rom_no_rom_returns_ff);
    RUN(gen_mem_unmapped_open_bus);
    RUN(gen_mem_rom_with_rom);
    RUN(gen_mem_joypad_default);
    RUN(gen_mem_joypad_custom);
    RUN(gen_mem_joypad_6button_xyz_mode);
    RUN(gen_mem_version_register);
    RUN(gen_mem_vdp_status_read);
    RUN(gen_mem_vdp_hv_read);
    RUN(gen_mem_vdp_write_ctrl);
    RUN(gen_mem_vdp_write_data);
    RUN(gen_mem_vdp_sprite_overflow);
    RUN(gen_mem_psg_write);
    RUN(gen_mem_z80_ram_readwrite);
    RUN(gen_mem_z80_ram_denied_without_busreq);
    RUN(gen_mem_z80_ram_denied_while_busack_pending);
    RUN(gen_cpu_sync_z80_cycles_from_68k_sane);
    RUN(gen_cpu_sync_m68k_bus_extra_zero);
    RUN(gen_cpu_sync_m68k_bus_extra_branch_hint);
    RUN(gen_cpu_sync_m68k_bus_extra_vdp_dma);
    RUN(gen_vdp_m68k_bus_slice_extra_matches_dma_stall);
    RUN(gen_z80_jp_hl_ex_de_ld_sp);
    RUN(gen_z80_ex_sp_hl_ld_a_i);
    RUN(gen_z80_rlca_rrca_rla_rra);
    RUN(gen_z80_batch_daa_ix_neg);
    RUN(gen_z80_ld_r_r_block);
    RUN(gen_z80_batch_ixiy_exaf_cb);
    RUN(gen_z80_ixiy_alu_ei);
    RUN(gen_z80_ld_indexed_rld_rrd);
    RUN(gen_z80_ed_ld_nn_pairs_adc_sbc_hl);
    RUN(gen_z80_ldi_ldd);
    RUN(gen_z80_cpi_cpd_outi_outd);
    RUN(gen_z80_in_out_c_retn);
    RUN(gen_mem_z80_bus_req);
    RUN(gen_mem_z80_reset);
    RUN(gen_mem_tmss_vdp_vram_read_gated);
    RUN(gen_mem_vdp_vram_read_word);
    RUN(gen_mem_vdp_cram_read_word);
    RUN(gen_mem_vdp_vsram_read_word);
    RUN(gen_z80_lddr_copies_down);
    RUN(gen_z80_otir_outputs);
    RUN(gen_z80_inir_reads_port);
    RUN(gen_z80_otdr_outputs_down);
    RUN(gen_mem_tmss_write);
    RUN(gen_mem_sram_a130f1);
    RUN(gen_mem_sram_header_size_mirror);
    RUN(gen_mem_lockon_detection);
    RUN(gen_mem_lockon_sram);
    RUN(gen_mem_lockon_patch);
    RUN(gen_mem_ssf2_bank_switch);
    RUN(gen_mem_rom_odd_size_bounds);
    RUN(gen_mem_ym_read);
    RUN(gen_mem_ym2612_timer_overflow_and_clear);
}
