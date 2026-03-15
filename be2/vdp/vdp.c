/**
 * BE2 - Sega Genesis: implementación VDP
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "vdp.h"
#include <string.h>

void gen_vdp_init(gen_vdp_t *vdp)
{
    memset(vdp, 0, sizeof(*vdp));
}

void gen_vdp_set_dma_read(gen_vdp_t *vdp, gen_vdp_dma_read_fn fn, void *ctx)
{
    vdp->dma_read_16 = fn;
    vdp->dma_read_ctx = ctx;
}

void gen_vdp_reset(gen_vdp_t *vdp)
{
    memset(vdp->regs, 0, GEN_VDP_REG_COUNT);
    memset(vdp->vram, 0, sizeof(vdp->vram));
    memset(vdp->cram, 0, sizeof(vdp->cram));
    memset(vdp->vsram, 0, sizeof(vdp->vsram));
    vdp->addr_reg = 0;
    vdp->code_reg = 0;
    vdp->pending_hi = 0;
    vdp->addr_inc = GEN_VDP_ADDR_INC_DEF;
    vdp->dma_fill_pending = 0;
    vdp->cycle_counter = 0;
    vdp->line_counter = 0;
    vdp->status_reg = 0;
    vdp->status_cache = 0;
    gen_vdp_render_test_pattern(vdp);
}

static void set_access_mode(gen_vdp_t *vdp, uint8_t code, uint16_t addr)
{
    vdp->code_reg = code;
    vdp->addr_reg = addr & GEN_VDP_ADDR_MASK;
}

void gen_vdp_write_ctrl(gen_vdp_t *vdp, uint16_t val)
{
    if (vdp->pending_hi)
    {
        /* Segunda palabra: comando 32-bit completo */
        uint32_t cmd = ((uint32_t)vdp->pending_hi << 16) | val;
        vdp->pending_hi = 0;

        if ((cmd & GEN_VDP_CMD_TYPE_MASK) == GEN_VDP_CMD_REG_WRITE)
        {
            int reg = (cmd >> GEN_VDP_CMD_REG_SHIFT) & GEN_VDP_CMD_REG_NUM;
            uint8_t data = (uint8_t)(cmd & GEN_VDP_CMD_DATA_MASK);
            if (reg < GEN_VDP_REG_COUNT)
                vdp->regs[reg] = data;
            if (reg == GEN_VDP_REG_ADDR_INC)
                vdp->addr_inc = data ? data : 1;
            return;
        }

        /* Comando de acceso a memoria */
        uint16_t addr = (uint16_t)((cmd >> 16) & GEN_VDP_CMD_ADDR_HI)
                     | (uint16_t)(((cmd >> 2) & GEN_VDP_CMD_ADDR_LO) << 14);
        uint8_t code = (uint8_t)(((cmd >> 30) & 3) | ((cmd >> 2) & 0x3C));
        int dma_type = (int)(vdp->regs[GEN_VDP_REG_DMA_SRC_EXT] >> 4);

        /* DMA: CD5 set, DMA enabled (reg 1 bit 4), code = VRAM/CRAM/VSRAM write */
        if ((code & (1 << GEN_VDP_CMD_CD5_BIT)) &&
            (vdp->regs[1] & (1 << GEN_VDP_DMA_ENABLE_BIT)) &&
            ((code & 0x0F) == GEN_VDP_CODE_VRAM_WRITE ||
             (code & 0x0F) == GEN_VDP_CODE_CRAM_WRITE ||
             (code & GEN_VDP_CODE_VSRAM_MASK) == GEN_VDP_CODE_VSRAM_BIT))
        {
            uint8_t dest_code = (uint8_t)((code & 0x0F) == GEN_VDP_CODE_CRAM_WRITE ? GEN_VDP_CODE_CRAM_WRITE :
                                          (code & GEN_VDP_CODE_VSRAM_MASK) == GEN_VDP_CODE_VSRAM_BIT ? GEN_VDP_CODE_VSRAM_WRITE :
                                          GEN_VDP_CODE_VRAM_WRITE);
            set_access_mode(vdp, dest_code, addr);
            uint32_t len = (uint32_t)((vdp->regs[GEN_VDP_REG_DMA_LEN_HI] << 8) | vdp->regs[GEN_VDP_REG_DMA_LEN_LO]);
            if (len == 0)
                len = 0x10000;

            if (dma_type >= 0x8 && dma_type <= 0xB)
            {
                vdp->dma_fill_pending = 1;
                vdp->status_reg |= GEN_VDP_STATUS_DMA;
            }
            else if (dma_type >= 0xC && dma_type <= 0xF)
            {
                /* VRAM copy: length en bytes, copiamos words */
                uint16_t src = (uint16_t)((vdp->regs[GEN_VDP_REG_DMA_SRC_HI] << 8) | vdp->regs[GEN_VDP_REG_DMA_SRC_LO]);
                for (uint32_t i = 0; i < len; i += 2)
                {
                    uint16_t w = vdp->vram[src & (GEN_VDP_VRAM_WORDS - 1)];
                    if (addr < GEN_VDP_VRAM_WORDS)
                        vdp->vram[addr] = w;
                    addr = (addr + (vdp->addr_inc & GEN_VDP_ADDR_INC_MASK)) & GEN_VDP_ADDR_MASK;
                    src++;
                }
                vdp->regs[GEN_VDP_REG_DMA_SRC_LO] = (uint8_t)(src & 0xFF);
                vdp->regs[GEN_VDP_REG_DMA_SRC_HI] = (uint8_t)(src >> 8);
                vdp->addr_reg = addr;
                vdp->regs[GEN_VDP_REG_DMA_SRC_LO] = (uint8_t)(src & 0xFF);
                vdp->regs[GEN_VDP_REG_DMA_SRC_HI] = (uint8_t)(src >> 8);
            }
            else if (vdp->dma_read_16 && vdp->dma_read_ctx)
            {
                /* 68k to VRAM/CRAM/VSRAM */
                uint32_t src = ((uint32_t)(vdp->regs[GEN_VDP_REG_DMA_SRC_EXT] & 0x3F) << 17)
                             | ((uint32_t)((vdp->regs[GEN_VDP_REG_DMA_SRC_HI] << 8) | vdp->regs[GEN_VDP_REG_DMA_SRC_LO]) << 1);
                uint16_t dest = addr;
                int is_cram = dest_code == GEN_VDP_CODE_CRAM_WRITE;
                int is_vsram = dest_code == GEN_VDP_CODE_VSRAM_WRITE;

                for (uint32_t i = 0; i < len; i += 2)
                {
                    uint16_t w = vdp->dma_read_16(vdp->dma_read_ctx, src);
                    if (is_cram)
                    {
                        int idx = (dest >> 1) & GEN_VDP_CRAM_MASK;
                        vdp->cram[idx] = w & GEN_VDP_CRAM_COLOR;
                    }
                    else if (is_vsram && (dest >> 1) < GEN_VDP_VSRAM_SIZE)
                        vdp->vsram[dest >> 1] = w & GEN_VDP_VSRAM_MASK;
                    else if (dest < GEN_VDP_VRAM_WORDS)
                        vdp->vram[dest] = w;
                    dest = (dest + (vdp->addr_inc & GEN_VDP_ADDR_INC_MASK)) & GEN_VDP_ADDR_MASK;
                    src += 2;
                    /* 128K DMA window: wrap lower 17 bits */
                    src = ((uint32_t)(vdp->regs[GEN_VDP_REG_DMA_SRC_EXT] & 0x3F) << 17) | (src & 0x1FFFF);
                }
                vdp->addr_reg = dest;
                vdp->regs[GEN_VDP_REG_DMA_SRC_LO] = (uint8_t)((src >> 1) & 0xFF);
                vdp->regs[GEN_VDP_REG_DMA_SRC_HI] = (uint8_t)((src >> 9) & 0xFF);
                vdp->status_reg &= ~GEN_VDP_STATUS_DMA;
            }
            return;
        }

        if ((code & 0x0F) == GEN_VDP_CODE_VRAM_WRITE)
            set_access_mode(vdp, GEN_VDP_CODE_VRAM_WRITE, addr);
        else if ((code & 0x0F) == GEN_VDP_CODE_CRAM_WRITE)
            set_access_mode(vdp, GEN_VDP_CODE_CRAM_WRITE, addr);
        else if ((code & 0x0F) == GEN_VDP_CODE_VSRAM_WRITE)
            set_access_mode(vdp, GEN_VDP_CODE_VSRAM_WRITE, addr);
        else
            set_access_mode(vdp, GEN_VDP_CODE_VRAM_READ, addr);
        return;
    }

    if ((val & GEN_VDP_REG16_MASK) == GEN_VDP_REG16_VAL)
    {
        int reg = (val >> GEN_VDP_REG16_REG) & GEN_VDP_CMD_REG_NUM;
        uint8_t data = (uint8_t)(val & GEN_VDP_CMD_DATA_MASK);
        if (reg < GEN_VDP_REG_COUNT)
            vdp->regs[reg] = data;
        if (reg == GEN_VDP_REG_ADDR_INC)
            vdp->addr_inc = data ? data : 1;
        return;
    }

    /* Primera palabra de comando 32-bit */
    vdp->pending_hi = val;
}

