/**
 * Tests: MBC2 cartridge mapper.
 * 512x4-bit RAM, bit-8 address banking, ROM bank select.
 */

#include "test_harness.h"
#include "be1/memory.h"
#include "be1/gb_constants.h"

static gb_mem_t mem;
static uint8_t rom[GB_ROM_BANK_SIZE * 4];

static void setup(void)
{
    memset(&mem, 0, sizeof(mem));
    gb_mem_init(&mem);
    memset(rom, 0, sizeof(rom));
    rom[0x147] = GB_CART_MBC2;
    mem.rom = rom;
    mem.rom_size = sizeof(rom);
    mem.cart_type = GB_CART_MBC2;
    gb_mem_reset(&mem);
}

/* MBC2 RAM writes only store lower 4 bits */
TEST(mbc2_ram_4bit)
{
    setup();
    mem.ext_ram_enabled = 1;
    gb_mem_write(&mem, GB_EXT_RAM_START, 0xFF);
    uint8_t val = gb_mem_read(&mem, GB_EXT_RAM_START);
    ASSERT_EQ(val & 0x0F, 0x0F);
    ASSERT_EQ(val & 0xF0, 0xF0);
}

/* MBC2 RAM enable: address bit 8 = 0 → RAM enable */
TEST(mbc2_ram_enable)
{
    setup();
    mem.ext_ram_enabled = 0;
    gb_mem_write(&mem, 0x0000, 0x0A);
    ASSERT_TRUE(mem.ext_ram_enabled);

    gb_mem_write(&mem, 0x0000, 0x00);
    ASSERT_FALSE(mem.ext_ram_enabled);
}

/* MBC2 ROM bank: address bit 8 = 1 → ROM bank select */
TEST(mbc2_rom_bank)
{
    setup();
    gb_mem_write(&mem, 0x0100, 0x03);
    ASSERT_EQ(mem.rom_bank, 3);

    gb_mem_write(&mem, 0x0100, 0x00);
    ASSERT_EQ(mem.rom_bank, 1);
}

/* MBC2 RAM disabled returns 0xFF */
TEST(mbc2_ram_disabled_reads_ff)
{
    setup();
    mem.ext_ram_enabled = 0;
    ASSERT_EQ(gb_mem_read(&mem, GB_EXT_RAM_START), 0xFF);
}

/* MBC2 RAM wraps within 512 bytes */
TEST(mbc2_ram_wraps)
{
    setup();
    mem.ext_ram_enabled = 1;
    gb_mem_write(&mem, GB_EXT_RAM_START, 0x05);
    uint8_t v1 = gb_mem_read(&mem, GB_EXT_RAM_START);
    uint8_t v2 = gb_mem_read(&mem, GB_EXT_RAM_START + GB_MBC2_RAM_SIZE);
    ASSERT_EQ(v1 & 0x0F, v2 & 0x0F);
}

void run_mbc2_tests(void)
{
    SUITE("MBC2");
    RUN(mbc2_ram_4bit);
    RUN(mbc2_ram_enable);
    RUN(mbc2_rom_bank);
    RUN(mbc2_ram_disabled_reads_ff);
    RUN(mbc2_ram_wraps);
}
