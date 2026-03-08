/**
 * Tests: Public bitemu API (bitemu.h).
 * Create/destroy, audio init/shutdown, unload, specs.
 */

#include "test_harness.h"
#include "bitemu.h"
#include <stdio.h>
#include <string.h>

/* --- Create returns valid handle --- */

TEST(api_create_destroy)
{
    bitemu_t *emu = bitemu_create();
    ASSERT_TRUE(emu != NULL);
    bitemu_destroy(emu);
}

/* --- Destroy NULL is safe --- */

TEST(api_destroy_null)
{
    bitemu_destroy(NULL);
    ASSERT_TRUE(1);
}

/* --- Load ROM with invalid path fails --- */

TEST(api_load_rom_invalid)
{
    bitemu_t *emu = bitemu_create();
    ASSERT_FALSE(bitemu_load_rom(emu, "/nonexistent/rom.gb"));
    bitemu_destroy(emu);
}

/* --- Load ROM with NULL args fails --- */

TEST(api_load_rom_null)
{
    ASSERT_FALSE(bitemu_load_rom(NULL, "test.gb"));

    bitemu_t *emu = bitemu_create();
    ASSERT_FALSE(bitemu_load_rom(emu, NULL));
    bitemu_destroy(emu);
}

/* --- Audio init and shutdown --- */

TEST(api_audio_init_shutdown)
{
    bitemu_t *emu = bitemu_create();
    int rc = bitemu_audio_init(emu, BITEMU_AUDIO_BACKEND_NULL, NULL);
    ASSERT_EQ(rc, 0);
    bitemu_audio_shutdown(emu);
    bitemu_destroy(emu);
}

/* --- Audio init NULL emu fails --- */

TEST(api_audio_init_null)
{
    int rc = bitemu_audio_init(NULL, BITEMU_AUDIO_BACKEND_NULL, NULL);
    ASSERT_EQ(rc, -1);
}

/* --- Double audio shutdown is safe --- */

TEST(api_audio_double_shutdown)
{
    bitemu_t *emu = bitemu_create();
    bitemu_audio_init(emu, BITEMU_AUDIO_BACKEND_NULL, NULL);
    bitemu_audio_shutdown(emu);
    bitemu_audio_shutdown(emu);
    ASSERT_TRUE(1);
    bitemu_destroy(emu);
}

/* --- Read audio with no init returns 0 --- */

TEST(api_read_audio_no_init)
{
    bitemu_t *emu = bitemu_create();
    int16_t buf[64];
    int n = bitemu_read_audio(emu, buf, 64);
    ASSERT_EQ(n, 0);
    bitemu_destroy(emu);
}

/* --- Audio spec returns correct defaults --- */

TEST(api_audio_spec)
{
    int rate = 0, chans = 0;
    bitemu_get_audio_spec(&rate, &chans);
    ASSERT_EQ(rate, 44100);
    ASSERT_EQ(chans, 1);
}

/* --- Unload ROM without load is safe --- */

TEST(api_unload_no_rom)
{
    bitemu_t *emu = bitemu_create();
    bitemu_unload_rom(emu);
    ASSERT_TRUE(1);
    bitemu_destroy(emu);
}

/* --- Unload NULL emu is safe --- */

TEST(api_unload_null)
{
    bitemu_unload_rom(NULL);
    ASSERT_TRUE(1);
}

/* --- Reset without load is safe --- */

TEST(api_reset_no_rom)
{
    bitemu_t *emu = bitemu_create();
    bitemu_reset(emu);
    ASSERT_TRUE(1);
    bitemu_destroy(emu);
}

/* --- is_running after create --- */

TEST(api_is_running)
{
    bitemu_t *emu = bitemu_create();
    ASSERT_TRUE(bitemu_is_running(emu));
    bitemu_stop(emu);
    ASSERT_FALSE(bitemu_is_running(emu));
    bitemu_destroy(emu);
}

/* --- Framebuffer returns non-NULL --- */

TEST(api_framebuffer)
{
    bitemu_t *emu = bitemu_create();
    const uint8_t *fb = bitemu_get_framebuffer(emu);
    ASSERT_TRUE(fb != NULL);
    bitemu_destroy(emu);
}

/* --- Save state without ROM fails --- */

TEST(api_save_state_no_rom)
{
    bitemu_t *emu = bitemu_create();
    int rc = bitemu_save_state(emu, "/tmp/bitemu_test.bst");
    ASSERT_EQ(rc, -1);
    bitemu_destroy(emu);
}

/* --- Save/load state NULL args fail --- */

