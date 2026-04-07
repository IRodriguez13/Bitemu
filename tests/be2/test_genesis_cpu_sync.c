/**
 * BE2 - Genesis: tests CPU↔Z80 synchronization
 *
 * Tests para BUSREQ/BUSACK, timing preciso, y sincronización de ciclos
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "test_harness.h"
#include "be2/cpu_sync/cpu_sync.h"
#include "be2/genesis_constants.h"
#include <string.h>

/* Test CPU sync state initialization */
TEST(gen_cpu_sync_state_init)
{
    gen_cpu_sync_state_t sync;
    gen_cpu_sync_init(&sync);
    
    ASSERT_EQ(sync.z80_cycle_acc, 0);
    ASSERT_EQ(sync.m68k_cycle_acc, 0);
    ASSERT_EQ(sync.last_sync_point, 0);
    ASSERT_EQ(sync.bus_req_state, 0);
    ASSERT_EQ(sync.bus_ack_state, 0);
    ASSERT_EQ(sync.z80_halted, 0);
    ASSERT_EQ(sync.bus_request_cycles, 0);
}

/* Test CPU sync state reset */
TEST(gen_cpu_sync_state_reset)
{
    gen_cpu_sync_state_t sync;
    gen_cpu_sync_init(&sync);
    
    /* Modify state */
    sync.z80_cycle_acc = 1000;
    sync.m68k_cycle_acc = 2000;
    sync.bus_req_state = 1;
    sync.bus_ack_state = 1;
    sync.z80_halted = 1;
    sync.bus_request_cycles = 500;
    
    /* Reset should clear all */
    gen_cpu_sync_reset(&sync);
    
    ASSERT_EQ(sync.z80_cycle_acc, 0);
    ASSERT_EQ(sync.m68k_cycle_acc, 0);
    ASSERT_EQ(sync.last_sync_point, 0);
    ASSERT_EQ(sync.bus_req_state, 0);
    ASSERT_EQ(sync.bus_ack_state, 0);
    ASSERT_EQ(sync.z80_halted, 0);
    ASSERT_EQ(sync.bus_request_cycles, 0);
}

/* Test Z80 cycle calculation from 68k cycles */
TEST(gen_cpu_sync_z80_cycles_from_68k_calculation)
{
    /* NTSC timing */
    int z80_cycles = gen_cpu_sync_z80_cycles_from_68k(100, 0);
    ASSERT_GT(z80_cycles, 0);
    ASSERT_LT(z80_cycles, 200); /* Should be reasonable ratio */
    
    /* PAL timing */
    z80_cycles = gen_cpu_sync_z80_cycles_from_68k(100, 1);
    ASSERT_GT(z80_cycles, 0);
    ASSERT_LT(z80_cycles, 200);
    
    /* Zero cycles should return zero */
    ASSERT_EQ(gen_cpu_sync_z80_cycles_from_68k(0, 0), 0);
    ASSERT_EQ(gen_cpu_sync_z80_cycles_from_68k(-10, 0), 0);
}

/* Test Z80 should run condition */
TEST(gen_cpu_sync_z80_should_run_conditions)
{
    /* Normal operation: BUSREQ=0, RESET=1 */
    ASSERT_EQ(gen_cpu_sync_z80_should_run(0, 1), 1);
    
    /* Bus requested: BUSREQ=1, RESET=1 */
    ASSERT_EQ(gen_cpu_sync_z80_should_run(1, 1), 0);
    
    /* Reset active: BUSREQ=0, RESET=0 */
    ASSERT_EQ(gen_cpu_sync_z80_should_run(0, 0), 0);
    
    /* Both active: BUSREQ=1, RESET=0 */
    ASSERT_EQ(gen_cpu_sync_z80_should_run(1, 0), 0);
}

