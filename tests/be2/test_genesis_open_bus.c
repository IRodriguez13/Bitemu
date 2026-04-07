/**
 * BE2 - Genesis: tests open bus behavior
 *
 * Tests para comportamiento de bus abierto en direcciones sin mapear
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "test_harness.h"
#include "be2/memory.h"
#include "be2/genesis_constants.h"
#include <string.h>

/* Test open bus byte read behavior */
TEST(genesis_open_bus_byte_read)
{
    genesis_mem_t mem;
    memset(&mem, 0, sizeof(mem));
    mem.bus_read_latch = 0x42;
    
    /* Test different unmapped addresses */
    uint8_t val1 = genesis_open_bus_read_u8_public(&mem, 0x500000);
    uint8_t val2 = genesis_open_bus_read_u8_public(&mem, 0x600000);
    uint8_t val3 = genesis_open_bus_read_u8_public(&mem, 0x700000);
    
    /* Should return different values for different addresses */
    ASSERT_NE(val1, val2);
    ASSERT_NE(val2, val3);
    
    /* Bit 7 should be set (open bus behavior) */
    ASSERT_EQ(val1 & 0x80, 0x80);
    ASSERT_EQ(val2 & 0x80, 0x80);
    ASSERT_EQ(val3 & 0x80, 0x80);
}

/* Test open bus word read behavior */
TEST(genesis_open_bus_word_read)
{
    genesis_mem_t mem;
    memset(&mem, 0, sizeof(mem));
    mem.bus_read_latch = 0x1234;
    
    uint16_t val = genesis_open_bus_read_u16_public(&mem, 0x500000);
    
    /* Should return a 16-bit value */
    ASSERT_NE(val, 0);
    
    /* Should be different from byte reads at same address */
    uint8_t low = genesis_open_bus_read_u8_public(&mem, 0x500000);
    uint8_t high = genesis_open_bus_read_u8_public(&mem, 0x500001);
    uint16_t expected = ((uint16_t)high << 8) | (uint16_t)low;
    ASSERT_EQ(val, expected);
}

/* Test open bus long read behavior */
TEST(genesis_open_bus_long_read)
{
    genesis_mem_t mem;
    memset(&mem, 0, sizeof(mem));
    mem.bus_read_latch = 0x5678;
    
    uint32_t val = genesis_open_bus_read_u32_public(&mem, 0x500000);
    
    /* Should return a 32-bit value */
    ASSERT_NE(val, 0);
    
    /* Should be constructed from two word reads */
    uint16_t low = genesis_open_bus_read_u16_public(&mem, 0x500000);
    uint16_t high = genesis_open_bus_read_u16_public(&mem, 0x500002);
    uint32_t expected = ((uint32_t)high << 16) | (uint32_t)low;
    ASSERT_EQ(val, expected);
}

/* Test address mapping detection */
TEST(genesis_addr_mapping_detection)
{
    /* Test mapped addresses */
    ASSERT_EQ(genesis_addr_is_unmapped(GEN_ADDR_ROM_START), 0);
    ASSERT_EQ(genesis_addr_is_unmapped(GEN_ADDR_RAM_START), 0);
    ASSERT_EQ(genesis_addr_is_unmapped(GEN_ADDR_SRAM_START), 0);
    ASSERT_EQ(genesis_addr_is_unmapped(GEN_ADDR_VDP_DATA), 0);
    ASSERT_EQ(genesis_addr_is_unmapped(GEN_ADDR_YM_START), 0);
    ASSERT_EQ(genesis_addr_is_unmapped(GEN_ADDR_PSG), 0);
    ASSERT_EQ(genesis_addr_is_unmapped(GEN_ADDR_Z80_START), 0);
    ASSERT_EQ(genesis_addr_is_unmapped(GEN_ADDR_JOYPAD1), 0);
    
    /* Test unmapped addresses */
    ASSERT_EQ(genesis_addr_is_unmapped(0x500000), 1);
    ASSERT_EQ(genesis_addr_is_unmapped(0x600000), 1);
    ASSERT_EQ(genesis_addr_is_unmapped(0x700000), 1);
    ASSERT_EQ(genesis_addr_is_unmapped(0x900000), 1);
    ASSERT_EQ(genesis_addr_is_unmapped(0xB00000), 1);
}

/* Test bus latch functionality */
TEST(genesis_bus_latch_functionality)
{
    genesis_mem_t mem;
    memset(&mem, 0, sizeof(mem));
    
    /* Initial latch should be 0 */
    ASSERT_EQ(genesis_get_bus_latch(&mem), 0);
    
    /* Update latch */
    genesis_update_bus_latch(&mem, 0xAB);
    ASSERT_EQ(genesis_get_bus_latch(&mem), 0xAB);
    
    /* Update again */
    genesis_update_bus_latch(&mem, 0xCD);
    ASSERT_EQ(genesis_get_bus_latch(&mem), 0xCD);
    
    /* Test with NULL pointer (should be safe) */
    ASSERT_EQ(genesis_get_bus_latch(NULL), 0xFF);
    genesis_update_bus_latch(NULL, 0xEF); /* Should not crash */
}