void gen_vdp_write_data(gen_vdp_t *vdp, uint16_t val)
{
    uint16_t addr = vdp->addr_reg;

    /* DMA Fill: valor escrito es el patrón de relleno */
    if (vdp->dma_fill_pending)
    {
        vdp->dma_fill_pending = 0;
        vdp->status_reg &= ~GEN_VDP_STATUS_DMA;
        uint32_t len = (uint32_t)((vdp->regs[GEN_VDP_REG_DMA_LEN_HI] << 8) | vdp->regs[GEN_VDP_REG_DMA_LEN_LO]);
        if (len == 0)
            len = 0x10000;
        uint8_t fill_byte = (uint8_t)(val >> 8);
        uint16_t fill_word = (uint16_t)((fill_byte << 8) | fill_byte);
        for (uint32_t i = 0; i < len; i += 2)
        {
            if (vdp->code_reg == GEN_VDP_CODE_VRAM_WRITE && addr < GEN_VDP_VRAM_WORDS)
                vdp->vram[addr] = fill_word;
            addr = (addr + (vdp->addr_inc & GEN_VDP_ADDR_INC_MASK)) & GEN_VDP_ADDR_MASK;
        }
        vdp->addr_reg = addr;
        return;
    }

    switch (vdp->code_reg)
    {
    case GEN_VDP_CODE_VRAM_WRITE:
        if (addr < GEN_VDP_VRAM_WORDS)
            vdp->vram[addr] = val;
        vdp->addr_reg = (addr + (vdp->addr_inc & GEN_VDP_ADDR_INC_MASK)) & GEN_VDP_ADDR_MASK;
        break;
    case GEN_VDP_CODE_CRAM_WRITE:
    {
        int idx = (addr >> 1) & GEN_VDP_CRAM_MASK;
        vdp->cram[idx] = val & GEN_VDP_CRAM_COLOR;
        vdp->addr_reg = (addr + (vdp->addr_inc & GEN_VDP_ADDR_INC_MASK)) & GEN_VDP_ADDR_MASK;
        break;
    }
    case GEN_VDP_CODE_VSRAM_WRITE:
        if (addr < GEN_VDP_VSRAM_SIZE * 2)
            vdp->vsram[addr >> 1] = val & GEN_VDP_VSRAM_MASK;
        vdp->addr_reg = (addr + (vdp->addr_inc & GEN_VDP_ADDR_INC_MASK)) & GEN_VDP_ADDR_MASK;
        break;
    default:
        break;
    }
}

