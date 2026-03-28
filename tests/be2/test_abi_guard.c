/**
 * BE2 - Genesis ABI Guard — Save State Binary Compatibility
 *
 * Congela el layout de memoria de las estructuras serializadas en .bst.
 * Si cambia el offset de un campo (reorden, inserción, cambio de tipo),
 * el test falla y el CI bloquea el release.
 *
 * REGLAS:
 *   - Nuevos campos SOLO al FINAL de la struct.
 *   - Los offsets existentes NO deben cambiar.
 *   - sizeof puede CRECER pero nunca DISMINUIR.
 *
 * Al añadir un campo al final:
 *   1. Actualizar la constante MINIMUM_SIZEOF de esa struct.
 *   2. Añadir una línea ASSERT_OFFSET para el nuevo campo.
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#define ASSERT_OFFSET(type, field, expected_off) \
    ASSERT_EQ((long long)offsetof(type, field), (long long)(expected_off))

#define ASSERT_MIN_SIZE(type, min_sz) \
    ASSERT_TRUE(sizeof(type) >= (min_sz))

#include "test_harness.h"
#include "be2/cpu/cpu.h"
#include "be2/vdp/vdp.h"
#include "be2/psg/psg.h"
#include "be2/ym2612/ym2612.h"
#include "be2/z80/z80.h"
#include "be2/genesis_constants.h"
#include <stddef.h>

/* ── CPU 68000 layout (frozen at BST v3) ── */

TEST(abi_gen_cpu_layout)
{
    ASSERT_MIN_SIZE(gen_cpu_t, 76);

    ASSERT_OFFSET(gen_cpu_t, d,      0);
    ASSERT_OFFSET(gen_cpu_t, a,      32);
    ASSERT_OFFSET(gen_cpu_t, pc,     64);
    ASSERT_OFFSET(gen_cpu_t, sr,     68);
    ASSERT_OFFSET(gen_cpu_t, cycles, 72);
}

/* ── VDP layout (frozen at BST v3) ── */

TEST(abi_gen_vdp_layout)
{
    ASSERT_MIN_SIZE(gen_vdp_t, 280864);

    ASSERT_OFFSET(gen_vdp_t, framebuffer, 0);
    ASSERT_OFFSET(gen_vdp_t, regs,        215040);
    ASSERT_OFFSET(gen_vdp_t, vram,         215064);
    ASSERT_OFFSET(gen_vdp_t, cram,         280600);
    ASSERT_OFFSET(gen_vdp_t, vsram,        280728);
    ASSERT_OFFSET(gen_vdp_t, addr_reg,     280808);
    ASSERT_OFFSET(gen_vdp_t, code_reg,     280810);
    ASSERT_OFFSET(gen_vdp_t, pending_hi,   280811);
    ASSERT_OFFSET(gen_vdp_t, addr_inc,     280812);
}

/* ── PSG layout (frozen at BST v3) ── */

TEST(abi_gen_psg_layout)
{
    ASSERT_MIN_SIZE(gen_psg_t, 36);

    ASSERT_OFFSET(gen_psg_t, regs,          0);
    ASSERT_OFFSET(gen_psg_t, latch,         8);
    ASSERT_OFFSET(gen_psg_t, tone,          10);
    ASSERT_OFFSET(gen_psg_t, vol,           16);
    ASSERT_OFFSET(gen_psg_t, noise_type,    20);
    ASSERT_OFFSET(gen_psg_t, noise_lfsr,   22);
    ASSERT_OFFSET(gen_psg_t, tone_counter,  24);
    ASSERT_OFFSET(gen_psg_t, tone_out,      30);
    ASSERT_OFFSET(gen_psg_t, noise_counter, 34);
    ASSERT_OFFSET(gen_psg_t, noise_out,    36);
}

/* ── YM2612 layout (frozen at BST v4) ── */

TEST(abi_gen_ym2612_layout)
{
    ASSERT_MIN_SIZE(gen_ym2612_t, 748);

    ASSERT_OFFSET(gen_ym2612_t, regs,        0);
    ASSERT_OFFSET(gen_ym2612_t, addr,       512);
    ASSERT_OFFSET(gen_ym2612_t, cycle_counter, 516);
    ASSERT_OFFSET(gen_ym2612_t, phase,      520);
    ASSERT_OFFSET(gen_ym2612_t, key,        616);
    ASSERT_OFFSET(gen_ym2612_t, fnum,       640);
    ASSERT_OFFSET(gen_ym2612_t, block,      652);
    ASSERT_OFFSET(gen_ym2612_t, env_state,  658);
    ASSERT_OFFSET(gen_ym2612_t, env_level,  682);
    ASSERT_OFFSET(gen_ym2612_t, busy_cycles, 732);
    ASSERT_OFFSET(gen_ym2612_t, timer_a_counter, 736);
    ASSERT_OFFSET(gen_ym2612_t, timer_b_counter, 738);
    ASSERT_OFFSET(gen_ym2612_t, timer_tick_acc, 740);
    ASSERT_OFFSET(gen_ym2612_t, timer_overflow, 744);
}

/* ── Z80 layout (frozen at BST v3) ── */

TEST(abi_gen_z80_layout)
{
    ASSERT_MIN_SIZE(gen_z80_t, 20);

    ASSERT_OFFSET(gen_z80_t, pc,   0);
    ASSERT_OFFSET(gen_z80_t, sp,   2);
    ASSERT_OFFSET(gen_z80_t, af,   4);
    ASSERT_OFFSET(gen_z80_t, bc,   6);
    ASSERT_OFFSET(gen_z80_t, de,   8);
    ASSERT_OFFSET(gen_z80_t, hl,  10);
    ASSERT_OFFSET(gen_z80_t, ix,  12);
    ASSERT_OFFSET(gen_z80_t, iy,  14);
    ASSERT_OFFSET(gen_z80_t, i,   16);
    ASSERT_OFFSET(gen_z80_t, r,   17);
    ASSERT_OFFSET(gen_z80_t, iff1, 18);
    ASSERT_OFFSET(gen_z80_t, iff2, 19);
    ASSERT_OFFSET(gen_z80_t, halted, 20);
    ASSERT_OFFSET(gen_z80_t, bank, 21);
}

void run_genesis_abi_guard_tests(void)
{
    SUITE("Genesis ABI Guard (save state compat)");
    RUN(abi_gen_cpu_layout);
    RUN(abi_gen_vdp_layout);
    RUN(abi_gen_psg_layout);
    RUN(abi_gen_ym2612_layout);
    RUN(abi_gen_z80_layout);
}
