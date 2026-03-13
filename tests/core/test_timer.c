/**
 * Tests: Game Boy timer subsystem.
 * DIV counter, TIMA increment, overflow, TMA reload, IF flag.
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "test_harness.h"
#include "be1/timer.h"
#include "be1/memory.h"
#include "be1/gb_constants.h"

static gb_mem_t mem;

static void setup(void)
{
    memset(&mem, 0, sizeof(mem));
    gb_mem_init(&mem);
}

/* --- DIV increments with cycles --- */

TEST(div_increments)
{
    setup();
    mem.timer_div = 0;
    gb_timer_step((struct gb_mem *)&mem, 256);
    ASSERT_EQ(mem.io[GB_IO_DIV], 1);
}

/* --- DIV wraps around --- */

TEST(div_wraps)
{
    setup();
    mem.timer_div = 0xFF00;
    gb_timer_step((struct gb_mem *)&mem, 256);
    ASSERT_EQ(mem.io[GB_IO_DIV], 0);
}

/* --- Writing to DIV resets internal counter --- */

TEST(div_write_resets)
{
    setup();
    mem.timer_div = 0x1234;
    gb_mem_write(&mem, 0xFF00 + GB_IO_DIV, 0xFF);
    ASSERT_EQ(mem.timer_div, 0);
    ASSERT_EQ(mem.io[GB_IO_DIV], 0);
}

/* --- TIMA does not tick when TAC disabled --- */

TEST(tima_disabled)
{
    setup();
    mem.io[GB_IO_TAC] = 0x00;
    mem.io[GB_IO_TIMA] = 0x00;

    gb_timer_step((struct gb_mem *)&mem, 1024);

    ASSERT_EQ(mem.io[GB_IO_TIMA], 0x00);
}

/* --- TIMA increments at rate 00 (1024 cycles) --- */

TEST(tima_rate00_increments)
{
    setup();
    mem.io[GB_IO_TAC] = GB_TAC_ENABLE | 0x00;
    mem.io[GB_IO_TIMA] = 0x00;
    mem.timer_div = 0;

    gb_timer_step((struct gb_mem *)&mem, 1024);

    ASSERT_EQ(mem.io[GB_IO_TIMA], 1);
}

/* --- TIMA increments at rate 01 (16 cycles) --- */

TEST(tima_rate01_increments)
{
    setup();
    mem.io[GB_IO_TAC] = GB_TAC_ENABLE | 0x01;
    mem.io[GB_IO_TIMA] = 0x00;
    mem.timer_div = 0;

    gb_timer_step((struct gb_mem *)&mem, 16);

    ASSERT_EQ(mem.io[GB_IO_TIMA], 1);
}

/* --- TIMA overflow reloads from TMA and sets IF --- */

TEST(tima_overflow_reload)
{
    setup();
    mem.io[GB_IO_TAC] = GB_TAC_ENABLE | 0x00;
    mem.io[GB_IO_TIMA] = 0xFF;
    mem.io[GB_IO_TMA] = 0x42;
    mem.io[GB_IO_IF] = 0x00;
    mem.timer_div = 0;

    gb_timer_step((struct gb_mem *)&mem, 1024);

    ASSERT_EQ(mem.io[GB_IO_TIMA], 0x42);
    ASSERT_TRUE(mem.io[GB_IO_IF] & GB_IF_TIMER);
}

/* --- Incremental TIMA ticks (step returns after first non-overflow tick) --- */

TEST(tima_incremental_ticks)
{
    setup();
    mem.io[GB_IO_TAC] = GB_TAC_ENABLE | 0x01;  /* rate 16 */
    mem.io[GB_IO_TIMA] = 0x00;
    mem.timer_div = 0;

    gb_timer_step((struct gb_mem *)&mem, 16);
    ASSERT_EQ(mem.io[GB_IO_TIMA], 1);

    gb_timer_step((struct gb_mem *)&mem, 16);
    ASSERT_EQ(mem.io[GB_IO_TIMA], 2);

    gb_timer_step((struct gb_mem *)&mem, 16);
    ASSERT_EQ(mem.io[GB_IO_TIMA], 3);
}

void run_timer_tests(void)
{
    SUITE("Timer");
    RUN(div_increments);
    RUN(div_wraps);
    RUN(div_write_resets);
    RUN(tima_disabled);
    RUN(tima_rate00_increments);
    RUN(tima_rate01_increments);
    RUN(tima_overflow_reload);
    RUN(tima_incremental_ticks);
}
