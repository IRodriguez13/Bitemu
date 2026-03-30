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
#include "cpu_sync/cpu_sync.h"
#include "be1/audio/audio.h"
#include "core/utils/crc32.h"
#include "core/utils/log.h"
#include "core/simd/simd.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define GEN_BST_EXT_V1_MAGIC 0x314E4547u  /* "GEN1" LE: extensión save state v1 */

typedef struct {
    uint8_t z80_ram[8192];
    uint8_t z80_bus_req;
    uint8_t z80_reset;
    uint16_t joypad_state;
    int32_t sample_clock;
    int32_t vdp_cycle_counter;
    int32_t vdp_line_counter;
    uint8_t vdp_status_reg;
    uint8_t _pad;
    uint16_t vdp_status_cache;
    uint8_t vdp_dma_fill_pending;
    uint8_t _pad_ext;
    int32_t vdp_hint_counter;
    uint32_t vdp_dma_stall_68k;
    uint8_t is_pal;
    uint8_t framebuffer[GEN_FB_SIZE];
    uint32_t z80_bus_ack_cycles;
    uint64_t stat_cpu_cyc;
    uint64_t stat_z80_cyc;
    uint64_t stat_dma_stall_consumed;
    uint16_t joypad2_state;
    uint8_t ssf2_bank[GEN_SSF2_SLOT_COUNT];
    uint8_t vdp_irq_vint_pending;
    uint8_t vdp_irq_hint_pending;
} gen_bst_ext_v1_t;

static void genesis_detect_region_pal(genesis_impl_t *impl, const uint8_t *rom, size_t size)
{
    impl->is_pal = 0;
    if (size < GEN_HEADER_REGION_OFF + 4)
        return;
    int has_j = 0, has_u = 0, has_e = 0;
    for (int i = 0; i < 4; i++)
    {
        char c = (char)rom[GEN_HEADER_REGION_OFF + (unsigned)i];
        if (c == 'J' || c == 'j')
            has_j = 1;
        else if (c == 'U' || c == 'u')
            has_u = 1;
        else if (c == 'E' || c == 'e')
            has_e = 1;
    }
    /* "JUE" incluye E pero es NTSC en práctica; EU sin J/U → PAL. */
    if (has_e && !has_u && !has_j)
        impl->is_pal = 1;
}

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
    genesis_mem_set_z80(&impl->mem, impl->z80_ram, &impl->z80_bus_req, &impl->z80_reset,
                        &impl->z80_bus_ack_cycles);
    gen_cpu_init(&impl->cpu);
    gen_vdp_init(&impl->vdp);
    gen_vdp_set_dma_read(&impl->vdp, dma_read_68k, &impl->mem);
    gen_ym2612_init(&impl->ym2612);
    gen_psg_init(&impl->psg);
    gen_z80_init(&impl->z80);
    impl->sample_clock = 0;
    impl->is_pal = 0;
    log_info("Genesis: init");
}

static void gen_reset(console_t *ctx)
{
    genesis_impl_t *impl = ctx->impl;
    memset(impl->z80_ram, 0, sizeof(impl->z80_ram));
    impl->z80_bus_req = 0;   /* BUSREQ sin asertar: Z80 puede correr; RAM Z80 no accesible por 68k (cpu_sync) */
    impl->z80_bus_ack_cycles = 0;
    impl->z80_reset = 0xFF;  /* Z80 no en reset (0xFF=released) */
    impl->sample_clock = 0;
    impl->stat_cpu_cyc = 0;
    impl->stat_z80_cyc = 0;
    impl->stat_dma_stall_consumed = 0;
    genesis_mem_reset(&impl->mem);
    gen_cpu_reset(&impl->cpu, &impl->mem);
    gen_vdp_reset(&impl->vdp);
    gen_vdp_set_pal(&impl->vdp, impl->is_pal);
    gen_ym2612_reset(&impl->ym2612);
    gen_psg_reset(&impl->psg);
    gen_z80_reset(&impl->z80);
}