void gen_vdp_render_test_pattern(gen_vdp_t *vdp)
{
    for (int y = 0; y < GEN_DISPLAY_HEIGHT; y++)
    {
        for (int x = 0; x < GEN_DISPLAY_WIDTH; x++)
        {
            int idx = (x / GEN_TEST_BAR_WIDTH) % GEN_TEST_PALETTE_MAX;
            uint8_t c = (uint8_t)(idx * 255 / (GEN_TEST_PALETTE_MAX - 1));
            vdp->framebuffer[y * GEN_DISPLAY_WIDTH + x] = c;
        }
    }
}

/* Convierte color Genesis (0x0BBBGGGRRR) a grayscale 0-255 para display */
static uint8_t cram_to_grayscale(uint16_t c)
{
    int r = (c >> 1) & GEN_VDP_CRAM_R_MAX;
    int g = (c >> 5) & GEN_VDP_CRAM_R_MAX;
    int b = (c >> 9) & GEN_VDP_CRAM_R_MAX;
    int gray = (r + g + b) * 255 / GEN_VDP_CRAM_GRAY_MAX;
    return (uint8_t)(gray > 255 ? 255 : gray);
}

/* Dibuja un sprite. priority_filter: 0=low, 1=high. display_w: 256 o 320. */
static void render_sprite(gen_vdp_t *vdp, int spr_idx, uint32_t sat_base, int priority_filter, int display_w)
{
    uint32_t off = sat_base + (uint32_t)(spr_idx * GEN_VDP_SAT_ENTRY_WORDS);
    if (off + 4 > GEN_VDP_VRAM_WORDS)
        return;

    int y_pos = (int)(vdp->vram[off + 0] & 0x3FF);
    uint16_t w1 = vdp->vram[off + 1];
    uint16_t w2 = vdp->vram[off + 2];
    int x_pos = (int)(vdp->vram[off + 3] & 0x3FF);

    int size_bits = (w1 >> GEN_VDP_SPR_SIZE_SHIFT) & GEN_VDP_SPR_SIZE_MASK;
    int width, height;
    switch (size_bits)
    {
    case GEN_VDP_SPR_SIZE_8x8:   width = 8;  height = 8;  break;
    case GEN_VDP_SPR_SIZE_8x16:  width = 8;  height = 16; break;
    case GEN_VDP_SPR_SIZE_16x16: width = 16; height = 16; break;
    case GEN_VDP_SPR_SIZE_8x32:  width = 8;  height = 32; break;
    default:                     width = 8;  height = 8;  break;
    }

    int spr_prio = (w2 >> GEN_VDP_SPR_PRIO_BIT) & 1;
    if (spr_prio != priority_filter)
        return;
    int pal = (w2 >> GEN_VDP_SPR_PAL_SHIFT) & GEN_VDP_SPR_PAL_MASK;
    int flip_x = (w2 >> GEN_VDP_SPR_FLIP_H) & 1;
    int flip_y = (w2 >> GEN_VDP_SPR_FLIP_V) & 1;
    int tile_idx = w2 & GEN_VDP_SPR_TILE_MASK;

    /* Patrón base: reg 6 */
    uint32_t pat_base = (uint32_t)((vdp->regs[6] & GEN_VDP_SPR_PAT_BASE_MASK) << GEN_VDP_SPR_PAT_BASE_SHIFT);
    uint32_t tile_addr = pat_base + (uint32_t)(tile_idx * GEN_VDP_TILE_BYTES);

    /* Y/X en Genesis: 0 = borde. Clamp a visible */
    if (y_pos >= 224 + 32)
        return;
    if (x_pos >= display_w + 32)
        return;

    int tiles_x = (width + 7) / GEN_VDP_TILE_SIZE;
    int tiles_y = (height + 7) / GEN_VDP_TILE_SIZE;

    for (int ty = 0; ty < tiles_y; ty++)
    {
        for (int tx = 0; tx < tiles_x; tx++)
        {
            int cur_tile = ty * tiles_x + tx;
            uint32_t cur_addr = tile_addr + (uint32_t)(cur_tile * GEN_VDP_TILE_BYTES);
            if (cur_addr + GEN_VDP_TILE_BYTES > GEN_VDP_VRAM_SIZE)
                continue;

            for (int py = 0; py < GEN_VDP_TILE_SIZE; py++)
            {
                int sy = flip_y ? (GEN_VDP_TILE_SIZE - 1 - py) : py;
                int dy = y_pos + ty * GEN_VDP_TILE_SIZE + py;
                if (dy < 0 || dy >= GEN_DISPLAY_HEIGHT)
                    continue;

                uint16_t w0 = vdp->vram[(cur_addr + sy * GEN_VDP_TILE_ROW_BYTES) / 2];
                uint16_t w1_t = vdp->vram[(cur_addr + sy * GEN_VDP_TILE_ROW_BYTES + 2) / 2];

                for (int px = 0; px < GEN_VDP_TILE_SIZE; px++)
                {
                    int sx = flip_x ? (GEN_VDP_TILE_SIZE - 1 - px) : px;
                    int dx = x_pos + tx * GEN_VDP_TILE_SIZE + px;
                    if (dx < 0 || dx >= display_w)
                        continue;

                    int shift = (GEN_VDP_TILE_SIZE - 1) - sx;
                    int b0 = (w0 >> shift) & 1;
                    int b1 = (w0 >> (GEN_VDP_TILE_HI_BYTE + shift)) & 1;
                    int b2 = (w1_t >> shift) & 1;
                    int b3 = (w1_t >> (GEN_VDP_TILE_HI_BYTE + shift)) & 1;
                    int pix = b0 | (b1 << 1) | (b2 << 2) | (b3 << 3);
                    if (pix == 0)
                        continue;

                    int color_idx = pal * GEN_VDP_PALETTE_SIZE + pix;
                    if (color_idx < GEN_VDP_CRAM_SIZE)
                        vdp->framebuffer[dy * GEN_DISPLAY_WIDTH + dx] =
                            cram_to_grayscale(vdp->cram[color_idx]);
                }
            }
        }
    }
}

