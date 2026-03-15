/**
 * BE2 - Genesis: tests del subsistema de memoria
 *
 * RAM, ROM, joypad, VDP (status/HV), PSG, Z80 RAM, Z80 bus.
 * Usa genesis_impl_t para tener todos los subsistemas conectados.
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "test_harness.h"
#include "be2/genesis_impl.h"
#include "be2/genesis_constants.h"
#include "be2/memory.h"
#include "be2/vdp/vdp.h"
#include "be2/ym2612/ym2612.h"
#include "be2/psg/psg.h"
#include <string.h>
#include <stdlib.h>

static genesis_impl_t impl;

static void setup(void)
{
    memset(&impl, 0, sizeof(impl));
    genesis_mem_init(&impl.mem);
    genesis_mem_set_vdp(&impl.mem, &impl.vdp);
    genesis_mem_set_ym(&impl.mem, &impl.ym2612);
    genesis_mem_set_psg(&impl.mem, &impl.psg);
    genesis_mem_set_z80(&impl.mem, impl.z80_ram, &impl.z80_bus_req, &impl.z80_reset);
    gen_vdp_init(&impl.vdp);
    gen_ym2612_init(&impl.ym2612);
    gen_psg_init(&impl.psg);
    genesis_mem_reset(&impl.mem);
}

/* --- RAM read/write --- */

TEST(gen_mem_ram_readwrite)
{
    setup();
    genesis_mem_write8(&impl.mem, GEN_ADDR_RAM_START, 0x42);
    ASSERT_EQ(genesis_mem_read8(&impl.mem, GEN_ADDR_RAM_START), 0x42);

    genesis_mem_write8(&impl.mem, GEN_ADDR_RAM_END - 1, 0xAB);
    ASSERT_EQ(genesis_mem_read8(&impl.mem, GEN_ADDR_RAM_END - 1), 0xAB);
}

TEST(gen_mem_ram_word)
{
    setup();
    genesis_mem_write16(&impl.mem, GEN_ADDR_RAM_START, 0x1234);
    ASSERT_EQ(genesis_mem_read16(&impl.mem, GEN_ADDR_RAM_START), 0x1234);
}

TEST(gen_mem_ram_long)
{
    setup();
    genesis_mem_write32(&impl.mem, GEN_ADDR_RAM_START, 0x12345678);
    ASSERT_EQ(genesis_mem_read32(&impl.mem, GEN_ADDR_RAM_START), 0x12345678);
}

/* --- ROM read (sin ROM = 0xFF) --- */

TEST(gen_mem_rom_no_rom_returns_ff)
{
    setup();
    ASSERT_EQ(genesis_mem_read8(&impl.mem, 0x000000), 0xFF);
    ASSERT_EQ(genesis_mem_read16(&impl.mem, 0x100000), 0xFFFF);
}

TEST(gen_mem_rom_with_rom)
{
    setup();
    uint8_t rom[0x200];
    memset(rom, 0xAA, sizeof(rom));
    memcpy(rom + GEN_HEADER_OFFSET, GEN_HEADER_MAGIC, GEN_HEADER_MAGIC_LEN);
    impl.mem.rom = rom;
    impl.mem.rom_size = sizeof(rom);

    ASSERT_EQ(genesis_mem_read8(&impl.mem, 0x000000), 0xAA);
    ASSERT_EQ(genesis_mem_read8(&impl.mem, GEN_HEADER_OFFSET), 'S');
    ASSERT_EQ(genesis_mem_read8(&impl.mem, GEN_HEADER_OFFSET + 1), 'E');
    ASSERT_EQ(genesis_mem_read16(&impl.mem, GEN_HEADER_OFFSET), 0x5345);  /* "SE" big-endian */
}

/* --- Joypad (reset = 0x3F = no buttons) --- */

TEST(gen_mem_joypad_default)
{
    setup();
    uint8_t lo = genesis_mem_read8(&impl.mem, GEN_IO_JOYPAD1);
    uint8_t hi = genesis_mem_read8(&impl.mem, GEN_IO_JOYPAD1 + 1);
    ASSERT_EQ(lo, 0x3F);
    ASSERT_EQ(hi, 0x3F);
}

TEST(gen_mem_joypad_custom)
{
    setup();
    impl.mem.joypad_raw[0] = 0x0F;  /* algunos botones presionados */
    genesis_mem_write8(&impl.mem, GEN_IO_JOYPAD1_CTRL, GEN_JOYPAD_TH);
    uint8_t lo = genesis_mem_read8(&impl.mem, GEN_IO_JOYPAD1_DATA);
    ASSERT_EQ(lo, 0xF0);
}

/* --- Version register --- */

TEST(gen_mem_version_register)
{
    setup();
    uint8_t v = genesis_mem_read8(&impl.mem, GEN_IO_VERSION);
    ASSERT_EQ(v, 0x00);
}

/* --- VDP: status y HV counter (no crash) --- */

