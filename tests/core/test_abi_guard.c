/**
 * ABI Guard — Save State Binary Compatibility
 *
 * These tests freeze the memory layout of serialized structs.
 * If a field's offset changes (reorder, insertion, type change),
 * the test fails and the CI pipeline blocks the release.
 *
 * RULES:
 *   - New fields MUST go at the END of the struct.
 *   - Existing field offsets MUST NOT change.
 *   - sizeof may GROW but never SHRINK.
 *
 * When you add a field at the end, update:
 *   1. The MINIMUM_SIZEOF constant for that struct.
 *   2. Add an ASSERT_OFFSET line for the new field.
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "test_harness.h"
#include "be1/cpu/cpu.h"
#include "be1/ppu.h"
#include "be1/apu.h"
#include <stddef.h>

#define ASSERT_OFFSET(type, field, expected_off) \
    ASSERT_EQ((long long)offsetof(type, field), (long long)(expected_off))

#define ASSERT_MIN_SIZE(type, min_sz) \
    ASSERT_TRUE(sizeof(type) >= (min_sz))

/* ── CPU layout (frozen at BST v2) ── */

TEST(abi_cpu_layout)
{
    ASSERT_MIN_SIZE(gb_cpu_t, 28);

    ASSERT_OFFSET(gb_cpu_t, pc,        0);
    ASSERT_OFFSET(gb_cpu_t, sp,        2);
    ASSERT_OFFSET(gb_cpu_t, a,         4);
    ASSERT_OFFSET(gb_cpu_t, b,         5);
    ASSERT_OFFSET(gb_cpu_t, c,         6);
    ASSERT_OFFSET(gb_cpu_t, d,         7);
    ASSERT_OFFSET(gb_cpu_t, e,         8);
    ASSERT_OFFSET(gb_cpu_t, h,         9);
    ASSERT_OFFSET(gb_cpu_t, l,        10);
    ASSERT_OFFSET(gb_cpu_t, f,        11);
    ASSERT_OFFSET(gb_cpu_t, halted,   12);
    ASSERT_OFFSET(gb_cpu_t, ime,      16);
    ASSERT_OFFSET(gb_cpu_t, ime_delay,20);
    ASSERT_OFFSET(gb_cpu_t, halt_bug, 24);
}

/* ── PPU layout (frozen at BST v2) ── */

TEST(abi_ppu_layout)
{
    ASSERT_MIN_SIZE(gb_ppu_t, 23060);

    ASSERT_OFFSET(gb_ppu_t, mode,           0);
    ASSERT_OFFSET(gb_ppu_t, dot_counter,    4);
    ASSERT_OFFSET(gb_ppu_t, framebuffer,    8);
    ASSERT_OFFSET(gb_ppu_t, mode3_dots,     23048);
    ASSERT_OFFSET(gb_ppu_t, window_line,    23052);
    ASSERT_OFFSET(gb_ppu_t, stat_irq_line,  23056);
    ASSERT_OFFSET(gb_ppu_t, latched_scy,    23057);
    ASSERT_OFFSET(gb_ppu_t, latched_scx,    23058);
}

/* ── APU layout (frozen at BST v2) ── */

TEST(abi_apu_layout)
{
    ASSERT_MIN_SIZE(gb_apu_t, 72);

    ASSERT_OFFSET(gb_apu_t, sample_clock,     0);
    ASSERT_OFFSET(gb_apu_t, frame_seq_cycles, 4);
    ASSERT_OFFSET(gb_apu_t, frame_seq_step,   8);
    ASSERT_OFFSET(gb_apu_t, sq1_duty_pos,    12);
    ASSERT_OFFSET(gb_apu_t, sq2_duty_pos,    13);
    ASSERT_OFFSET(gb_apu_t, wave_pos,        14);
    ASSERT_OFFSET(gb_apu_t, lfsr,            16);
    ASSERT_OFFSET(gb_apu_t, sq1_timer,       20);
    ASSERT_OFFSET(gb_apu_t, sq2_timer,       24);
    ASSERT_OFFSET(gb_apu_t, wave_timer,      28);
    ASSERT_OFFSET(gb_apu_t, noise_timer,     32);
    ASSERT_OFFSET(gb_apu_t, sq1_env_vol,     36);
    ASSERT_OFFSET(gb_apu_t, sq2_env_vol,     37);
    ASSERT_OFFSET(gb_apu_t, noise_env_vol,   38);
    ASSERT_OFFSET(gb_apu_t, sq1_env_timer,   39);
    ASSERT_OFFSET(gb_apu_t, sq2_env_timer,   40);
    ASSERT_OFFSET(gb_apu_t, noise_env_timer, 41);
    ASSERT_OFFSET(gb_apu_t, sq1_length,      44);
    ASSERT_OFFSET(gb_apu_t, sq2_length,      48);
    ASSERT_OFFSET(gb_apu_t, wave_length,     52);
    ASSERT_OFFSET(gb_apu_t, noise_length,    56);
    ASSERT_OFFSET(gb_apu_t, sweep_timer,     60);
    ASSERT_OFFSET(gb_apu_t, sweep_freq,      64);
    ASSERT_OFFSET(gb_apu_t, sweep_enabled,   68);
}

void run_abi_guard_tests(void)
{
    SUITE("ABI Guard (save state compat)");
    RUN(abi_cpu_layout);
    RUN(abi_ppu_layout);
    RUN(abi_apu_layout);
}
