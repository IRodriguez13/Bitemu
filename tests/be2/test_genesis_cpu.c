/**
 * BE2 - Genesis: tests CPU 68000
 *
 * RTD, TRAPV, modos EA (d16,PC), (d8,An,Xn).
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "test_harness.h"
#include "be2/genesis_impl.h"
#include "be2/genesis_constants.h"
#include "be2/cpu/cpu.h"
#include "be2/cpu/cycle_sym.h"
#include "be2/memory.h"
#include "be2/vdp/vdp.h"
#include "be2/ym2612/ym2612.h"
#include "be2/psg/psg.h"
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

/* RTD: 0x4E74, pop PC, SP += imm. Stack tiene ret addr en [SP]. */
TEST(gen_cpu_rtd)
{
    setup();
    uint8_t rom[0x200];
    memset(rom, 0, sizeof(rom));
    rom[0] = 0x4E;
    rom[1] = 0x74;
    rom[2] = 0xFF;
    rom[3] = 0xFC;
    impl.mem.rom = rom;
    impl.mem.rom_size = sizeof(rom);

    impl.cpu.pc = 0;
    impl.cpu.a[7] = 0x00FF0000;
    genesis_mem_write32(&impl.mem, 0x00FF0000, 0x00100000);

    int c = gen_cpu_step(&impl.cpu, &impl.mem, 100);
    ASSERT_TRUE(c > 0);
    ASSERT_EQ(impl.cpu.pc, 0x00100000);
    ASSERT_EQ(impl.cpu.a[7], 0x00FF0000);
}

/* TRAPV: 0x4E76, trap si V=1. Con V=0 no trap. */
TEST(gen_cpu_trapv_no_trap)
{
    setup();
    uint8_t rom[0x200];
    memset(rom, 0, sizeof(rom));
    rom[0] = 0x4E;
    rom[1] = 0x76;
    impl.mem.rom = rom;
    impl.mem.rom_size = sizeof(rom);

    impl.cpu.pc = 0;
    impl.cpu.sr = 0x2700;
    genesis_mem_write32(&impl.mem, 39 * 4, 0x00200000);

    int c = gen_cpu_step(&impl.cpu, &impl.mem, 100);
    ASSERT_TRUE(c > 0);
    ASSERT_EQ(impl.cpu.pc, 2);
}

/* LEA (d16,PC),A0: 0x41FA xxxx, A0 = PC + 2 + disp */
TEST(gen_cpu_lea_pc_rel)
{
    setup();
    uint8_t rom[0x200];
    memset(rom, 0, sizeof(rom));
    rom[0] = 0x41;
    rom[1] = 0xFA;
    rom[2] = 0x00;
    rom[3] = 0x10;
    impl.mem.rom = rom;
    impl.mem.rom_size = sizeof(rom);

    impl.cpu.pc = 0;
    gen_cpu_reset(&impl.cpu, &impl.mem);
    impl.cpu.pc = 0;

    int c = gen_cpu_step(&impl.cpu, &impl.mem, 100);
    ASSERT_TRUE(c > 0);
    ASSERT_EQ(impl.cpu.a[0], 0x12);
}

/* CHK.W D0,D1: 0x4181 — sin trap si 0 <= D0.word <= D1.word */
TEST(gen_cpu_chk_ok)
{
    setup();
    uint8_t rom[0x200];
    memset(rom, 0, sizeof(rom));
    rom[0] = 0x41;
    rom[1] = 0x81; /* CHK D0,D1 */
    impl.mem.rom = rom;
    impl.mem.rom_size = sizeof(rom);

    impl.cpu.pc = 0;
    impl.cpu.d[0] = 5;
    impl.cpu.d[1] = 10;

    int c = gen_cpu_step(&impl.cpu, &impl.mem, 100);
    ASSERT_TRUE(c > 0);
    ASSERT_EQ(impl.cpu.pc, 2);
    ASSERT_EQ(impl.cpu.d[0] & 0xFFFF, 5);
}