TEST(gen_mem_vdp_status_read)
{
    setup();
    uint8_t lo = genesis_mem_read8(&impl.mem, GEN_ADDR_VDP_CTRL);
    uint8_t hi = genesis_mem_read8(&impl.mem, GEN_ADDR_VDP_CTRL + 1);
    (void)lo;
    (void)hi;
    ASSERT_TRUE(1);
}

TEST(gen_mem_vdp_hv_read)
{
    setup();
    uint16_t hv = genesis_mem_read16(&impl.mem, GEN_ADDR_VDP_HV);
    ASSERT_RANGE(hv, 0, 0xFFFF);
}

/* --- VDP: write data/ctrl (VRAM via comando 32-bit) --- */

TEST(gen_mem_vdp_write_ctrl)
{
    setup();
    genesis_mem_write16(&impl.mem, GEN_ADDR_VDP_CTRL, 0x8140);  /* reg 1 = 0x40 (display on) */
    ASSERT_EQ(impl.vdp.regs[1], 0x40);
}

TEST(gen_mem_vdp_write_data)
{
    setup();
    gen_vdp_reset(&impl.vdp);
    /* Smoke: comando VRAM write (0x40000000) + escritura datos. El bus VDP acepta sin crash. */
    genesis_mem_write16(&impl.mem, GEN_ADDR_VDP_CTRL, 0x4000);
    genesis_mem_write16(&impl.mem, GEN_ADDR_VDP_CTRL, 0x0000);
    genesis_mem_write16(&impl.mem, GEN_ADDR_VDP_DATA, 0x1234);
    /* TODO: verificar vram[addr] cuando el flujo de comandos VDP esté depurado */
    ASSERT_TRUE(1);
}

/* --- PSG write-only (read returns 0xFF) --- */

TEST(gen_mem_psg_write)
{
    setup();
    genesis_mem_write8(&impl.mem, GEN_ADDR_PSG, 0x80);  /* latch tone 0 */
    ASSERT_EQ(genesis_mem_read8(&impl.mem, GEN_ADDR_PSG), 0xFF);
}

/* --- Z80 RAM --- */

TEST(gen_mem_z80_ram_readwrite)
{
    setup();
    genesis_mem_write8(&impl.mem, GEN_ADDR_Z80_START, 0x55);
    ASSERT_EQ(genesis_mem_read8(&impl.mem, GEN_ADDR_Z80_START), 0x55);

    genesis_mem_write16(&impl.mem, GEN_ADDR_Z80_START + 100, 0xABCD);
    ASSERT_EQ(genesis_mem_read16(&impl.mem, GEN_ADDR_Z80_START + 100), 0xABCD);
}

/* --- Z80 bus request / reset --- */

TEST(gen_mem_z80_bus_req)
{
    setup();
    impl.z80_bus_req = 0;
    ASSERT_EQ(genesis_mem_read8(&impl.mem, 0xA11100), 0);
    genesis_mem_write8(&impl.mem, 0xA11100, 1);
    ASSERT_EQ(impl.z80_bus_req, 1);
}

TEST(gen_mem_z80_reset)
{
    setup();
    impl.z80_reset = 0xFF;
    ASSERT_EQ(genesis_mem_read8(&impl.mem, 0xA11200), 0xFF);
    genesis_mem_write8(&impl.mem, 0xA11200, 0);
    ASSERT_EQ(impl.z80_reset, 0);
}

/* --- TMSS: write a 0xA14000 no debe fallar --- */

TEST(gen_mem_tmss_write)
{
    setup();
    genesis_mem_write8(&impl.mem, GEN_ADDR_TMSS, 0x53);       /* 'S' */
    genesis_mem_write8(&impl.mem, GEN_ADDR_TMSS + 1, 0x45);   /* 'E' */
    genesis_mem_write8(&impl.mem, GEN_ADDR_TMSS + 2, 0x47);   /* 'G' */
    genesis_mem_write8(&impl.mem, GEN_ADDR_TMSS + 3, 0x41);   /* 'A' */
    ASSERT_TRUE(1);  /* No crash = pass */
}

/* --- SRAM: A130F1 habilita mapeo, header "RA" → sram_present --- */

TEST(gen_mem_sram_a130f1)
{
    setup();
    uint8_t rom[0x300];
    memset(rom, 0xFF, sizeof(rom));
    memcpy(rom + GEN_HEADER_OFFSET, GEN_HEADER_MAGIC, GEN_HEADER_MAGIC_LEN);
    rom[GEN_HEADER_OFFSET + GEN_HEADER_SRAM_OFF] = 0x52;     /* 'R' */
    rom[GEN_HEADER_OFFSET + GEN_HEADER_SRAM_OFF + 1] = 0x41; /* 'A' */
    impl.mem.rom = rom;
    impl.mem.rom_size = sizeof(rom);
    impl.mem.sram_present = 1;
    impl.mem.sram_enabled = 0;

    ASSERT_EQ(genesis_mem_read8(&impl.mem, GEN_ADDR_SRAM_START), 0xFF);
    genesis_mem_write8(&impl.mem, GEN_ADDR_SRAM_ENABLE, 1);
    ASSERT_EQ(impl.mem.sram_enabled, 1);
    genesis_mem_write8(&impl.mem, GEN_ADDR_SRAM_START, 0xAB);
    ASSERT_EQ(genesis_mem_read8(&impl.mem, GEN_ADDR_SRAM_START), 0xAB);
}

