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
    RUN(gen_cpu_rtd);
    RUN(gen_cpu_trapv_no_trap);
    RUN(gen_cpu_lea_pc_rel);
    RUN(gen_cpu_chk_ok);
    RUN(gen_cpu_chk_trap_high);
    RUN(gen_cpu_chk_trap_negative);
    RUN(gen_cpu_dbra_loop);
    RUN(gen_cpu_line_f_illegal);
}
