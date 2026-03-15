/**
 * BE2 - Sega Genesis: implementación de la interfaz core
 *
 * Conecta console_t con genesis_impl_t. Usa módulos: memory, cpu, vdp.
 * Sin magic numbers: todo en genesis_constants.h
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "core/console.h"
#include "genesis_impl.h"
#include "genesis_constants.h"
#include "memory.h"
#include "cpu/cpu.h"
#include "vdp/vdp.h"
#include "psg/psg.h"
#include "core/utils/crc32.h"
#include "core/utils/log.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define BST_MAGIC       "BEMU"
#define BST_VERSION     3
#define BST_CONSOLE_GB  1
#define BST_CONSOLE_GEN 2

static int write_all(FILE *f, const void *buf, size_t n)
{
    return fwrite(buf, 1, n, f) == n ? 0 : -1;
}
static int read_all(FILE *f, void *buf, size_t n)
{
    return fread(buf, 1, n, f) == n ? 0 : -1;
}
static int write_section(FILE *f, const void *data, uint32_t size)
{
    int err = write_all(f, &size, sizeof(size));
    err |= write_all(f, data, size);
    return err;
}
static int read_section(FILE *f, void *dest, uint32_t dest_size)
{
    uint32_t stored;
    if (read_all(f, &stored, sizeof(stored)) != 0)
        return -1;
    memset(dest, 0, dest_size);
    uint32_t to_read = stored < dest_size ? stored : dest_size;
    if (to_read > 0 && read_all(f, dest, to_read) != 0)
        return -1;
    if (stored > dest_size && fseek(f, (long)(stored - dest_size), SEEK_CUR) != 0)
        return -1;
    return 0;
}

static void gen_init(console_t *ctx)
{
    genesis_impl_t *impl = ctx->impl;
    genesis_mem_init(&impl->mem);
    genesis_mem_set_vdp(&impl->mem, &impl->vdp);
    genesis_mem_set_ym(&impl->mem, &impl->ym2612);
    genesis_mem_set_psg(&impl->mem, &impl->psg);
    genesis_mem_set_z80(&impl->mem, impl->z80_ram, &impl->z80_bus_req, &impl->z80_reset);
    gen_cpu_init(&impl->cpu);
    gen_vdp_init(&impl->vdp);
    gen_ym2612_init(&impl->ym2612);
    gen_psg_init(&impl->psg);
    log_info("Genesis: init (modular)");
}

static void gen_reset(console_t *ctx)
{
    genesis_impl_t *impl = ctx->impl;
    memset(impl->z80_ram, 0, sizeof(impl->z80_ram));
    impl->z80_bus_req = 0;   /* 68k tiene bus (0) */
    impl->z80_reset = 0xFF;  /* Z80 no en reset (0xFF=released) */
    genesis_mem_reset(&impl->mem);
    gen_cpu_reset(&impl->cpu, &impl->mem);
    gen_vdp_reset(&impl->vdp);
    gen_ym2612_reset(&impl->ym2612);
    gen_psg_reset(&impl->psg);
}

static void gen_step(console_t *ctx, int cycles)
{
    genesis_impl_t *impl = ctx->impl;
    int consumed = 0;

    /* Mapear joypad: Right,Left,Down,Up,Start,A → bits 0-5; 0=presionado (mismo que GB) */
    uint8_t g = (impl->joypad_state & 0x01) | ((impl->joypad_state >> 1) & 1) << 1
              | ((impl->joypad_state >> 3) & 1) << 2 | ((impl->joypad_state >> 2) & 1) << 3
              | ((impl->joypad_state >> 7) & 1) << 4 | ((impl->joypad_state >> 4) & 1) << 5;
    impl->mem.joypad[0] = 0x3F & g;

    while (consumed < cycles)
    {
        int step = gen_cpu_step(&impl->cpu, &impl->mem, cycles - consumed);
        if (step == 0 && gen_cpu_stopped(&impl->cpu))
            break;
        consumed += step;
        gen_vdp_step(&impl->vdp, step);
        gen_ym2612_step(&impl->ym2612, step, impl->audio_output);
        gen_psg_step(&impl->psg, step, impl->audio_output);
    }
    if (impl->vdp.regs[1] & 0x40)  /* Display enable */
        gen_vdp_render(&impl->vdp);
    else
        gen_vdp_render_test_pattern(&impl->vdp);
}

static bool gen_load_rom(console_t *ctx, const char *path, const uint8_t *data, size_t size)
{
    genesis_impl_t *impl = ctx->impl;
    genesis_mem_t *mem = &impl->mem;

    if (mem->rom)
        free(mem->rom);

    mem->rom = malloc(size);
    if (!mem->rom)
        return false;

    memcpy(mem->rom, data, size);
    mem->rom_size = size;
    impl->rom_path[0] = '\0';
    if (path)
    {
        size_t len = strlen(path);
        if (len >= GEN_ROM_PATH_MAX - 1)
            len = GEN_ROM_PATH_MAX - 1;
        memcpy(impl->rom_path, path, len);
        impl->rom_path[len] = '\0';
    }

    gen_reset(ctx);
    log_info("Genesis ROM loaded: %zu bytes", size);
    return true;
}

static void gen_unload_rom(console_t *ctx)
{
    genesis_impl_t *impl = ctx->impl;
    genesis_mem_t *mem = &impl->mem;

    free(mem->rom);
    mem->rom = NULL;
    mem->rom_size = 0;
    impl->rom_path[0] = '\0';
}

static int gen_cycles_per_frame(console_t *ctx)
{
    (void)ctx;
    return GEN_CYCLES_PER_FRAME;  /* 127856 ciclos NTSC 60Hz */
}