/* CHK trap vector 6 cuando Dn > upper bound (vector en ROM de prueba) */
TEST(gen_cpu_chk_trap_high)
{
    setup();
    uint8_t rom[0x400];
    memset(rom, 0, sizeof(rom));
    rom[0] = 0x41;
    rom[1] = 0x81;
    /* Vector 6 @ 0x18 → manejador en 0x100 (dentro de la ROM fake) */
    rom[0x18] = 0x00;
    rom[0x19] = 0x00;
    rom[0x1A] = 0x01;
    rom[0x1B] = 0x00;
    impl.mem.rom = rom;
    impl.mem.rom_size = sizeof(rom);

    impl.cpu.pc = 0;
    impl.cpu.a[7] = 0x00FF0000;
    impl.cpu.d[0] = 100;
    impl.cpu.d[1] = 10;

    int c = gen_cpu_step(&impl.cpu, &impl.mem, 100);
    ASSERT_TRUE(c > 0);
    ASSERT_EQ(impl.cpu.pc, 0x100);
}

/* CHK trap cuando Dn.word < 0 */
TEST(gen_cpu_chk_trap_negative)
{
    setup();
    uint8_t rom[0x400];
    memset(rom, 0, sizeof(rom));
    rom[0] = 0x41;
    rom[1] = 0x81;
    rom[0x18] = 0x00;
    rom[0x19] = 0x00;
    rom[0x1A] = 0x01;
    rom[0x1B] = 0x00;
    impl.mem.rom = rom;
    impl.mem.rom_size = sizeof(rom);

    impl.cpu.pc = 0;
    impl.cpu.a[7] = 0x00FF0000;
    impl.cpu.d[0] = 0xFFFFFFFF; /* .w = -1 */
    impl.cpu.d[1] = 10;

    int c = gen_cpu_step(&impl.cpu, &impl.mem, 100);
    ASSERT_TRUE(c > 0);
    ASSERT_EQ(impl.cpu.pc, 0x100);
}

/* DBRA D0,displ: cond "false" = 0001; 51C8 + disp — bucle 3 veces, luego cae detrás */
TEST(gen_cpu_dbra_loop)
{
    setup();
    uint8_t rom[0x200];
    memset(rom, 0, sizeof(rom));
    /* 0: DBRA D0, -4  → 51 C8 FF FC */
    rom[0] = 0x51;
    rom[1] = 0xC8;
    rom[2] = 0xFF;
    rom[3] = 0xFC;
    rom[4] = 0x4E;
    rom[5] = 0x71; /* NOP */
    impl.mem.rom = rom;
    impl.mem.rom_size = sizeof(rom);

    impl.cpu.pc = 0;
    impl.cpu.d[0] = 2;

    ASSERT_TRUE(gen_cpu_step(&impl.cpu, &impl.mem, 100) > 0);
    ASSERT_EQ(impl.cpu.d[0] & 0xFFFF, 1);
    ASSERT_EQ(impl.cpu.pc, 0);

    ASSERT_TRUE(gen_cpu_step(&impl.cpu, &impl.mem, 100) > 0);
    ASSERT_EQ(impl.cpu.d[0] & 0xFFFF, 0);
    ASSERT_EQ(impl.cpu.pc, 0);

    ASSERT_TRUE(gen_cpu_step(&impl.cpu, &impl.mem, 100) > 0);
    ASSERT_EQ(impl.cpu.d[0] & 0xFFFF, 0xFFFF);
    ASSERT_EQ(impl.cpu.pc, 4); /* siguiente instrucción (NOP en 4) */
}

