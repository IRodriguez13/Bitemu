/**
 * BE2 - Genesis: tests VDP timing (PAL, H-int reg 10).
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "test_harness.h"
#include "be2/vdp/vdp.h"
#include "be2/genesis_constants.h"
#include <string.h>

/* Nivel IRQ que ve el 68k cuando el VDP tiene F + IE + HB/VB (contrato gen_vdp_pending_irq_level). */
TEST(gen_vdp_pending_irq_hblank_level)
{
    gen_vdp_t vdp;
    gen_vdp_init(&vdp);
    gen_vdp_reset(&vdp);
    gen_vdp_set_pal(&vdp, 0);
    vdp.regs[0] = GEN_VDP_REG0_IE1;
    vdp.line_counter = 10;
    vdp.irq_hint_pending = 1;
    ASSERT_EQ(gen_vdp_pending_irq_level(&vdp), GEN_IRQ_LEVEL_HBLANK);
}

TEST(gen_vdp_pending_irq_vblank_level)
{
    gen_vdp_t vdp;
    gen_vdp_init(&vdp);
    gen_vdp_reset(&vdp);
    gen_vdp_set_pal(&vdp, 0);
    vdp.regs[1] = GEN_VDP_REG1_IE0;
    vdp.line_counter = 225;
    vdp.status_reg = GEN_VDP_STATUS_VB;
    vdp.irq_vint_pending = 1;
    ASSERT_EQ(gen_vdp_pending_irq_level(&vdp), GEN_IRQ_LEVEL_VBLANK);
}

TEST(gen_vdp_pending_irq_vblank_wins_over_hint_flags)
{
    gen_vdp_t vdp;
    gen_vdp_init(&vdp);
    gen_vdp_reset(&vdp);
    gen_vdp_set_pal(&vdp, 0);
    vdp.regs[0] = GEN_VDP_REG0_IE1;
    vdp.regs[1] = GEN_VDP_REG1_IE0;
    vdp.line_counter = 225;
    vdp.status_reg = (uint8_t)(GEN_VDP_STATUS_VB | GEN_VDP_STATUS_HB);
    vdp.irq_vint_pending = 1;
    vdp.irq_hint_pending = 1;
    ASSERT_EQ(gen_vdp_pending_irq_level(&vdp), GEN_IRQ_LEVEL_VBLANK);
}

/* H-int: nivel visible aun sin bit HB (p.ej. durante porción activa de línea). */
TEST(gen_vdp_hint_irq_not_lost_when_hblank_cleared)
{
    gen_vdp_t vdp;
    gen_vdp_init(&vdp);
    gen_vdp_reset(&vdp);
    gen_vdp_set_pal(&vdp, 0);
    vdp.regs[0] = GEN_VDP_REG0_IE1;
    vdp.line_counter = 5;
    vdp.irq_hint_pending = 1;
    vdp.status_reg = 0;
    ASSERT_EQ(gen_vdp_pending_irq_level(&vdp), GEN_IRQ_LEVEL_HBLANK);
}

TEST(gen_vdp_pal_wraps_313_lines)
{
    gen_vdp_t vdp;
    gen_vdp_init(&vdp);
    gen_vdp_reset(&vdp);
    gen_vdp_set_pal(&vdp, 1);
    gen_vdp_step(&vdp, GEN_SCANLINES_TOTAL_PAL * GEN_CYCLES_PER_LINE_PAL);
    ASSERT_EQ(vdp.line_counter, 0);
}

TEST(gen_vdp_ntsc_wraps_262_lines)
{
    gen_vdp_t vdp;
    gen_vdp_init(&vdp);
    gen_vdp_reset(&vdp);
    gen_vdp_set_pal(&vdp, 0);
    gen_vdp_step(&vdp, GEN_SCANLINES_TOTAL * GEN_CYCLES_PER_LINE);
    ASSERT_EQ(vdp.line_counter, 0);
}

/* Reg 10 = 2 → H-int cada ~3 líneas con IE1 */
TEST(gen_vdp_hint_reg10)
{
    gen_vdp_t vdp;
    gen_vdp_init(&vdp);
    gen_vdp_reset(&vdp);
    gen_vdp_set_pal(&vdp, 0);
    vdp.regs[0] = GEN_VDP_REG0_IE1;
    vdp.regs[GEN_VDP_REG_HINT] = 2;
    gen_vdp_reload_hint_counter(&vdp);

    vdp.status_reg &= (uint8_t)~GEN_VDP_STATUS_F;
    gen_vdp_step(&vdp, 3 * GEN_CYCLES_PER_LINE);
    ASSERT_TRUE((vdp.status_reg & GEN_VDP_STATUS_F) != 0);
}