typedef struct {
    char magic[4];
    uint32_t version;
    uint32_t console_id;
    uint32_t rom_crc32;
    uint32_t state_size;
} bst_header_t;

static int gen_save_state(console_t *ctx, const char *path)
{
    genesis_impl_t *impl = ctx->impl;
    genesis_mem_t *m = &impl->mem;

    if (!m->rom || m->rom_size == 0)
        return -1;

    FILE *f = fopen(path, "wb");
    if (!f)
        return -1;

    bst_header_t hdr;
    memcpy(hdr.magic, BST_MAGIC, 4);
    hdr.version = BST_VERSION;
    hdr.console_id = BST_CONSOLE_GEN;
    hdr.rom_crc32 = bitemu_crc32(m->rom, m->rom_size);
    hdr.state_size = 0;

    int err = 0;
    err |= write_all(f, &hdr, sizeof(hdr));
    err |= write_section(f, &impl->cpu, (uint32_t)sizeof(gen_cpu_t));
    err |= write_section(f, m->ram, GEN_RAM_SIZE);
    err |= write_all(f, m->joypad, sizeof(m->joypad));
    err |= write_section(f, impl->vdp.regs, GEN_VDP_REG_COUNT);
    err |= write_section(f, impl->vdp.vram, sizeof(impl->vdp.vram));
    err |= write_section(f, impl->vdp.cram, sizeof(impl->vdp.cram));
    err |= write_section(f, impl->vdp.vsram, sizeof(impl->vdp.vsram));
    err |= write_all(f, &impl->vdp.addr_reg, sizeof(impl->vdp.addr_reg));
    err |= write_all(f, &impl->vdp.code_reg, sizeof(impl->vdp.code_reg));
    err |= write_all(f, &impl->vdp.pending_hi, sizeof(impl->vdp.pending_hi));
    err |= write_all(f, &impl->vdp.addr_inc, sizeof(impl->vdp.addr_inc));

    fclose(f);
    if (err)
    {
        log_error("Error writing Genesis save state: %s", path);
        return -1;
    }
    log_info("Genesis state saved: %s", path);
    return 0;
}

static int gen_load_state(console_t *ctx, const char *path)
{
    genesis_impl_t *impl = ctx->impl;
    genesis_mem_t *m = &impl->mem;

    if (!m->rom || m->rom_size == 0)
        return -1;

    FILE *f = fopen(path, "rb");
    if (!f)
        return -1;

    bst_header_t hdr;
    if (read_all(f, &hdr, sizeof(hdr)) != 0)
    {
        fclose(f);
        return -1;
    }

    if (memcmp(hdr.magic, BST_MAGIC, 4) != 0 || hdr.console_id != BST_CONSOLE_GEN)
    {
        log_error("Invalid Genesis save state header: %s", path);
        fclose(f);
        return -1;
    }

    if (hdr.version < BST_VERSION)
    {
        log_error("Genesis save state v%u is obsolete (need v%u).", (unsigned)hdr.version, BST_VERSION);
        fclose(f);
        return -1;
    }
    if (hdr.version > BST_VERSION)
    {
        log_error("Genesis save state v%u was created by a newer Bitemu.", (unsigned)hdr.version);
        fclose(f);
        return -1;
    }

    uint32_t current_crc = bitemu_crc32(m->rom, m->rom_size);
    if (hdr.rom_crc32 != current_crc)
    {
        log_error("ROM CRC mismatch in Genesis state (expected %08X, got %08X)",
                  (unsigned)hdr.rom_crc32, (unsigned)current_crc);
        fclose(f);
        return -1;
    }

    int err = 0;
    err |= read_section(f, &impl->cpu, (uint32_t)sizeof(gen_cpu_t));
    err |= read_section(f, m->ram, GEN_RAM_SIZE);
    err |= read_all(f, m->joypad, sizeof(m->joypad));
    err |= read_section(f, impl->vdp.regs, GEN_VDP_REG_COUNT);
    err |= read_section(f, impl->vdp.vram, sizeof(impl->vdp.vram));
    err |= read_section(f, impl->vdp.cram, sizeof(impl->vdp.cram));
    err |= read_section(f, impl->vdp.vsram, sizeof(impl->vdp.vsram));
    err |= read_all(f, &impl->vdp.addr_reg, sizeof(impl->vdp.addr_reg));
    err |= read_all(f, &impl->vdp.code_reg, sizeof(impl->vdp.code_reg));
    err |= read_all(f, &impl->vdp.pending_hi, sizeof(impl->vdp.pending_hi));
    err |= read_all(f, &impl->vdp.addr_inc, sizeof(impl->vdp.addr_inc));

    fclose(f);
    if (err)
    {
        log_error("Error reading Genesis save state: %s", path);
        return -1;
    }
    log_info("Genesis state loaded: %s", path);
    return 0;
}

static void gen_get_video_info(const console_t *ctx, int *width, int *height)
{
    (void)ctx;
    if (width)
        *width = GEN_DISPLAY_WIDTH;
    if (height)
        *height = GEN_DISPLAY_HEIGHT;
}

const console_ops_t genesis_console_ops = {
    .name = "Genesis",
    .init = gen_init,
    .reset = gen_reset,
    .step = gen_step,
    .load_rom = gen_load_rom,
    .unload_rom = gen_unload_rom,
    .cycles_per_frame = gen_cycles_per_frame,
    .save_state = gen_save_state,
    .load_state = gen_load_state,
    .get_video_info = gen_get_video_info,
    .load_media = NULL,
};
