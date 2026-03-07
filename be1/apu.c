/**
 * Bitemu - Game Boy APU
 *
 * Canales: Square1, Square2, Wave, Noise.
 * Temporizadores por canal cuentan hacia abajo; al llegar a 0 avanzan
 * la posición de duty/onda/LFSR y recargan el periodo.
 *
 * Frame sequencer (512 Hz): controla length counters (256 Hz) y
 * volume envelope (64 Hz).  Sweep (ch1, 128 Hz) queda como TODO.
 */

#include "apu.h"
#include "memory.h"
#include "gb_constants.h"
#include "audio/audio.h"

static const uint8_t gb_duty[APU_DUTY_PATTERNS][APU_DUTY_STEPS] = 
{
    {0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 1, 1, 1},
    {0, 1, 1, 1, 1, 1, 1, 0},
};

void gb_apu_init(gb_apu_t *apu)
{
    apu->sample_clock = 0;
    apu->frame_seq_cycles = 0;
    apu->frame_seq_step = 0;
    apu->sq1_duty_pos = 0;
    apu->sq2_duty_pos = 0;
    apu->wave_pos = 0;
    apu->lfsr = APU_LFSR_INIT;
    apu->sq1_timer = 0;
    apu->sq2_timer = 0;
    apu->wave_timer = 0;
    apu->noise_timer = 0;
    apu->sq1_env_vol = 0;
    apu->sq2_env_vol = 0;
    apu->noise_env_vol = 0;
    apu->sq1_env_timer = 0;
    apu->sq2_env_timer = 0;
    apu->noise_env_timer = 0;
    apu->sq1_length = 0;
    apu->sq2_length = 0;
    apu->wave_length = 0;
    apu->noise_length = 0;
}

/* ----- Frame sequencer helpers ----- */

static void clock_length(gb_apu_t *apu, gb_mem_t *m)
{
    if ((m->io[GB_IO_NR14] & APU_LENGTH_ENABLE) && apu->sq1_length > 0)
        if (--apu->sq1_length == 0)
            m->io[GB_IO_NR52] &= ~APU_NR52_CH1;

    if ((m->io[GB_IO_NR24] & APU_LENGTH_ENABLE) && apu->sq2_length > 0)
        if (--apu->sq2_length == 0)
            m->io[GB_IO_NR52] &= ~APU_NR52_CH2;

    if ((m->io[GB_IO_NR34] & APU_LENGTH_ENABLE) && apu->wave_length > 0)
        if (--apu->wave_length == 0)
            m->io[GB_IO_NR52] &= ~APU_NR52_CH3;

    if ((m->io[GB_IO_NR44] & APU_LENGTH_ENABLE) && apu->noise_length > 0)
        if (--apu->noise_length == 0)
            m->io[GB_IO_NR52] &= ~APU_NR52_CH4;
}

static void clock_envelope_ch(uint8_t *vol, uint8_t *timer, uint8_t nrx2)
{
    int period = nrx2 & APU_ENV_PERIOD_MASK;
    if (period == 0)
        return;
    if (*timer > 0)
        (*timer)--;
    if (*timer == 0)
    {
        *timer = (uint8_t)period;
        if (nrx2 & APU_ENV_DIR_BIT)
        {
            if (*vol < APU_DAC_MAX) (*vol)++;
        }
        else
        {
            if (*vol > 0) (*vol)--;
        }
    }
}

static void clock_envelope(gb_apu_t *apu, gb_mem_t *m)
{
    clock_envelope_ch(&apu->sq1_env_vol, &apu->sq1_env_timer, m->io[GB_IO_NR12]);
    clock_envelope_ch(&apu->sq2_env_vol, &apu->sq2_env_timer, m->io[GB_IO_NR22]);
    clock_envelope_ch(&apu->noise_env_vol, &apu->noise_env_timer, m->io[GB_IO_NR42]);
}

/* ----- Trigger ----- */

