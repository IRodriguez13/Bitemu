/**
 * BE2 - Sega Genesis: implementación VDP
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "vdp.h"
#include "../memory.h"
#include <string.h>
#include <stddef.h>

/* Cobertura sprite/píxel (H40 máx.): COL vs sprites superpuestos no transparentes */
static uint8_t g_spr_occ[GEN_DISPLAY_WIDTH * GEN_DISPLAY_HEIGHT];

void gen_vdp_init(gen_vdp_t *vdp)
{
    memset(vdp, 0, sizeof(*vdp));
}

void gen_vdp_set_dma_read(gen_vdp_t *vdp, gen_vdp_dma_read_fn fn, void *ctx)
{
    vdp->dma_read_16 = fn;
    vdp->dma_read_ctx = ctx;
}

static int vdp_hint_reload(const gen_vdp_t *vdp)
{
    int r = vdp->regs[GEN_VDP_REG_HINT] & 0xFF;
    return r ? r : 256;
}

static int vdp_in_vblank_for_dma(const gen_vdp_t *vdp)
{
    int lines_vis = vdp->is_pal ? GEN_SCANLINES_VISIBLE_PAL : GEN_SCANLINES_VISIBLE;
    return (vdp->line_counter >= lines_vis) || ((vdp->status_reg & GEN_VDP_STATUS_VB) != 0);
}

static void vdp_dma_stall_fill_words(gen_vdp_t *vdp, uint32_t nwords)
{
    uint32_t per = vdp_in_vblank_for_dma(vdp) ? (uint32_t)GEN_VDP_DMA_STALL_FILL_PER_WORD_VBLANK
                                                : (uint32_t)GEN_VDP_DMA_STALL_FILL_PER_WORD_ACTIVE;
    vdp->dma_stall_68k += nwords * per;
}

static void vdp_dma_stall_copy_words(gen_vdp_t *vdp, uint32_t nwords)
{
    uint32_t per = vdp_in_vblank_for_dma(vdp) ? (uint32_t)GEN_VDP_DMA_STALL_COPY_PER_WORD_VBLANK
                                                : (uint32_t)GEN_VDP_DMA_STALL_COPY_PER_WORD_ACTIVE;
    vdp->dma_stall_68k += nwords * per;
}

static void vdp_dma_stall_68k_words(gen_vdp_t *vdp, uint32_t nwords)
{
    uint32_t per = vdp_in_vblank_for_dma(vdp) ? (uint32_t)GEN_VDP_DMA_STALL_68K_PER_WORD_VBLANK
                                                : (uint32_t)GEN_VDP_DMA_STALL_68K_PER_WORD_ACTIVE;
    vdp->dma_stall_68k += nwords * per;
}

static void vdp_hint_fire_at_hblank(gen_vdp_t *vdp)
{
    if (vdp->regs[0] & GEN_VDP_REG0_IE1)
    {
        vdp->hint_counter--;
        if (vdp->hint_counter < 0)
        {
            vdp->irq_hint_pending = 1;
            vdp->status_reg |= GEN_VDP_STATUS_F;
            vdp->hint_counter = vdp_hint_reload(vdp);
        }
    }
}

static void vdp_sync_hblank_bit(gen_vdp_t *vdp, int active_end)
{
    if (vdp->cycle_counter < active_end)
        vdp->status_reg &= ~GEN_VDP_STATUS_HB;
    else
        vdp->status_reg |= GEN_VDP_STATUS_HB;
}

void gen_vdp_set_pal(gen_vdp_t *vdp, int is_pal)
{
    vdp->is_pal = is_pal ? 1u : 0;
}

void gen_vdp_reload_hint_counter(gen_vdp_t *vdp)
{
    vdp->hint_counter = vdp_hint_reload(vdp);
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
    vdp->dma_stall_68k = 0;
    vdp->cycle_counter = 0;
    vdp->line_counter = 0;
    vdp->status_reg = 0;
    vdp->status_cache = 0;
    vdp->hint_counter = vdp_hint_reload(vdp);
    vdp->irq_vint_pending = 0;
    vdp->irq_hint_pending = 0;
    gen_vdp_render_test_pattern(vdp);
}

