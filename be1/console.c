/**
 * BE1 - Game Boy: implementación de la interfaz core
 *
 * Conecta console_t con gb_impl_t: init/reset, step (poll input + CPU hasta
 * completar un frame de ciclos), load_rom/unload_rom (incluye .sav si hay batería).
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "core/console.h"
#include "core/savestate.h"
#include "gb_impl.h"
#include "cpu/cpu.h"
#include "ppu.h"
#include "apu.h"
#include "timer.h"
#include "memory.h"
#include "input.h"
#include "gb_constants.h"
#include "audio/audio.h"
#include "core/utils/crc32.h"
#include "core/utils/log.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

static void gb_init(console_t *ctx)
{
    gb_impl_t *impl = ctx->impl;
    gb_mem_init(&impl->mem);
    gb_cpu_init(&impl->cpu);
    gb_ppu_init(&impl->ppu);
    gb_apu_init(&impl->apu);
}

static void gb_reset(console_t *ctx)
{
    gb_impl_t *impl = ctx->impl;
    gb_mem_reset(&impl->mem);
    gb_cpu_reset(&impl->cpu);
}

static void gb_step(console_t *ctx, int cycles)
{
    gb_impl_t *impl = ctx->impl;
    int consumed = 0;
    bitemu_audio_t *audio = (bitemu_audio_t *)impl->audio_output;

    while (consumed < cycles)
    {
        int step_cycles = gb_cpu_step(&impl->cpu, &impl->mem);
        consumed += step_cycles;
        gb_timer_step(&impl->mem, step_cycles);
        gb_ppu_step(&impl->ppu, &impl->mem, step_cycles);
        gb_apu_step(&impl->apu, &impl->mem, step_cycles);

        if (audio)
        {
            impl->apu.sample_clock += step_cycles;
            while (impl->apu.sample_clock >= GB_CYCLES_PER_SAMPLE)
            {
                apu_mix_sample(&impl->apu, &impl->mem, audio);
                impl->apu.sample_clock -= GB_CYCLES_PER_SAMPLE;
            }
        }
    }
}

static bool gb_load_rom(console_t *ctx, const char *path, const uint8_t *data, size_t size)
{
    gb_impl_t *impl = ctx->impl;
    if (impl->mem.rom)
        free(impl->mem.rom);

    impl->mem.rom = malloc(size);
    
    if (!impl->mem.rom)
        return false;
    
    memcpy(impl->mem.rom, data, size);
    
    impl->mem.rom_size = size;
    impl->mem.cart_type = (size > 0x148) ? data[0x147] : 0x01;
    impl->rom_path[0] = '\0';
    if (path) {
        size_t len = strlen(path);
        if (len >= GB_ROM_PATH_MAX)
            len = GB_ROM_PATH_MAX - 1;
        memcpy(impl->rom_path, path, len);
        impl->rom_path[len] = '\0';
    }
    gb_mem_reset(&impl->mem);
    gb_cpu_reset(&impl->cpu);
    if (path)
        gb_mem_load_sav(&impl->mem, path);
    return true;
}

static void gb_unload_rom(console_t *ctx)
{
    gb_impl_t *impl = ctx->impl;
    if (impl->rom_path[0])
        gb_mem_save_sav(&impl->mem, impl->rom_path);
    free(impl->mem.rom);
    impl->mem.rom = NULL;
    impl->mem.rom_size = 0;
    impl->rom_path[0] = '\0';
    gb_mem_reset(&impl->mem);  /* limpia rom0_ptr/rom1_ptr */
}

static int gb_cycles_per_frame(console_t *ctx)
{
    (void)ctx;
    return GB_DOTS_PER_FRAME;
}

static void gb_get_video_info(const console_t *ctx, int *width, int *height)
{
    (void)ctx;
    if (width) *width = GB_LCD_WIDTH;
    if (height) *height = GB_LCD_HEIGHT;
}

/* ----- Save state (.bst) ----- */

