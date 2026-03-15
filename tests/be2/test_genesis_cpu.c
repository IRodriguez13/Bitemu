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
    genesis_mem_set_z80(&impl.mem, impl.z80_ram, &impl.z80_bus_req, &impl.z80_reset);
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

void run_genesis_cpu_tests(void)
{
    SUITE("Genesis CPU 68000");
    RUN(gen_cpu_rtd);
    RUN(gen_cpu_trapv_no_trap);
    RUN(gen_cpu_lea_pc_rel);
}