/* Test open bus patterns for specific addresses */
TEST(genesis_open_bus_address_patterns)
{
    genesis_mem_t mem;
    memset(&mem, 0, sizeof(mem));
    mem.bus_read_latch = 0x42;
    
    /* Test 1MB boundary addresses (should tend to 0xFF) */
    uint8_t val1 = genesis_open_bus_read_u8_public(&mem, 0x000000);
    uint8_t val2 = genesis_open_bus_read_u8_public(&mem, 0x100000);
    uint8_t val3 = genesis_open_bus_read_u8_public(&mem, 0x200000);
    
    ASSERT_EQ(val1, 0xFF);
    ASSERT_EQ(val2, 0xFF);
    ASSERT_EQ(val3, 0xFF);
    
    /* Test page boundary addresses */
    uint8_t page1 = genesis_open_bus_read_u8_public(&mem, 0x1000);
    uint8_t page2 = genesis_open_bus_read_u8_public(&mem, 0x2000);
    uint8_t page3 = genesis_open_bus_read_u8_public(&mem, 0x3000);
    
    /* Should have specific pattern based on page number */
    ASSERT_EQ(page1, 0x40u | ((0x1000 >> 12) & 0x3Fu));
    ASSERT_EQ(page2, 0x40u | ((0x2000 >> 12) & 0x3Fu));
    ASSERT_EQ(page3, 0x40u | ((0x3000 >> 12) & 0x3Fu));
}

/* Test open bus consistency */
TEST(genesis_open_bus_consistency)
{
    genesis_mem_t mem;
    memset(&mem, 0, sizeof(mem));
    mem.bus_read_latch = 0x99;
    
    uint32_t addr = 0x500000;
    
    /* Multiple reads of same address should return same value */
    uint8_t val1 = genesis_open_bus_read_u8_public(&mem, addr);
    uint8_t val2 = genesis_open_bus_read_u8_public(&mem, addr);
    uint8_t val3 = genesis_open_bus_read_u8_public(&mem, addr);
    
    ASSERT_EQ(val1, val2);
    ASSERT_EQ(val2, val3);
    
    /* Different addresses should return different values */
    uint8_t val4 = genesis_open_bus_read_u8_public(&mem, addr + 1);
    uint8_t val5 = genesis_open_bus_read_u8_public(&mem, addr + 2);
    
    ASSERT_NE(val1, val4);
    ASSERT_NE(val4, val5);
}

/* Test bus latch influence on open bus */
TEST(genesis_open_bus_latch_influence)
{
    genesis_mem_t mem;
    memset(&mem, 0, sizeof(mem));
    
    uint32_t addr = 0x500000;
    
    /* Test with different latch values */
    mem.bus_read_latch = 0x00;
    uint8_t val1 = genesis_open_bus_read_u8_public(&mem, addr);
    
    mem.bus_read_latch = 0xFF;
    uint8_t val2 = genesis_open_bus_read_u8_public(&mem, addr);
    
    mem.bus_read_latch = 0x55;
    uint8_t val3 = genesis_open_bus_read_u8_public(&mem, addr);
    
    /* Should return different values for different latch values */
    ASSERT_NE(val1, val2);
    ASSERT_NE(val2, val3);
    ASSERT_NE(val1, val3);
    
    /* All should have bit 7 set */
    ASSERT_EQ(val1 & 0x80, 0x80);
    ASSERT_EQ(val2 & 0x80, 0x80);
    ASSERT_EQ(val3 & 0x80, 0x80);
}

/* Test edge case addresses */
TEST(genesis_open_bus_edge_cases)
{
    genesis_mem_t mem;
    memset(&mem, 0, sizeof(mem));
    mem.bus_read_latch = 0x33;
    
    /* Test very high addresses */
    uint8_t val1 = genesis_open_bus_read_u8_public(&mem, 0xFFFFFF);
    uint8_t val2 = genesis_open_bus_read_u8_public(&mem, 0xFFFFFE);
    
    /* Should still work and return valid bytes */
    ASSERT_NE(val1, 0);
    ASSERT_NE(val2, 0);
    ASSERT_EQ(val1 & 0x80, 0x80);
    ASSERT_EQ(val2 & 0x80, 0x80);
    
    /* Test address 0 */
    uint8_t val3 = genesis_open_bus_read_u8_public(&mem, 0x000000);
    ASSERT_EQ(val3, 0xFF); /* 1MB boundary should return 0xFF */
}

/* Test open bus vs real memory access */
TEST(genesis_open_bus_vs_real_memory)
{
    genesis_mem_t mem;
    memset(&mem, 0, sizeof(mem));
    
    /* Setup some real memory */
    mem.ram[0] = 0x12;
    mem.ram[1] = 0x34;
    mem.bus_read_latch = 0x99;
    
    /* Real memory access should return actual values */
    uint8_t ram_val = genesis_mem_read8(&mem, GEN_ADDR_RAM_START);
    ASSERT_EQ(ram_val, 0x12);
    
    /* Open bus access should return different values */
    uint8_t open_val = genesis_open_bus_read_u8_public(&mem, 0x500000);
    ASSERT_NE(open_val, 0x12);
    ASSERT_EQ(open_val & 0x80, 0x80);
}
