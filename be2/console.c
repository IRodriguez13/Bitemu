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
#include "core/savestate.h"
#include "genesis_impl.h"
#include "genesis_constants.h"
#include "memory.h"
#include "cpu/cpu.h"
#include "vdp/vdp.h"
#include "psg/psg.h"
#include "ym2612/ym2612.h"
#include "z80/z80.h"
#include "be1/audio/audio.h"
#include "core/utils/crc32.h"
#include "core/utils/log.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static uint16_t dma_read_68k(void *ctx, uint32_t addr)
{
    return genesis_mem_read16((genesis_mem_t *)ctx, addr);
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
    gen_vdp_set_dma_read(&impl->vdp, dma_read_68k, &impl->mem);
    gen_ym2612_init(&impl->ym2612);
    gen_psg_init(&impl->psg);
    gen_z80_init(&impl->z80);
    impl->sample_clock = 0;
    log_info("Genesis: init (modular)");
}

static void gen_reset(console_t *ctx)
{
    genesis_impl_t *impl = ctx->impl;
    memset(impl->z80_ram, 0, sizeof(impl->z80_ram));
    impl->z80_bus_req = 0;   /* 68k tiene bus (0) */
    impl->z80_reset = 0xFF;  /* Z80 no en reset (0xFF=released) */
    impl->sample_clock = 0;
    genesis_mem_reset(&impl->mem);
    gen_cpu_reset(&impl->cpu, &impl->mem);
    gen_vdp_reset(&impl->vdp);
    gen_ym2612_reset(&impl->ym2612);
    gen_psg_reset(&impl->psg);
    gen_z80_reset(&impl->z80);
}