TEST(api_save_state_null)
{
    ASSERT_EQ(bitemu_save_state(NULL, "/tmp/x.bst"), -1);

    bitemu_t *emu = bitemu_create();
    ASSERT_EQ(bitemu_save_state(emu, NULL), -1);
    bitemu_destroy(emu);
}

TEST(api_load_state_null)
{
    ASSERT_EQ(bitemu_load_state(NULL, "/tmp/x.bst"), -1);

    bitemu_t *emu = bitemu_create();
    ASSERT_EQ(bitemu_load_state(emu, NULL), -1);
    bitemu_destroy(emu);
}

/* --- Load state with invalid path fails --- */

TEST(api_load_state_invalid_path)
{
    bitemu_t *emu = bitemu_create();
    int rc = bitemu_load_state(emu, "/nonexistent/state.bst");
    ASSERT_EQ(rc, -1);
    bitemu_destroy(emu);
}

/* --- Save and load round-trip with a fake ROM --- */

static const char *FAKE_ROM_PATH = "/tmp/bitemu_test_rom.gb";
static const char *FAKE_ROM2_PATH = "/tmp/bitemu_test_rom2.gb";

static void write_fake_rom(const char *path, uint8_t fill, size_t size)
{
    FILE *f = fopen(path, "wb");
    if (!f) return;
    uint8_t buf[512];
    memset(buf, fill, sizeof(buf));
    buf[0x147] = 0x00;
    size_t n = size < sizeof(buf) ? size : sizeof(buf);
    fwrite(buf, 1, n, f);
    fclose(f);
}

static bitemu_t *create_with_rom(const char *rom_path)
{
    bitemu_t *emu = bitemu_create();
    bitemu_load_rom(emu, rom_path);
    return emu;
}

TEST(api_save_load_roundtrip)
{
    const char *state_path = "/tmp/bitemu_test_roundtrip.bst";
    write_fake_rom(FAKE_ROM_PATH, 0x00, 512);
    bitemu_t *emu = create_with_rom(FAKE_ROM_PATH);

    /* Modify framebuffer before saving */
    uint8_t *fb = (uint8_t *)bitemu_get_framebuffer(emu);
    fb[0] = 0xAB;
    fb[100] = 0xCD;

    ASSERT_EQ(bitemu_save_state(emu, state_path), 0);

    /* Overwrite to verify load restores it */
    fb[0] = 0x00;
    fb[100] = 0x00;

    ASSERT_EQ(bitemu_load_state(emu, state_path), 0);

    fb = (uint8_t *)bitemu_get_framebuffer(emu);
    ASSERT_EQ(fb[0], 0xAB);
    ASSERT_EQ(fb[100], 0xCD);

    bitemu_destroy(emu);
    remove(state_path);
    remove(FAKE_ROM_PATH);
}

/* --- Load state with different ROM fails (CRC mismatch) --- */

TEST(api_load_state_crc_mismatch)
{
    const char *state_path = "/tmp/bitemu_test_crc.bst";
    write_fake_rom(FAKE_ROM_PATH, 0x00, 512);
    write_fake_rom(FAKE_ROM2_PATH, 0xFF, 512);

    bitemu_t *emu1 = create_with_rom(FAKE_ROM_PATH);
    ASSERT_EQ(bitemu_save_state(emu1, state_path), 0);
    bitemu_destroy(emu1);

    bitemu_t *emu2 = create_with_rom(FAKE_ROM2_PATH);
    ASSERT_EQ(bitemu_load_state(emu2, state_path), -1);

    bitemu_destroy(emu2);
    remove(state_path);
    remove(FAKE_ROM_PATH);
    remove(FAKE_ROM2_PATH);
}

void run_api_tests(void)
{
    SUITE("Public API");
    RUN(api_create_destroy);
    RUN(api_destroy_null);
    RUN(api_load_rom_invalid);
    RUN(api_load_rom_null);
    RUN(api_audio_init_shutdown);
    RUN(api_audio_init_null);
    RUN(api_audio_double_shutdown);
    RUN(api_read_audio_no_init);
    RUN(api_audio_spec);
    RUN(api_unload_no_rom);
    RUN(api_unload_null);
    RUN(api_reset_no_rom);
    RUN(api_is_running);
    RUN(api_framebuffer);
    RUN(api_save_state_no_rom);
    RUN(api_save_state_null);
    RUN(api_load_state_null);
    RUN(api_load_state_invalid_path);
    RUN(api_save_load_roundtrip);
    RUN(api_load_state_crc_mismatch);
}
