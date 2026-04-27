/**
 * BE2 - Genesis: open bus helpers
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "test_harness.h"
#include "be2/memory.h"
#include "be2/genesis_constants.h"
#include <string.h>

TEST(genesis_open_bus_u8_deterministic)
{
    genesis_mem_t mem;
    memset(&mem, 0, sizeof(mem));
    mem.bus_read_latch = 0x42;

    uint8_t a = genesis_open_bus_read_u8_public(&mem, 0x500000);
    uint8_t b = genesis_open_bus_read_u8_public(&mem, 0x500000);
    uint8_t c = genesis_open_bus_read_u8_public(&mem, 0x500001);

    ASSERT_EQ(a, b);
    ASSERT_NEQ(a, c);
}

TEST(genesis_open_bus_u16_u32_layout)
{
    genesis_mem_t mem;
    memset(&mem, 0, sizeof(mem));
    mem.bus_read_latch = 0x99;

    uint16_t w = genesis_open_bus_read_u16_public(&mem, 0x500000);
    uint8_t b0 = genesis_open_bus_read_u8_public(&mem, 0x500000);
    uint8_t b1 = genesis_open_bus_read_u8_public(&mem, 0x500001);
    uint16_t expected_w = (uint16_t)((uint16_t)b1 << 8) | b0;
    ASSERT_EQ(w, expected_w);

    uint32_t l = genesis_open_bus_read_u32_public(&mem, 0x500000);
    uint16_t w0 = genesis_open_bus_read_u16_public(&mem, 0x500000);
    uint16_t w1 = genesis_open_bus_read_u16_public(&mem, 0x500002);
    uint32_t expected_l = ((uint32_t)w1 << 16) | w0;
    ASSERT_EQ(l, expected_l);
}

TEST(genesis_addr_is_unmapped_basic)
{
    ASSERT_EQ(genesis_addr_is_unmapped(GEN_ADDR_ROM_START), 0);
    ASSERT_EQ(genesis_addr_is_unmapped(GEN_ADDR_RAM_START), 0);
    ASSERT_EQ(genesis_addr_is_unmapped(GEN_ADDR_VDP_DATA), 0);
    ASSERT_EQ(genesis_addr_is_unmapped(0x500000), 1);
}

TEST(genesis_bus_latch_helpers)
{
    genesis_mem_t mem;
    memset(&mem, 0, sizeof(mem));

    ASSERT_EQ(genesis_get_bus_latch(&mem), 0);
    genesis_update_bus_latch(&mem, 0xAB);
    ASSERT_EQ(genesis_get_bus_latch(&mem), 0xAB);
    ASSERT_EQ(genesis_get_bus_latch(NULL), 0xFF);
    genesis_update_bus_latch(NULL, 0xCD);
}

void run_genesis_open_bus_tests(void)
{
    SUITE("Genesis open bus");
    RUN(genesis_open_bus_u8_deterministic);
    RUN(genesis_open_bus_u16_u32_layout);
    RUN(genesis_addr_is_unmapped_basic);
    RUN(genesis_bus_latch_helpers);
}