static void gen_step(console_t *ctx, int cycles)
{
    genesis_impl_t *impl = ctx->impl;
    int consumed = 0;

    impl->mem.joypad_raw[0] = impl->joypad_state & 0x0FFF;
    impl->mem.joypad_raw[1] = 0;

    while (consumed < cycles)
    {
        int step = gen_cpu_step(&impl->cpu, &impl->mem, cycles - consumed);
        if (step == 0 && gen_cpu_stopped(&impl->cpu))
            break;
        consumed += step;
        gen_vdp_step(&impl->vdp, step);
        gen_ym2612_step(&impl->ym2612, step, impl->audio_output);
        gen_psg_step(&impl->psg, step, impl->audio_output);
        /* Z80 @ 3.58 MHz cuando tiene el bus */
        if (impl->z80_bus_req == 0 && impl->z80_reset != 0)
        {
            int z80_cycles = (int)((uint64_t)step * GEN_PSG_HZ / GEN_68000_HZ);
            if (z80_cycles > 0)
                gen_z80_step(&impl->z80, impl, z80_cycles);
        }
    }

    impl->sample_clock += consumed;
    bitemu_audio_t *audio = (bitemu_audio_t *)impl->audio_output;
    if (audio && audio->enabled && audio->buffer && audio->buffer_size > 0)
    {
        size_t buf_len = audio->buffer_size;
        size_t wr = audio->write_pos;
        size_t rd = audio->read_pos;
        const size_t samples_per_write = (audio->channels == 2) ? 2u : 1u;

        while (impl->sample_clock >= GEN_CYCLES_PER_SAMPLE)
        {
            size_t used = (wr >= rd) ? (wr - rd) : (buf_len - rd + wr);
            if (used >= buf_len - samples_per_write)
                break;
            impl->sample_clock -= GEN_CYCLES_PER_SAMPLE;
            int psg_out = gen_psg_run_cycles(&impl->psg, GEN_PSG_CYCLES_PER_SAMPLE);
            int16_t ym_l = 0, ym_r = 0;
            gen_ym2612_run_cycles(&impl->ym2612, GEN_CYCLES_PER_SAMPLE, &ym_l, &ym_r);

            int32_t left = psg_out + ym_l;
            int32_t right = psg_out + ym_r;
            if (left > 32767)  left = 32767;
            if (left < -32768) left = -32768;
            if (right > 32767)  right = 32767;
            if (right < -32768) right = -32768;

            if (audio->channels == 2)
            {
                audio->buffer[wr % buf_len] = (int16_t)left;
                wr = (wr + 1) % buf_len;
                audio->buffer[wr % buf_len] = (int16_t)right;
                wr = (wr + 1) % buf_len;
            }
            else
            {
                audio->buffer[wr % buf_len] = (int16_t)((left + right) / 2);
                wr = (wr + 1) % buf_len;
            }
            audio->write_pos = wr;
        }
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

    /* Lock-on: S&K 2MB + locked 2MB (≥4MB). Sonic 2 & K: +256KB patch. */
    mem->lockon = (size >= GEN_LOCKON_SIZE) ? 1 : 0;
    mem->lockon_has_patch = (size >= GEN_LOCKON_SIZE + GEN_LOCKON_PATCH) ? 1 : 0;

    /* SRAM: "RA" en header. Lock-on: A130F1 controla mapeo, empezar con sram_enabled=0. */
    mem->sram_present = 0;
    mem->sram_enabled = 0;
    if (mem->lockon && size > 0x200000 + GEN_HEADER_OFFSET + GEN_HEADER_SRAM_OFF + 2)
    {
        const uint8_t *hdr = data + 0x200000 + GEN_HEADER_OFFSET + GEN_HEADER_SRAM_OFF;
        uint16_t ra = (uint16_t)(hdr[0] << 8) | hdr[1];
        if (ra == GEN_HEADER_SRAM_MAGIC)
            mem->sram_present = 1;
    }
    else if (size > GEN_HEADER_OFFSET + GEN_HEADER_SRAM_OFF + 2)
    {
        uint16_t ra = (uint16_t)(data[GEN_HEADER_OFFSET + GEN_HEADER_SRAM_OFF] << 8)
                    | data[GEN_HEADER_OFFSET + GEN_HEADER_SRAM_OFF + 1];
        if (ra == GEN_HEADER_SRAM_MAGIC)
        {
            mem->sram_present = 1;
            mem->sram_enabled = 1;  /* Juegos normales: SRAM habilitada por defecto */
        }
    }

    gen_reset(ctx);
    if (path && mem->sram_present)
        genesis_mem_load_sav(mem, path);
    log_info("Genesis ROM loaded: %zu bytes%s%s", size,
             mem->sram_present ? " (SRAM)" : "",
             mem->lockon ? " [lock-on]" : "");
    return true;
}

static void gen_unload_rom(console_t *ctx)
{
    genesis_impl_t *impl = ctx->impl;
    genesis_mem_t *mem = &impl->mem;

    if (impl->rom_path[0] && mem->sram_present)
        genesis_mem_save_sav(mem, impl->rom_path);
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

static int gen_save_state(console_t *ctx, const char *path)
{
    genesis_impl_t *impl = ctx->impl;
    genesis_mem_t *m = &impl->mem;

    if (!m->rom || m->rom_size == 0)
        return -1;

    FILE *f = fopen(path, "wb");
    if (!f)
        return -1;

    int err = 0;
    err |= bst_write_header(f, BST_CONSOLE_GEN, bitemu_crc32(m->rom, m->rom_size));
    err |= bst_write_section(f, &impl->cpu, (uint32_t)sizeof(gen_cpu_t));
    err |= bst_write_section(f, m->ram, GEN_RAM_SIZE);
    err |= bst_write_all(f, m->joypad, sizeof(m->joypad));
    err |= bst_write_all(f, m->joypad_raw, sizeof(m->joypad_raw));
    err |= bst_write_all(f, m->joypad_ctrl, sizeof(m->joypad_ctrl));
    err |= bst_write_all(f, m->joypad_cycle, sizeof(m->joypad_cycle));
    err |= bst_write_section(f, impl->vdp.regs, GEN_VDP_REG_COUNT);
    err |= bst_write_section(f, impl->vdp.vram, sizeof(impl->vdp.vram));
    err |= bst_write_section(f, impl->vdp.cram, sizeof(impl->vdp.cram));
    err |= bst_write_section(f, impl->vdp.vsram, sizeof(impl->vdp.vsram));
    err |= bst_write_all(f, &impl->vdp.addr_reg, sizeof(impl->vdp.addr_reg));
    err |= bst_write_all(f, &impl->vdp.code_reg, sizeof(impl->vdp.code_reg));
    err |= bst_write_all(f, &impl->vdp.pending_hi, sizeof(impl->vdp.pending_hi));
    err |= bst_write_all(f, &impl->vdp.addr_inc, sizeof(impl->vdp.addr_inc));
    err |= bst_write_section(f, &impl->psg, (uint32_t)sizeof(gen_psg_t));
    err |= bst_write_section(f, &impl->ym2612, (uint32_t)sizeof(gen_ym2612_t));
    err |= bst_write_section(f, &impl->z80, (uint32_t)sizeof(gen_z80_t));

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
    if (bst_read_header(f, &hdr) != 0)
    {
        fclose(f);
        return -1;
    }
    if (bst_validate_header(&hdr, BST_CONSOLE_GEN, bitemu_crc32(m->rom, m->rom_size)) != 0)
    {
        fclose(f);
        return -1;
    }

    int err = 0;
    err |= bst_read_section(f, &impl->cpu, (uint32_t)sizeof(gen_cpu_t));
    err |= bst_read_section(f, m->ram, GEN_RAM_SIZE);
    err |= bst_read_all(f, m->joypad, sizeof(m->joypad));
    err |= bst_read_all(f, m->joypad_raw, sizeof(m->joypad_raw));
    err |= bst_read_all(f, m->joypad_ctrl, sizeof(m->joypad_ctrl));
    err |= bst_read_all(f, m->joypad_cycle, sizeof(m->joypad_cycle));
    err |= bst_read_section(f, impl->vdp.regs, GEN_VDP_REG_COUNT);
    err |= bst_read_section(f, impl->vdp.vram, sizeof(impl->vdp.vram));
    err |= bst_read_section(f, impl->vdp.cram, sizeof(impl->vdp.cram));
    err |= bst_read_section(f, impl->vdp.vsram, sizeof(impl->vdp.vsram));
    err |= bst_read_all(f, &impl->vdp.addr_reg, sizeof(impl->vdp.addr_reg));
    err |= bst_read_all(f, &impl->vdp.code_reg, sizeof(impl->vdp.code_reg));
    err |= bst_read_all(f, &impl->vdp.pending_hi, sizeof(impl->vdp.pending_hi));
    err |= bst_read_all(f, &impl->vdp.addr_inc, sizeof(impl->vdp.addr_inc));
    err |= bst_read_section(f, &impl->psg, (uint32_t)sizeof(gen_psg_t));
    err |= bst_read_section(f, &impl->ym2612, (uint32_t)sizeof(gen_ym2612_t));
    err |= bst_read_section(f, &impl->z80, (uint32_t)sizeof(gen_z80_t));

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
    /* Framebuffer siempre 320×224 (stride fijo). H32 dibuja solo 256 px; el resto es bg. */
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
