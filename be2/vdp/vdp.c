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

void gen_vdp_reset(gen_vdp_t *vdp)
{
    memset(vdp->regs, 0, GEN_VDP_REG_COUNT);
    memset(vdp->vram, 0, sizeof(vdp->vram));
    memset(vdp->cram, 0, sizeof(vdp->cram));
    memset(vdp->vsram, 0, sizeof(vdp->vsram));
    vdp->addr_reg = 0;
    vdp->code_reg = 0;
    vdp->pending_hi = 0;
    vdp->addr_inc = 2;
    vdp->cycle_counter = 0;
    vdp->line_counter = 0;
    vdp->status_reg = 0;
    vdp->status_cache = 0;
    gen_vdp_render_test_pattern(vdp);
}

static void set_access_mode(gen_vdp_t *vdp, uint8_t code, uint16_t addr)
{
    vdp->code_reg = code;
    vdp->addr_reg = addr & 0x3FFF;
}

void gen_vdp_write_ctrl(gen_vdp_t *vdp, uint16_t val)
{
    if (vdp->pending_hi)
    {
        /* Segunda palabra: comando 32-bit completo */
        uint32_t cmd = ((uint32_t)vdp->pending_hi << 16) | val;
        vdp->pending_hi = 0;

        if ((cmd & 0xC0000000) == 0x80000000)
        {
            /* Escritura de registro: bits 10?R RRRR DDDD DDDD */
            int reg = (cmd >> 24) & 0x1F;
            uint8_t data = (uint8_t)(cmd & 0xFF);
            if (reg < GEN_VDP_REG_COUNT)
                vdp->regs[reg] = data;
            if (reg == 0x0F)
                vdp->addr_inc = data ? data : 1;
            return;
        }

        /* Comando de acceso a memoria */
        uint16_t addr = (uint16_t)((cmd >> 16) & 0x3FFF)
                     | (uint16_t)(((cmd >> 2) & 3) << 14);
        uint8_t code = (uint8_t)((cmd >> 28) & 0x0F);
        if (code == 1)
            set_access_mode(vdp, 1, addr);  /* VRAM write */
        else if (code == 3)
            set_access_mode(vdp, 3, addr);  /* CRAM write */
        else if ((code & 0x0E) == 4)
            set_access_mode(vdp, 5, addr);  /* VSRAM write */
        else
            set_access_mode(vdp, 0, addr);
        return;
    }

    if ((val & 0xC000) == 0x8000)
    {
        /* Comando de registro (16-bit) */
        int reg = (val >> 8) & 0x1F;
        uint8_t data = (uint8_t)(val & 0xFF);
        if (reg < GEN_VDP_REG_COUNT)
            vdp->regs[reg] = data;
        if (reg == 0x0F)
            vdp->addr_inc = data ? data : 1;
        return;
    }

    /* Primera palabra de comando 32-bit */
    vdp->pending_hi = val;
}