/* BSR.W: enlace = PC tras instrucción (4); salto = 4+4 → 8 */
TEST(gen_cpu_bsr_word)
{
    setup();
    uint8_t rom[0x200];
    memset(rom, 0, sizeof(rom));
    rom[0] = 0x61;
    rom[1] = 0x00;
    rom[2] = 0x00;
    rom[3] = 0x04;
    impl.mem.rom = rom;
    impl.mem.rom_size = sizeof(rom);
    impl.cpu.pc = 0;
    /* WRAM 0xFF0000–0xFFFFFF: el push es (SP-4)…SP debe ser >0xFF0003 */
    impl.cpu.a[7] = 0x00FF0004;
    ASSERT_TRUE(gen_cpu_step(&impl.cpu, &impl.mem, 100) > 0);
    ASSERT_EQ(impl.cpu.pc, 8u);
    ASSERT_EQ(impl.cpu.a[7], 0x00FF0000u);
    ASSERT_EQ(genesis_mem_read32(&impl.mem, 0x00FF0000u), 4u);
}

/* BRA.W: dirección = PC tras la instrucción completa (4) + ext (+4) → 8 */
TEST(gen_cpu_bra_word)
{
    setup();
    uint8_t rom[0x200];
    memset(rom, 0, sizeof(rom));
    rom[0] = 0x60;
    rom[1] = 0x00;
    rom[2] = 0x00;
    rom[3] = 0x04;
    rom[4] = 0x4E;
    rom[5] = 0x71;
    impl.mem.rom = rom;
    impl.mem.rom_size = sizeof(rom);
    impl.cpu.pc = 0;
    ASSERT_TRUE(gen_cpu_step(&impl.cpu, &impl.mem, 100) > 0);
    ASSERT_EQ(impl.cpu.pc, 8u);
}

/* MOVE.B D1,D0 (0x1001): flags N según byte 0x80 */
TEST(gen_cpu_move_b_sets_nz)
{
    setup();
    uint8_t rom[0x200];
    memset(rom, 0, sizeof(rom));
    rom[0] = 0x10;
    rom[1] = 0x01;
    impl.mem.rom = rom;
    impl.mem.rom_size = sizeof(rom);
    impl.cpu.pc = 0;
    impl.cpu.sr = 0x2700;
    impl.cpu.d[0] = 0;
    impl.cpu.d[1] = 0x80;
    ASSERT_TRUE(gen_cpu_step(&impl.cpu, &impl.mem, 100) > 0);
    ASSERT_EQ(impl.cpu.d[0] & 0xFFu, 0x80u);
    ASSERT_TRUE((impl.cpu.sr & 0x0008u) != 0); /* N */
    ASSERT_EQ(impl.cpu.sr & 0x0004u, 0u);      /* Z */
}

/* LSL.W D1,D0: 0xE341 (1110 001 1 01 000 001) — contador en D1=2, D0=1 → 4 */
TEST(gen_cpu_lsl_reg_count_d1_d0)
{
    setup();
    uint8_t rom[0x200];
    memset(rom, 0, sizeof(rom));
    rom[0] = 0xE3;
    rom[1] = 0x41;
    impl.mem.rom = rom;
    impl.mem.rom_size = sizeof(rom);
    impl.cpu.pc = 0;
    impl.cpu.sr = 0;
    impl.cpu.d[0] = 1;
    impl.cpu.d[1] = 2;
    ASSERT_TRUE(gen_cpu_step(&impl.cpu, &impl.mem, 100) > 0);
    ASSERT_EQ(impl.cpu.d[0] & 0xFFFFu, 4u);
}

/* Scc/ST D0: 0x50C0 — cond 0 (siempre), byte D0 = 0xFF (antes no: Scc sólo en case 0 muerto). */
TEST(gen_cpu_scc_st_d0)
{
    setup();
    uint8_t rom[0x200];
    memset(rom, 0, sizeof(rom));
    rom[0] = 0x50;
    rom[1] = 0xC0;
    rom[2] = 0x4E;
    rom[3] = 0x71;
    impl.mem.rom = rom;
    impl.mem.rom_size = sizeof(rom);
    impl.cpu.pc = 0;
    impl.cpu.sr = 0x2700;
    impl.cpu.d[0] = 0x12345600;
    ASSERT_TRUE(gen_cpu_step(&impl.cpu, &impl.mem, 100) > 0);
    ASSERT_EQ(impl.cpu.d[0] & 0xFFu, 0xFFu);
    ASSERT_EQ(impl.cpu.pc, 2u);
}

