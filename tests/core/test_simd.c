/**
 * Tests: core SIMD saturate + ring audio helper.
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "test_harness.h"
#include "core/simd/simd.h"
#include <stdint.h>

TEST(simd_clip_i32_saturates)
{
    bitemu_simd_init();
    int32_t in[] = { 4000000, -4000000, 32767, -32768, 100, -100 };
    int16_t out[6];
    bitemu_clip_i32_to_i16(in, out, 6);
    ASSERT_EQ(out[0], 32767);
    ASSERT_EQ(out[1], -32768);
    ASSERT_EQ(out[2], 32767);
    ASSERT_EQ(out[3], -32768);
    ASSERT_EQ(out[4], 100);
    ASSERT_EQ(out[5], -100);
}

TEST(simd_ring_pull_unity_volume_wraps_rd)
{
    bitemu_simd_init();
    int16_t ring[] = { 10, -20, 30 };
    int16_t dst[4];
    size_t rd = 2;
    rd = bitemu_audio_ring_pull_scaled_clip_i16(ring, 3, rd, 1.0f, 4, dst);
    ASSERT_EQ(dst[0], 30);
    ASSERT_EQ(dst[1], 10);
    ASSERT_EQ(dst[2], -20);
    ASSERT_EQ(dst[3], 30);
    ASSERT_EQ((int)rd, 0);
}

TEST(simd_ring_pull_scaled_clip)
{
    bitemu_simd_init();
    int16_t ring[] = { 1000, -2000 };
    int16_t dst[2];
    size_t rd = bitemu_audio_ring_pull_scaled_clip_i16(ring, 2, 0, 0.5f, 2, dst);
    ASSERT_EQ((int)rd, 0);
    ASSERT_EQ((int)dst[0], (int)(int32_t)(1000 * 0.5f));
    ASSERT_EQ((int)dst[1], (int)(int32_t)(-2000 * 0.5f));
}

void run_simd_tests(void)
{
    SUITE("SIMD");
    RUN(simd_clip_i32_saturates);
    RUN(simd_ring_pull_unity_volume_wraps_rd);
    RUN(simd_ring_pull_scaled_clip);
}
