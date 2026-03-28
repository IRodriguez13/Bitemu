/**
 * Bitemu - Game Boy PPU
 *
 * Responsabilidades:
 * - Máquina de estados por scanline: OAM (80 dots) -> Transfer (172+ dots) -> HBLANK -> siguiente LY.
 * - VBLANK: LY 144-153, disparo de interrupción VBLANK y STAT según modo.
 * - Renderizado: fondo (tile map + tile data), sprites (OAM), paletas BGP/OBP0/OBP1.
 * - STAT: comparación LYC=LY, interrupciones por modo (HBL, VBL, OAM, LYC).
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include <stdint.h>

/* Paleta DMG: índice de color 0-3 -> nivel de gris (255, 170, 85, 0) */
static const uint8_t gb_dmg_palette[4] = {0xFF, 0xAA, 0x55, 0x00};

#include "ppu.h"
#include "memory.h"
#include "gb_constants.h"
#include <string.h>

/* STAT IRQ uses a combined OR line; interrupt fires on rising edge only */
static void update_stat_irq(gb_ppu_t *ppu, gb_mem_t *mem)
{
    uint8_t stat = mem->io[GB_IO_STAT];
    uint8_t line = 0;
    if ((stat & GB_STAT_LYC_INT) && (stat & GB_STAT_LYC_EQ))
        line = 1;
    if ((stat & GB_STAT_HBL_INT) && ((stat & GB_STAT_MODE_MASK) == GB_PPU_MODE_HBLANK))
        line = 1;
    if ((stat & GB_STAT_VBL_INT) && ((stat & GB_STAT_MODE_MASK) == GB_PPU_MODE_VBLANK))
        line = 1;
    if ((stat & GB_STAT_OAM_INT) && ((stat & GB_STAT_MODE_MASK) == GB_PPU_MODE_OAM))
        line = 1;

    if (line && !ppu->stat_irq_line)
        mem->io[GB_IO_IF] |= GB_IF_LCDC;
    ppu->stat_irq_line = line;
}

static uint8_t get_pixel_from_tile(const uint8_t *vram, uint8_t tile_id,
                                   int x, int y, int signed_addr)
{
    uint16_t base = signed_addr ? GB_TILE_DATA_SIGNED : GB_TILE_DATA_UNSIGNED;
    int16_t offset;
    if (signed_addr)
        offset = (int16_t)(int8_t)tile_id;
    else
        offset = tile_id & GB_BYTE_LO;
    uint16_t addr = base + offset * 16 + y * 2;
    if (addr < GB_VRAM_START || addr + 1 > GB_VRAM_END)
        return 0;
    uint8_t lo = vram[addr - GB_VRAM_START];
    uint8_t hi = vram[addr - GB_VRAM_START + 1];
    int bit = 7 - x;
    return ((hi >> bit) << 1 | (lo >> bit)) & 3;
}

/* Decodifica una fila de 8 píxeles de un tile (evita 8 llamadas a get_pixel_from_tile) */
static void decode_tile_row(const uint8_t *vram, uint8_t tile_id, int py,
                            int signed_addr, uint8_t out[8])
{
    uint16_t base = signed_addr ? GB_TILE_DATA_SIGNED : GB_TILE_DATA_UNSIGNED;
    int16_t offset = signed_addr ? (int16_t)(int8_t)tile_id : (tile_id & GB_BYTE_LO);
    uint16_t addr = base + offset * 16 + py * 2 - GB_VRAM_START;
    uint8_t lo = vram[addr];
    uint8_t hi = vram[addr + 1];
    for (int i = 0; i < 8; i++)
    {
        int bit = 7 - i;
        out[i] = ((hi >> bit) << 1 | (lo >> bit)) & 3;
    }
}

/* ---------------------------------------------------------------------------
 * Dibujar una scanline: fondo (BG) y sprites (OBJ) según LCDC
 * --------------------------------------------------------------------------- */
