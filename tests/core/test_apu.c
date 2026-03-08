/**
 * Tests: Game Boy APU subsystem.
 * Trigger, envelope, length counter, frame sequencer, mixer.
 */

#include "test_harness.h"
#include "be1/apu.h"
#include "be1/memory.h"
#include "be1/gb_constants.h"
#include "be1/audio/audio.h"

static gb_apu_t apu;
static gb_mem_t mem;
static bitemu_audio_t audio;

static void setup(void)
{
    memset(&mem, 0, sizeof(mem));
    gb_mem_init(&mem);
    gb_apu_init(&apu);
    memset(&audio, 0, sizeof(audio));
    audio.buffer = calloc(4096, sizeof(int16_t));
    audio.buffer_size = 4096;
    audio.enabled = true;
    audio.sample_rate = GB_AUDIO_SAMPLE_RATE;
    audio.channels = 1;
    mem.io[GB_IO_NR52] = APU_NR52_POWER;
}

static void teardown(void)
{
    free(audio.buffer);
    audio.buffer = NULL;
}

/* --- Init zeroes state --- */

TEST(apu_init_zeroes)
{
    gb_apu_t a;
    gb_apu_init(&a);
    ASSERT_EQ(a.sample_clock, 0);
    ASSERT_EQ(a.sq1_duty_pos, 0);
    ASSERT_EQ(a.sq2_duty_pos, 0);
    ASSERT_EQ(a.wave_pos, 0);
    ASSERT_EQ(a.lfsr, APU_LFSR_INIT);
    ASSERT_EQ(a.sq1_env_vol, 0);
    ASSERT_EQ(a.frame_seq_step, 0);
}

/* --- Trigger ch1 resets state --- */

TEST(trigger_ch1_resets_state)
{
    setup();
    mem.io[GB_IO_NR12] = 0xF3;  /* vol=15, decrease, period=3 */
    mem.io[GB_IO_NR13] = 0x00;
    mem.io[GB_IO_NR14] = 0x00;
    apu.sq1_duty_pos = 5;
    apu.sq1_length = 0;

    gb_apu_trigger(&apu, (struct gb_mem *)&mem, 1);

    ASSERT_EQ(apu.sq1_duty_pos, 0);
    ASSERT_EQ(apu.sq1_env_vol, 15);
    ASSERT_EQ(apu.sq1_env_timer, 3);
    ASSERT_EQ(apu.sq1_length, APU_SQ_LENGTH_MAX);
    ASSERT_TRUE(apu.sq1_timer > 0);
    teardown();
}

/* --- Trigger ch1 preserves non-zero length --- */

TEST(trigger_ch1_keeps_length)
{
    setup();
    mem.io[GB_IO_NR12] = 0xA0;
    apu.sq1_length = 20;

    gb_apu_trigger(&apu, (struct gb_mem *)&mem, 1);

    ASSERT_EQ(apu.sq1_length, 20);
    teardown();
}

/* --- Trigger ch2 sets envelope --- */

TEST(trigger_ch2_envelope)
{
    setup();
    mem.io[GB_IO_NR22] = 0x73;  /* vol=7, decrease, period=3 */

    gb_apu_trigger(&apu, (struct gb_mem *)&mem, 2);

    ASSERT_EQ(apu.sq2_env_vol, 7);
    ASSERT_EQ(apu.sq2_env_timer, 3);
    teardown();
}

/* --- Trigger ch3 (wave) resets position --- */

TEST(trigger_ch3_wave)
{
    setup();
    apu.wave_pos = 10;
    apu.wave_length = 0;

    gb_apu_trigger(&apu, (struct gb_mem *)&mem, 3);

    ASSERT_EQ(apu.wave_pos, 0);
    ASSERT_EQ(apu.wave_length, APU_WAVE_LENGTH_MAX);
    teardown();
}

/* --- Trigger ch4 (noise) resets LFSR --- */