/* Test 68k access to Z80 work RAM */
TEST(gen_cpu_sync_m68k_z80_work_ram_access)
{
    uint8_t bus_req = 0;
    uint32_t bus_ack = 0;
    
    /* No bus request should deny access */
    ASSERT_EQ(gen_cpu_sync_m68k_may_access_z80_work_ram(&bus_req, &bus_ack), 0);
    
    /* Bus request but not acknowledged should deny access */
    bus_req = 1;
    ASSERT_EQ(gen_cpu_sync_m68k_may_access_z80_work_ram(&bus_req, &bus_ack), 0);
    
    /* Bus request and acknowledged should allow access */
    bus_ack = 0; /* BUSACK = 0 means acknowledged */
    ASSERT_EQ(gen_cpu_sync_m68k_may_access_z80_work_ram(&bus_req, &bus_ack), 1);
    
    /* Null bus_ack pointer should still work */
    ASSERT_EQ(gen_cpu_sync_m68k_may_access_z80_work_ram(&bus_req, NULL), 1);
}

/* Test CPU sync state updates */
TEST(gen_cpu_sync_state_updates)
{
    gen_cpu_sync_state_t sync;
    gen_cpu_sync_init(&sync);
    
    /* Initial update */
    gen_cpu_sync_update(&sync, 100, 200, 1, 0);
    
    ASSERT_EQ(sync.m68k_cycle_acc, 100);
    ASSERT_EQ(sync.z80_cycle_acc, 200);
    ASSERT_EQ(sync.bus_req_state, 1);
    ASSERT_EQ(sync.bus_ack_state, 0);
    ASSERT_EQ(sync.bus_request_cycles, 100);
    
    /* Second update with same bus request */
    gen_cpu_sync_update(&sync, 50, 100, 1, 1);
    
    ASSERT_EQ(sync.m68k_cycle_acc, 150);
    ASSERT_EQ(sync.z80_cycle_acc, 300);
    ASSERT_EQ(sync.bus_req_state, 1);
    ASSERT_EQ(sync.bus_ack_state, 1);
    ASSERT_EQ(sync.bus_request_cycles, 150); /* Should accumulate */
    
    /* Change bus request state */
    gen_cpu_sync_update(&sync, 25, 50, 0, 0);
    
    ASSERT_EQ(sync.m68k_cycle_acc, 175);
    ASSERT_EQ(sync.z80_cycle_acc, 350);
    ASSERT_EQ(sync.bus_req_state, 0);
    ASSERT_EQ(sync.bus_ack_state, 0);
    ASSERT_EQ(sync.bus_request_cycles, 25); /* Should reset on state change */
}

/* Test bus request pending detection */
TEST(gen_cpu_sync_bus_request_pending)
{
    gen_cpu_sync_state_t sync;
    gen_cpu_sync_init(&sync);
    
    /* Initially no pending request */
    ASSERT_EQ(gen_cpu_sync_is_bus_request_pending(&sync), 0);
    
    /* Set bus request */
    sync.bus_req_state = 1;
    ASSERT_EQ(gen_cpu_sync_is_bus_request_pending(&sync), 1);
    
    /* Clear bus request */
    sync.bus_req_state = 0;
    ASSERT_EQ(gen_cpu_sync_is_bus_request_pending(&sync), 0);
    
    /* Null pointer should be safe */
    ASSERT_EQ(gen_cpu_sync_is_bus_request_pending(NULL), 0);
}

/* Test bus request timing */
TEST(gen_cpu_sync_bus_request_timing)
{
    gen_cpu_sync_state_t sync;
    gen_cpu_sync_init(&sync);
    
    /* Initially no cycles */
    ASSERT_EQ(gen_cpu_sync_get_bus_request_cycles(&sync), 0);
    
    /* Update with some cycles */
    gen_cpu_sync_update(&sync, 100, 200, 1, 0);
    ASSERT_EQ(gen_cpu_sync_get_bus_request_cycles(&sync), 100);
    
    /* More cycles */
    gen_cpu_sync_update(&sync, 50, 100, 1, 0);
    ASSERT_EQ(gen_cpu_sync_get_bus_request_cycles(&sync), 150);
    
    /* Change bus request should reset counter */
    gen_cpu_sync_update(&sync, 25, 50, 0, 0);
    ASSERT_EQ(gen_cpu_sync_get_bus_request_cycles(&sync), 25);
    
    /* Null pointer should be safe */
    ASSERT_EQ(gen_cpu_sync_get_bus_request_cycles(NULL), 0);
}

