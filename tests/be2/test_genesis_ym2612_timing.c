/**
 * BE2 - Genesis: YM2612 timing helper tests
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "test_harness.h"
#include "be2/ym2612/ym2612.h"
#include "be2/genesis_constants.h"

TEST(gen_ym2612_busy_after_address_write)
{
    gen_ym2612_t ym;
    gen_ym2612_init(&ym);
    gen_ym2612_reset(&ym);

    gen_ym2612_write_port(&ym, 0, 0x2A);
    ASSERT_EQ(gen_ym2612_is_busy(&ym), 1);
    ASSERT_EQ(gen_ym2612_busy_cycles_remaining(&ym), GEN_YM2612_ADDR_BUSY_CYCLES_68K);
    ASSERT_EQ(gen_ym2612_read_port(&ym, 0) & 0x80, 0x80);
}

TEST(gen_ym2612_busy_after_data_write)
{
    gen_ym2612_t ym;
    gen_ym2612_init(&ym);
    gen_ym2612_reset(&ym);

    gen_ym2612_write_port(&ym, 0, 0xA0);
    gen_ym2612_step(&ym, GEN_YM2612_ADDR_BUSY_CYCLES_68K, NULL);
    gen_ym2612_write_port(&ym, 1, 0x34);
    ASSERT_EQ(gen_ym2612_busy_cycles_remaining(&ym), GEN_YM2612_WRITE_BUSY_CYCLES_68K);
}

TEST(gen_ym2612_busy_clears_with_exact_cycles)
{
    gen_ym2612_t ym;
    gen_ym2612_init(&ym);
    gen_ym2612_reset(&ym);

    gen_ym2612_write_port(&ym, 0, 0xA0);
    gen_ym2612_write_port(&ym, 1, 0x55);

    for (int i = 0; i < GEN_YM2612_WRITE_BUSY_CYCLES_68K; i++) {
        ASSERT_EQ(gen_ym2612_is_busy(&ym), 1);
        gen_ym2612_step(&ym, 1, NULL);
    }
    ASSERT_EQ(gen_ym2612_is_busy(&ym), 0);
    ASSERT_EQ(gen_ym2612_busy_cycles_remaining(&ym), 0);
}

TEST(gen_ym2612_timing_metadata_helpers)
{
    gen_ym2612_t ym;
    int busy = 0;
    uint8_t port = 0;
    uint32_t ts = 0;

    gen_ym2612_init(&ym);
    gen_ym2612_reset(&ym);
    gen_ym2612_write_port_with_timing(&ym, 2, 0x2A, 1234);
    gen_ym2612_get_timing_info(&ym, &busy, &port, &ts);
    ASSERT_EQ(port, 2);
    ASSERT_EQ(ts, 1234u);
    ASSERT_EQ(gen_ym2612_validate_write_timing(&ym, 1234 + GEN_YM2612_ADDR_BUSY_CYCLES_68K,
                                               GEN_YM2612_ADDR_BUSY_CYCLES_68K), 1);
}

void run_genesis_ym2612_timing_tests(void)
{
    SUITE("Genesis YM2612 timing");
    RUN(gen_ym2612_busy_after_address_write);
    RUN(gen_ym2612_busy_after_data_write);
    RUN(gen_ym2612_busy_clears_with_exact_cycles);
    RUN(gen_ym2612_timing_metadata_helpers);
}