uint32_t gen_vdp_take_dma_stall(gen_vdp_t *vdp)
{
    uint32_t s = vdp->dma_stall_68k;
    vdp->dma_stall_68k = 0;
    return s;
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
                vdp_dma_stall_copy_words(vdp, len / 2u);
            }
            else if (vdp->dma_read_16 && vdp->dma_read_ctx)
            {
                /* 68k to VRAM/CRAM/VSRAM */
                uint32_t src = ((uint32_t)(vdp->regs[GEN_VDP_REG_DMA_SRC_EXT] & 0x3F) << 17)
                             | ((uint32_t)((vdp->regs[GEN_VDP_REG_DMA_SRC_HI] << 8) | vdp->regs[GEN_VDP_REG_DMA_SRC_LO]) << 1);
                uint16_t dest = addr;
                int is_cram = dest_code == GEN_VDP_CODE_CRAM_WRITE;
                int is_vsram = dest_code == GEN_VDP_CODE_VSRAM_WRITE;
                uint32_t src_step = 2u;
                if (dma_type < 8 && (dma_type & GEN_VDP_DMA68K_MOVE8_NIBBLE_BIT))
                    src_step = 1u;
                genesis_mem_t *mem68 = (genesis_mem_t *)vdp->dma_read_ctx;

                for (uint32_t i = 0; i < len; i += 2)
                {
                    uint16_t w;
                    if (src_step == 2u)
                        w = vdp->dma_read_16(vdp->dma_read_ctx, src);
                    else
                    {
                        w = (uint16_t)(((uint16_t)genesis_mem_read8(mem68, src) << 8)
                                     | genesis_mem_read8(mem68, src + 1u));
                    }
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
                    src += src_step;
                    /* 128K DMA window: wrap lower 17 bits */
                    src = ((uint32_t)(vdp->regs[GEN_VDP_REG_DMA_SRC_EXT] & 0x3F) << 17) | (src & 0x1FFFF);
                }
                vdp->addr_reg = dest;
                vdp->regs[GEN_VDP_REG_DMA_SRC_LO] = (uint8_t)((src >> 1) & 0xFF);
                vdp->regs[GEN_VDP_REG_DMA_SRC_HI] = (uint8_t)((src >> 9) & 0xFF);
                vdp->status_reg &= ~GEN_VDP_STATUS_DMA;
                vdp_dma_stall_68k_words(vdp, len / 2u);
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
        vdp_dma_stall_fill_words(vdp, len / 2u);
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
            uint8_t *p = &vdp->framebuffer[y * GEN_FB_STRIDE + x * GEN_FB_BYTES_PER_PIXEL];
            p[0] = p[1] = p[2] = c;
        }
    }
}

/* CRAM word (9-bit BGR en bits típicos del VDP) → RGB888 en framebuffer */
static void cram_to_rgb888(uint16_t c, uint8_t *out)
{
    int r = (c >> 1) & GEN_VDP_CRAM_R_MAX;
    int g = (c >> 5) & GEN_VDP_CRAM_R_MAX;
    int b = (c >> 9) & GEN_VDP_CRAM_R_MAX;
    out[0] = (uint8_t)((r * 255 + 3) / 7);
    out[1] = (uint8_t)((g * 255 + 3) / 7);
    out[2] = (uint8_t)((b * 255 + 3) / 7);
}

static void vdp_plot_cram(gen_vdp_t *vdp, int x, int y, uint16_t cram_word)
{
    uint8_t *p = &vdp->framebuffer[y * GEN_FB_STRIDE + x * GEN_FB_BYTES_PER_PIXEL];
    cram_to_rgb888(cram_word, p);
}

static int vdp_sprite_height_pixels(int size_bits)
{
    switch (size_bits)
    {
    case GEN_VDP_SPR_SIZE_8x8:   return 8;
    case GEN_VDP_SPR_SIZE_8x16:  return 16;
    case GEN_VDP_SPR_SIZE_16x16: return 16;
    case GEN_VDP_SPR_SIZE_8x32:  return 32;
    default: return 8;
    }
}