/* Dibuja el window plane (sin scroll). Región: (0,0) a (win_w, win_h). */
static void render_window(gen_vdp_t *vdp, uint32_t window_addr, int priority_filter,
                          int win_w, int win_h, int map_width_tiles)
{
    if (win_w <= 0 || win_h <= 0)
        return;
    for (int y = 0; y < win_h && y < GEN_DISPLAY_HEIGHT; y++)
    {
        int tile_row = y / GEN_VDP_TILE_SIZE;
        int py = y % GEN_VDP_TILE_SIZE;
        for (int x = 0; x < win_w && x < GEN_DISPLAY_WIDTH; x++)
        {
            int tile_col = x / GEN_VDP_TILE_SIZE;
            if (tile_col >= map_width_tiles)
                continue;
            int px = x % GEN_VDP_TILE_SIZE;

            uint32_t map_idx = window_addr / 2 + (uint32_t)(tile_row * GEN_VDP_NAMETABLE_W + tile_col);
            if (map_idx >= GEN_VDP_VRAM_WORDS)
                continue;
            uint16_t tile_word = vdp->vram[map_idx];
            if (((tile_word >> GEN_VDP_TILE_PRIO_BIT) & 1) != priority_filter)
                continue;
            int tile_idx = tile_word & GEN_VDP_TILE_IDX_MASK;
            int pal = (tile_word >> GEN_VDP_TILE_PAL_SHIFT) & GEN_VDP_TILE_PAL_MASK;
            int flip_x = (tile_word >> GEN_VDP_TILE_FLIP_X) & 1;
            int flip_y = (tile_word >> GEN_VDP_TILE_FLIP_Y) & 1;

            int sy = flip_y ? (GEN_VDP_TILE_SIZE - 1 - py) : py;
            int sx = flip_x ? (GEN_VDP_TILE_SIZE - 1 - px) : px;

            uint32_t tile_addr = (uint32_t)(tile_idx * GEN_VDP_TILE_BYTES);
            if (tile_addr + GEN_VDP_TILE_BYTES > GEN_VDP_VRAM_SIZE)
                continue;

            uint16_t w0 = vdp->vram[(tile_addr + sy * GEN_VDP_TILE_ROW_BYTES) / 2];
            uint16_t w1 = vdp->vram[(tile_addr + sy * GEN_VDP_TILE_ROW_BYTES + 2) / 2];
            int shift = (GEN_VDP_TILE_SIZE - 1) - sx;
            int b0 = (w0 >> shift) & 1;
            int b1 = (w0 >> (GEN_VDP_TILE_HI_BYTE + shift)) & 1;
            int b2 = (w1 >> shift) & 1;
            int b3 = (w1 >> (GEN_VDP_TILE_HI_BYTE + shift)) & 1;
            int pix = b0 | (b1 << 1) | (b2 << 2) | (b3 << 3);
            if (pix == 0)
                continue;
            int color_idx = pal * GEN_VDP_PALETTE_SIZE + pix;
            if (color_idx < GEN_VDP_CRAM_SIZE)
                vdp->framebuffer[y * GEN_DISPLAY_WIDTH + x] =
                    cram_to_grayscale(vdp->cram[color_idx]);
        }
    }
}

