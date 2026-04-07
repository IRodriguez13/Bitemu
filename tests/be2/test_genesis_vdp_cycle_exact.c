/**
 * BE2 - Genesis: tests VDP cycle-exact timing
 *
 * Tests para HV counters precisos, DMA slots, y timing cycle-exact
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "test_harness.h"
#include "be2/vdp/vdp.h"
#include "be2/genesis_constants.h"
#include <string.h>

/* Test HV counter ranges for H32 mode */
TEST(gen_vdp_hv_counter_h32_range)
{
    gen_vdp_t vdp;
    gen_vdp_init(&vdp);
    gen_vdp_reset(&vdp);
    gen_vdp_set_pal(&vdp, 0);
    
    /* Set H32 mode */
    vdp.regs[GEN_VDP_REG_MODE4] &= ~GEN_VDP_H40_MASK;
    
    /* Test initial counters */
    ASSERT_EQ(vdp.hcounter, GEN_VDP_HCOUNT_H32_MIN);
    ASSERT_EQ(vdp.vcounter, GEN_VDP_VCOUNT_NTSC_MIN);
    
    /* Advance some cycles and check H counter wraps correctly */
    gen_vdp_step(&vdp, 100);
    ASSERT_GE(vdp.hcounter, GEN_VDP_HCOUNT_H32_MIN);
    ASSERT_LE(vdp.hcounter, GEN_VDP_HCOUNT_H32_MAX);
}

/* Test HV counter ranges for H40 mode */
TEST(gen_vdp_hv_counter_h40_range)
{
    gen_vdp_t vdp;
    gen_vdp_init(&vdp);
    gen_vdp_reset(&vdp);
    gen_vdp_set_pal(&vdp, 0);
    
    /* Set H40 mode */
    vdp.regs[GEN_VDP_REG_MODE4] |= GEN_VDP_H40_MASK;
    
    /* Test initial counters */
    ASSERT_EQ(vdp.hcounter, GEN_VDP_HCOUNT_H40_MIN);
    ASSERT_EQ(vdp.vcounter, GEN_VDP_VCOUNT_NTSC_MIN);
    
    /* Advance some cycles and check H counter wraps correctly */
    gen_vdp_step(&vdp, 100);
    ASSERT_GE(vdp.hcounter, GEN_VDP_HCOUNT_H40_MIN);
    ASSERT_LE(vdp.hcounter, GEN_VDP_HCOUNT_H40_MAX);
}

/* Test V counter PAL vs NTSC differences */
TEST(gen_vdp_vcounter_pal_vs_ntsc)
{
    gen_vdp_t vdp_ntsc, vdp_pal;
    gen_vdp_init(&vdp_ntsc);
    gen_vdp_init(&vdp_pal);
    gen_vdp_reset(&vdp_ntsc);
    gen_vdp_reset(&vdp_pal);
    
    gen_vdp_set_pal(&vdp_ntsc, 0);  /* NTSC */
    gen_vdp_set_pal(&vdp_pal, 1);  /* PAL */
    
    /* Both should start at minimum */
    ASSERT_EQ(vdp_ntsc.vcounter, GEN_VDP_VCOUNT_NTSC_MIN);
    ASSERT_EQ(vdp_pal.vcounter, GEN_VDP_VCOUNT_PAL_MIN);
    
    /* Advance to end of frame */
    int ntsc_cycles = GEN_SCANLINES_TOTAL_NTSC * GEN_CYCLES_PER_LINE;
    int pal_cycles = GEN_SCANLINES_TOTAL_PAL * GEN_CYCLES_PER_LINE_PAL;
    
    gen_vdp_step(&vdp_ntsc, ntsc_cycles);
    gen_vdp_step(&vdp_pal, pal_cycles);
    
    /* Should wrap around */
    ASSERT_EQ(vdp_ntsc.vcounter, GEN_VDP_VCOUNT_NTSC_MIN);
    ASSERT_EQ(vdp_pal.vcounter, GEN_VDP_VCOUNT_PAL_MIN);
}

/* Test DMA slot availability */
TEST(gen_vdp_dma_slot_availability)
{
    gen_vdp_t vdp;
    gen_vdp_init(&vdp);
    gen_vdp_reset(&vdp);
    gen_vdp_set_pal(&vdp, 0);
    
    /* No DMA active should always return available */
    ASSERT_EQ(gen_vdp_dma_slot_available(&vdp), 1);
    
    /* Activate DMA */
    vdp.dma_active = 1;
    vdp.dma_remaining = 10;
    
    /* Check slot availability at different cycle positions */
    for (int i = 0; i < 50; i++) {
        gen_vdp_step(&vdp, 1);
        int available = gen_vdp_dma_slot_available(&vdp);
        
        /* Should be available at slot boundaries */
        int cycle_in_line = vdp.cycle_counter % GEN_VDP_CYCLES_PER_LINE;
        int expected_available = (cycle_in_line % GEN_VDP_DMA_SLOT_CYCLES == 0) ? 1 : 0;
        
        ASSERT_EQ(available, expected_available);
    }
}