TEST(trigger_ch4_noise)
{
    setup();
    apu.lfsr = 0x1234;
    apu.noise_length = 0;
    mem.io[GB_IO_NR42] = 0xB2;  /* vol=11, decrease, period=2 */
    mem.io[GB_IO_NR43] = 0x00;

    gb_apu_trigger(&apu, (struct gb_mem *)&mem, 4);

    ASSERT_EQ(apu.lfsr, APU_LFSR_INIT);
    ASSERT_EQ(apu.noise_env_vol, 11);
    ASSERT_EQ(apu.noise_length, APU_SQ_LENGTH_MAX);
    teardown();
}

/* --- Step with power off does nothing --- */

TEST(step_power_off_noop)
{
    setup();
    mem.io[GB_IO_NR52] = 0x00;
    apu.sq1_timer = 100;
    int old_timer = apu.sq1_timer;

    gb_apu_step(&apu, (struct gb_mem *)&mem, 100);

    ASSERT_EQ(apu.sq1_timer, old_timer);
    teardown();
}

/* --- Frame sequencer advances --- */

TEST(frame_seq_advances)
{
    setup();
    apu.frame_seq_cycles = 0;
    apu.frame_seq_step = 0;

    gb_apu_step(&apu, (struct gb_mem *)&mem, APU_FRAME_SEQ_PERIOD);

    ASSERT_EQ(apu.frame_seq_step, 1);
    teardown();
}

/* --- Frame sequencer wraps around after 8 steps --- */

TEST(frame_seq_wraps)
{
    setup();
    apu.frame_seq_step = 7;
    apu.frame_seq_cycles = 0;

    gb_apu_step(&apu, (struct gb_mem *)&mem, APU_FRAME_SEQ_PERIOD);

    ASSERT_EQ(apu.frame_seq_step, 0);
    teardown();
}

/* --- Length counter decrements and disables channel --- */

TEST(length_counter_disables_channel)
{
    setup();
    mem.io[GB_IO_NR52] = APU_NR52_POWER | APU_NR52_CH1;
    mem.io[GB_IO_NR14] = APU_LENGTH_ENABLE;
    apu.sq1_length = 1;
    apu.frame_seq_step = 0;
    apu.frame_seq_cycles = APU_FRAME_SEQ_PERIOD - 1;

    gb_apu_step(&apu, (struct gb_mem *)&mem, 1);

    ASSERT_EQ(apu.sq1_length, 0);
    ASSERT_FALSE(mem.io[GB_IO_NR52] & APU_NR52_CH1);
    teardown();
}

/* --- Mix sample writes to audio buffer --- */

TEST(mix_sample_writes_buffer)
{
    setup();
    mem.io[GB_IO_NR50] = 0x77;  /* max volume L+R */
    mem.io[GB_IO_NR51] = 0xFF;  /* all channels to both L+R */
    mem.io[GB_IO_NR52] = APU_NR52_POWER | APU_NR52_CH1;
    apu.sq1_env_vol = 15;

    ASSERT_EQ(audio.write_pos, 0);
    apu_mix_sample(&apu, (struct gb_mem *)&mem, &audio);
    ASSERT_TRUE(audio.write_pos > 0);
    teardown();
}

/* --- Mix sample respects buffer full condition --- */

TEST(mix_sample_buffer_full)
{
    setup();
    audio.write_pos = audio.buffer_size - 1;
    audio.read_pos = 0;
    size_t old_wp = audio.write_pos;

    apu_mix_sample(&apu, (struct gb_mem *)&mem, &audio);

    ASSERT_EQ(audio.write_pos, old_wp);
    teardown();
}

void run_apu_tests(void)
{
    SUITE("APU");
    RUN(apu_init_zeroes);
    RUN(trigger_ch1_resets_state);
    RUN(trigger_ch1_keeps_length);
    RUN(trigger_ch2_envelope);
    RUN(trigger_ch3_wave);
    RUN(trigger_ch4_noise);
    RUN(step_power_off_noop);
    RUN(frame_seq_advances);
    RUN(frame_seq_wraps);
    RUN(length_counter_disables_channel);
    RUN(mix_sample_writes_buffer);
    RUN(mix_sample_buffer_full);
}