void gb_apu_trigger(gb_apu_t *apu, struct gb_mem *mem, int channel)
{
    gb_mem_t *m = (gb_mem_t *)mem;
    switch (channel)
    {
    case 1: {
        int fv = ((m->io[GB_IO_NR14] & APU_FREQ_HI_MASK) << 8) | m->io[GB_IO_NR13];
        apu->sq1_timer = (APU_FREQ_BASE - fv) * APU_SQUARE_PERIOD_MUL;
        apu->sq1_duty_pos = 0;
        uint8_t nr12 = m->io[GB_IO_NR12];
        apu->sq1_env_vol = (nr12 >> APU_ENV_VOL_POS) & APU_NIBBLE_MASK;
        apu->sq1_env_timer = nr12 & APU_ENV_PERIOD_MASK;
        if (apu->sq1_length == 0)
            apu->sq1_length = APU_SQ_LENGTH_MAX;
        break;
    }
    case 2: {
        int fv = ((m->io[GB_IO_NR24] & APU_FREQ_HI_MASK) << 8) | m->io[GB_IO_NR23];
        apu->sq2_timer = (APU_FREQ_BASE - fv) * APU_SQUARE_PERIOD_MUL;
        apu->sq2_duty_pos = 0;
        uint8_t nr22 = m->io[GB_IO_NR22];
        apu->sq2_env_vol = (nr22 >> APU_ENV_VOL_POS) & APU_NIBBLE_MASK;
        apu->sq2_env_timer = nr22 & APU_ENV_PERIOD_MASK;
        if (apu->sq2_length == 0)
            apu->sq2_length = APU_SQ_LENGTH_MAX;
        break;
    }
    case 3: {
        int fv = ((m->io[GB_IO_NR34] & APU_FREQ_HI_MASK) << 8) | m->io[GB_IO_NR33];
        apu->wave_timer = (APU_FREQ_BASE - fv) * APU_WAVE_PERIOD_MUL;
        apu->wave_pos = 0;
        if (apu->wave_length == 0)
            apu->wave_length = APU_WAVE_LENGTH_MAX;
        break;
    }
    case 4: {
        uint8_t nr43 = m->io[GB_IO_NR43];
        int div = (nr43 & APU_NOISE_DIV_MASK) * APU_NOISE_DIV_MUL;
        if (div == 0) div = APU_NOISE_DIV_ZERO;
        apu->noise_timer = div << ((nr43 >> APU_NOISE_SHIFT_POS) & APU_NIBBLE_MASK);
        apu->lfsr = APU_LFSR_INIT;
        uint8_t nr42 = m->io[GB_IO_NR42];
        apu->noise_env_vol = (nr42 >> APU_ENV_VOL_POS) & APU_NIBBLE_MASK;
        apu->noise_env_timer = nr42 & APU_ENV_PERIOD_MASK;
        if (apu->noise_length == 0)
            apu->noise_length = APU_SQ_LENGTH_MAX;
        break;
    }
    }
}

/* ----- Step (per CPU cycle) ----- */