/* Test bus timing validation */
TEST(gen_cpu_sync_bus_timing_validation)
{
    gen_cpu_sync_state_t sync;
    gen_cpu_sync_init(&sync);
    
    uint32_t current_cycle = 1000;
    uint32_t expected_min = 100;
    
    /* Should be invalid initially (no sync point set) */
    ASSERT_EQ(gen_cpu_sync_validate_bus_timing(&sync, current_cycle, expected_min), 0);
    
    /* Set sync point */
    sync.last_sync_point = current_cycle - 50;
    
    /* Should be valid (50 cycles elapsed >= 100 expected? No) */
    ASSERT_EQ(gen_cpu_sync_validate_bus_timing(&sync, current_cycle, expected_min), 0);
    
    /* Advance current cycle */
    current_cycle += 100;
    
    /* Should be valid (150 cycles elapsed >= 100 expected) */
    ASSERT_EQ(gen_cpu_sync_validate_bus_timing(&sync, current_cycle, expected_min), 1);
    
    /* Null pointer should be safe */
    ASSERT_EQ(gen_cpu_sync_validate_bus_timing(NULL, current_cycle, expected_min), 0);
}

/* Test sync point calculation */
TEST(gen_cpu_sync_sync_point_calculation)
{
    gen_cpu_sync_state_t sync;
    gen_cpu_sync_init(&sync);
    
    /* Set some base cycles */
    sync.m68k_cycle_acc = 1000;
    sync.z80_cycle_acc = 2000;
    
    /* Calculate sync point */
    uint32_t sync_point = gen_cpu_sync_calculate_sync_point(&sync, 100, 200);
    
    /* Should be reasonable (not zero, not too large) */
    ASSERT_GT(sync_point, 0);
    ASSERT_LT(sync_point, 1000000);
    
    /* Should be multiple of both targets */
    ASSERT_EQ(sync_point % (sync.m68k_cycle_acc + 100), 0);
    ASSERT_EQ(sync_point % (sync.z80_cycle_acc + 200), 0);
    
    /* Null pointer should be safe */
    ASSERT_EQ(gen_cpu_sync_calculate_sync_point(NULL, 100, 200), 0);
}

/* Test Z80 RAM contention read */
TEST(gen_cpu_sync_z80_ram_contention)
{
    /* Test different addresses for different patterns */
    uint8_t val1 = gen_cpu_sync_z80_ram_contention_read(0xA00000);
    uint8_t val2 = gen_cpu_sync_z80_ram_contention_read(0xA00001);
    uint8_t val3 = gen_cpu_sync_z80_ram_contention_read(0xA00100);
    uint8_t val4 = gen_cpu_sync_z80_ram_contention_read(0xA01000);
    
    /* Should return different values for different addresses */
    /* Note: This tests the algorithm, not specific expected values */
    ASSERT_NE(val1, val2); /* Adjacent addresses should differ */
    
    /* Values should be in valid byte range */
    ASSERT_LE(val1, 0xFF);
    ASSERT_LE(val2, 0xFF);
    ASSERT_LE(val3, 0xFF);
    ASSERT_LE(val4, 0xFF);
}

/* Test 68k extra cycles calculation */
TEST(gen_cpu_sync_m68k_extra_cycles)
{
    /* Test with NULL pointers (should be safe) */
    int extra = gen_cpu_sync_m68k_bus_extra_cycles(10, NULL, NULL);
    ASSERT_GE(extra, 0);
    ASSERT_LE(extra, 3);
    
    /* Test with different cycle slices */
    extra = gen_cpu_sync_m68k_bus_extra_cycles(1, NULL, NULL);
    ASSERT_GE(extra, 0);
    ASSERT_LE(extra, 3);
    
    extra = gen_cpu_sync_m68k_bus_extra_cycles(100, NULL, NULL);
    ASSERT_GE(extra, 0);
    ASSERT_LE(extra, 3);
}