static void render_scanline(gb_ppu_t *ppu, gb_mem_t *mem)
{
    uint8_t lcdc = mem->io[GB_IO_LCDC];
    if (!(lcdc & GB_LCDC_ON))
        return;

    uint8_t ly = mem->io[GB_IO_LY];
    if (ly >= GB_LCD_HEIGHT)
        return;

    uint8_t scy = ppu->latched_scy;
    uint8_t scx = ppu->latched_scx;
    uint8_t bgp = mem->io[GB_IO_BGP];
    uint16_t bg_map = (lcdc & GB_LCDC_BG_TILEMAP) ? GB_BG_MAP_HI : GB_BG_MAP_LO;
    int signed_tiles = !(lcdc & GB_LCDC_BG_TILES);

    uint8_t *fb = ppu->framebuffer + ly * GB_LCD_WIDTH;
    uint8_t bg_indices[GB_LCD_WIDTH];
    memset(fb, gb_dmg_palette[bgp & 3], GB_LCD_WIDTH);
    memset(bg_indices, 0, GB_LCD_WIDTH);

    if (!(lcdc & GB_LCDC_BG_ON))
        goto sprites;

    int tile_y = (ly + scy) & GB_BYTE_LO;
    int tile_row = tile_y / GB_TILE_SIZE;
    int py = tile_y % GB_TILE_SIZE;
    int tile_start = scx / GB_TILE_SIZE;
    int tx_start = scx % GB_TILE_SIZE;
    int px_out = 0;

    /* Render por tiles: 1 decode por tile vs 8 por tile (menos accesos VRAM) */
    for (int tc = tile_start; px_out < GB_LCD_WIDTH; tc++)
    {
        int tile_col = tc & 31;
        uint16_t map_addr = bg_map + tile_row * 32 + tile_col;
        uint8_t tile_id = mem->vram[map_addr - GB_VRAM_START];
        uint8_t row[8];
        decode_tile_row(mem->vram, tile_id, py, signed_tiles, row);
        for (int tx = tx_start; tx < GB_TILE_SIZE && px_out < GB_LCD_WIDTH; tx++, px_out++)
        {
            uint8_t color_idx = row[tx];
            bg_indices[px_out] = color_idx;
            fb[px_out] = gb_dmg_palette[(bgp >> (color_idx * 2)) & 3];
        }
        tx_start = 0;
    }

    if ((lcdc & GB_LCDC_WIN_ON) && (lcdc & GB_LCDC_ON))
    {
        uint8_t wy = mem->io[GB_IO_WY];
        uint8_t wx = mem->io[GB_IO_WX];
        if (ly >= wy && wx <= 166)
        {
            int window_left = (int)wx - 7;
            if (window_left < GB_LCD_WIDTH)
            {
                int win_y = ppu->window_line;
                uint16_t win_map = (lcdc & GB_LCDC_WIN_TILEMAP) ? GB_BG_MAP_HI : GB_BG_MAP_LO;
                int win_tile_row = win_y / GB_TILE_SIZE;
                int win_py = win_y % GB_TILE_SIZE;
                int win_tx_start = (window_left < 0) ? -window_left : 0;
                int px_out = (window_left > 0) ? window_left : 0;
                for (int tc = 0; px_out < GB_LCD_WIDTH; tc++)
                {
                    int tile_col = tc & 31;
                    uint16_t map_addr = win_map + win_tile_row * 32 + tile_col;
                    uint8_t tile_id = mem->vram[map_addr - GB_VRAM_START];
                    uint8_t row[8];
                    decode_tile_row(mem->vram, tile_id, win_py, signed_tiles, row);
                    for (int tx = win_tx_start; tx < GB_TILE_SIZE && px_out < GB_LCD_WIDTH; tx++, px_out++)
                    {
                        uint8_t color_idx = row[tx];
                        bg_indices[px_out] = color_idx;
                        fb[px_out] = gb_dmg_palette[(bgp >> (color_idx * 2)) & 3];
                    }
                    win_tx_start = 0;  /* siguientes tiles: fila completa */
                }
                ppu->window_line++;
            }
        }
    }

sprites:
    if (!(lcdc & GB_LCDC_OBJ_ON))
        return;

    int obj_h = (lcdc & GB_LCDC_OBJ_H) ? 16 : 8;
    int sprite_count = 0;
    int sprite_oam_indices[GB_OAM_SPRITES_PER_LINE];
    for (int i = 0; i < GB_OAM_SIZE && sprite_count < GB_OAM_SPRITES_PER_LINE; i += 4)
    {
        uint8_t y = mem->oam[i];
        if (y == 0 || y >= 160)
            continue;
        int obj_y = y - 16;
        if (ly < obj_y || ly >= obj_y + obj_h)
            continue;
        sprite_oam_indices[sprite_count++] = i;
    }

    /* DMG: sort by X-coordinate (stable: equal X keeps OAM order) */
    for (int a = 1; a < sprite_count; a++)
    {
        int key = sprite_oam_indices[a];
        uint8_t key_x = mem->oam[key + 1];
        int b = a - 1;
        while (b >= 0 && mem->oam[sprite_oam_indices[b] + 1] > key_x)
        {
            sprite_oam_indices[b + 1] = sprite_oam_indices[b];
            b--;
        }
        sprite_oam_indices[b + 1] = key;
    }

    for (int s = sprite_count - 1; s >= 0; s--)
    {
        int i = sprite_oam_indices[s];
        uint8_t y = mem->oam[i];
        uint8_t x = mem->oam[i + 1];
        uint8_t tile = mem->oam[i + 2];
        uint8_t flags = mem->oam[i + 3];
        int obj_y = y - 16;
        int obj_x = x - 8;
        if (obj_x <= -8 || obj_x >= GB_LCD_WIDTH)
            continue;

        int bg_priority = flags & 0x80;
        int line = (flags & 0x40) ? (obj_h - 1 - (ly - obj_y)) : (ly - obj_y);
        uint8_t pal = (flags & 0x10) ? mem->io[GB_IO_OBP1] : mem->io[GB_IO_OBP0];

        for (int j = 0; j < 8; j++)
        {
            int screen_x = obj_x + j;
            if (screen_x < 0 || screen_x >= GB_LCD_WIDTH)
                continue;
            if (bg_priority && bg_indices[screen_x] != 0)
                continue;
            int tile_x = (flags & 0x20) ? (7 - j) : j;
            uint8_t tile_id;
            int ty;
            if (obj_h == 16)
            {
                tile_id = (line >= 8) ? (tile | 0x01) : (tile & 0xFE);
                ty = (line >= 8) ? line - 8 : line;
            }
            else
            {
                tile_id = tile;
                ty = line;
            }
            uint8_t color_idx = get_pixel_from_tile(mem->vram, tile_id, tile_x, ty, 0);
            if (color_idx != 0)
                fb[screen_x] = gb_dmg_palette[(pal >> (color_idx * 2)) & 3];
        }
    }
}