void gb_apu_step(gb_apu_t *apu, struct gb_mem *mem, int cycles)
{
    gb_mem_t *m = (gb_mem_t *)mem;
    if (!(m->io[GB_IO_NR52] & APU_NR52_POWER))
        return;

    if (m->apu_trigger_flags)
    {
        for (int ch = 1; ch <= 4; ch++)
            if (m->apu_trigger_flags & (1 << (ch - 1)))
                gb_apu_trigger(apu, mem, ch);
        m->apu_trigger_flags = 0;
    }

    for (int i = 0; i < cycles; i++)
    {
        /* Frame sequencer: 512 Hz tick */
        if (++apu->frame_seq_cycles >= APU_FRAME_SEQ_PERIOD)
        {
            apu->frame_seq_cycles = 0;
            switch (apu->frame_seq_step)
            {
            case 0: case 4:
                clock_length(apu, m);
                break;
            case 2: case 6:
                clock_length(apu, m);
                /* TODO: ch1 frequency sweep */
                break;
            case 7:
                clock_envelope(apu, m);
                break;
            }
            apu->frame_seq_step = (apu->frame_seq_step + 1)
                                & (APU_FRAME_SEQ_STEPS - 1);
        }

        /* Channel frequency timers */
        if (--apu->sq1_timer <= 0)
        {
            int fv = ((m->io[GB_IO_NR14] & APU_FREQ_HI_MASK) << 8)
                   | m->io[GB_IO_NR13];
            apu->sq1_timer = (APU_FREQ_BASE - fv) * APU_SQUARE_PERIOD_MUL;
            if (apu->sq1_timer <= 0)
                apu->sq1_timer = APU_SQUARE_PERIOD_MUL;
            apu->sq1_duty_pos = (apu->sq1_duty_pos + 1) & APU_DUTY_STEP_MASK;
        }

        if (--apu->sq2_timer <= 0)
        {
            int fv = ((m->io[GB_IO_NR24] & APU_FREQ_HI_MASK) << 8)
                   | m->io[GB_IO_NR23];
            apu->sq2_timer = (APU_FREQ_BASE - fv) * APU_SQUARE_PERIOD_MUL;
            if (apu->sq2_timer <= 0)
                apu->sq2_timer = APU_SQUARE_PERIOD_MUL;
            apu->sq2_duty_pos = (apu->sq2_duty_pos + 1) & APU_DUTY_STEP_MASK;
        }

        if (--apu->wave_timer <= 0)
        {
            int fv = ((m->io[GB_IO_NR34] & APU_FREQ_HI_MASK) << 8)
                   | m->io[GB_IO_NR33];
            apu->wave_timer = (APU_FREQ_BASE - fv) * APU_WAVE_PERIOD_MUL;
            if (apu->wave_timer <= 0)
                apu->wave_timer = APU_WAVE_PERIOD_MUL;
            apu->wave_pos = (apu->wave_pos + 1) & APU_WAVE_STEP_MASK;
        }

        if (--apu->noise_timer <= 0)
        {
            uint8_t nr43 = m->io[GB_IO_NR43];
            int div = (nr43 & APU_NOISE_DIV_MASK) * APU_NOISE_DIV_MUL;
            if (div == 0)
                div = APU_NOISE_DIV_ZERO;
            apu->noise_timer = div << ((nr43 >> APU_NOISE_SHIFT_POS) & APU_NIBBLE_MASK);
            if (apu->noise_timer <= 0)
                apu->noise_timer = 1;

            uint16_t bit = (apu->lfsr ^ (apu->lfsr >> 1)) & 1;
            apu->lfsr = (apu->lfsr >> 1) | (bit << APU_LFSR_TAP_BIT);
            if (nr43 & APU_NOISE_WIDTH_BIT)
                apu->lfsr = (apu->lfsr & APU_LFSR_SHORT_CLR) | (bit << APU_LFSR_SHORT_TAP);
        }
    }
}

/* ----- Channel outputs (use envelope volume) ----- */

static int apu_square1_output(gb_apu_t *apu, gb_mem_t *m)
{
    if (!(m->io[GB_IO_NR52] & APU_NR52_CH1))
        return 0;
    int duty = (m->io[GB_IO_NR11] >> APU_DUTY_POS) & APU_DUTY_MASK;
    int vol = apu->sq1_env_vol;
    if (vol == 0)
        return 0;
    return gb_duty[duty][apu->sq1_duty_pos & APU_DUTY_STEP_MASK] ? vol : 0;
}

static int apu_square2_output(gb_apu_t *apu, gb_mem_t *m)
{
    if (!(m->io[GB_IO_NR52] & APU_NR52_CH2))
        return 0;
    int duty = (m->io[GB_IO_NR21] >> APU_DUTY_POS) & APU_DUTY_MASK;
    int vol = apu->sq2_env_vol;
    if (vol == 0)
        return 0;
    return gb_duty[duty][apu->sq2_duty_pos & APU_DUTY_STEP_MASK] ? vol : 0;
}