/* MOVEP.W D0,(0,A0): empaqueta D0 en (A0) y (A0+2) */
TEST(gen_cpu_movep_word_reg_to_mem)
{
    setup();
    uint8_t rom[0x200];
    memset(rom, 0, sizeof(rom));
    rom[0] = 0x01;
    rom[1] = 0xA8; /* D0,(d16,A0) word, reg→mem */
    rom[2] = 0x00;
    rom[3] = 0x00; /* d16 = 0 */
    impl.mem.rom = rom;
    impl.mem.rom_size = sizeof(rom);
    impl.cpu.pc = 0;
    impl.cpu.a[0] = GEN_ADDR_RAM_START;
    impl.cpu.d[0] = 0x1234;
    genesis_mem_write8(&impl.mem, GEN_ADDR_RAM_START, 0);
    genesis_mem_write8(&impl.mem, GEN_ADDR_RAM_START + 2, 0);
    ASSERT_TRUE(gen_cpu_step(&impl.cpu, &impl.mem, 100) > 0);
    ASSERT_EQ(genesis_mem_read8(&impl.mem, GEN_ADDR_RAM_START), 0x12);
    ASSERT_EQ(genesis_mem_read8(&impl.mem, GEN_ADDR_RAM_START + 2), 0x34);
}

/* CLR.B D1: 0x4201 — antes del fix sólo coincidía 0x42C0 (tamaño inválido). */
TEST(gen_cpu_clr_b_d1)
{
    setup();
    uint8_t rom[0x200];
    memset(rom, 0, sizeof(rom));
    rom[0] = 0x42;
    rom[1] = 0x01;
    impl.mem.rom = rom;
    impl.mem.rom_size = sizeof(rom);
    impl.cpu.pc = 0;
    impl.cpu.sr = 0x2700;
    impl.cpu.d[1] = 0xDEADBEEF;
    ASSERT_TRUE(gen_cpu_step(&impl.cpu, &impl.mem, 100) > 0);
    ASSERT_EQ(impl.cpu.d[1], 0xDEADBE00u);
    ASSERT_EQ(impl.cpu.pc, 2u);
}

/* ORI.B #$10,CCR: 0x003C; CCR |= 0x10 (bit X debía quedar en 1 si empezaba en 0). */
TEST(gen_cpu_ori_ccr)
{
    setup();
    uint8_t rom[0x200];
    memset(rom, 0, sizeof(rom));
    rom[0] = 0x00;
    rom[1] = 0x3C;
    rom[2] = 0x00;
    rom[3] = 0x10;
    impl.mem.rom = rom;
    impl.mem.rom_size = sizeof(rom);
    impl.cpu.pc = 0;
    impl.cpu.sr = 0x2700;  /* CCR = 0x00 */
    ASSERT_TRUE(gen_cpu_step(&impl.cpu, &impl.mem, 100) > 0);
    ASSERT_EQ(impl.cpu.sr & 0xFFu, 0x10u);
    ASSERT_EQ(impl.cpu.pc, 4u);
}

/* ABCD D1,D0: 0x18 + 0x29 en BCD = 0x47 */
TEST(gen_cpu_abcd_d1_d0)
{
    setup();
    uint8_t rom[0x200];
    memset(rom, 0, sizeof(rom));
    rom[0] = 0xC1;
    rom[1] = 0x01;
    rom[2] = 0x4E; /* NOP */
    rom[3] = 0x71;
    impl.mem.rom = rom;
    impl.mem.rom_size = sizeof(rom);
    impl.cpu.pc = 0;
    impl.cpu.sr = 0x2700;
    impl.cpu.d[0] = 0x29;
    impl.cpu.d[1] = 0x18;
    ASSERT_TRUE(gen_cpu_step(&impl.cpu, &impl.mem, 100) > 0);
    ASSERT_EQ(impl.cpu.d[0] & 0xFFu, 0x47u);
    ASSERT_EQ(impl.cpu.pc, 2u);
}