/* Dibuja un plano con scroll. priority_filter: 0=low, 1=high. width_tiles: 32 o 40 */
static void render_plane(gen_vdp_t *vdp, uint32_t plane_addr, int vscroll, uint32_t hscroll_base, int priority_filter, int width_tiles)
{
    for (int y = 0; y < GEN_DISPLAY_HEIGHT; y++)
    {
        uint32_t row_offset = hscroll_base + (uint32_t)(y * 2);
        int hscroll = (row_offset < GEN_VDP_VRAM_WORDS) ?
            (int)(vdp->vram[row_offset] & 0x3FF) : 0;

        int display_w = (width_tiles == 32) ? GEN_DISPLAY_WIDTH_H32 : GEN_DISPLAY_WIDTH;
        for (int x = 0; x < display_w; x++)
        {
            int src_y = (y + vscroll) & GEN_VDP_VSRAM_MASK;
            int src_x = (x + hscroll) & 0x3FF;
            int tile_row = (src_y / GEN_VDP_TILE_SIZE) % GEN_VDP_TILES_H;
            int tile_col = (src_x / GEN_VDP_TILE_SIZE) % width_tiles;
            int py = src_y % GEN_VDP_TILE_SIZE;
            int px = src_x % GEN_VDP_TILE_SIZE;

            uint32_t map_idx = plane_addr / 2 + (uint32_t)(tile_row * GEN_VDP_NAMETABLE_W + tile_col);
            if (map_idx >= GEN_VDP_VRAM_WORDS)
                continue;
            uint16_t tile_word = vdp->vram[map_idx];
            if (((tile_word >> GEN_VDP_TILE_PRIO_BIT) & 1) != priority_filter)
                continue;
            int tile_idx = tile_word & GEN_VDP_TILE_IDX_MASK;
            int pal = (tile_word >> GEN_VDP_TILE_PAL_SHIFT) & GEN_VDP_TILE_PAL_MASK;
            int flip_x = (tile_word >> GEN_VDP_TILE_FLIP_X) & 1;
            int flip_y = (tile_word >> GEN_VDP_TILE_FLIP_Y) & 1;

            int sy = flip_y ? (GEN_VDP_TILE_SIZE - 1 - py) : py;
            int sx = flip_x ? (GEN_VDP_TILE_SIZE - 1 - px) : px;

            uint32_t tile_addr = (uint32_t)(tile_idx * GEN_VDP_TILE_BYTES);
            if (tile_addr + GEN_VDP_TILE_BYTES > GEN_VDP_VRAM_SIZE)
                continue;

            uint16_t w0 = vdp->vram[(tile_addr + sy * GEN_VDP_TILE_ROW_BYTES) / 2];
            uint16_t w1 = vdp->vram[(tile_addr + sy * GEN_VDP_TILE_ROW_BYTES + 2) / 2];
            int shift = (GEN_VDP_TILE_SIZE - 1) - sx;
            int b0 = (w0 >> shift) & 1;
            int b1 = (w0 >> (GEN_VDP_TILE_HI_BYTE + shift)) & 1;
            int b2 = (w1 >> shift) & 1;
            int b3 = (w1 >> (GEN_VDP_TILE_HI_BYTE + shift)) & 1;
            int pix = b0 | (b1 << 1) | (b2 << 2) | (b3 << 3);
            if (pix == 0)
                continue;
            int color_idx = pal * GEN_VDP_PALETTE_SIZE + pix;
            if (color_idx < GEN_VDP_CRAM_SIZE)
                vdp->framebuffer[y * GEN_DISPLAY_WIDTH + x] =
                    cram_to_grayscale(vdp->cram[color_idx]);
        }
    }
}

