/**
 * Bitemu - Game Boy APU
 *
 * Canales: Square1, Square2, Wave, Noise. Lee registros NRxx de memoria.
 * Paso por ciclos: actualiza contadores de duty/onda/LFSR según frecuencias.
 * NR52 bit 7 = APU on; si está apagado no se avanza ningún canal.
 */

#include "apu.h"
#include "memory.h"
#include "gb_constants.h"
#include "audio/audio.h"

/* Wave pattern RAM: 0xFF30-0xFF3F (16 bytes = 32 muestras de 4 bits) */
#define GB_IO_WAVE_RAM  0x30

static const uint8_t gb_duty[4][8] = {
    {0, 0, 0, 0, 0, 0, 0, 1}, {1, 0, 0, 0, 0, 0, 0, 1}, {1, 0, 0, 0, 0, 1, 1, 1}, {0, 1, 1, 1, 1, 1, 1, 0}};

void gb_apu_init(gb_apu_t *apu)
{
    apu->cycle_counter = 0;
    apu->sample_clock = 0;
    apu->sq1_duty_pos = 0;
    apu->sq2_duty_pos = 0;
    apu->wave_pos = 0;
    apu->lfsr = 0x7FFF;
}


void gb_apu_step(gb_apu_t *apu, struct gb_mem *mem, int cycles)
{
    gb_mem_t *m = (gb_mem_t *)mem;
    uint8_t nr52 = m->io[GB_IO_NR52];
    if (!(nr52 & 0x80))
        return;

    uint8_t nr13 = m->io[GB_IO_NR13];
    uint8_t nr14 = m->io[GB_IO_NR14];
    uint8_t nr23 = m->io[GB_IO_NR23];
    uint8_t nr24 = m->io[GB_IO_NR24];
    uint8_t nr33 = m->io[GB_IO_NR33];
    uint8_t nr34 = m->io[GB_IO_NR34];
    uint8_t nr43 = m->io[GB_IO_NR43];
    uint8_t nr44 = m->io[GB_IO_NR44];

    for (int i = 0; i < cycles; i++)
    {
        apu->cycle_counter++;

        if (nr52 & 0x01 && (nr14 & 0x40))
        {
            int freq = 131072 / (2048 - ((nr14 & 7) << 8 | nr13));
            if (freq > 0 && apu->cycle_counter % (GB_CPU_HZ / (freq * 8)) == 0)
                apu->sq1_duty_pos = (apu->sq1_duty_pos + 1) & 7;
        }
        if (nr52 & 0x02 && (nr24 & 0x40))
        {
            int freq = 131072 / (2048 - ((nr24 & 7) << 8 | nr23));
            if (freq > 0 && apu->cycle_counter % (GB_CPU_HZ / (freq * 8)) == 0)
                apu->sq2_duty_pos = (apu->sq2_duty_pos + 1) & 7;
        }
        if (nr52 & 0x04 && (nr34 & 0x80))
        {
            int freq = 65536 / (2048 - ((nr34 & 7) << 8 | nr33));
            if (freq > 0 && apu->cycle_counter % (GB_CPU_HZ / (freq * 32)) == 0)
                apu->wave_pos = (apu->wave_pos + 1) & 31;
        }
        if (nr52 & 0x08 && (nr44 & 0x40))
        {
            int div = (nr43 & 7) * 16;
            if (div == 0)
                div = 8;
            int freq = GB_CPU_HZ / (div << (nr43 >> 4));
            if (freq > 0 && apu->cycle_counter % (GB_CPU_HZ / freq) == 0)
            {
                uint16_t bit = (apu->lfsr ^ (apu->lfsr >> 1)) & 1;
                apu->lfsr = (apu->lfsr >> 1) | (bit << 14);
                if (nr43 & 8)
                    apu->lfsr = (apu->lfsr & 0x7FBF) | (bit << 6);
            }
        }
    }
}