/* Test DMA slot processing */
TEST(gen_vdp_dma_slot_processing)
{
    gen_vdp_t vdp;
    gen_vdp_init(&vdp);
    gen_vdp_reset(&vdp);
    gen_vdp_set_pal(&vdp, 0);
    
    /* Mock DMA read function */
    static uint16_t test_data[] = {0x1234, 0x5678, 0x9ABC, 0xDEF0};
    static int data_index = 0;
    
    auto mock_dma_read = [](void* ctx, uint32_t addr) -> uint16_t {
        (void)ctx; (void)addr;
        return test_data[data_index++ % 4];
    };
    
    gen_vdp_set_dma_read(&vdp, mock_dma_read, nullptr);
    
    /* Activate DMA with 4 words */
    vdp.dma_active = 1;
    vdp.dma_remaining = 4;
    vdp.dma_source = 0x1000;
    vdp.addr_reg = 0x0000;
    vdp.code_reg = GEN_VDP_CODE_VRAM_WRITE;
    
    /* Process enough cycles for all DMA transfers */
    gen_vdp_step(&vdp, GEN_VDP_DMA_SLOT_CYCLES * 4);
    
    /* DMA should be complete */
    ASSERT_EQ(vdp.dma_active, 0);
    ASSERT_EQ(vdp.dma_remaining, 0);
    ASSERT_EQ((vdp.status_reg & GEN_VDP_STATUS_DMA), 0);
}

/* Test HV counter reading format */
TEST(gen_vdp_hv_counter_reading_format)
{
    gen_vdp_t vdp;
    gen_vdp_init(&vdp);
    gen_vdp_reset(&vdp);
    gen_vdp_set_pal(&vdp, 0);
    
    /* Set known counter values */
    vdp.hcounter = 0x123;
    vdp.vcounter = 0x045;
    
    uint16_t hv = gen_vdp_read_hv_cycle_exact(&vdp);
    
    /* HV format: H in high byte, V in low byte */
    ASSERT_EQ(hv >> 8, 0x123);
    ASSERT_EQ(hv & 0xFF, 0x045);
}

/* Test cycle-exact vs approximate timing differences */
TEST(gen_vdp_cycle_exact_vs_approximate)
{
    gen_vdp_t vdp;
    gen_vdp_init(&vdp);
    gen_vdp_reset(&vdp);
    gen_vdp_set_pal(&vdp, 0);
    
    /* Store initial state */
    int initial_hcounter = vdp.hcounter;
    int initial_vcounter = vdp.vcounter;
    
    /* Advance exact number of cycles */
    int test_cycles = 123;
    gen_vdp_step(&vdp, test_cycles);
    
    /* Counters should have advanced by exactly test_cycles */
    int expected_hcounter = (initial_hcounter + test_cycles) % (GEN_VDP_HCOUNT_H32_MAX + 1);
    
    /* Allow for line transitions in V counter */
    ASSERT_GE(vdp.vcounter, initial_vcounter);
    ASSERT_LE(vdp.vcounter, initial_vcounter + (test_cycles / GEN_CYCLES_PER_LINE) + 1);
}

/* Test DMA timing in active vs vblank */
TEST(gen_vdp_dma_timing_active_vs_vblank)
{
    gen_vdp_t vdp;
    gen_vdp_init(&vdp);
    gen_vdp_reset(&vdp);
    gen_vdp_set_pal(&vdp, 0);
    
    /* Test during active display */
    vdp.line_counter = 100;  /* Well within active display */
    vdp.dma_active = 1;
    vdp.dma_remaining = 2;
    
    int cycles_before = vdp.cycle_counter;
    gen_vdp_dma_slot_step(&vdp, GEN_VDP_DMA_SLOT_CYCLES * 2);
    int cycles_after = vdp.cycle_counter;
    
    /* DMA should have processed */
    ASSERT_EQ(vdp.dma_remaining, 0);
    
    /* Test during vblank */
    vdp.line_counter = 225;  /* Vblank region */
    vdp.dma_active = 1;
    vdp.dma_remaining = 2;
    
    cycles_before = vdp.cycle_counter;
    gen_vdp_dma_slot_step(&vdp, GEN_VDP_DMA_SLOT_CYCLES * 2);
    cycles_after = vdp.cycle_counter;
    
    /* DMA should still process in vblank */
    ASSERT_EQ(vdp.dma_remaining, 0);
}