/* Compute memory-section byte count (all fields after rom/rom_size). */
static uint32_t mem_section_size(const gb_mem_t *m)
{
    (void)m;
    uint32_t sz = 0;
    sz += 1 + 1 + 1;                       /* rom_bank, cart_type, mbc5_rom_bank_high */
    sz += (uint32_t)sizeof(((gb_mem_t *)0)->ext_ram) + 1 + 1;
    sz += (uint32_t)sizeof(((gb_mem_t *)0)->wram);
    sz += (uint32_t)sizeof(((gb_mem_t *)0)->vram);
    sz += (uint32_t)sizeof(((gb_mem_t *)0)->oam);
    sz += (uint32_t)sizeof(((gb_mem_t *)0)->hram);
    sz += (uint32_t)sizeof(((gb_mem_t *)0)->io);
    sz += 1 + 2;                            /* ie, timer_div */
    sz += 5 + 5 + 1 + 8;                   /* rtc, rtc_latched, rtc_latch_prev, rtc_last_sync */
    sz += 1 + 1;                            /* joypad_state, apu_trigger_flags */
    sz += 1;                                /* mbc1_mode */
    return sz;
}

static int write_mem_fields(FILE *f, const gb_mem_t *m)
{
    int err = 0;
    uint32_t sz = mem_section_size(m);
    err |= bst_write_all(f, &sz, sizeof(sz));

    err |= bst_write_all(f, &m->rom_bank, 1);
    err |= bst_write_all(f, &m->cart_type, 1);
    err |= bst_write_all(f, &m->mbc5_rom_bank_high, 1);
    err |= bst_write_all(f, m->ext_ram, sizeof(m->ext_ram));
    err |= bst_write_all(f, &m->ext_ram_bank, 1);
    err |= bst_write_all(f, &m->ext_ram_enabled, 1);
    err |= bst_write_all(f, m->wram, sizeof(m->wram));
    err |= bst_write_all(f, m->vram, sizeof(m->vram));
    err |= bst_write_all(f, m->oam, sizeof(m->oam));
    err |= bst_write_all(f, m->hram, sizeof(m->hram));
    err |= bst_write_all(f, m->io, sizeof(m->io));
    err |= bst_write_all(f, &m->ie, 1);
    err |= bst_write_all(f, &m->timer_div, 2);
    err |= bst_write_all(f, &m->rtc_s, 1);
    err |= bst_write_all(f, &m->rtc_m, 1);
    err |= bst_write_all(f, &m->rtc_h, 1);
    err |= bst_write_all(f, &m->rtc_dl, 1);
    err |= bst_write_all(f, &m->rtc_dh, 1);
    err |= bst_write_all(f, m->rtc_latched, sizeof(m->rtc_latched));
    err |= bst_write_all(f, &m->rtc_latch_prev, 1);
    err |= bst_write_all(f, &m->rtc_last_sync, sizeof(m->rtc_last_sync));
    err |= bst_write_all(f, &m->joypad_state, 1);
    err |= bst_write_all(f, &m->apu_trigger_flags, 1);
    err |= bst_write_all(f, &m->mbc1_mode, 1);
    return err;
}