TEST(gen_vdp_window_right_vs_left)
{
    gen_vdp_t vdp;
    gen_vdp_init(&vdp);
    gen_vdp_reset(&vdp);
    vdp.regs[1] |= 0x40;
    vdp.regs[GEN_VDP_REG_MODE4] = (uint8_t)GEN_VDP_H40_VAL;
    /* Ventana: nametable en 0xE000 B (reg 3 bits 6-4=7); tile 0 vacío (planos); tile 1 sólido */
    vdp.regs[GEN_VDP_REG_WINDOW] = (uint8_t)(7u << GEN_VDP_WINDOW_SHIFT);
    vdp.regs[GEN_VDP_REG_WX] = 1;
    vdp.regs[GEN_VDP_REG_WY] = (uint8_t)(GEN_DISPLAY_HEIGHT - 1);
    vdp.cram[0] = 0;
    vdp.cram[1] = GEN_VDP_CRAM_COLOR;
    for (int r = 0; r < 8; r++)
    {
        vdp.vram[r * 2] = 0;
        vdp.vram[r * 2 + 1] = 0;
    }
    for (int r = 0; r < 8; r++)
    {
        vdp.vram[16 + r * 2] = 0x00FF;
        vdp.vram[16 + r * 2 + 1] = 0;
    }
    vdp.vram[(7u * GEN_VDP_PLANE_UNIT) / 2u] = 1;
    vdp.vram[(7u * GEN_VDP_PLANE_UNIT) / 2u + 1] = 1;
    gen_vdp_render(&vdp);
    uint8_t *pl = &vdp.framebuffer[4 * GEN_FB_STRIDE + 4 * GEN_FB_BYTES_PER_PIXEL];
    uint8_t *pr = &vdp.framebuffer[4 * GEN_FB_STRIDE + 300 * GEN_FB_BYTES_PER_PIXEL];
    ASSERT_NEQ(pl[0] | pl[1] | pl[2], 0);
    ASSERT_EQ(pr[0] | pr[1] | pr[2], 0);

    vdp.regs[GEN_VDP_REG_WH] = GEN_VDP_WH_RIGHT_MASK;
    gen_vdp_render(&vdp);
    uint8_t *pl2 = &vdp.framebuffer[4 * GEN_FB_STRIDE + 4 * GEN_FB_BYTES_PER_PIXEL];
    uint8_t *pr2 = &vdp.framebuffer[4 * GEN_FB_STRIDE + 312 * GEN_FB_BYTES_PER_PIXEL];
    ASSERT_EQ(pl2[0] | pl2[1] | pl2[2], 0);
    ASSERT_NEQ(pr2[0] | pr2[1] | pr2[2], 0);
}

TEST(gen_vdp_status_read_clears_col)
{
    gen_vdp_t vdp;
    gen_vdp_init(&vdp);
    gen_vdp_reset(&vdp);
    vdp.status_reg |= GEN_VDP_STATUS_COL;
    gen_vdp_read_status(&vdp);
    ASSERT_EQ(vdp.status_reg & GEN_VDP_STATUS_COL, 0);
}

TEST(gen_vdp_status_includes_pal_bit)
{
    gen_vdp_t vdp;
    gen_vdp_init(&vdp);
    gen_vdp_reset(&vdp);
    gen_vdp_set_pal(&vdp, 1);
    uint16_t st = gen_vdp_read_status(&vdp);
    ASSERT_NEQ(st & GEN_VDP_STATUS_PAL, 0);
    gen_vdp_set_pal(&vdp, 0);
    st = gen_vdp_read_status(&vdp);
    ASSERT_EQ(st & GEN_VDP_STATUS_PAL, 0);
}

TEST(gen_vdp_status_fifo_mvp)
{
    gen_vdp_t vdp;
    gen_vdp_init(&vdp);
    gen_vdp_reset(&vdp);
    vdp.code_reg = GEN_VDP_CODE_VRAM_WRITE;
    vdp.addr_reg = 0;
    uint16_t st0 = gen_vdp_read_status(&vdp);
    ASSERT_NEQ(st0 & GEN_VDP_STATUS_FIFO_EMPTY, 0);
    gen_vdp_write_data(&vdp, 1);
    uint16_t st1 = gen_vdp_read_status(&vdp);
    ASSERT_EQ(st1 & GEN_VDP_STATUS_FIFO_EMPTY, 0);
    for (int i = 0; i < 3; i++)
        gen_vdp_write_data(&vdp, (uint16_t)(i + 2));
    uint16_t stf = gen_vdp_read_status(&vdp);
    ASSERT_NEQ(stf & GEN_VDP_STATUS_FIFO_FULL, 0);
    gen_vdp_step(&vdp, 4);
    uint16_t st2 = gen_vdp_read_status(&vdp);
    ASSERT_EQ(st2 & GEN_VDP_STATUS_FIFO_FULL, 0);
}