void gen_vdp_render(gen_vdp_t *vdp)
{
    /* Fondo: reg 7 (palette + index) */
    int bg_pal = (vdp->regs[7] >> GEN_VDP_BG_PAL_SHIFT) & GEN_VDP_BG_PAL_MASK;
    int bg_idx = vdp->regs[7] & GEN_VDP_BG_IDX_MASK;
    int bg_color = bg_pal * GEN_VDP_PALETTE_SIZE + bg_idx;
    uint8_t bg_gray = bg_color < GEN_VDP_CRAM_SIZE ?
        cram_to_grayscale(vdp->cram[bg_color]) : 0;
    for (int i = 0; i < GEN_FB_SIZE; i++)
        vdp->framebuffer[i] = bg_gray;

    int vscroll = vdp->vsram[0] & GEN_VDP_VSRAM_MASK;
    uint32_t hscroll_base = (uint32_t)((vdp->regs[GEN_VDP_REG_HSCROLL] & GEN_VDP_HSCROLL_BASE_MASK) * GEN_VDP_HSCROLL_BASE_WORDS);

    /* H32 (32 tiles) vs H40 (40 tiles) - reg 12 bits 1-0 */
    int h40 = (vdp->regs[GEN_VDP_REG_MODE4] & GEN_VDP_H40_MASK) == GEN_VDP_H40_VAL;
    int width_tiles = h40 ? GEN_VDP_TILES_W : 32;

    /* Orden: B(lo), A(lo), sprites(lo), B(hi), A(hi), sprites(hi) */
    int plane_b = (vdp->regs[4] >> GEN_VDP_PLANE_B_SHIFT) & 7;
    int plane_a = (vdp->regs[2] >> GEN_VDP_PLANE_A_SHIFT) & GEN_VDP_TILE_PAL_MASK;
    uint32_t plane_b_addr = (uint32_t)(plane_b * GEN_VDP_PLANE_UNIT);
    uint32_t plane_a_addr = (uint32_t)(plane_a * GEN_VDP_PLANE_UNIT);
    uint32_t sat_base = (uint32_t)((vdp->regs[5] & GEN_VDP_SAT_BASE_MASK) * 0x100);

    int display_w = (width_tiles == 32) ? GEN_DISPLAY_WIDTH_H32 : GEN_DISPLAY_WIDTH;

    /* Window: reg 3 = base, reg 18 = WX (win_w = (WX+1)*8), reg 19 = WY (win_h = WY+1) */
    uint32_t window_addr = (uint32_t)((vdp->regs[GEN_VDP_REG_WINDOW] & GEN_VDP_WINDOW_MASK) >> GEN_VDP_WINDOW_SHIFT) * GEN_VDP_PLANE_UNIT;
    int wx = vdp->regs[GEN_VDP_REG_WX] & GEN_VDP_WX_MASK;
    int wy = vdp->regs[GEN_VDP_REG_WY] & GEN_VDP_WY_MASK;
    int win_w = (wx + 1) * GEN_VDP_TILE_SIZE;
    int win_h = wy + 1;
    if (win_w > display_w)
        win_w = display_w;
    if (win_h > GEN_DISPLAY_HEIGHT)
        win_h = GEN_DISPLAY_HEIGHT;

    /* Orden: B(lo), Window(lo), A(lo), sprites(lo), B(hi), Window(hi), A(hi), sprites(hi) */
    render_plane(vdp, plane_b_addr, vscroll, hscroll_base, 0, width_tiles);
    render_window(vdp, window_addr, 0, win_w, win_h, width_tiles);
    render_plane(vdp, plane_a_addr, vscroll, hscroll_base, 0, width_tiles);
    for (int s = GEN_VDP_SPRITE_MAX - 1; s >= 0; s--)
        render_sprite(vdp, s, sat_base, 0, display_w);

    render_plane(vdp, plane_b_addr, vscroll, hscroll_base, 1, width_tiles);
    render_window(vdp, window_addr, 1, win_w, win_h, width_tiles);
    render_plane(vdp, plane_a_addr, vscroll, hscroll_base, 1, width_tiles);
    for (int s = GEN_VDP_SPRITE_MAX - 1; s >= 0; s--)
        render_sprite(vdp, s, sat_base, 1, display_w);
}

