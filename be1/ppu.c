/**
 * Bitemu - Game Boy PPU
 * Modos, LY, STAT, renderizado a framebuffer
 */

#include "ppu.h"
#include "memory.h"
#include "gb_constants.h"
#include <string.h>

/* Paleta DMG: índice 0-3 -> gris 0-3 */
static const uint8_t gb_dmg_palette[4] = {0xFF, 0xAA, 0x55, 0x00};

/* STAT interrupt: if enabled (bits 3–6) and condition matches, set IF bit 1 */
static void trigger_stat_if_needed(gb_mem_t *mem)
{
    uint8_t s = mem->io[GB_IO_STAT];
    if ((s & 0x40) && (s & 4))
        mem->io[GB_IO_IF] |= GB_IF_LCDC;
    if ((s & 0x08) && ((s & 3) == 0))
        mem->io[GB_IO_IF] |= GB_IF_LCDC;
    if ((s & 0x10) && ((s & 3) == 1))
        mem->io[GB_IO_IF] |= GB_IF_LCDC;
    if ((s & 0x20) && ((s & 3) == 2))
        mem->io[GB_IO_IF] |= GB_IF_LCDC;
}

static uint8_t get_pixel_from_tile(const uint8_t *vram, uint8_t tile_id,
                                   int x, int y, int signed_addr)
{
    uint16_t base = 0x8000;
    int16_t offset;
    if (signed_addr)
    {
        base = 0x9000; /* signed: tiles -128..127 at 0x9000 */
        offset = (int16_t)(int8_t)tile_id;
    }
    else
    {
        offset = tile_id & 0xFF;
    }
    uint16_t addr = base + offset * 16 + y * 2;
    if (addr < GB_VRAM_START || addr + 1 > GB_VRAM_END)
        return 0;
    uint8_t lo = vram[addr - GB_VRAM_START];
    uint8_t hi = vram[addr - GB_VRAM_START + 1];
    int bit = 7 - x;
    return ((hi >> bit) << 1 | (lo >> bit)) & 3;
}

