/**
 * Tests: Game Boy PPU subsystem.
 * Sprite BG priority (OAM flag bit 7).
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "test_harness.h"
#include "be1/ppu.h"
#include "be1/memory.h"
#include "be1/gb_constants.h"

static gb_ppu_t ppu;
static gb_mem_t mem;

static void setup(void)
{
    memset(&mem, 0, sizeof(mem));
    gb_mem_init(&mem);
    gb_ppu_init(&ppu);
    mem.io[GB_IO_LCDC] = GB_LCDC_ON | GB_LCDC_BG_ON | GB_LCDC_OBJ_ON | GB_LCDC_BG_TILES;
    mem.io[GB_IO_BGP] = 0xE4;   /* 11 10 01 00 → colors 3,2,1,0 */
    mem.io[GB_IO_OBP0] = 0xE4;
    mem.io[GB_IO_LY] = 0;
    mem.io[GB_IO_SCX] = 0;
    mem.io[GB_IO_SCY] = 0;
}

static void place_sprite(gb_mem_t *m, int slot, uint8_t y, uint8_t x,
                          uint8_t tile, uint8_t flags)
{
    int i = slot * 4;
    m->oam[i]     = y;
    m->oam[i + 1] = x;
    m->oam[i + 2] = tile;
    m->oam[i + 3] = flags;
}

static void write_tile_solid(gb_mem_t *m, uint8_t tile_id, uint8_t color_idx)
{
    uint16_t base = 0x8000 + tile_id * 16;
    uint8_t lo = 0, hi = 0;
    if (color_idx & 1) lo = 0xFF;
    if (color_idx & 2) hi = 0xFF;
    for (int row = 0; row < 8; row++)
    {
        m->vram[base - GB_VRAM_START + row * 2]     = lo;
        m->vram[base - GB_VRAM_START + row * 2 + 1] = hi;
    }
}

static void render_line_0(void)
{
    mem.io[GB_IO_LY] = 0;
    ppu.mode = GB_PPU_MODE_OAM;
    ppu.dot_counter = 0;
    gb_ppu_step(&ppu, (struct gb_mem *)&mem, GB_DOTS_PER_SCANLINE);
}

/* Sprite without BG priority renders over non-zero BG */
TEST(sprite_no_priority_over_bg)
{
    setup();
    write_tile_solid(&mem, 0, 1);
    mem.vram[GB_BG_MAP_LO - GB_VRAM_START] = 0;

    write_tile_solid(&mem, 1, 2);
    place_sprite(&mem, 0, 16, 8, 1, 0x00);

    render_line_0();

    /* BGP=0xE4: idx1→palette[1]=0xAA, sprite color idx2→palette[2]=0x55 */
    /* Sprite has no BG priority, so it draws over BG */
    uint8_t sprite_color = 0x55;  /* palette index 2 */
    ASSERT_EQ(ppu.framebuffer[0], sprite_color);
}

/* Sprite with BG priority (flag bit 7) hides behind non-zero BG */
TEST(sprite_bg_priority_hidden)
{
    setup();
    write_tile_solid(&mem, 0, 1);
    mem.vram[GB_BG_MAP_LO - GB_VRAM_START] = 0;

    write_tile_solid(&mem, 1, 2);
    place_sprite(&mem, 0, 16, 8, 1, 0x80);

    render_line_0();

    /* BG color index is 1 (non-zero), sprite has BG priority → BG wins */
    uint8_t bg_color_1 = 0xAA;  /* palette index 1 */
    ASSERT_EQ(ppu.framebuffer[0], bg_color_1);
}

/* Sprite with BG priority shows over BG color 0 */
TEST(sprite_bg_priority_over_color0)
{
    setup();
    write_tile_solid(&mem, 0, 0);
    mem.vram[GB_BG_MAP_LO - GB_VRAM_START] = 0;

    write_tile_solid(&mem, 1, 2);
    place_sprite(&mem, 0, 16, 8, 1, 0x80);

    render_line_0();

    /* BG color index is 0, so sprite with BG priority still shows */
    uint8_t sprite_color = 0x55;  /* palette index 2 */
    ASSERT_EQ(ppu.framebuffer[0], sprite_color);
}

void run_ppu_tests(void)
{
    SUITE("PPU");
    RUN(sprite_no_priority_over_bg);
    RUN(sprite_bg_priority_hidden);
    RUN(sprite_bg_priority_over_color0);
}
