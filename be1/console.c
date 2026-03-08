/**
 * BE1 - Game Boy: implementación de la interfaz core
 *
 * Conecta console_t con gb_impl_t: init/reset, step (poll input + CPU hasta
 * completar un frame de ciclos), load_rom/unload_rom (incluye .sav si hay batería).
 */

#include "core/console.h"
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

/* .bst save state format constants */
#define BST_MAGIC       "BEMU"
#define BST_VERSION     2
#define BST_CONSOLE_GB  1

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
}

static int gb_cycles_per_frame(console_t *ctx)
{
    (void)ctx;
    return GB_DOTS_PER_FRAME;
}

/* ----- Save state (.bst) ----- */

typedef struct 
{
    char     magic[4];
    uint32_t version;
    uint32_t console_id;
    uint32_t rom_crc32;
    uint32_t state_size;
} bst_header_t;

static int write_all(FILE *f, const void *buf, size_t n)
{
    return fwrite(buf, 1, n, f) == n ? 0 : -1;
}

static int read_all(FILE *f, void *buf, size_t n)
{
    return fread(buf, 1, n, f) == n ? 0 : -1;
}

/*
 * Section-based I/O: each section is prefixed with a uint32_t byte count.
 * On load, if the stored section is smaller than the current struct, the
 * extra fields (which MUST live at the end of the struct) are zero-filled.
 * If the stored section is larger (save from a newer Bitemu), the unknown
 * tail bytes are skipped.  This gives forward+backward compat for free
 * as long as new fields are always appended.
 */
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
    if (stored > dest_size)
    {
        if (fseek(f, (long)(stored - dest_size), SEEK_CUR) != 0)
            return -1;
    }
    return 0;
}

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
    sz += 5 + 5 + 1;                        /* rtc, rtc_latched, rtc_latch_prev */
    sz += 1 + 1;                            /* joypad_state, apu_trigger_flags */
    return sz;
}

static int write_mem_fields(FILE *f, const gb_mem_t *m)
{
    int err = 0;
    uint32_t sz = mem_section_size(m);
    err |= write_all(f, &sz, sizeof(sz));

    err |= write_all(f, &m->rom_bank, 1);
    err |= write_all(f, &m->cart_type, 1);
    err |= write_all(f, &m->mbc5_rom_bank_high, 1);
    err |= write_all(f, m->ext_ram, sizeof(m->ext_ram));
    err |= write_all(f, &m->ext_ram_bank, 1);
    err |= write_all(f, &m->ext_ram_enabled, 1);
    err |= write_all(f, m->wram, sizeof(m->wram));
    err |= write_all(f, m->vram, sizeof(m->vram));
    err |= write_all(f, m->oam, sizeof(m->oam));
    err |= write_all(f, m->hram, sizeof(m->hram));
    err |= write_all(f, m->io, sizeof(m->io));
    err |= write_all(f, &m->ie, 1);
    err |= write_all(f, &m->timer_div, 2);
    err |= write_all(f, &m->rtc_s, 1);
    err |= write_all(f, &m->rtc_m, 1);
    err |= write_all(f, &m->rtc_h, 1);
    err |= write_all(f, &m->rtc_dl, 1);
    err |= write_all(f, &m->rtc_dh, 1);
    err |= write_all(f, m->rtc_latched, sizeof(m->rtc_latched));
    err |= write_all(f, &m->rtc_latch_prev, 1);
    err |= write_all(f, &m->joypad_state, 1);
    err |= write_all(f, &m->apu_trigger_flags, 1);
    return err;
}