static void render_scanline(gb_ppu_t *ppu, gb_mem_t *mem)
{
    uint8_t lcdc = mem->io[GB_IO_LCDC];
    if (!(lcdc & 0x80))
        return;

    uint8_t ly = mem->io[GB_IO_LY];
    if (ly >= GB_LCD_HEIGHT)
        return;

    uint8_t scy = mem->io[GB_IO_SCY];
    uint8_t scx = mem->io[GB_IO_SCX];
    uint8_t bgp = mem->io[GB_IO_BGP];
    uint16_t bg_map = (lcdc & 0x08) ? 0x9C00 : 0x9800;
    int signed_tiles = !(lcdc & 0x10);

    uint8_t *fb = ppu->framebuffer + ly * GB_LCD_WIDTH;
    memset(fb, gb_dmg_palette[bgp & 3], GB_LCD_WIDTH);

    if (!(lcdc & 0x01))
        return;

    int tile_y = (ly + scy) & 0xFF;
    int tile_row = tile_y / GB_TILE_SIZE;
    int py = tile_y % GB_TILE_SIZE;

    for (int px = 0; px < GB_LCD_WIDTH; px++)
    {
        int tile_x = (px + scx) & 0xFF;
        int tile_col = tile_x / GB_TILE_SIZE;
        int tx = tile_x % GB_TILE_SIZE;

        uint16_t map_addr = bg_map + tile_row * 32 + tile_col;
        uint8_t tile_id = mem->vram[map_addr - GB_VRAM_START];
        uint8_t color_idx = get_pixel_from_tile(mem->vram, tile_id, tx, py, signed_tiles);
        fb[px] = gb_dmg_palette[(bgp >> (color_idx * 2)) & 3];
    }

    if (!(lcdc & 0x02))
        return;

    int obj_h = (lcdc & 0x04) ? 16 : 8;
    for (int i = GB_OAM_SIZE - 4; i >= 0; i -= 4)
    {
        uint8_t y = mem->oam[i];
        uint8_t x = mem->oam[i + 1];
        uint8_t tile = mem->oam[i + 2];
        uint8_t flags = mem->oam[i + 3];
        if (y == 0 || y >= 160)
            continue;
        int obj_y = y - 16;
        if (ly < obj_y || ly >= obj_y + obj_h)
            continue;
        int obj_x = x - 8;
        if (obj_x <= -8 || obj_x >= GB_LCD_WIDTH)
            continue;

        int line = (flags & 0x40) ? (obj_h - 1 - (ly - obj_y)) : (ly - obj_y);
        uint8_t pal = (flags & 0x10) ? mem->io[GB_IO_OBP1] : mem->io[GB_IO_OBP0];

        for (int j = 0; j < 8; j++)
        {
            int screen_x = obj_x + j;
            if (screen_x < 0 || screen_x >= GB_LCD_WIDTH)
                continue;
            int tile_x = (flags & 0x20) ? (7 - j) : j;
            uint8_t tile_id = (obj_h == 16 && line >= 8) ? tile | 1 : tile;
            int ty = (obj_h == 16 && line >= 8) ? line - 8 : line;
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
}

void gb_ppu_step(gb_ppu_t *ppu, struct gb_mem *mem, int cycles)
{
    gb_mem_t *m = (gb_mem_t *)mem;
    uint8_t lcdc = m->io[GB_IO_LCDC];

    if (!(lcdc & 0x80))
    {
        m->io[GB_IO_LY] = 0;
        m->io[GB_IO_STAT] = (m->io[GB_IO_STAT] & 0xFC) | GB_PPU_MODE_HBLANK;
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
                    ppu->mode = GB_PPU_MODE_TRANSFER;
                    *stat = (*stat & 0xFC) | GB_PPU_MODE_TRANSFER;
                    trigger_stat_if_needed(m);
                }
                else
                {
                    break;
                }
            }
            else if (ppu->mode == GB_PPU_MODE_TRANSFER)
            {
                int mode3 = GB_PPU_MODE3_MIN + (m->io[GB_IO_SCX] % GB_TILE_SIZE);
                if (ppu->dot_counter >= mode3)
                {
                    ppu->dot_counter -= mode3;
                    render_scanline(ppu, m);
                    ppu->mode = GB_PPU_MODE_HBLANK;
                    *stat = (*stat & 0xFC) | GB_PPU_MODE_HBLANK;
                    trigger_stat_if_needed(m);
                }
                else
                {
                    break;
                }
            }
            else
            {
                if (ppu->dot_counter >= GB_DOTS_PER_SCANLINE - GB_PPU_MODE2_DOTS - GB_PPU_MODE3_MIN)
                {
                    ppu->dot_counter -= (GB_DOTS_PER_SCANLINE - GB_PPU_MODE2_DOTS - GB_PPU_MODE3_MIN);
                    ly++;
                    m->io[GB_IO_LY] = ly;
                    *stat = (*stat & 0xFB) | ((ly == lyc) ? 4 : 0);
                    if (ly < GB_SCANLINES_VISIBLE)
                    {
                        ppu->mode = GB_PPU_MODE_OAM;
                        *stat = (*stat & 0xFC) | GB_PPU_MODE_OAM;
                    }
                    trigger_stat_if_needed(m);
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
                *stat = (*stat & 0xFC) | GB_PPU_MODE_VBLANK;
                m->io[GB_IO_IF] |= GB_IF_VBLANK;
                trigger_stat_if_needed(m);
            }
            if (ppu->dot_counter >= GB_DOTS_PER_SCANLINE)
            {
                ppu->dot_counter -= GB_DOTS_PER_SCANLINE;
                ly = (ly + 1) % GB_SCANLINES_TOTAL;
                m->io[GB_IO_LY] = ly;
                *stat = (*stat & 0xFB) | ((ly == lyc) ? 4 : 0);
                if (ly == 0)
                {
                    ppu->mode = GB_PPU_MODE_OAM;
                    *stat = (*stat & 0xFC) | GB_PPU_MODE_OAM;
                }
                trigger_stat_if_needed(m);
            }
            else
            {
                break;
            }
        }
    }
}
