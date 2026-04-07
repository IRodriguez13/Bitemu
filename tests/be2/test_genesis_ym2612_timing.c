/**
 * BE2 - Genesis: tests YM2612 cycle-exact timing
 *
 * Tests para busy bits precisos, timing de escritura, y validación de ciclos
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "test_harness.h"
#include "be2/ym2612/ym2612.h"
#include "be2/genesis_constants.h"
#include <string.h>

/* Test YM2612 busy flag after address write */
TEST(gen_ym2612_busy_after_address_write)
{
    gen_ym2612_t ym;
    gen_ym2612_init(&ym);
    gen_ym2612_reset(&ym);
    
    /* Write to address port should set busy */
    gen_ym2612_write_port(&ym, 0, 0x2A);  /* Port 0, address 0x2A */
    
    ASSERT_EQ(gen_ym2612_is_busy(&ym), 1);
    ASSERT_EQ(gen_ym2612_busy_cycles_remaining(&ym), GEN_YM2612_ADDR_BUSY_CYCLES_68K);
    
    /* Status should show busy flag */
    uint8_t status = gen_ym2612_read_port(&ym, 0);
    ASSERT_EQ(status & 0x80, 0x80);
}

/* Test YM2612 busy flag after data write */
TEST(gen_ym2612_busy_after_data_write)
{
    gen_ym2612_t ym;
    gen_ym2612_init(&ym);
    gen_ym2612_reset(&ym);
    
    /* First write address */
    gen_ym2612_write_port(&ym, 0, 0xA0);  /* Port 0, address 0xA0 */
    
    /* Clear busy by stepping */
    gen_ym2612_step(&ym, GEN_YM2612_ADDR_BUSY_CYCLES_68K, nullptr);
    
    /* Write data should set busy with longer period */
    gen_ym2612_write_port(&ym, 1, 0x1234 & 0xFF);  /* Port 1, data */
    
    ASSERT_EQ(gen_ym2612_is_busy(&ym), 1);
    ASSERT_EQ(gen_ym2612_busy_cycles_remaining(&ym), GEN_YM2612_WRITE_BUSY_CYCLES_68K);
}

/* Test YM2612 busy flag clears after correct cycles */
TEST(gen_ym2612_busy_clears_after_timing)
{
    gen_ym2612_t ym;
    gen_ym2612_init(&ym);
    gen_ym2612_reset(&ym);
    
    /* Write data */
    gen_ym2612_write_port(&ym, 0, 0xA0);
    gen_ym2612_write_port(&ym, 1, 0x1234 & 0xFF);
    
    /* Should be busy */
    ASSERT_EQ(gen_ym2612_is_busy(&ym), 1);
    
    /* Step exactly the busy period */
    gen_ym2612_step(&ym, GEN_YM2612_WRITE_BUSY_CYCLES_68K, nullptr);
    
    /* Should no longer be busy */
    ASSERT_EQ(gen_ym2612_is_busy(&ym), 0);
    ASSERT_EQ(gen_ym2612_busy_cycles_remaining(&ym), 0);
    
    /* Status should not show busy flag */
    uint8_t status = gen_ym2612_read_port(&ym, 0);
    ASSERT_EQ(status & 0x80, 0);
}

/* Test YM2612 timing with enhanced status read */
TEST(gen_ym2612_enhanced_status_read)
{
    gen_ym2612_t ym;
    gen_ym2612_init(&ym);
    gen_ym2612_reset(&ym);
    
    /* Set timer overflow flags */
    ym.timer_overflow = 0x03;  /* Both timers overflowed */
    
    /* Write to set busy */
    gen_ym2612_write_port(&ym, 0, 0x2A);
    
    /* Enhanced status should show both busy and timer flags */
    uint8_t status = gen_ym2612_read_status_enhanced(&ym, 0);
    ASSERT_EQ(status & 0x80, 0x80);  /* Busy flag */
    ASSERT_EQ(status & 0x03, 0x03);  /* Timer flags */
    
    /* Timer flags should be cleared after reading */
    status = gen_ym2612_read_status_enhanced(&ym, 0);
    ASSERT_EQ(status & 0x03, 0x00);  /* Timer flags cleared */
    ASSERT_EQ(status & 0x80, 0x80);  /* Busy flag still set */
}

/* Test YM2612 write timing validation */
TEST(gen_ym2612_write_timing_validation)
{
    gen_ym2612_t ym;
    gen_ym2612_init(&ym);
    gen_ym2612_reset(&ym);
    
    uint32_t start_cycle = 1000;
    
    /* Write with timing */
    gen_ym2612_write_port_with_timing(&ym, 0, 0x2A, start_cycle);
    
    /* Should not be valid before busy period */
    int valid = gen_ym2612_validate_write_timing(&ym, start_cycle + 10, GEN_YM2612_ADDR_BUSY_CYCLES_68K);
    ASSERT_EQ(valid, 0);
    
    /* Should be valid after busy period */
    valid = gen_ym2612_validate_write_timing(&ym, start_cycle + GEN_YM2612_ADDR_BUSY_CYCLES_68K, GEN_YM2612_ADDR_BUSY_CYCLES_68K);
    ASSERT_EQ(valid, 1);
}