/* Salidas por canal: valor 0..15 (envelope/volumen del GB). Luego se escala a int16 en apply_volume_and_pan. */
static int apu_square1_output(gb_apu_t *apu, gb_mem_t *m)
{
    uint8_t nr52 = m->io[GB_IO_NR52];
    if (!(nr52 & 0x01))
        return 0;
    uint8_t nr11 = m->io[GB_IO_NR11];
    uint8_t nr12 = m->io[GB_IO_NR12];
    int duty = (nr11 >> 6) & 3;
    int vol = (nr12 >> 4) & 0x0F;
    if (vol == 0)
        return 0;
    return gb_duty[duty][apu->sq1_duty_pos & 7] ? vol : 0;
}

static int apu_square2_output(gb_apu_t *apu, gb_mem_t *m)
{
    uint8_t nr52 = m->io[GB_IO_NR52];
    if (!(nr52 & 0x02))
        return 0;
    uint8_t nr21 = m->io[GB_IO_NR21];
    uint8_t nr22 = m->io[GB_IO_NR22];
    int duty = (nr21 >> 6) & 3;
    int vol = (nr22 >> 4) & 0x0F;
    if (vol == 0)
        return 0;
    return gb_duty[duty][apu->sq2_duty_pos & 7] ? vol : 0;
}

static int apu_wave_output(gb_apu_t *apu, gb_mem_t *m)
{
    uint8_t nr52 = m->io[GB_IO_NR52];
    uint8_t nr30 = m->io[GB_IO_NR30];
    if (!(nr52 & 0x04) || !(nr30 & 0x80))
        return 0;
    uint8_t nr32 = m->io[GB_IO_NR32];
    int shift = (nr32 >> 5) & 3;  /* 0=mute, 1=100%, 2=50%, 3=25% */
    if (shift == 0)
        return 0;
    int pos = apu->wave_pos & 31;
    int byte_idx = pos >> 1;
    int nibble = pos & 1;
    uint8_t b = m->io[GB_IO_WAVE_RAM + byte_idx];
    int sample = (nibble == 0) ? (b >> 4) : (b & 0x0F);
    sample >>= (shift - 1);  /* 1->sample, 2->sample/2, 3->sample/4 */
    return sample > 15 ? 15 : sample;
}

static int apu_noise_output(gb_apu_t *apu, gb_mem_t *m)
{
    uint8_t nr52 = m->io[GB_IO_NR52];
    if (!(nr52 & 0x08))
        return 0;
    uint8_t nr42 = m->io[GB_IO_NR42];
    int vol = (nr42 >> 4) & 0x0F;
    if (vol == 0)
        return 0;
    /* Salida del LFSR: bit 0 en 0 = sonido, en 1 = silencio */
    return (apu->lfsr & 1) ? 0 : vol;
}

/* NR50: bits 3:0 = volumen L (0-7), bits 7:4 = volumen R. NR51: bit 0=ch1 L, 1=ch2 L, 2=ch3 L, 3=ch4 L; 4=ch1 R, 5=ch2 R, 6=ch3 R, 7=ch4 R. */
static int16_t apply_volume_and_pan(int ch1, int ch2, int ch3, int ch4,
    uint8_t nr50, uint8_t nr51, int left)
{
    int vol = left ? ((nr50 & 7) + 1) : (((nr50 >> 4) & 7) + 1);
    int shift = left ? 0 : 4;
    int sum = 0;
    if (nr51 & (1 << shift))     sum += ch1;
    if (nr51 & (2 << shift))    sum += ch2;
    if (nr51 & (4 << shift))    sum += ch3;
    if (nr51 & (8 << shift))    sum += ch4;
    /* sum en 0..60 (4 canales * 15). Escala a int16 sin recorte brutal: (sum-30)*vol*136 ≈ ±32640 */
    int32_t s = (int32_t)(sum - 30) * vol * 136;
    if (s > 32767)  s = 32767;
    if (s < -32768) s = -32768;
    return (int16_t)s;
}

void apu_mix_sample(gb_apu_t *apu, struct gb_mem *mem, bitemu_audio_t *audio)
{
    if (!audio || !audio->enabled || !audio->buffer)
        return;

    size_t buf_len = audio->buffer_size;
    if (buf_len == 0) return;

    /* No sobrescribir si el buffer está lleno (evita glitches por overflow) */
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