static int vdp_sprite_intersects_line(gen_vdp_t *vdp, uint32_t sat_base, int s, int line)
{
    uint32_t off = sat_base + (uint32_t)(s * GEN_VDP_SAT_ENTRY_WORDS);
    if (off + 4 > GEN_VDP_VRAM_WORDS)
        return 0;
    int y_pos = (int)(vdp->vram[off + 0] & 0x3FF);
    uint16_t w1 = vdp->vram[off + 1];
    int size_bits = (w1 >> GEN_VDP_SPR_SIZE_SHIFT) & GEN_VDP_SPR_SIZE_MASK;
    int h = vdp_sprite_height_pixels(size_bits);
    return (line >= y_pos && line < y_pos + h);
}

/** Orden SAT (0…spr_idx) en esta línea; -1 si spr_idx no cruza la línea. */
static int vdp_sprite_line_slot(gen_vdp_t *vdp, uint32_t sat_base, int line, int spr_idx)
{
    if (!vdp_sprite_intersects_line(vdp, sat_base, spr_idx, line))
        return -1;
    int cnt = 0;
    for (int s = 0; s <= spr_idx; s++)
    {
        if (vdp_sprite_intersects_line(vdp, sat_base, s, line))
        {
            cnt++;
            if (s == spr_idx)
                return cnt;
        }
    }
    return -1;
}

/* Dibuja un sprite. priority_filter: 0=low, 1=high. display_w: 256 o 320. spr_occ: NULL o stride display_w×224 para COL. */
static void render_sprite(gen_vdp_t *vdp, int spr_idx, uint32_t sat_base, int priority_filter, int display_w,
                          uint8_t *spr_occ)
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
                {
                    int slot = vdp_sprite_line_slot(vdp, sat_base, dy, spr_idx);
                    if (slot < 0 || slot > GEN_VDP_MAX_SPRITES_PER_LINE)
                        continue;
                }

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

                    if (spr_occ)
                    {
                        size_t o = (size_t)dy * (size_t)display_w + (size_t)dx;
                        if (spr_occ[o])
                            vdp->status_reg |= GEN_VDP_STATUS_COL;
                        spr_occ[o] = 1;
                    }
                    int color_idx = pal * GEN_VDP_PALETTE_SIZE + pix;
                    if (color_idx < GEN_VDP_CRAM_SIZE)
                        vdp_plot_cram(vdp, dx, dy, vdp->cram[color_idx]);
                }
            }
        }
    }
}

/* Dibuja el window plane (sin scroll). win_x0: borde izquierdo en pantalla (reg 17 bit7 = derecha). */
static void render_window(gen_vdp_t *vdp, uint32_t window_addr, int priority_filter,
                          int win_x0, int win_w, int win_h, int map_width_tiles, int display_w)
{
    if (win_w <= 0 || win_h <= 0)
        return;
    for (int y = 0; y < win_h && y < GEN_DISPLAY_HEIGHT; y++)
    {
        int tile_row = y / GEN_VDP_TILE_SIZE;
        int py = y % GEN_VDP_TILE_SIZE;
        for (int x = 0; x < win_w; x++)
        {
            int sx = win_x0 + x;
            if (sx < 0 || sx >= display_w)
                continue;
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
            int sx2 = flip_x ? (GEN_VDP_TILE_SIZE - 1 - px) : px;

            uint32_t tile_addr = (uint32_t)(tile_idx * GEN_VDP_TILE_BYTES);
            if (tile_addr + GEN_VDP_TILE_BYTES > GEN_VDP_VRAM_SIZE)
                continue;

            uint16_t w0 = vdp->vram[(tile_addr + sy * GEN_VDP_TILE_ROW_BYTES) / 2];
            uint16_t w1 = vdp->vram[(tile_addr + sy * GEN_VDP_TILE_ROW_BYTES + 2) / 2];
            int shift = (GEN_VDP_TILE_SIZE - 1) - sx2;
            int b0 = (w0 >> shift) & 1;
            int b1 = (w0 >> (GEN_VDP_TILE_HI_BYTE + shift)) & 1;
            int b2 = (w1 >> shift) & 1;
            int b3 = (w1 >> (GEN_VDP_TILE_HI_BYTE + shift)) & 1;
            int pix = b0 | (b1 << 1) | (b2 << 2) | (b3 << 3);
            if (pix == 0)
                continue;
            int color_idx = pal * GEN_VDP_PALETTE_SIZE + pix;
            if (color_idx < GEN_VDP_CRAM_SIZE)
                vdp_plot_cram(vdp, sx, y, vdp->cram[color_idx]);
        }
    }
}

