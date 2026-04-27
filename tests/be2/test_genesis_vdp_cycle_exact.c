/**
 * BE2 - Genesis: VDP cycle/timing helper tests
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "test_harness.h"
#include "be2/vdp/vdp.h"
#include "be2/genesis_constants.h"

static uint16_t s_dma_words[8] = {0x1234, 0x5678, 0x9ABC, 0xDEF0, 0x1111, 0x2222, 0x3333, 0x4444};
static int s_dma_idx = 0;

static uint16_t test_dma_read(void *ctx, uint32_t addr)
{
    (void)ctx;
    (void)addr;
    uint16_t w = s_dma_words[s_dma_idx & 7];
    s_dma_idx++;
    return w;
}

TEST(gen_vdp_hcounter_wraps_h32_h40)
{
    gen_vdp_t vdp;
    gen_vdp_init(&vdp);
    gen_vdp_reset(&vdp);
    gen_vdp_set_pal(&vdp, 0);

    vdp.regs[GEN_VDP_REG_MODE4] &= (uint8_t)~GEN_VDP_H40_MASK;
    gen_vdp_step(&vdp, GEN_VDP_HCOUNT_H32_MAX + 8);
    ASSERT_RANGE(vdp.hcounter, GEN_VDP_HCOUNT_H32_MIN, GEN_VDP_HCOUNT_H32_MAX);

    gen_vdp_reset(&vdp);
    vdp.regs[GEN_VDP_REG_MODE4] |= GEN_VDP_H40_MASK;
    gen_vdp_step(&vdp, GEN_VDP_HCOUNT_H40_MAX + 8);
    ASSERT_RANGE(vdp.hcounter, GEN_VDP_HCOUNT_H40_MIN, GEN_VDP_HCOUNT_H40_MAX);
}

TEST(gen_vdp_hv_reads_active_then_hblank)
{
    gen_vdp_t vdp;
    gen_vdp_init(&vdp);
    gen_vdp_reset(&vdp);
    gen_vdp_set_pal(&vdp, 0);

    vdp.cycle_counter = 0;
    uint16_t hv0 = gen_vdp_read_hv_cycle_exact(&vdp);
    vdp.cycle_counter = GEN_CYCLES_PER_LINE - GEN_VDP_HBLANK_CYCLES;
    uint16_t hv1 = gen_vdp_read_hv_cycle_exact(&vdp);
    ASSERT_TRUE(((hv0 >> 8) & 0xFF) < ((hv1 >> 8) & 0xFF));
    ASSERT_EQ(hv0 & 0xFF, hv1 & 0xFF);
}

TEST(gen_vdp_dma_slot_available_matches_cycle_position)
{
    gen_vdp_t vdp;
    gen_vdp_init(&vdp);
    gen_vdp_reset(&vdp);
    vdp.dma_active = 1;
    vdp.dma_remaining = 4;

    vdp.cycle_counter = 0;
    ASSERT_EQ(gen_vdp_dma_slot_available(&vdp), 1);
    vdp.cycle_counter = 1;
    ASSERT_EQ(gen_vdp_dma_slot_available(&vdp), 0);
    vdp.cycle_counter = GEN_VDP_DMA_SLOT_CYCLES;
    ASSERT_EQ(gen_vdp_dma_slot_available(&vdp), 1);
}

TEST(gen_vdp_dma_slot_step_completes_transfer)
{
    gen_vdp_t vdp;
    gen_vdp_init(&vdp);
    gen_vdp_reset(&vdp);
    gen_vdp_set_dma_read(&vdp, test_dma_read, NULL);
    s_dma_idx = 0;

    vdp.dma_active = 1;
    vdp.dma_remaining = 4;
    vdp.dma_source = 0x1000;
    vdp.addr_reg = 0;
    vdp.code_reg = GEN_VDP_CODE_VRAM_WRITE;
    vdp.status_reg |= GEN_VDP_STATUS_DMA;

    gen_vdp_dma_slot_step(&vdp, GEN_VDP_DMA_SLOT_CYCLES * 4);
    ASSERT_EQ(vdp.dma_remaining, 0);
    ASSERT_EQ(vdp.dma_active, 0);
    ASSERT_EQ(vdp.status_reg & GEN_VDP_STATUS_DMA, 0);
}

void run_genesis_vdp_cycle_exact_tests(void)
{
    SUITE("Genesis VDP cycle helpers");
    RUN(gen_vdp_hcounter_wraps_h32_h40);
    RUN(gen_vdp_hv_reads_active_then_hblank);
    RUN(gen_vdp_dma_slot_available_matches_cycle_position);
    RUN(gen_vdp_dma_slot_step_completes_transfer);
}