/* Test YM2612 timing information retrieval */
TEST(gen_ym2612_timing_info_retrieval)
{
    gen_ym2612_t ym;
    gen_ym2612_init(&ym);
    gen_ym2612_reset(&ym);
    
    uint32_t test_cycle = 2000;
    uint8_t test_port = 1;
    
    /* Write with timing */
    gen_ym2612_write_port_with_timing(&ym, test_port, 0x2A, test_cycle);
    
    /* Get timing info */
    int busy_cycles;
    uint8_t last_port;
    uint32_t last_timestamp;
    
    gen_ym2612_get_timing_info(&ym, &busy_cycles, &last_port, &last_timestamp);
    
    ASSERT_EQ(busy_cycles, GEN_YM2612_ADDR_BUSY_CYCLES_68K);
    ASSERT_EQ(last_port, test_port);
    ASSERT_EQ(last_timestamp, test_cycle);
}

/* Test YM2612 busy during consecutive writes */
TEST(gen_ym2612_busy_consecutive_writes)
{
    gen_ym2612_t ym;
    gen_ym2612_init(&ym);
    gen_ym2612_reset(&ym);
    
    /* First write */
    gen_ym2612_write_port(&ym, 0, 0x2A);
    ASSERT_EQ(gen_ym2612_is_busy(&ym), 1);
    
    /* Second write during busy should extend busy period */
    gen_ym2612_write_port(&ym, 1, 0x1234 & 0xFF);
    ASSERT_EQ(gen_ym2612_busy_cycles_remaining(&ym), GEN_YM2612_WRITE_BUSY_CYCLES_68K);
    
    /* Should remain busy for the full period */
    gen_ym2612_step(&ym, GEN_YM2612_WRITE_BUSY_CYCLES_68K - 1, nullptr);
    ASSERT_EQ(gen_ym2612_is_busy(&ym), 1);
    
    /* Clear on final cycle */
    gen_ym2612_step(&ym, 1, nullptr);
    ASSERT_EQ(gen_ym2612_is_busy(&ym), 0);
}

/* Test YM2612 different port timing */
TEST(gen_ym2612_different_port_timing)
{
    gen_ym2612_t ym;
    gen_ym2612_init(&ym);
    gen_ym2612_reset(&ym);
    
    /* Port 0 and 2 (address ports) should have same timing */
    gen_ym2612_write_port(&ym, 0, 0x2A);
    ASSERT_EQ(gen_ym2612_busy_cycles_remaining(&ym), GEN_YM2612_ADDR_BUSY_CYCLES_68K);
    
    gen_ym2612_step(&ym, GEN_YM2612_ADDR_BUSY_CYCLES_68K, nullptr);
    
    gen_ym2612_write_port(&ym, 2, 0x2A);
    ASSERT_EQ(gen_ym2612_busy_cycles_remaining(&ym), GEN_YM2612_ADDR_BUSY_CYCLES_68K);
    
    gen_ym2612_step(&ym, GEN_YM2612_ADDR_BUSY_CYCLES_68K, nullptr);
    
    /* Port 1 and 3 (data ports) should have same timing */
    gen_ym2612_write_port(&ym, 0, 0xA0);  /* Set address first */
    gen_ym2612_write_port(&ym, 1, 0x1234 & 0xFF);
    ASSERT_EQ(gen_ym2612_busy_cycles_remaining(&ym), GEN_YM2612_WRITE_BUSY_CYCLES_68K);
    
    gen_ym2612_step(&ym, GEN_YM2612_WRITE_BUSY_CYCLES_68K, nullptr);
    
    gen_ym2612_write_port(&ym, 2, 0xA0);  /* Set address first */
    gen_ym2612_write_port(&ym, 3, 0x1234 & 0xFF);
    ASSERT_EQ(gen_ym2612_busy_cycles_remaining(&ym), GEN_YM2612_WRITE_BUSY_CYCLES_68K);
}

/* Test YM2612 busy flag precision */
TEST(gen_ym2612_busy_flag_precision)
{
    gen_ym2612_t ym;
    gen_ym2612_init(&ym);
    gen_ym2612_reset(&ym);
    
    /* Write data */
    gen_ym2612_write_port(&ym, 0, 0xA0);
    gen_ym2612_write_port(&ym, 1, 0x1234 & 0xFF);
    
    /* Step one by one to check precision */
    for (int i = 0; i < GEN_YM2612_WRITE_BUSY_CYCLES_68K; i++) {
        ASSERT_EQ(gen_ym2612_is_busy(&ym), 1);
        ASSERT_EQ(gen_ym2612_busy_cycles_remaining(&ym), GEN_YM2612_WRITE_BUSY_CYCLES_68K - i);
        gen_ym2612_step(&ym, 1, nullptr);
    }
    
    /* Should be clear exactly after the specified cycles */
    ASSERT_EQ(gen_ym2612_is_busy(&ym), 0);
    ASSERT_EQ(gen_ym2612_busy_cycles_remaining(&ym), 0);
}