static int vdp_hscroll_line_index(int y, int h_mode)
{
    switch (h_mode)
    {
    case 2:  return y / 8;
    case 3:  return y;
    case 1:  /* inválido en HW */
    default: return 0;
    }
}

static int vdp_vscroll_for_line(const gen_vdp_t *vdp, int y, int v_mode)
{
    int vi;
    switch (v_mode)
    {
    case 2:  vi = (y / 8) % GEN_VDP_VSRAM_SIZE; break;
    case 3:  vi = y % GEN_VDP_VSRAM_SIZE; break;
    case 1:
    default: vi = 0; break;
    }
    return (int)(vdp->vsram[vi] & GEN_VDP_VSRAM_MASK);
}

/* Dibuja un plano con scroll. plane_a: 0=B (word par en tabla H), 1=A (word impar). width_tiles: 32 o 40 */
static void render_plane(gen_vdp_t *vdp, uint32_t plane_addr, uint32_t hscroll_base, int plane_a, int h_mode, int v_mode,
                         int priority_filter, int width_tiles)
{
    for (int y = 0; y < GEN_DISPLAY_HEIGHT; y++)
    {
        int vscroll = vdp_vscroll_for_line(vdp, y, v_mode);
        int h_li = vdp_hscroll_line_index(y, h_mode);
        uint32_t row_offset = hscroll_base + (uint32_t)(h_li * 2) + (plane_a ? 1u : 0u);
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
                vdp_plot_cram(vdp, x, y, vdp->cram[color_idx]);
        }
    }
}

/* Más de GEN_VDP_MAX_SPRITES_PER_LINE sprites intersectando una línea → bit SOVR en status */
static void vdp_detect_sprite_overflow(gen_vdp_t *vdp)
{
    uint32_t sat_base = (uint32_t)((vdp->regs[5] & GEN_VDP_SAT_BASE_MASK) * 0x100);
    for (int line = 0; line < GEN_DISPLAY_HEIGHT; line++)
    {
        int cnt = 0;
        for (int s = 0; s < GEN_VDP_SPRITE_MAX; s++)
        {
            uint32_t off = sat_base + (uint32_t)(s * GEN_VDP_SAT_ENTRY_WORDS);
            if (off + 4 > GEN_VDP_VRAM_WORDS)
                break;
            int y_pos = (int)(vdp->vram[off + 0] & 0x3FF);
            uint16_t w1 = vdp->vram[off + 1];
            int size_bits = (w1 >> GEN_VDP_SPR_SIZE_SHIFT) & GEN_VDP_SPR_SIZE_MASK;
            int h = vdp_sprite_height_pixels(size_bits);
            if (line >= y_pos && line < y_pos + h)
                cnt++;
        }
        if (cnt > GEN_VDP_MAX_SPRITES_PER_LINE)
        {
            vdp->status_reg |= GEN_VDP_STATUS_SOVR;
            return;
        }
    }
}

