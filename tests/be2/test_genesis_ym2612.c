/**
 * BE2 - Genesis: tests YM2612
 *
 * ADSR: key on → ATTACK, key off → RELEASE.
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "test_harness.h"
#include "be2/ym2612/ym2612.h"
#include "be2/genesis_constants.h"
#include <string.h>

/* env_state: 0=ATTACK, 1=DECAY1, 2=DECAY2, 3=RELEASE, 4=OFF */

TEST(gen_ym2612_read_port_mvp)
{
    gen_ym2612_t ym;
    gen_ym2612_init(&ym);
    gen_ym2612_reset(&ym);
    ASSERT_EQ(gen_ym2612_read_port(&ym, 0), 0);
    gen_ym2612_write_port(&ym, 0, 0x30);
    gen_ym2612_write_port(&ym, 1, 0);
    ASSERT_EQ(gen_ym2612_read_port(&ym, 0), 0x80);
    gen_ym2612_step(&ym, 200, NULL);
    ASSERT_EQ(gen_ym2612_read_port(&ym, 3), 0);
}

/* Bit 7 busy tras escribir dato; decae con ciclos de chip (aprox. test ROMs). */
TEST(gen_ym2612_addr_write_busy)
{
    gen_ym2612_t ym;
    gen_ym2612_init(&ym);
    gen_ym2612_reset(&ym);
    gen_ym2612_write_port(&ym, 0, 0x30);
    ASSERT_EQ(gen_ym2612_read_port(&ym, 0) & 0x80u, 0x80u);
    gen_ym2612_step(&ym, GEN_YM2612_ADDR_BUSY_CYCLES_68K, NULL);
    ASSERT_EQ(gen_ym2612_read_port(&ym, 0) & 0x80u, 0u);
}

TEST(gen_ym2612_busy_clears_after_chip_cycles)
{
    gen_ym2612_t ym;
    gen_ym2612_init(&ym);
    gen_ym2612_reset(&ym);
    gen_ym2612_write_port(&ym, 0, 0x30);
    gen_ym2612_step(&ym, GEN_YM2612_ADDR_BUSY_CYCLES_68K, NULL);
    ASSERT_EQ(gen_ym2612_read_port(&ym, 0) & 0x80u, 0u);
    gen_ym2612_write_port(&ym, 1, 0);
    ASSERT_EQ(gen_ym2612_read_port(&ym, 0), 0x80u);
    gen_ym2612_step(&ym, 200, NULL);
    ASSERT_EQ(gen_ym2612_read_port(&ym, 0) & 0x80u, 0u);
}

TEST(gen_ym2612_key_on_attack)
{
    gen_ym2612_t ym;
    gen_ym2612_init(&ym);
    gen_ym2612_reset(&ym);

    gen_ym2612_write_port(&ym, 0, 0x28);
    gen_ym2612_write_port(&ym, 1, 0x10);
    ASSERT_EQ(ym.key[0][0], 1);
    ASSERT_EQ(ym.env_state[0][0], 0);
    ASSERT_EQ(ym.env_level[0][0], 0);
}

TEST(gen_ym2612_key_off_release)
{
    gen_ym2612_t ym;
    gen_ym2612_init(&ym);
    gen_ym2612_reset(&ym);

    gen_ym2612_write_port(&ym, 0, 0x28);
    gen_ym2612_write_port(&ym, 1, 0x10);
    gen_ym2612_write_port(&ym, 1, 0x00);
    ASSERT_EQ(ym.key[0][0], 0);
    ASSERT_EQ(ym.env_state[0][0], 3);
}

TEST(gen_ym2612_run_produces_output)
{
    gen_ym2612_t ym;
    gen_ym2612_init(&ym);
    gen_ym2612_reset(&ym);

    gen_ym2612_write_port(&ym, 0, 0xA4);
    gen_ym2612_write_port(&ym, 1, 0x22);
    gen_ym2612_write_port(&ym, 0, 0xA0);
    gen_ym2612_write_port(&ym, 1, 0x00);
    for (int r = 0x50; r <= 0x5C; r += 4)
    {
        gen_ym2612_write_port(&ym, 0, (uint8_t)r);
        gen_ym2612_write_port(&ym, 1, 0x1F);
    }
    gen_ym2612_write_port(&ym, 0, 0x28);
    gen_ym2612_write_port(&ym, 1, 0xF0);

    int16_t l = 0, r = 0;
    for (int i = 0; i < 200; i++)
        gen_ym2612_run_cycles(&ym, GEN_CYCLES_PER_SAMPLE, &l, &r);
    ASSERT_TRUE(l != 0 || r != 0);
}

void run_genesis_ym2612_tests(void)
{
    SUITE("Genesis YM2612");
    RUN(gen_ym2612_read_port_mvp);
    RUN(gen_ym2612_addr_write_busy);
    RUN(gen_ym2612_busy_clears_after_chip_cycles);
    RUN(gen_ym2612_key_on_attack);
    RUN(gen_ym2612_key_off_release);
    RUN(gen_ym2612_run_produces_output);
}