static int read_mem_fields(FILE *f, gb_mem_t *m)
{
    uint32_t stored;
    if (read_all(f, &stored, sizeof(stored)) != 0)
        return -1;

    long start = ftell(f);
    int err = 0;
    err |= read_all(f, &m->rom_bank, 1);
    err |= read_all(f, &m->cart_type, 1);
    err |= read_all(f, &m->mbc5_rom_bank_high, 1);
    err |= read_all(f, m->ext_ram, sizeof(m->ext_ram));
    err |= read_all(f, &m->ext_ram_bank, 1);
    err |= read_all(f, &m->ext_ram_enabled, 1);
    err |= read_all(f, m->wram, sizeof(m->wram));
    err |= read_all(f, m->vram, sizeof(m->vram));
    err |= read_all(f, m->oam, sizeof(m->oam));
    err |= read_all(f, m->hram, sizeof(m->hram));
    err |= read_all(f, m->io, sizeof(m->io));
    err |= read_all(f, &m->ie, 1);
    err |= read_all(f, &m->timer_div, 2);
    err |= read_all(f, &m->rtc_s, 1);
    err |= read_all(f, &m->rtc_m, 1);
    err |= read_all(f, &m->rtc_h, 1);
    err |= read_all(f, &m->rtc_dl, 1);
    err |= read_all(f, &m->rtc_dh, 1);
    err |= read_all(f, m->rtc_latched, sizeof(m->rtc_latched));
    err |= read_all(f, &m->rtc_latch_prev, 1);
    err |= read_all(f, &m->joypad_state, 1);
    err |= read_all(f, &m->apu_trigger_flags, 1);

    long consumed = ftell(f) - start;
    if (consumed >= 0 && (uint32_t)consumed < stored)
        fseek(f, (long)(stored - (uint32_t)consumed), SEEK_CUR);

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

    bst_header_t hdr;
    memcpy(hdr.magic, BST_MAGIC, 4);
    hdr.version    = BST_VERSION;
    hdr.console_id = BST_CONSOLE_GB;
    hdr.rom_crc32  = bitemu_crc32(m->rom, m->rom_size);
    hdr.state_size = 0;

    int err = 0;
    err |= write_all(f, &hdr, sizeof(hdr));
    err |= write_section(f, &impl->cpu, (uint32_t)sizeof(gb_cpu_t));
    err |= write_section(f, &impl->ppu, (uint32_t)sizeof(gb_ppu_t));
    err |= write_section(f, &impl->apu, (uint32_t)sizeof(gb_apu_t));
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
    if (read_all(f, &hdr, sizeof(hdr)) != 0)
    {
        fclose(f);
        return -1;
    }

    if (memcmp(hdr.magic, BST_MAGIC, 4) != 0 ||
        hdr.console_id != BST_CONSOLE_GB)
    {
        log_error("Invalid save state header: %s", path);
        fclose(f);
        return -1;
    }

    if (hdr.version < BST_VERSION)
    {
        log_error("Save state v%u is obsolete (need v%u). "
                  "Please load the ROM and re-save.", hdr.version, BST_VERSION);
        fclose(f);
        return -1;
    }

    if (hdr.version > BST_VERSION)
    {
        log_error("Save state v%u was created by a newer Bitemu (current: v%u). "
                  "Please update Bitemu.", hdr.version, BST_VERSION);
        fclose(f);
        return -1;
    }

    uint32_t current_crc = bitemu_crc32(m->rom, m->rom_size);
    if (hdr.rom_crc32 != current_crc)
    {
        log_error("ROM CRC mismatch in state file (expected %08X, got %08X)",
                  current_crc, hdr.rom_crc32);
        fclose(f);
        return -1;
    }

    uint8_t *saved_rom = m->rom;
    size_t saved_rom_size = m->rom_size;
    void *saved_audio = impl->audio_output;

    int err = 0;
    err |= read_section(f, &impl->cpu, (uint32_t)sizeof(gb_cpu_t));
    err |= read_section(f, &impl->ppu, (uint32_t)sizeof(gb_ppu_t));
    err |= read_section(f, &impl->apu, (uint32_t)sizeof(gb_apu_t));
    err |= read_mem_fields(f, m);

    fclose(f);

    m->rom = saved_rom;
    m->rom_size = saved_rom_size;
    impl->audio_output = saved_audio;

    if (err)
    {
        log_error("Error reading save state: %s", path);
        return -1;
    }
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
};