void gen_vdp_render(gen_vdp_t *vdp)
{
    vdp->status_reg &= (uint8_t)~(GEN_VDP_STATUS_SOVR | GEN_VDP_STATUS_COL);

    /* Fondo: reg 7 (palette + index) */
    int bg_pal = (vdp->regs[7] >> GEN_VDP_BG_PAL_SHIFT) & GEN_VDP_BG_PAL_MASK;
    int bg_idx = vdp->regs[7] & GEN_VDP_BG_IDX_MASK;
    int bg_color = bg_pal * GEN_VDP_PALETTE_SIZE + bg_idx;
    uint8_t bg_rgb[3] = {0, 0, 0};
    if (bg_color < GEN_VDP_CRAM_SIZE)
        cram_to_rgb888(vdp->cram[bg_color], bg_rgb);
    for (int i = 0; i < GEN_FB_SIZE; i += GEN_FB_BYTES_PER_PIXEL)
    {
        vdp->framebuffer[i]     = bg_rgb[0];
        vdp->framebuffer[i + 1] = bg_rgb[1];
        vdp->framebuffer[i + 2] = bg_rgb[2];
    }

    uint8_t mode3 = vdp->regs[GEN_VDP_REG_MODE3];
    int h_mode = mode3 & GEN_VDP_MODE3_HSCROLL_MASK;
    if (h_mode == 1)
        h_mode = 0;
    int v_mode = (mode3 >> GEN_VDP_MODE3_VSCROLL_SHIFT) & GEN_VDP_MODE3_VSCROLL_MASK;
    if (v_mode == 1)
        v_mode = 0;

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

    int win_x0 = 0;
    if ((vdp->regs[GEN_VDP_REG_WH] & GEN_VDP_WH_RIGHT_MASK) != 0)
        win_x0 = display_w - win_w;
    if (win_x0 < 0)
        win_x0 = 0;

    memset(g_spr_occ, 0, (size_t)display_w * GEN_DISPLAY_HEIGHT);

    /* Orden: B(lo), Window(lo), A(lo), sprites(lo), B(hi), Window(hi), A(hi), sprites(hi) */
    render_plane(vdp, plane_b_addr, hscroll_base, 0, h_mode, v_mode, 0, width_tiles);
    render_window(vdp, window_addr, 0, win_x0, win_w, win_h, width_tiles, display_w);
    render_plane(vdp, plane_a_addr, hscroll_base, 1, h_mode, v_mode, 0, width_tiles);
    for (int s = GEN_VDP_SPRITE_MAX - 1; s >= 0; s--)
        render_sprite(vdp, s, sat_base, 0, display_w, g_spr_occ);

    render_plane(vdp, plane_b_addr, hscroll_base, 0, h_mode, v_mode, 1, width_tiles);
    render_window(vdp, window_addr, 1, win_x0, win_w, win_h, width_tiles, display_w);
    render_plane(vdp, plane_a_addr, hscroll_base, 1, h_mode, v_mode, 1, width_tiles);
    for (int s = GEN_VDP_SPRITE_MAX - 1; s >= 0; s--)
        render_sprite(vdp, s, sat_base, 1, display_w, g_spr_occ);

    vdp_detect_sprite_overflow(vdp);
}

void gen_vdp_step(gen_vdp_t *vdp, int cycles)
{
    int lines_total = vdp->is_pal ? GEN_SCANLINES_TOTAL_PAL : GEN_SCANLINES_TOTAL;
    int lines_vis = vdp->is_pal ? GEN_SCANLINES_VISIBLE_PAL : GEN_SCANLINES_VISIBLE;
    int cycles_line = vdp->is_pal ? GEN_CYCLES_PER_LINE_PAL : GEN_CYCLES_PER_LINE;
    int active_end = cycles_line - GEN_VDP_HBLANK_CYCLES;
    int rem = cycles;

    while (rem > 0)
    {
        int cc = vdp->cycle_counter;
        if (cc < active_end)
        {
            int d = active_end - cc;
            int t = rem < d ? rem : d;
            vdp->cycle_counter = cc + t;
            rem -= t;
            if (t == d)
            {
                vdp->status_reg |= GEN_VDP_STATUS_HB;
                vdp_hint_fire_at_hblank(vdp);
            }
        }
        else
        {
            int d = cycles_line - cc;
            int t = rem < d ? rem : d;
            vdp->cycle_counter = cc + t;
            rem -= t;
            if (t == d)
            {
                vdp->cycle_counter = 0;
                vdp->line_counter++;
                vdp->status_reg &= ~GEN_VDP_STATUS_HB;

                if (vdp->line_counter >= lines_total)
                {
                    vdp->line_counter = 0;
                    vdp->status_reg &= ~GEN_VDP_STATUS_VB;
                    vdp->hint_counter = vdp_hint_reload(vdp);
                }
                else if (vdp->line_counter == lines_vis)
                {
                    vdp->status_reg |= GEN_VDP_STATUS_VB;
                    if (vdp->regs[1] & GEN_VDP_REG1_IE0)
                    {
                        vdp->irq_vint_pending = 1;
                        vdp->status_reg |= GEN_VDP_STATUS_F;
                    }
                }

                if (vdp->line_counter & 1)
                    vdp->status_reg |= GEN_VDP_STATUS_ODD;
                else
                    vdp->status_reg &= ~GEN_VDP_STATUS_ODD;
            }
        }
        vdp_sync_hblank_bit(vdp, active_end);
    }

    if (cycles == 0)
        vdp_sync_hblank_bit(vdp, active_end);
}