void gen_vdp_write_data(gen_vdp_t *vdp, uint16_t val)
{
    uint16_t addr = vdp->addr_reg;

    switch (vdp->code_reg)
    {
    case 1:  /* VRAM write */
        if (addr < GEN_VDP_VRAM_SIZE / 2)
            vdp->vram[addr] = val;
        vdp->addr_reg = (addr + (vdp->addr_inc & 0xFF)) & 0x3FFF;
        break;
    case 3:  /* CRAM write */
    {
        int idx = (addr >> 1) & 0x3F;
        vdp->cram[idx] = val & 0x0EEE;  /* 9-bit color, low bit always 0 */
        vdp->addr_reg = (addr + (vdp->addr_inc & 0xFF)) & 0x3FFF;
        break;
    }
    case 5:  /* VSRAM write */
        if (addr < GEN_VDP_VSRAM_SIZE * 2)
            vdp->vsram[addr >> 1] = val & 0x3FF;
        vdp->addr_reg = (addr + (vdp->addr_inc & 0xFF)) & 0x3FFF;
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
    int r = (c >> 1) & 7;
    int g = (c >> 5) & 7;
    int b = (c >> 9) & 7;
    int gray = (r + g + b) * 255 / 21;  /* max 21 → 255 */
    return (uint8_t)(gray > 255 ? 255 : gray);
}

void gen_vdp_render(gen_vdp_t *vdp)
{
    /* Plane A: reg 2 bits 4-2 = high 3 bits of VRAM base (×$2000) */
    int plane_a = (vdp->regs[2] >> 3) & 7;
    uint32_t plane_a_addr = (uint32_t)(plane_a * 0x2000);

    for (int ty = 0; ty < GEN_VDP_TILES_H; ty++)
    {
        for (int tx = 0; tx < GEN_VDP_TILES_W; tx++)
        {
            uint32_t map_idx = plane_a_addr / 2 + ty * 64 + tx;
            if (map_idx >= GEN_VDP_VRAM_SIZE / 2)
                continue;
            uint16_t tile_word = vdp->vram[map_idx];
            int tile_idx = tile_word & 0x7FF;
            int pal = (tile_word >> 13) & 7;
            int flip_x = (tile_word >> 11) & 1;
            int flip_y = (tile_word >> 12) & 1;

            /* Tile data: 8×8, 4 bpp, 32 bytes per tile */
            uint32_t tile_addr = (uint32_t)(tile_idx * 32);
            if (tile_addr + 32 > GEN_VDP_VRAM_SIZE)
                continue;

            for (int py = 0; py < GEN_VDP_TILE_SIZE; py++)
            {
                int sy = flip_y ? (GEN_VDP_TILE_SIZE - 1 - py) : py;
                uint16_t w0 = vdp->vram[(tile_addr + sy * 4) / 2];
                uint16_t w1 = vdp->vram[(tile_addr + sy * 4 + 2) / 2];

                for (int px = 0; px < GEN_VDP_TILE_SIZE; px++)
                {
                    int sx = flip_x ? (GEN_VDP_TILE_SIZE - 1 - px) : px;
                    int shift = 7 - sx;
                    int b0 = (w0 >> shift) & 1;
                    int b1 = (w0 >> (8 + shift)) & 1;
                    int b2 = (w1 >> shift) & 1;
                    int b3 = (w1 >> (8 + shift)) & 1;
                    int pix = b0 | (b1 << 1) | (b2 << 2) | (b3 << 3);
                    int color_idx = pal * 16 + pix;
                    if (color_idx < GEN_VDP_CRAM_SIZE)
                    {
                        int y = ty * GEN_VDP_TILE_SIZE + py;
                        int x = tx * GEN_VDP_TILE_SIZE + px;
                        if (y < GEN_DISPLAY_HEIGHT && x < GEN_DISPLAY_WIDTH)
                            vdp->framebuffer[y * GEN_DISPLAY_WIDTH + x] =
                                cram_to_grayscale(vdp->cram[color_idx]);
                    }
                }
            }
        }
    }
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
    if (vdp->cycle_counter < GEN_CYCLES_PER_LINE - 80)
        vdp->status_reg &= ~GEN_VDP_STATUS_HB;
}

uint16_t gen_vdp_read_status(gen_vdp_t *vdp)
{
    vdp->status_cache = (uint16_t)(vdp->status_reg & 0xFF);
    vdp->status_reg &= ~GEN_VDP_STATUS_F;  /* Clear F on read */
    return vdp->status_cache;
}

/* Para lecturas byte-a-byte: fetch=1 en primera lectura (0xC00004) */
uint8_t gen_vdp_read_status_byte(gen_vdp_t *vdp, int fetch)
{
    if (fetch)
    {
        vdp->status_cache = (uint16_t)(vdp->status_reg & 0xFF);
        vdp->status_reg &= ~GEN_VDP_STATUS_F;
    }
    return fetch ? (uint8_t)(vdp->status_cache >> 8) : (uint8_t)(vdp->status_cache & 0xFF);
}

uint16_t gen_vdp_read_hv(gen_vdp_t *vdp)
{
    /* H: 0-0x93 visible, 0xE9-0xFF blank (simplificado: cycle-based) */
    int h = (vdp->cycle_counter * 0x94) / GEN_CYCLES_PER_LINE;
    if (h > 0x93)
        h = 0xE9 + (h - 0x94);
    int v = vdp->line_counter & 0xFF;
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