TEST(gen_vdp_dma_fill_adds_68k_stall)
{
    gen_vdp_t vdp;
    gen_vdp_init(&vdp);
    gen_vdp_reset(&vdp);
    vdp.dma_fill_pending = 1;
    vdp.code_reg = GEN_VDP_CODE_VRAM_WRITE;
    vdp.addr_reg = 0;
    vdp.regs[GEN_VDP_REG_DMA_LEN_LO] = 4;
    vdp.regs[GEN_VDP_REG_DMA_LEN_HI] = 0;
    vdp.dma_stall_68k = 0;
    gen_vdp_write_data(&vdp, 0xABCD);
    ASSERT_EQ(vdp.dma_stall_68k, 2u * (uint32_t)GEN_VDP_DMA_STALL_FILL_PER_WORD_ACTIVE);
}

TEST(gen_vdp_dma_fill_stall_lower_in_vblank)
{
    gen_vdp_t vdp;
    gen_vdp_init(&vdp);
    gen_vdp_reset(&vdp);
    vdp.line_counter = GEN_SCANLINES_VISIBLE + 1;
    vdp.status_reg |= GEN_VDP_STATUS_VB;
    vdp.dma_fill_pending = 1;
    vdp.code_reg = GEN_VDP_CODE_VRAM_WRITE;
    vdp.addr_reg = 0;
    vdp.regs[GEN_VDP_REG_DMA_LEN_LO] = 4;
    vdp.regs[GEN_VDP_REG_DMA_LEN_HI] = 0;
    vdp.dma_stall_68k = 0;
    gen_vdp_write_data(&vdp, 0xABCD);
    ASSERT_EQ(vdp.dma_stall_68k, 2u * (uint32_t)GEN_VDP_DMA_STALL_FILL_PER_WORD_VBLANK);
}

/* DMA fill hacia CRAM (no solo VRAM). */
TEST(gen_vdp_dma_fill_cram)
{
    gen_vdp_t vdp;
    gen_vdp_init(&vdp);
    gen_vdp_reset(&vdp);
    vdp.dma_fill_pending = 1;
    vdp.code_reg = GEN_VDP_CODE_CRAM_WRITE;
    vdp.addr_reg = 0;
    vdp.addr_inc = 2;
    vdp.regs[GEN_VDP_REG_DMA_LEN_LO] = 4;
    vdp.regs[GEN_VDP_REG_DMA_LEN_HI] = 0;
    gen_vdp_write_data(&vdp, 0xCDEF);
    ASSERT_EQ((unsigned)vdp.cram[0], (unsigned)(0xCDCD & GEN_VDP_CRAM_COLOR));
    ASSERT_EQ((unsigned)vdp.cram[1], (unsigned)(0xCDCD & GEN_VDP_CRAM_COLOR));
}

/* Reg 12 bit3 + índice fondo 15: resalte MVP (píxel más claro). */
TEST(gen_vdp_shadow_highlight_bg15)
{
    gen_vdp_t vdp;
    gen_vdp_init(&vdp);
    gen_vdp_reset(&vdp);
    /* Color no saturado para que +40 en resalte aumente el canal R */
    vdp.cram[31] = 0x0004u;
    vdp.regs[7] = (uint8_t)((1u << GEN_VDP_BG_PAL_SHIFT) | 15u);
    gen_vdp_render(&vdp);
    uint8_t r0 = vdp.framebuffer[0];
    vdp.regs[GEN_VDP_REG_MODE4] |= (uint8_t)GEN_VDP_MODE4_SH_BIT;
    gen_vdp_render(&vdp);
    uint8_t r1 = vdp.framebuffer[0];
    ASSERT_TRUE(r1 > r0);
}