static uint8_t vdp_status_byte_for_read(const gen_vdp_t *vdp)
{
    uint8_t b = (uint8_t)(vdp->status_reg & GEN_VDP_CMD_DATA_MASK);
    b = (uint8_t)((b & (uint8_t)~GEN_VDP_STATUS_PAL) | (vdp->is_pal ? GEN_VDP_STATUS_PAL : 0));
    return b;
}

uint16_t gen_vdp_read_status(gen_vdp_t *vdp)
{
    uint8_t merged = vdp_status_byte_for_read(vdp);
    vdp->status_cache = (uint16_t)(((uint16_t)merged << 8) | (uint16_t)merged);
    vdp->status_reg &= (uint8_t)~(GEN_VDP_STATUS_F | GEN_VDP_STATUS_SOVR | GEN_VDP_STATUS_COL);
    vdp->irq_vint_pending = 0;
    vdp->irq_hint_pending = 0;
    return vdp->status_cache;
}

/* Para lecturas byte-a-byte: fetch=1 en primera lectura (0xC00004) */
uint8_t gen_vdp_read_status_byte(gen_vdp_t *vdp, int fetch)
{
    if (fetch)
    {
        uint8_t merged = vdp_status_byte_for_read(vdp);
        vdp->status_cache = (uint16_t)(((uint16_t)merged << 8) | (uint16_t)merged);
        vdp->status_reg &= (uint8_t)~(GEN_VDP_STATUS_F | GEN_VDP_STATUS_SOVR | GEN_VDP_STATUS_COL);
        vdp->irq_vint_pending = 0;
        vdp->irq_hint_pending = 0;
    }
    return fetch ? (uint8_t)(vdp->status_cache >> 8) : (uint8_t)(vdp->status_cache & GEN_VDP_CMD_DATA_MASK);
}

uint16_t gen_vdp_read_hv(gen_vdp_t *vdp)
{
    int cycles_line = vdp->is_pal ? GEN_CYCLES_PER_LINE_PAL : GEN_CYCLES_PER_LINE;
    int active = cycles_line - GEN_VDP_HBLANK_CYCLES;
    int cc = vdp->cycle_counter;
    int h;
    if (cc < active)
    {
        h = active > 0 ? (cc * (int)GEN_VDP_H_VISIBLE_N) / active : 0;
        if (h > GEN_VDP_H_VISIBLE_END)
            h = GEN_VDP_H_VISIBLE_END;
    }
    else
    {
        int hb_cyc = cc - active;
        int hb_max = 0xFF - (int)GEN_VDP_H_BLANK_START;
        int hb_den = GEN_VDP_HBLANK_CYCLES - 1;
        if (hb_den < 1)
            hb_den = 1;
        h = (int)GEN_VDP_H_BLANK_START + (hb_cyc * hb_max) / hb_den;
        if (h > 0xFF)
            h = 0xFF;
    }
    int v = vdp->line_counter & GEN_VDP_HV_V_MASK;
    return (uint16_t)((h << 8) | v);
}

int gen_vdp_pending_irq_level(gen_vdp_t *vdp)
{
    if ((vdp->regs[1] & GEN_VDP_REG1_IE0) && vdp->irq_vint_pending
        && (vdp->status_reg & GEN_VDP_STATUS_VB))
        return GEN_IRQ_LEVEL_VBLANK;
    if ((vdp->regs[0] & GEN_VDP_REG0_IE1) && vdp->irq_hint_pending)
    {
        int lines_vis = vdp->is_pal ? GEN_SCANLINES_VISIBLE_PAL : GEN_SCANLINES_VISIBLE;
        if (vdp->line_counter < lines_vis)
            return GEN_IRQ_LEVEL_HBLANK;
    }
    return 0;
}
