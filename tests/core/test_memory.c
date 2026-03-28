/**
 * Tests: Game Boy memory subsystem.
 * WRAM, HRAM, Echo RAM, I/O registers, NR52 protection, APU triggers.
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "test_harness.h"
#include "be1/memory.h"
#include "be1/gb_constants.h"

static gb_mem_t mem;

static void setup(void)
{
    memset(&mem, 0, sizeof(mem));
    gb_mem_init(&mem);
}

/* --- WRAM read/write --- */

TEST(wram_readwrite)
{
    setup();
    gb_mem_write(&mem, GB_WRAM_START, 0x42);
    ASSERT_EQ(gb_mem_read(&mem, GB_WRAM_START), 0x42);

    gb_mem_write(&mem, GB_WRAM_END, 0xAB);
    ASSERT_EQ(gb_mem_read(&mem, GB_WRAM_END), 0xAB);
}

/* --- HRAM read/write --- */

TEST(hram_readwrite)
{
    setup();
    gb_mem_write(&mem, GB_HRAM_START, 0x11);
    ASSERT_EQ(gb_mem_read(&mem, GB_HRAM_START), 0x11);

    gb_mem_write(&mem, GB_HRAM_END, 0xFF);
    ASSERT_EQ(gb_mem_read(&mem, GB_HRAM_END), 0xFF);
}

/* --- Echo RAM mirrors WRAM (0xE000-0xFDFF -> 0xC000-0xDDFF) --- */

TEST(echo_ram_mirrors_wram)
{
    setup();
    gb_mem_write(&mem, GB_WRAM_START + 5, 0x77);
    ASSERT_EQ(gb_mem_read(&mem, GB_ECHO_START + 5), 0x77);

    gb_mem_write(&mem, GB_ECHO_START + 10, 0x99);
    ASSERT_EQ(gb_mem_read(&mem, GB_WRAM_START + 10), 0x99);
}

/* --- VRAM read/write --- */

TEST(vram_readwrite)
{
    setup();
    gb_mem_write(&mem, GB_VRAM_START, 0xDE);
    ASSERT_EQ(gb_mem_read(&mem, GB_VRAM_START), 0xDE);
}

/* --- IE register at 0xFFFF --- */

TEST(ie_register)
{
    setup();
    gb_mem_write(&mem, GB_IE_ADDR, 0x1F);
    ASSERT_EQ(gb_mem_read(&mem, GB_IE_ADDR), 0x1F);
}

/* --- I/O: generic register write/read --- */

TEST(io_register_generic)
{
    setup();
    gb_mem_write(&mem, 0xFF00 + GB_IO_SCY, 0x55);
    ASSERT_EQ(gb_mem_read(&mem, 0xFF00 + GB_IO_SCY), 0x55);
}

/* --- NR52 write protection: bits 0-3 are read-only (channel status) --- */

TEST(nr52_write_protection)
{
    setup();
    mem.io[GB_IO_NR52] = APU_NR52_POWER | APU_NR52_CH1 | APU_NR52_CH3;

    gb_mem_write(&mem, 0xFF00 + GB_IO_NR52, APU_NR52_POWER);
    uint8_t val = gb_mem_read(&mem, 0xFF00 + GB_IO_NR52);
    ASSERT_TRUE(val & APU_NR52_POWER);
    ASSERT_TRUE(val & APU_NR52_CH1);
    ASSERT_TRUE(val & APU_NR52_CH3);
}

/* --- NR52 power off clears APU registers --- */

TEST(nr52_power_off_clears_apu)
{
    setup();
    mem.io[GB_IO_NR52] = APU_NR52_POWER;
    mem.io[GB_IO_NR11] = 0x80;
    mem.io[GB_IO_NR12] = 0xF3;
    mem.io[GB_IO_NR50] = 0x77;

    gb_mem_write(&mem, 0xFF00 + GB_IO_NR52, 0x00);
    ASSERT_EQ(mem.io[GB_IO_NR11], 0x00);
    ASSERT_EQ(mem.io[GB_IO_NR12], 0x00);
    ASSERT_EQ(mem.io[GB_IO_NR50], 0x00);
    ASSERT_FALSE(mem.io[GB_IO_NR52] & APU_NR52_POWER);
}

/* --- APU trigger flags set on NRx4 bit 7 write --- */

TEST(apu_trigger_flags)
{
    setup();
    mem.io[GB_IO_NR52] = APU_NR52_POWER;
    mem.apu_trigger_flags = 0;

    gb_mem_write(&mem, 0xFF00 + GB_IO_NR14, APU_TRIGGER_BIT);
    ASSERT_TRUE(mem.apu_trigger_flags & APU_NR52_CH1);
    ASSERT_TRUE(mem.io[GB_IO_NR52] & APU_NR52_CH1);

    gb_mem_write(&mem, 0xFF00 + GB_IO_NR24, APU_TRIGGER_BIT);
    ASSERT_TRUE(mem.apu_trigger_flags & APU_NR52_CH2);

    gb_mem_write(&mem, 0xFF00 + GB_IO_NR34, APU_TRIGGER_BIT);
    ASSERT_TRUE(mem.apu_trigger_flags & APU_NR52_CH3);

    gb_mem_write(&mem, 0xFF00 + GB_IO_NR44, APU_TRIGGER_BIT);
    ASSERT_TRUE(mem.apu_trigger_flags & APU_NR52_CH4);
}

/* --- 16-bit read/write (little-endian) --- */

TEST(mem16_readwrite)
{
    setup();
    gb_mem_write16(&mem, GB_WRAM_START, 0xBEEF);
    ASSERT_EQ(gb_mem_read16(&mem, GB_WRAM_START), 0xBEEF);
    ASSERT_EQ(gb_mem_read(&mem, GB_WRAM_START), 0xEF);
    ASSERT_EQ(gb_mem_read(&mem, GB_WRAM_START + 1), 0xBE);
}

/* LYC escrito actualiza STAT LYC=LY sin esperar al próximo paso de PPU */
TEST(lyc_write_updates_stat_eq)
{
    setup();
    mem.io[GB_IO_LY] = 72;
    mem.io[GB_IO_STAT] = 0;
    gb_mem_write(&mem, 0xFF00 + GB_IO_LYC, 72);
    ASSERT_TRUE(mem.io[GB_IO_STAT] & GB_STAT_LYC_EQ);
    gb_mem_write(&mem, 0xFF00 + GB_IO_LYC, 71);
    ASSERT_EQ(mem.io[GB_IO_STAT] & GB_STAT_LYC_EQ, 0);
}

void run_memory_tests(void)
{
    SUITE("Memory");
    RUN(wram_readwrite);
    RUN(hram_readwrite);
    RUN(echo_ram_mirrors_wram);
    RUN(vram_readwrite);
    RUN(ie_register);
    RUN(io_register_generic);
    RUN(nr52_write_protection);
    RUN(nr52_power_off_clears_apu);
    RUN(apu_trigger_flags);
    RUN(mem16_readwrite);
    RUN(lyc_write_updates_stat_eq);
}