/* SBCD D1,D0: 0x8101 — BCD 0x47 - 0x18 = 0x29 */
TEST(gen_cpu_sbcd_d1_d0)
{
    setup();
    uint8_t rom[0x200];
    memset(rom, 0, sizeof(rom));
    rom[0] = 0x81;
    rom[1] = 0x01;
    impl.mem.rom = rom;
    impl.mem.rom_size = sizeof(rom);
    impl.cpu.pc = 0;
    impl.cpu.sr = 0x2700;
    impl.cpu.d[0] = 0x47;
    impl.cpu.d[1] = 0x18;
    ASSERT_TRUE(gen_cpu_step(&impl.cpu, &impl.mem, 100) > 0);
    ASSERT_EQ(impl.cpu.d[0] & 0xFFu, 0x29u);
    ASSERT_EQ(impl.cpu.pc, 2u);
}

/* NBCD D0: 0x4800 — 0 - 0x05 = BCD 0x95, C/X */
TEST(gen_cpu_nbcd_d0)
{
    setup();
    uint8_t rom[0x200];
    memset(rom, 0, sizeof(rom));
    rom[0] = 0x48;
    rom[1] = 0x00;
    impl.mem.rom = rom;
    impl.mem.rom_size = sizeof(rom);
    impl.cpu.pc = 0;
    impl.cpu.sr = 0x2700 & ~0x0011u;  /* X=C=0 */
    impl.cpu.d[0] = 5;
    ASSERT_TRUE(gen_cpu_step(&impl.cpu, &impl.mem, 100) > 0);
    ASSERT_EQ(impl.cpu.d[0] & 0xFFu, 0x95u);
    ASSERT_TRUE((impl.cpu.sr & 0x0001u) != 0); /* C */
    ASSERT_TRUE((impl.cpu.sr & 0x0010u) != 0); /* X */
    ASSERT_EQ(impl.cpu.pc, 2u);
}

/* DIVU.W D0,D1 con D0=0 → vector 5 (zero divide) */
TEST(gen_cpu_divu_zero_vector5)
{
    setup();
    uint8_t rom[0x400];
    memset(rom, 0, sizeof(rom));
    rom[0] = 0x82;
    rom[1] = 0xC0;
    rom[0x14] = 0x00;
    rom[0x15] = 0x00;
    rom[0x16] = 0x03;
    rom[0x17] = 0x00;
    impl.mem.rom = rom;
    impl.mem.rom_size = sizeof(rom);
    impl.cpu.pc = 0;
    impl.cpu.a[7] = 0x00FF0004;
    impl.cpu.d[0] = 0;
    impl.cpu.d[1] = 0x12345678;
    ASSERT_TRUE(gen_cpu_step(&impl.cpu, &impl.mem, 100) > 0);
    ASSERT_EQ(impl.cpu.pc, 0x300u);
}

/* Tabla gperf: cada línea 0..15 tiene ciclos de referencia > 0 (ancla frente a gentest). */
TEST(gen_cpu_line_cycles_ref_table_nonempty)
{
    for (unsigned ln = 0; ln < 16u; ln++)
        ASSERT_TRUE(gen_cpu_line_cycles_ref(ln) > 0);
}

TEST(gen_cpu_cycles_ref_line_nibble_moveq)
{
    uint16_t op = 0x7201; /* MOVEQ #1,D0 → línea 7 */
    ASSERT_EQ(gen_cpu_cycles_ref_line_nibble(op), gen_cpu_line_cycles_ref(7u));
}

