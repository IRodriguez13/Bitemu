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

#include "test_harness.h"
#include "be2/cpu/cpu.h"
#include "be2/vdp/vdp.h"
#include "be2/genesis_constants.h"
#include <stddef.h>

#define ASSERT_OFFSET(type, field, expected_off) \
    ASSERT_EQ((long long)offsetof(type, field), (long long)(expected_off))

#define ASSERT_MIN_SIZE(type, min_sz) \
    ASSERT_TRUE(sizeof(type) >= (min_sz))

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
    ASSERT_MIN_SIZE(gen_vdp_t, 137456);

    ASSERT_OFFSET(gen_vdp_t, framebuffer, 0);
    ASSERT_OFFSET(gen_vdp_t, regs,        71680);
    ASSERT_OFFSET(gen_vdp_t, vram,         71704);
    ASSERT_OFFSET(gen_vdp_t, cram,         137240);
    ASSERT_OFFSET(gen_vdp_t, vsram,        137368);
    ASSERT_OFFSET(gen_vdp_t, addr_reg,     137448);
    ASSERT_OFFSET(gen_vdp_t, code_reg,     137450);
    ASSERT_OFFSET(gen_vdp_t, pending_hi,   137451);
    ASSERT_OFFSET(gen_vdp_t, addr_inc,     137452);
}

void run_genesis_abi_guard_tests(void)
{
    SUITE("Genesis ABI Guard (save state compat)");
    RUN(abi_gen_cpu_layout);
    RUN(abi_gen_vdp_layout);
}