static int read_mem_fields(FILE *f, gb_mem_t *m, uint32_t version)
{
    uint32_t stored;
    if (bst_read_all(f, &stored, sizeof(stored)) != 0)
        return -1;

    long start = ftell(f);
    int err = 0;
    err |= bst_read_all(f, &m->rom_bank, 1);
    err |= bst_read_all(f, &m->cart_type, 1);
    err |= bst_read_all(f, &m->mbc5_rom_bank_high, 1);
    err |= bst_read_all(f, m->ext_ram, sizeof(m->ext_ram));
    err |= bst_read_all(f, &m->ext_ram_bank, 1);
    err |= bst_read_all(f, &m->ext_ram_enabled, 1);
    err |= bst_read_all(f, m->wram, sizeof(m->wram));
    err |= bst_read_all(f, m->vram, sizeof(m->vram));
    err |= bst_read_all(f, m->oam, sizeof(m->oam));
    err |= bst_read_all(f, m->hram, sizeof(m->hram));
    err |= bst_read_all(f, m->io, sizeof(m->io));
    err |= bst_read_all(f, &m->ie, 1);
    err |= bst_read_all(f, &m->timer_div, 2);
    err |= bst_read_all(f, &m->rtc_s, 1);
    err |= bst_read_all(f, &m->rtc_m, 1);
    err |= bst_read_all(f, &m->rtc_h, 1);
    err |= bst_read_all(f, &m->rtc_dl, 1);
    err |= bst_read_all(f, &m->rtc_dh, 1);
    err |= bst_read_all(f, m->rtc_latched, sizeof(m->rtc_latched));
    err |= bst_read_all(f, &m->rtc_latch_prev, 1);
    if (version >= 3 && stored - (uint32_t)(ftell(f) - start) >= sizeof(m->rtc_last_sync))
        err |= bst_read_all(f, &m->rtc_last_sync, sizeof(m->rtc_last_sync));
    else if (m->cart_type >= 0x0F && m->cart_type <= 0x13)
        m->rtc_last_sync = (uint64_t)time(NULL);
    err |= bst_read_all(f, &m->joypad_state, 1);
    err |= bst_read_all(f, &m->apu_trigger_flags, 1);

    long consumed = ftell(f) - start;
    if (consumed >= 0 && (uint32_t)consumed < stored)
    {
        m->mbc1_mode = 0;
        if (stored - (uint32_t)consumed >= 1)
        {
            err |= bst_read_all(f, &m->mbc1_mode, 1);
            consumed++;
        }
        if ((uint32_t)consumed < stored)
            fseek(f, (long)(stored - (uint32_t)consumed), SEEK_CUR);
    }
    else
    {
        m->mbc1_mode = 0;
    }

    return err;
}

static int gb_save_state(console_t *ctx, const char *path)
{
    gb_impl_t *impl = ctx->impl;
    gb_mem_t *m = &impl->mem;

    if (!m->rom || m->rom_size == 0)
        return -1;

    FILE *f = fopen(path, "wb");
    if (!f)
        return -1;

    int err = 0;
    err |= bst_write_header(f, BST_CONSOLE_GB, bitemu_crc32(m->rom, m->rom_size));
    err |= bst_write_section(f, &impl->cpu, (uint32_t)sizeof(gb_cpu_t));
    err |= bst_write_section(f, &impl->ppu, (uint32_t)sizeof(gb_ppu_t));
    err |= bst_write_section(f, &impl->apu, (uint32_t)sizeof(gb_apu_t));
    err |= write_mem_fields(f, m);

    fclose(f);

    if (err)
    {
        log_error("Error writing save state: %s", path);
        return -1;
    }
    log_info("State saved: %s", path);
    return 0;
}

static int gb_load_state(console_t *ctx, const char *path)
{
    gb_impl_t *impl = ctx->impl;
    gb_mem_t *m = &impl->mem;

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
    if (bst_validate_header(&hdr, BST_CONSOLE_GB, bitemu_crc32(m->rom, m->rom_size)) != 0)
    {
        fclose(f);
        return -1;
    }

    uint8_t *saved_rom = m->rom;
    size_t saved_rom_size = m->rom_size;
    void *saved_audio = impl->audio_output;

    int err = 0;
    err |= bst_read_section(f, &impl->cpu, (uint32_t)sizeof(gb_cpu_t));
    err |= bst_read_section(f, &impl->ppu, (uint32_t)sizeof(gb_ppu_t));
    err |= bst_read_section(f, &impl->apu, (uint32_t)sizeof(gb_apu_t));
    err |= read_mem_fields(f, m, hdr.version);

    fclose(f);

    m->rom = saved_rom;
    m->rom_size = saved_rom_size;
    impl->audio_output = saved_audio;

    if (err)
    {
        log_error("Error reading save state: %s", path);
        return -1;
    }
    gb_mem_update_rom_ptrs(m);
    log_info("State loaded: %s", path);
    return 0;
}

const console_ops_t gb_console_ops = {
    .name = "Game Boy",
    .init = gb_init,
    .reset = gb_reset,
    .step = gb_step,
    .load_rom = gb_load_rom,
    .unload_rom = gb_unload_rom,
    .cycles_per_frame = gb_cycles_per_frame,
    .save_state = gb_save_state,
    .load_state = gb_load_state,
    .get_video_info = gb_get_video_info,
};
