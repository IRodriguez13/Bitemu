/**
 * Bitemu test runner - invokes all core test suites.
 * Build: see Makefile target 'test-core'.
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "test_harness.h"

int _th_pass;
int _th_fail;

extern void run_memory_tests(void);
extern void run_apu_tests(void);
extern void run_timer_tests(void);
extern void run_api_tests(void);
extern void run_ppu_tests(void);
extern void run_mbc2_tests(void);
extern void run_abi_guard_tests(void);
extern void run_genesis_abi_guard_tests(void);

int main(void)
{
    printf("Bitemu Core Tests\n");

    run_memory_tests();
    run_apu_tests();
    run_timer_tests();
    run_api_tests();
    run_ppu_tests();
    run_mbc2_tests();
    run_abi_guard_tests();
    run_genesis_abi_guard_tests();

    REPORT();
}