/* HV: H interpolado en zona activa vs tramo hblank (no mezclar toda la línea). */
TEST(gen_vdp_read_hv_active_and_hblank_ranges)
{
    gen_vdp_t vdp;
    gen_vdp_init(&vdp);
    gen_vdp_reset(&vdp);
    gen_vdp_set_pal(&vdp, 0);
    vdp.cycle_counter = 0;
    uint16_t hv0 = gen_vdp_read_hv(&vdp);
    ASSERT_EQ((hv0 >> 8) & 0xFF, 0);
    vdp.cycle_counter = GEN_CYCLES_PER_LINE / 4;
    uint16_t hv1 = gen_vdp_read_hv(&vdp);
    ASSERT_TRUE(((hv1 >> 8) & 0xFF) > 0 && ((hv1 >> 8) & 0xFF) < (int)GEN_VDP_H_BLANK_START);
    vdp.cycle_counter = GEN_CYCLES_PER_LINE - GEN_VDP_HBLANK_CYCLES;
    uint16_t hv2 = gen_vdp_read_hv(&vdp);
    ASSERT_EQ((hv2 >> 8) & 0xFF, (unsigned)GEN_VDP_H_BLANK_START);
}

/* H32 (reg12 bits1-0=00): contador H activo escala ~256/320 respecto a H40. */
TEST(gen_vdp_read_hv_vblank_h_blank_range)
{
    gen_vdp_t vdp;
    gen_vdp_init(&vdp);
    gen_vdp_reset(&vdp);
    gen_vdp_set_pal(&vdp, 0);
    vdp.line_counter = GEN_SCANLINES_VISIBLE;
    vdp.cycle_counter = GEN_CYCLES_PER_LINE / 3;
    uint16_t hv = gen_vdp_read_hv(&vdp);
    ASSERT_TRUE(((hv >> 8) & 0xFF) >= (unsigned)GEN_VDP_H_BLANK_START);
}

TEST(gen_vdp_read_hv_h32_smaller_active_h)
{
    gen_vdp_t vdp;
    gen_vdp_init(&vdp);
    gen_vdp_reset(&vdp);
    gen_vdp_set_pal(&vdp, 0);
    vdp.regs[GEN_VDP_REG_MODE4] = 0;
    vdp.cycle_counter = GEN_CYCLES_PER_LINE / 4;
    uint16_t hv32 = gen_vdp_read_hv(&vdp);
    vdp.regs[GEN_VDP_REG_MODE4] = GEN_VDP_H40_VAL;
    vdp.cycle_counter = GEN_CYCLES_PER_LINE / 4;
    uint16_t hv40 = gen_vdp_read_hv(&vdp);
    ASSERT_TRUE(((hv32 >> 8) & 0xFF) < ((hv40 >> 8) & 0xFF));
}

TEST(gen_vdp_hint_reloads_on_frame_wrap)
{
    gen_vdp_t vdp;
    gen_vdp_init(&vdp);
    gen_vdp_reset(&vdp);
    vdp.regs[GEN_VDP_REG_HINT] = 7;
    gen_vdp_reload_hint_counter(&vdp);
    vdp.hint_counter = 1;
    gen_vdp_step(&vdp, GEN_SCANLINES_TOTAL * GEN_CYCLES_PER_LINE);
    ASSERT_EQ(vdp.line_counter, 0);
    ASSERT_EQ(vdp.hint_counter, 7);
}

void run_genesis_vdp_tests(void)
{
    SUITE("Genesis VDP timing");
    RUN(gen_vdp_pal_wraps_313_lines);
    RUN(gen_vdp_ntsc_wraps_262_lines);
    RUN(gen_vdp_pending_irq_hblank_level);
    RUN(gen_vdp_pending_irq_vblank_level);
    RUN(gen_vdp_pending_irq_vblank_wins_over_hint_flags);
    RUN(gen_vdp_hint_irq_not_lost_when_hblank_cleared);
    RUN(gen_vdp_hint_reg10);
    RUN(gen_vdp_window_right_vs_left);
    RUN(gen_vdp_status_read_clears_col);
    RUN(gen_vdp_status_includes_pal_bit);
    RUN(gen_vdp_status_fifo_mvp);
    RUN(gen_vdp_dma_fill_adds_68k_stall);
    RUN(gen_vdp_dma_fill_stall_lower_in_vblank);
    RUN(gen_vdp_dma_fill_cram);
    RUN(gen_vdp_shadow_highlight_bg15);
    RUN(gen_vdp_read_hv_active_and_hblank_ranges);
    RUN(gen_vdp_read_hv_vblank_h_blank_range);
    RUN(gen_vdp_read_hv_h32_smaller_active_h);
    RUN(gen_vdp_hint_reloads_on_frame_wrap);
}