void gen_vdp_step(gen_vdp_t *vdp, int cycles)
{
    vdp->cycle_counter += cycles;

    while (vdp->cycle_counter >= GEN_CYCLES_PER_LINE)
    {
        vdp->cycle_counter -= GEN_CYCLES_PER_LINE;
        vdp->line_counter++;

        /* Nueva línea: clear HB (visible part) */
        vdp->status_reg &= ~GEN_VDP_STATUS_HB;

        if (vdp->line_counter >= GEN_SCANLINES_TOTAL)
        {
            vdp->line_counter = 0;
            vdp->status_reg &= ~GEN_VDP_STATUS_VB;
        }
        else if (vdp->line_counter == GEN_SCANLINES_VISIBLE)
        {
            /* VBlank: al inicio de la línea 224 */
            vdp->status_reg |= GEN_VDP_STATUS_VB | GEN_VDP_STATUS_F;
            if (vdp->regs[1] & GEN_VDP_REG1_IE0)
                vdp->status_reg |= GEN_VDP_STATUS_F;  /* VBlank IRQ (F ya puesto) */
        }

        /* HBlank al final de línea + IRQ si habilitado */
        vdp->status_reg |= GEN_VDP_STATUS_HB;
        if (vdp->regs[0] & GEN_VDP_REG0_IE1)
            vdp->status_reg |= GEN_VDP_STATUS_F;

        if (vdp->line_counter & 1)
            vdp->status_reg |= GEN_VDP_STATUS_ODD;
        else
            vdp->status_reg &= ~GEN_VDP_STATUS_ODD;
    }

    /* Visible part: clear HB (solo set durante transición de línea) */
    if (vdp->cycle_counter < GEN_CYCLES_PER_LINE - GEN_VDP_HBLANK_CYCLES)
        vdp->status_reg &= ~GEN_VDP_STATUS_HB;
}