static void gen_step(console_t *ctx, int cycles)
{
    genesis_impl_t *impl = ctx->impl;
    int consumed = 0;
    uint64_t z80_cyc_acc = 0;

    impl->mem.joypad_raw[0] = impl->joypad_state & 0x0FFF;
    impl->mem.joypad_raw[1] = impl->joypad2_state & 0x0FFF;

    while (consumed < cycles)
    {
        while (impl->vdp.dma_stall_68k > 0U && consumed < cycles)
        {
            uint32_t need = impl->vdp.dma_stall_68k;
            int room = cycles - consumed;
            int chunk = need < (uint32_t)room ? (int)need : room;
            if (chunk <= 0)
                break;
            impl->vdp.dma_stall_68k -= (uint32_t)chunk;
            consumed += chunk;
            if (impl->z80_bus_ack_cycles > (uint32_t)chunk)
                impl->z80_bus_ack_cycles -= (uint32_t)chunk;
            else
                impl->z80_bus_ack_cycles = 0;
            impl->stat_dma_stall_consumed += (uint64_t)chunk;
            gen_vdp_step(&impl->vdp, chunk);
            gen_ym2612_step(&impl->ym2612, chunk, impl->audio_output);
            gen_psg_step(&impl->psg, chunk, impl->audio_output);
            if (gen_cpu_sync_z80_should_run(impl->z80_bus_req, impl->z80_reset))
            {
                int z80_cycles = gen_cpu_sync_z80_cycles_from_68k(chunk, impl->is_pal);
                if (z80_cycles > 0)
                {
                    gen_z80_step(&impl->z80, impl, z80_cycles);
                    z80_cyc_acc += (uint64_t)(unsigned)z80_cycles;
                }
            }
        }
        if (consumed >= cycles)
            break;

        int step = gen_cpu_step(&impl->cpu, &impl->mem, cycles - consumed);
        if (step == 0 && gen_cpu_stopped(&impl->cpu))
            break;
        consumed += step;
        if (impl->z80_bus_ack_cycles > (uint32_t)step)
            impl->z80_bus_ack_cycles -= (uint32_t)step;
        else
            impl->z80_bus_ack_cycles = 0;
        gen_vdp_step(&impl->vdp, step);
        gen_ym2612_step(&impl->ym2612, step, impl->audio_output);
        gen_psg_step(&impl->psg, step, impl->audio_output);
        /* Z80 @ 3.58 MHz cuando tiene el bus */
        if (gen_cpu_sync_z80_should_run(impl->z80_bus_req, impl->z80_reset))
        {
            int z80_cycles = gen_cpu_sync_z80_cycles_from_68k(step, impl->is_pal);
            if (z80_cycles > 0)
            {
                gen_z80_step(&impl->z80, impl, z80_cycles);
                z80_cyc_acc += (uint64_t)(unsigned)z80_cycles;
            }
        }
    }

    impl->stat_cpu_cyc += (uint64_t)consumed;
    impl->stat_z80_cyc += z80_cyc_acc;
    impl->sample_clock += consumed;
    bitemu_audio_t *audio = (bitemu_audio_t *)impl->audio_output;
    if (audio && audio->enabled && audio->buffer && audio->buffer_size > 0)
    {
        int cps = impl->is_pal ? GEN_CYCLES_PER_SAMPLE_PAL : GEN_CYCLES_PER_SAMPLE;
        int psg_cps = impl->is_pal ? GEN_PSG_CYCLES_PER_SAMPLE_PAL : GEN_PSG_CYCLES_PER_SAMPLE;
        size_t buf_len = audio->buffer_size;
        size_t wr = audio->write_pos;
        size_t rd = audio->read_pos;
        const size_t samples_per_write = (audio->channels == 2) ? 2u : 1u;

        while (impl->sample_clock >= cps)
        {
            size_t used = (wr >= rd) ? (wr - rd) : (buf_len - rd + wr);
            if (used >= buf_len - samples_per_write)
                break;
            impl->sample_clock -= cps;
            int psg_out = gen_psg_run_cycles(&impl->psg, psg_cps);
            int16_t ym_l = 0, ym_r = 0;
            gen_ym2612_run_cycles(&impl->ym2612, cps, &ym_l, &ym_r);

            int32_t left = psg_out + ym_l;
            int32_t right = psg_out + ym_r;
            int16_t sl = bitemu_sat_i16_i32(left);
            int16_t sr = bitemu_sat_i16_i32(right);

            if (audio->channels == 2)
            {
                audio->buffer[wr % buf_len] = sl;
                wr = (wr + 1) % buf_len;
                audio->buffer[wr % buf_len] = sr;
                wr = (wr + 1) % buf_len;
            }
            else
            {
                audio->buffer[wr % buf_len] = bitemu_sat_i16_i32(((int32_t)sl + (int32_t)sr) / 2);
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
    genesis_detect_region_pal(impl, mem->rom, size);
    impl->rom_path[0] = '\0';
    if (path)
    {
        size_t len = strlen(path);
        if (len >= GEN_ROM_PATH_MAX - 1)
            len = GEN_ROM_PATH_MAX - 1;
        memcpy(impl->rom_path, path, len);
        impl->rom_path[len] = '\0';
    }

    /* ROM > 4MB: mapper SSF2 (512KB slots); no lock-on en ese caso (p. ej. Super Street Fighter II 5MB). */
    mem->mapper_ssf2 = (size > 0x400000u) ? 1u : 0u;
    if (mem->mapper_ssf2)
    {
        for (int i = 0; i < GEN_SSF2_SLOT_COUNT; i++)
            mem->ssf2_bank[i] = (uint8_t)i;
    }
    /* Lock-on: S&K + locked (≥4MB y sin mapper bancario). */
    mem->lockon = (size >= GEN_LOCKON_SIZE && !mem->mapper_ssf2) ? 1 : 0;
    mem->lockon_has_patch = (size >= GEN_LOCKON_SIZE + GEN_LOCKON_PATCH && !mem->mapper_ssf2) ? 1 : 0;

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

    if (mem->sram_present)
    {
        size_t ie_off = (size_t)GEN_HEADER_SRAM_START_OFF;
        if (mem->lockon)
            ie_off = (size_t)(0x200000u + GEN_HEADER_OFFSET + (GEN_HEADER_SRAM_START_OFF - GEN_HEADER_OFFSET));
        genesis_mem_apply_sram_header_ie32(mem, (size >= ie_off + 8) ? mem->rom + ie_off : NULL);
    }
    else
        genesis_mem_apply_sram_header_ie32(mem, NULL);

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
    impl->is_pal = 0;
}

static int gen_cycles_per_frame(console_t *ctx)
{
    genesis_impl_t *impl = ctx->impl;
    return impl->is_pal ? GEN_CYCLES_PER_FRAME_PAL : GEN_CYCLES_PER_FRAME;
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

    {
        gen_bst_ext_v1_t ext;
        memset(&ext, 0, sizeof(ext));
        memcpy(ext.z80_ram, impl->z80_ram, sizeof(ext.z80_ram));
        ext.z80_bus_req = impl->z80_bus_req;
        ext.z80_reset = impl->z80_reset;
        ext.joypad_state = impl->joypad_state;
        ext.joypad2_state = impl->joypad2_state;
        ext.sample_clock = impl->sample_clock;
        ext.vdp_cycle_counter = impl->vdp.cycle_counter;
        ext.vdp_line_counter = impl->vdp.line_counter;
        ext.vdp_status_reg = impl->vdp.status_reg;
        ext.vdp_status_cache = impl->vdp.status_cache;
        ext.vdp_dma_fill_pending = impl->vdp.dma_fill_pending;
        ext.vdp_hint_counter = (int32_t)impl->vdp.hint_counter;
        ext.vdp_dma_stall_68k = impl->vdp.dma_stall_68k;
        ext.is_pal = impl->is_pal;
        memcpy(ext.framebuffer, impl->vdp.framebuffer, sizeof(ext.framebuffer));
        ext.z80_bus_ack_cycles = impl->z80_bus_ack_cycles;
        ext.stat_cpu_cyc = impl->stat_cpu_cyc;
        ext.stat_z80_cyc = impl->stat_z80_cyc;
        ext.stat_dma_stall_consumed = impl->stat_dma_stall_consumed;
        if (impl->mem.mapper_ssf2)
            memcpy(ext.ssf2_bank, impl->mem.ssf2_bank, sizeof(ext.ssf2_bank));
        ext.vdp_irq_vint_pending = impl->vdp.irq_vint_pending;
        ext.vdp_irq_hint_pending = impl->vdp.irq_hint_pending;
        uint32_t mag = GEN_BST_EXT_V1_MAGIC;
        uint32_t extsz = (uint32_t)sizeof(ext);
        err |= bst_write_all(f, &mag, sizeof(mag));
        err |= bst_write_all(f, &extsz, sizeof(extsz));
        err |= bst_write_all(f, &ext, sizeof(ext));
    }

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

    memset(impl->z80_ram, 0, sizeof(impl->z80_ram));
    impl->z80_bus_req = 0;
    impl->z80_bus_ack_cycles = 0;
    impl->z80_reset = 0xFF;
    impl->joypad_state = 0;
    impl->joypad2_state = 0;
    impl->sample_clock = 0;
    impl->stat_cpu_cyc = 0;
    impl->stat_z80_cyc = 0;
    impl->stat_dma_stall_consumed = 0;
    int ext_applied = 0;
    {
        long pos = ftell(f);
        if (pos >= 0 && fseek(f, 0, SEEK_END) == 0)
        {
            long end = ftell(f);
            if (fseek(f, pos, SEEK_SET) == 0 && end - pos >= (long)sizeof(uint32_t) * 2)
            {
                uint32_t mag = 0, extsz = 0;
                int hdr_ok = (fread(&mag, sizeof(mag), 1, f) == 1
                              && fread(&extsz, sizeof(extsz), 1, f) == 1);
                long after_hdr = ftell(f);
                if (hdr_ok && mag == GEN_BST_EXT_V1_MAGIC && after_hdr >= 0
                    && extsz > 0 && extsz <= (uint32_t)sizeof(gen_bst_ext_v1_t)
                    && end - after_hdr >= (long)extsz)
                {
                    gen_bst_ext_v1_t ext;
                    memset(&ext, 0, sizeof(ext));
                    if (fread(&ext, (size_t)extsz, 1, f) != 1)
                        err = -1;
                    else
                    {
                        memcpy(impl->z80_ram, ext.z80_ram, sizeof(impl->z80_ram));
                        impl->z80_bus_req = ext.z80_bus_req;
                        impl->z80_reset = ext.z80_reset;
                        impl->joypad_state = ext.joypad_state;
                        if (extsz >= (uint32_t)offsetof(gen_bst_ext_v1_t, joypad2_state) + (uint32_t)sizeof(uint16_t))
                            impl->joypad2_state = ext.joypad2_state;
                        else
                            impl->joypad2_state = 0;
                        if (impl->mem.mapper_ssf2
                            && extsz >= (uint32_t)offsetof(gen_bst_ext_v1_t, ssf2_bank) + (uint32_t)sizeof(ext.ssf2_bank))
                            memcpy(impl->mem.ssf2_bank, ext.ssf2_bank, sizeof(impl->mem.ssf2_bank));
                        impl->sample_clock = (int)ext.sample_clock;
                        impl->vdp.cycle_counter = ext.vdp_cycle_counter;
                        impl->vdp.line_counter = ext.vdp_line_counter;
                        impl->vdp.status_reg = ext.vdp_status_reg;
                        impl->vdp.status_cache = ext.vdp_status_cache;
                        impl->vdp.dma_fill_pending = ext.vdp_dma_fill_pending;
                        impl->vdp.hint_counter = (int)ext.vdp_hint_counter;
                        impl->vdp.dma_stall_68k = ext.vdp_dma_stall_68k;
                        impl->is_pal = ext.is_pal;
                        gen_vdp_set_pal(&impl->vdp, impl->is_pal);
                        memcpy(impl->vdp.framebuffer, ext.framebuffer, sizeof(impl->vdp.framebuffer));
                        if (extsz >= (uint32_t)offsetof(gen_bst_ext_v1_t, z80_bus_ack_cycles) + (uint32_t)sizeof(uint32_t))
                            impl->z80_bus_ack_cycles = ext.z80_bus_ack_cycles;
                        else
                            impl->z80_bus_ack_cycles = 0;
                        if (extsz >= (uint32_t)sizeof(gen_bst_ext_v1_t))
                        {
                            impl->stat_cpu_cyc = ext.stat_cpu_cyc;
                            impl->stat_z80_cyc = ext.stat_z80_cyc;
                            impl->stat_dma_stall_consumed = ext.stat_dma_stall_consumed;
                        }
                        if (extsz >= (uint32_t)offsetof(gen_bst_ext_v1_t, vdp_irq_hint_pending) + 2u)
                        {
                            impl->vdp.irq_vint_pending = ext.vdp_irq_vint_pending;
                            impl->vdp.irq_hint_pending = ext.vdp_irq_hint_pending;
                        }
                        else
                        {
                            impl->vdp.irq_vint_pending = 0;
                            impl->vdp.irq_hint_pending = 0;
                        }
                        ext_applied = 1;
                    }
                }
                else if (!hdr_ok || mag != GEN_BST_EXT_V1_MAGIC)
                {
                    if (fseek(f, pos, SEEK_SET) != 0)
                        err = -1;
                }
                else
                {
                    if (fseek(f, pos, SEEK_SET) != 0)
                        err = -1;
                }
            }
            else if (pos >= 0)
                (void)fseek(f, pos, SEEK_SET);
        }
    }
    if (!ext_applied && m->rom && m->rom_size > 0)
        genesis_detect_region_pal(impl, m->rom, m->rom_size);
    if (!ext_applied)
    {
        impl->vdp.cycle_counter = 0;
        impl->vdp.line_counter = 0;
        impl->vdp.status_cache = 0;
        impl->vdp.dma_fill_pending = 0;
        impl->vdp.dma_stall_68k = 0;
        gen_vdp_reload_hint_counter(&impl->vdp);
    }

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