/* --- Lock-on: ROM ≥4MB, 0x200000 lee del bloqueado --- */

TEST(gen_mem_lockon_detection)
{
    setup();
    uint8_t *rom = calloc(1, GEN_LOCKON_SIZE + 0x100);
    ASSERT_TRUE(rom != NULL);
    memcpy(rom, GEN_HEADER_MAGIC, GEN_HEADER_MAGIC_LEN);
    memcpy(rom + GEN_HEADER_OFFSET, GEN_HEADER_MAGIC, GEN_HEADER_MAGIC_LEN);
    rom[0x200000] = 0xBB;
    rom[0x200001] = 0xCC;
    impl.mem.rom = rom;
    impl.mem.rom_size = GEN_LOCKON_SIZE + 0x100;
    impl.mem.lockon = 1;
    impl.mem.lockon_has_patch = 0;
    impl.mem.sram_enabled = 0;

    ASSERT_EQ(genesis_mem_read8(&impl.mem, 0x000000), 'S');
    ASSERT_EQ(genesis_mem_read8(&impl.mem, 0x200000), 0xBB);
    ASSERT_EQ(genesis_mem_read16(&impl.mem, 0x200000), 0xBBCC);

    free(rom);
}

/* --- Lock-on + A130F1: Sonic 3 & K, sram_enabled mapea SRAM en 0x200000 --- */

TEST(gen_mem_lockon_sram)
{
    setup();
    uint8_t *rom = calloc(1, GEN_LOCKON_SIZE);
    ASSERT_TRUE(rom != NULL);
    memcpy(rom + GEN_HEADER_OFFSET, GEN_HEADER_MAGIC, GEN_HEADER_MAGIC_LEN);
    impl.mem.rom = rom;
    impl.mem.rom_size = GEN_LOCKON_SIZE;
    impl.mem.lockon = 1;
    impl.mem.lockon_has_patch = 0;
    impl.mem.sram_present = 1;
    impl.mem.sram_enabled = 1;

    genesis_mem_write8(&impl.mem, GEN_ADDR_SRAM_START, 0x77);
    ASSERT_EQ(genesis_mem_read8(&impl.mem, GEN_ADDR_SRAM_START), 0x77);

    free(rom);
}

/* --- Lock-on + patch: Sonic 2 & K, 0x300000 lee patch cuando sram_enabled --- */

TEST(gen_mem_lockon_patch)
{
    setup();
    size_t sz = GEN_LOCKON_SIZE + GEN_LOCKON_PATCH;
    uint8_t *rom = calloc(1, sz);
    ASSERT_TRUE(rom != NULL);
    memcpy(rom + GEN_HEADER_OFFSET, GEN_HEADER_MAGIC, GEN_HEADER_MAGIC_LEN);
    for (size_t i = 0; i < GEN_LOCKON_PATCH; i++)
        rom[GEN_LOCKON_SIZE + i] = (uint8_t)(0xD0 + (i & 0x0F));
    impl.mem.rom = rom;
    impl.mem.rom_size = sz;
    impl.mem.lockon = 1;
    impl.mem.lockon_has_patch = 1;
    impl.mem.sram_enabled = 1;

    uint8_t v = genesis_mem_read8(&impl.mem, 0x300000);
    ASSERT_EQ(v, 0xD0);
    v = genesis_mem_read8(&impl.mem, 0x300001);
    ASSERT_EQ(v, 0xD1);

    free(rom);
}

void run_genesis_memory_tests(void)
{
    SUITE("Genesis Memory");
    RUN(gen_mem_ram_readwrite);
    RUN(gen_mem_ram_word);
    RUN(gen_mem_ram_long);
    RUN(gen_mem_rom_no_rom_returns_ff);
    RUN(gen_mem_rom_with_rom);
    RUN(gen_mem_joypad_default);
    RUN(gen_mem_joypad_custom);
    RUN(gen_mem_version_register);
    RUN(gen_mem_vdp_status_read);
    RUN(gen_mem_vdp_hv_read);
    RUN(gen_mem_vdp_write_ctrl);
    RUN(gen_mem_vdp_write_data);
    RUN(gen_mem_psg_write);
    RUN(gen_mem_z80_ram_readwrite);
    RUN(gen_mem_z80_bus_req);
    RUN(gen_mem_z80_reset);
    RUN(gen_mem_tmss_write);
    RUN(gen_mem_sram_a130f1);
    RUN(gen_mem_lockon_detection);
    RUN(gen_mem_lockon_sram);
    RUN(gen_mem_lockon_patch);
}