static int apu_wave_output(gb_apu_t *apu, gb_mem_t *m)
{
    uint8_t nr30 = m->io[GB_IO_NR30];
    if (!(m->io[GB_IO_NR52] & APU_NR52_CH3) || !(nr30 & APU_WAVE_DAC_ON))
        return 0;
    uint8_t nr32 = m->io[GB_IO_NR32];
    int shift = (nr32 >> APU_WAVE_VOL_POS) & APU_WAVE_VOL_MASK;
    if (shift == 0)
        return 0;
    int pos = apu->wave_pos & APU_WAVE_STEP_MASK;
    int byte_idx = pos >> 1;
    int nibble = pos & 1;
    uint8_t b = m->io[APU_WAVE_RAM_OFFSET + byte_idx];
    int sample = (nibble == 0) ? (b >> APU_NIBBLE_SHIFT) : (b & APU_NIBBLE_MASK);
    sample >>= (shift - 1);
    return sample > APU_DAC_MAX ? APU_DAC_MAX : sample;
}

static int apu_noise_output(gb_apu_t *apu, gb_mem_t *m)
{
    if (!(m->io[GB_IO_NR52] & APU_NR52_CH4))
        return 0;
    int vol = apu->noise_env_vol;
    if (vol == 0)
        return 0;
    return (apu->lfsr & 1) ? 0 : vol;
}

/* ----- Mixing ----- */

static int16_t apply_volume_and_pan(int ch1, int ch2, int ch3, int ch4,
    uint8_t nr50, uint8_t nr51, int left)
{
    int vol = left
        ? (nr50 & APU_MASTER_VOL_MASK) + 1
        : ((nr50 >> APU_MASTER_VOL_R_POS) & APU_MASTER_VOL_MASK) + 1;
    int shift = left ? 0 : APU_PAN_RIGHT_SHIFT;

    int sum = 0;
    int active = 0;
    if (nr51 & (1 << shift)) { sum += ch1; active++; }
    if (nr51 & (2 << shift)) { sum += ch2; active++; }
    if (nr51 & (4 << shift)) { sum += ch3; active++; }
    if (nr51 & (8 << shift)) { sum += ch4; active++; }

    if (active == 0)
        return 0;

    int centered = sum * 2 - active * APU_DAC_MAX;
    int32_t s = (int32_t)centered * vol * APU_MIX_SCALE;
    if (s > APU_SAMPLE_MAX)  s = APU_SAMPLE_MAX;
    if (s < APU_SAMPLE_MIN)  s = APU_SAMPLE_MIN;
    return (int16_t)s;
}

void apu_mix_sample(gb_apu_t *apu, struct gb_mem *mem, bitemu_audio_t *audio)
{
    if (!audio || !audio->enabled || !audio->buffer)
        return;

    size_t buf_len = audio->buffer_size;
    if (buf_len == 0) return;

    size_t wr = audio->write_pos;
    size_t rd = audio->read_pos;
    size_t used = (wr >= rd) ? (wr - rd) : (buf_len - rd + wr);
    if (used >= buf_len - (audio->channels == 2 ? 2u : 1u))
        return;

    gb_mem_t *m = (gb_mem_t *)mem;
    uint8_t nr50 = m->io[GB_IO_NR50];
    uint8_t nr51 = m->io[GB_IO_NR51];

    int ch1 = apu_square1_output(apu, m);
    int ch2 = apu_square2_output(apu, m);
    int ch3 = apu_wave_output(apu, m);
    int ch4 = apu_noise_output(apu, m);

    int16_t left  = apply_volume_and_pan(ch1, ch2, ch3, ch4, nr50, nr51, 1);
    int16_t right = apply_volume_and_pan(ch1, ch2, ch3, ch4, nr50, nr51, 0);

    if (audio->channels == 2)
    {
        audio->buffer[audio->write_pos % buf_len] = left;
        audio->write_pos = (audio->write_pos + 1) % buf_len;
        audio->buffer[audio->write_pos % buf_len] = right;
        audio->write_pos = (audio->write_pos + 1) % buf_len;
    }
    else
    {
        audio->buffer[audio->write_pos % buf_len] = (left + right) / 2;
        audio->write_pos = (audio->write_pos + 1) % buf_len;
    }
}