uint16_t gen_vdp_read_status(gen_vdp_t *vdp)
{
    vdp->status_cache = (uint16_t)(vdp->status_reg & GEN_VDP_CMD_DATA_MASK);
    vdp->status_reg &= ~GEN_VDP_STATUS_F;  /* Clear F on read */
    return vdp->status_cache;
}

/* Para lecturas byte-a-byte: fetch=1 en primera lectura (0xC00004) */
uint8_t gen_vdp_read_status_byte(gen_vdp_t *vdp, int fetch)
{
    if (fetch)
    {
        vdp->status_cache = (uint16_t)(vdp->status_reg & GEN_VDP_CMD_DATA_MASK);
        vdp->status_reg &= ~GEN_VDP_STATUS_F;
    }
    return fetch ? (uint8_t)(vdp->status_cache >> 8) : (uint8_t)(vdp->status_cache & GEN_VDP_CMD_DATA_MASK);
}

uint16_t gen_vdp_read_hv(gen_vdp_t *vdp)
{
    int h = (vdp->cycle_counter * GEN_VDP_H_VISIBLE_N) / GEN_CYCLES_PER_LINE;
    if (h > GEN_VDP_H_VISIBLE_END)
        h = GEN_VDP_H_BLANK_START + (h - GEN_VDP_H_VISIBLE_N);
    int v = vdp->line_counter & GEN_VDP_HV_V_MASK;
    return (uint16_t)((h << 8) | v);
}

int gen_vdp_pending_irq_level(gen_vdp_t *vdp)
{
    if (!(vdp->status_reg & GEN_VDP_STATUS_F))
        return 0;
    if ((vdp->regs[1] & GEN_VDP_REG1_IE0) && (vdp->status_reg & GEN_VDP_STATUS_VB))
        return GEN_IRQ_LEVEL_VBLANK;
    if ((vdp->regs[0] & GEN_VDP_REG0_IE1) && (vdp->status_reg & GEN_VDP_STATUS_HB))
        return GEN_IRQ_LEVEL_HBLANK;
    return 0;
}
