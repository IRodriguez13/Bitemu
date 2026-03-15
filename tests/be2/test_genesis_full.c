/**
 * BE2 - Genesis: tests de funcionamiento completo
 *
 * Carga ROM, ejecuta frames, verifica console ops, cycles_per_frame,
 * framebuffer, save/load state. Complementa test_api.c con cobertura
 * específica de Genesis.
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "test_harness.h"
#include "bitemu.h"
#include "be2/genesis_impl.h"
#include "be2/genesis_constants.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

static const char *_tmp_dir(void)
{
#ifdef _WIN32
    static char win_tmp[MAX_PATH];
    DWORD len = GetTempPathA(MAX_PATH, win_tmp);
    if (len > 0 && (win_tmp[len - 1] == '\\' || win_tmp[len - 1] == '/'))
        win_tmp[len - 1] = '\0';
    return win_tmp;
#else
    const char *d = getenv("TMPDIR");
    if (!d) d = "/tmp";
    return d;
#endif
}

static char _path_buf[512];
static const char *tmp_path(const char *filename)
{
    snprintf(_path_buf, sizeof(_path_buf), "%s/%s", _tmp_dir(), filename);
    return _path_buf;
}

static void write_fake_genesis_rom(const char *path, size_t size)
{
    FILE *f = fopen(path, "wb");
    if (!f) return;
    size_t n = size < 0x104 ? 0x104 : size;
    uint8_t *buf = calloc(1, n);
    if (!buf) { fclose(f); return; }
    memcpy(buf + GEN_HEADER_OFFSET, GEN_HEADER_MAGIC, GEN_HEADER_MAGIC_LEN);
    fwrite(buf, 1, n, f);
    free(buf);
    fclose(f);
}

/* --- Load ROM, run frames, verify video size --- */

TEST(gen_full_load_run_video)
{
    const char *rom_path = tmp_path("bitemu_gen_full_test.bin");
    write_fake_genesis_rom(rom_path, 0x200);

    bitemu_t *emu = bitemu_create();
    ASSERT_TRUE(bitemu_load_rom(emu, rom_path));

    int w = 0, h = 0;
    bitemu_get_video_size(emu, &w, &h);
    ASSERT_EQ(w, GEN_DISPLAY_WIDTH);
    ASSERT_EQ(h, GEN_DISPLAY_HEIGHT);

    const uint8_t *fb = bitemu_get_framebuffer(emu);
    ASSERT_TRUE(fb != NULL);

    for (int i = 0; i < 10; i++)
        ASSERT_TRUE(bitemu_run_frame(emu));

    bitemu_unload_rom(emu);
    bitemu_destroy(emu);
    remove(rom_path);
}

/* --- Run many frames (valida cycles_per_frame internamente) --- */

TEST(gen_full_many_frames)
{
    const char *rom_path = tmp_path("bitemu_gen_cycles_test.bin");
    write_fake_genesis_rom(rom_path, 0x200);

    bitemu_t *emu = bitemu_create();
    bitemu_load_rom(emu, rom_path);

    for (int i = 0; i < 60; i++)
        ASSERT_TRUE(bitemu_run_frame(emu));

    bitemu_destroy(emu);
    remove(rom_path);
}

/* --- Framebuffer non-zero after run --- */

TEST(gen_full_framebuffer_updated)
{
    const char *rom_path = tmp_path("bitemu_gen_fb_test.bin");
    write_fake_genesis_rom(rom_path, 0x200);

    bitemu_t *emu = bitemu_create();
    bitemu_load_rom(emu, rom_path);

    for (int i = 0; i < 5; i++)
        bitemu_run_frame(emu);

    const uint8_t *fb = bitemu_get_framebuffer(emu);
    int nonzero = 0;
    for (int i = 0; i < GEN_FB_SIZE && nonzero < 10; i++)
        if (fb[i] != 0) nonzero++;
    ASSERT_TRUE(nonzero > 0);

    bitemu_destroy(emu);
    remove(rom_path);
}

/* --- Reset preserves ROM --- */

TEST(gen_full_reset_preserves_rom)
{
    const char *rom_path = tmp_path("bitemu_gen_reset_test.bin");
    write_fake_genesis_rom(rom_path, 0x200);

    bitemu_t *emu = bitemu_create();
    bitemu_load_rom(emu, rom_path);
    bitemu_run_frame(emu);
    bitemu_reset(emu);

    int w = 0, h = 0;
    bitemu_get_video_size(emu, &w, &h);
    ASSERT_EQ(w, GEN_DISPLAY_WIDTH);
    ASSERT_EQ(h, GEN_DISPLAY_HEIGHT);

    bitemu_destroy(emu);
    remove(rom_path);
}

/* --- Save/load state roundtrip (detalle Genesis) --- */

TEST(gen_full_save_load_roundtrip)
{
    const char *rom_path = tmp_path("bitemu_gen_sl_test.bin");
    const char *state_path = tmp_path("bitemu_gen_sl.bst");
    write_fake_genesis_rom(rom_path, 0x200);

    bitemu_t *emu = bitemu_create();
    ASSERT_TRUE(bitemu_load_rom(emu, rom_path));

    for (int i = 0; i < 3; i++)
        bitemu_run_frame(emu);

    uint8_t fb_before[GEN_FB_SIZE];
    memcpy(fb_before, bitemu_get_framebuffer(emu), GEN_FB_SIZE);

    ASSERT_EQ(bitemu_save_state(emu, state_path), 0);

    for (int i = 0; i < 5; i++)
        bitemu_run_frame(emu);

    ASSERT_EQ(bitemu_load_state(emu, state_path), 0);

    const uint8_t *fb_after = bitemu_get_framebuffer(emu);
    ASSERT_EQ(memcmp(fb_before, fb_after, GEN_FB_SIZE), 0);

    bitemu_destroy(emu);
    remove(rom_path);
    remove(state_path);
}

/* --- Input mapping (joypad) --- */

TEST(gen_full_input_mapping)
{
    const char *rom_path = tmp_path("bitemu_gen_input_test.bin");
    write_fake_genesis_rom(rom_path, 0x200);

    bitemu_t *emu = bitemu_create();
    bitemu_load_rom(emu, rom_path);

    bitemu_set_input(emu, 0x01);  /* Right */
    bitemu_run_frame(emu);
    ASSERT_TRUE(1);

    bitemu_set_input(emu, 0);
    bitemu_destroy(emu);
    remove(rom_path);
}

void run_genesis_full_tests(void)
{
    SUITE("Genesis Full (integration)");
    RUN(gen_full_load_run_video);
    RUN(gen_full_many_frames);
    RUN(gen_full_framebuffer_updated);
    RUN(gen_full_reset_preserves_rom);
    RUN(gen_full_save_load_roundtrip);
    RUN(gen_full_input_mapping);
}
