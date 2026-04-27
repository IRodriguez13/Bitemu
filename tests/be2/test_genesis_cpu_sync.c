/**
 * BE2 - Genesis: CPU<->Z80 synchronization helpers
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "test_harness.h"
#include "be2/cpu_sync/cpu_sync.h"

TEST(gen_cpu_sync_state_init_and_reset)
{
    gen_cpu_sync_state_t sync;
    gen_cpu_sync_init(&sync);
    ASSERT_EQ(sync.m68k_cycle_acc, 0);
    ASSERT_EQ(sync.z80_cycle_acc, 0);

    gen_cpu_sync_update(&sync, 100, 50, 1, 1);
    ASSERT_EQ(sync.m68k_cycle_acc, 100);
    ASSERT_EQ(sync.z80_cycle_acc, 50);
    ASSERT_EQ(sync.bus_req_state, 1);

    gen_cpu_sync_reset(&sync);
    ASSERT_EQ(sync.m68k_cycle_acc, 0);
    ASSERT_EQ(sync.z80_cycle_acc, 0);
    ASSERT_EQ(sync.bus_req_state, 0);
}

TEST(gen_cpu_sync_z80_cycles_from_68k_range)
{
    int ntsc = gen_cpu_sync_z80_cycles_from_68k(100, 0);
    int pal = gen_cpu_sync_z80_cycles_from_68k(100, 1);
    ASSERT_TRUE(ntsc > 0);
    ASSERT_TRUE(pal > 0);
    ASSERT_TRUE(ntsc < 200);
    ASSERT_TRUE(pal < 200);
    ASSERT_EQ(gen_cpu_sync_z80_cycles_from_68k(0, 0), 0);
}

TEST(gen_cpu_sync_run_and_bus_access_rules)
{
    ASSERT_EQ(gen_cpu_sync_z80_should_run(0, 1), 1);
    ASSERT_EQ(gen_cpu_sync_z80_should_run(1, 1), 0);
    ASSERT_EQ(gen_cpu_sync_z80_should_run(0, 0), 0);

    uint8_t req = 1;
    uint32_t ack = 2;
    ASSERT_EQ(gen_cpu_sync_m68k_may_access_z80_work_ram(&req, &ack), 0);
    ack = 0;
    ASSERT_EQ(gen_cpu_sync_m68k_may_access_z80_work_ram(&req, &ack), 1);
}

TEST(gen_cpu_sync_timing_helpers)
{
    gen_cpu_sync_state_t sync;
    gen_cpu_sync_init(&sync);
    sync.last_sync_point = 100;
    ASSERT_EQ(gen_cpu_sync_validate_bus_timing(&sync, 150, 60), 0);
    ASSERT_EQ(gen_cpu_sync_validate_bus_timing(&sync, 200, 60), 1);

    sync.m68k_cycle_acc = 1000;
    sync.z80_cycle_acc = 2000;
    uint32_t p = gen_cpu_sync_calculate_sync_point(&sync, 100, 200);
    ASSERT_TRUE(p > 0);
    ASSERT_EQ(p % 1100u, 0u);
    ASSERT_EQ(p % 2200u, 0u);
}

TEST(gen_cpu_sync_contention_and_extra_cycles)
{
    uint8_t a = gen_cpu_sync_z80_ram_contention_read(0xA00000);
    uint8_t b = gen_cpu_sync_z80_ram_contention_read(0xA00001);
    ASSERT_NEQ(a, b);

    int extra = gen_cpu_sync_m68k_bus_extra_cycles(64, NULL, NULL);
    ASSERT_RANGE(extra, 0, 3);
}

void run_genesis_cpu_sync_tests(void)
{
    SUITE("Genesis CPU sync");
    RUN(gen_cpu_sync_state_init_and_reset);
    RUN(gen_cpu_sync_z80_cycles_from_68k_range);
    RUN(gen_cpu_sync_run_and_bus_access_rules);
    RUN(gen_cpu_sync_timing_helpers);
    RUN(gen_cpu_sync_contention_and_extra_cycles);
}