/* PC impar al opcode → vector 3 (Address Error) */
TEST(gen_cpu_odd_pc_address_error)
{
    setup();
    uint8_t rom[0x400];
    memset(rom, 0, sizeof(rom));
    rom[0x0C] = 0x00;
    rom[0x0D] = 0x00;
    rom[0x0E] = 0x05;
    rom[0x0F] = 0x00;
    impl.mem.rom = rom;
    impl.mem.rom_size = sizeof(rom);
    impl.cpu.pc = 1;
    impl.cpu.a[7] = 0x00FF0000;
    int c = gen_cpu_step(&impl.cpu, &impl.mem, 100);
    ASSERT_EQ(c, GEN_CYCLES_ADDRESS_ERROR);
    ASSERT_EQ(impl.cpu.pc, 0x500u);
}

/* MOVE.W (A0),D0 con A0=1 → word en dirección impar → vector 3 */
TEST(gen_cpu_move_w_odd_ea_address_error)
{
    setup();
    uint8_t rom[0x400];
    memset(rom, 0, sizeof(rom));
    /* MOVE.W (A0),D0 — 0x3010 */
    rom[0] = 0x30;
    rom[1] = 0x10;
    rom[0x0C] = 0x00;
    rom[0x0D] = 0x00;
    rom[0x0E] = 0x04;
    rom[0x0F] = 0x00;
    impl.mem.rom = rom;
    impl.mem.rom_size = sizeof(rom);
    impl.cpu.pc = 0;
    impl.cpu.a[0] = 1;
    impl.cpu.d[0] = 0x11111111u;
    impl.cpu.a[7] = 0x00FF0000;
    int c = gen_cpu_step(&impl.cpu, &impl.mem, 100);
    ASSERT_EQ(c, GEN_CYCLES_ADDRESS_ERROR);
    ASSERT_EQ(impl.cpu.pc, 0x400u);
    ASSERT_EQ(impl.cpu.d[0], 0x11111111u);
    ASSERT_EQ(impl.cpu.a[0], 1u);
}

/* LINE F (0xF000): ilegal en 68000 base → vector 4 */
TEST(gen_cpu_line_f_illegal)
{
    setup();
    uint8_t rom[0x400];
    memset(rom, 0, sizeof(rom));
    rom[0] = 0xF0;
    rom[1] = 0x00;
    /* Vector 4 @ 0x10 */
    rom[0x10] = 0x00;
    rom[0x11] = 0x00;
    rom[0x12] = 0x02;
    rom[0x13] = 0x00;
    impl.mem.rom = rom;
    impl.mem.rom_size = sizeof(rom);

    impl.cpu.pc = 0;
    impl.cpu.a[7] = 0x00FF0000;

    int c = gen_cpu_step(&impl.cpu, &impl.mem, 100);
    ASSERT_TRUE(c > 0);
    ASSERT_EQ(impl.cpu.pc, 0x200);
}

void run_genesis_cpu_tests(void)
{
    SUITE("Genesis CPU 68000");
    RUN(gen_cpu_line_cycles_ref_table_nonempty);
    RUN(gen_cpu_cycles_ref_line_nibble_moveq);
    RUN(gen_cpu_rtd);
    RUN(gen_cpu_trapv_no_trap);
    RUN(gen_cpu_lea_pc_rel);
    RUN(gen_cpu_chk_ok);
    RUN(gen_cpu_chk_trap_high);
    RUN(gen_cpu_chk_trap_negative);
    RUN(gen_cpu_dbra_loop);
    RUN(gen_cpu_bsr_word);
    RUN(gen_cpu_bra_word);
    RUN(gen_cpu_move_b_sets_nz);
    RUN(gen_cpu_lsl_reg_count_d1_d0);
    RUN(gen_cpu_scc_st_d0);
    RUN(gen_cpu_movep_word_reg_to_mem);
    RUN(gen_cpu_clr_b_d1);
    RUN(gen_cpu_ori_ccr);
    RUN(gen_cpu_abcd_d1_d0);
    RUN(gen_cpu_sbcd_d1_d0);
    RUN(gen_cpu_nbcd_d0);
    RUN(gen_cpu_divu_zero_vector5);
    RUN(gen_cpu_odd_pc_address_error);
    RUN(gen_cpu_move_w_odd_ea_address_error);
    RUN(gen_cpu_line_f_illegal);
}