void gb_ppu_init(gb_ppu_t *ppu)
{
    memset(ppu, 0, sizeof(gb_ppu_t));
    ppu->mode = GB_PPU_MODE_OAM;
    ppu->mode3_dots = GB_PPU_MODE3_MIN;
}

/* ---------------------------------------------------------------------------
 * Avance de la PPU por ciclos: actualiza modo, LY, STAT e interrupciones
 * --------------------------------------------------------------------------- */
void gb_ppu_step(gb_ppu_t *ppu, struct gb_mem *mem, int cycles)
{
    gb_mem_t *m = (gb_mem_t *)mem;
    uint8_t lcdc = m->io[GB_IO_LCDC];

    if (!(lcdc & GB_LCDC_ON))
    {
        m->io[GB_IO_LY] = 0;
        m->io[GB_IO_STAT] = (m->io[GB_IO_STAT] & ~GB_STAT_MODE_MASK) | GB_PPU_MODE_HBLANK;
        return;
    }

    uint8_t ly = m->io[GB_IO_LY];
    uint8_t lyc = m->io[GB_IO_LYC];
    uint8_t *stat = &m->io[GB_IO_STAT];

    ppu->dot_counter += cycles;

    while (ppu->dot_counter > 0)
    {
        if (ly < GB_SCANLINES_VISIBLE)
        {
            if (ppu->mode == GB_PPU_MODE_OAM)
            {
                if (ppu->dot_counter >= GB_PPU_MODE2_DOTS)
                {
                    ppu->dot_counter -= GB_PPU_MODE2_DOTS;
                    ppu->latched_scy = m->io[GB_IO_SCY];
                    ppu->latched_scx = m->io[GB_IO_SCX];
                    ppu->mode = GB_PPU_MODE_TRANSFER;
                    *stat = (*stat & ~GB_STAT_MODE_MASK) | GB_PPU_MODE_TRANSFER;
                    update_stat_irq(ppu, m);
                }
                else
                {
                    break;
                }
            }
            else if (ppu->mode == GB_PPU_MODE_TRANSFER)
            {
                ppu->mode3_dots = GB_PPU_MODE3_MIN + (ppu->latched_scx % GB_TILE_SIZE);
                if (ppu->dot_counter >= ppu->mode3_dots)
                {
                    ppu->dot_counter -= ppu->mode3_dots;
                    render_scanline(ppu, m);
                    ppu->mode = GB_PPU_MODE_HBLANK;
                    *stat = (*stat & ~GB_STAT_MODE_MASK) | GB_PPU_MODE_HBLANK;
                    update_stat_irq(ppu, m);
                }
                else
                {
                    break;
                }
            }
            else
            {
                int hblank_dots = GB_DOTS_PER_SCANLINE - GB_PPU_MODE2_DOTS - ppu->mode3_dots;
                if (ppu->dot_counter >= hblank_dots)
                {
                    ppu->dot_counter -= hblank_dots;
                    ly++;
                    m->io[GB_IO_LY] = ly;
                    *stat = (*stat & ~GB_STAT_LYC_EQ) | ((ly == lyc) ? GB_STAT_LYC_EQ : 0);
                    if (ly < GB_SCANLINES_VISIBLE)
                    {
                        ppu->mode = GB_PPU_MODE_OAM;
                        *stat = (*stat & ~GB_STAT_MODE_MASK) | GB_PPU_MODE_OAM;
                    }
                    update_stat_irq(ppu, m);
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            if (ly == GB_SCANLINES_VISIBLE && ppu->mode != GB_PPU_MODE_VBLANK)
            {
                ppu->mode = GB_PPU_MODE_VBLANK;
                *stat = (*stat & ~GB_STAT_MODE_MASK) | GB_PPU_MODE_VBLANK;
                m->io[GB_IO_IF] |= GB_IF_VBLANK;
                update_stat_irq(ppu, m);
            }
            if (ppu->dot_counter >= GB_DOTS_PER_SCANLINE)
            {
                ppu->dot_counter -= GB_DOTS_PER_SCANLINE;
                ly = (ly + 1) % GB_SCANLINES_TOTAL;
                m->io[GB_IO_LY] = ly;
                *stat = (*stat & ~GB_STAT_LYC_EQ) | ((ly == lyc) ? GB_STAT_LYC_EQ : 0);
                if (ly == 0)
                {
                    ppu->mode = GB_PPU_MODE_OAM;
                    *stat = (*stat & ~GB_STAT_MODE_MASK) | GB_PPU_MODE_OAM;
                    ppu->window_line = 0;
                }
                update_stat_irq(ppu, m);
            }
            else
            {
                break;
            }
        }
    }
}
