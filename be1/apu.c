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

static const uint8_t gb_duty[4][8] = {
    {0, 0, 0, 0, 0, 0, 0, 1}, {1, 0, 0, 0, 0, 0, 0, 1}, {1, 0, 0, 0, 0, 1, 1, 1}, {0, 1, 1, 1, 1, 1, 1, 0}};

void gb_apu_init(gb_apu_t *apu)
{
    apu->cycle_counter = 0;
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

    uint8_t nr11 = m->io[GB_IO_NR11];
    uint8_t nr12 = m->io[GB_IO_NR12];
    uint8_t nr13 = m->io[GB_IO_NR13];
    uint8_t nr14 = m->io[GB_IO_NR14];
    uint8_t nr21 = m->io[GB_IO_NR21];
    uint8_t nr22 = m->io[GB_IO_NR22];
    uint8_t nr23 = m->io[GB_IO_NR23];
    uint8_t nr24 = m->io[GB_IO_NR24];
    uint8_t nr30 = m->io[GB_IO_NR30];
    uint8_t nr31 = m->io[GB_IO_NR31];
    uint8_t nr32 = m->io[GB_IO_NR32];
    uint8_t nr33 = m->io[GB_IO_NR33];
    uint8_t nr34 = m->io[GB_IO_NR34];
    uint8_t nr41 = m->io[GB_IO_NR41];
    uint8_t nr42 = m->io[GB_IO_NR42];
    uint8_t nr43 = m->io[GB_IO_NR43];
    uint8_t nr44 = m->io[GB_IO_NR44];
    uint8_t nr50 = m->io[GB_IO_NR50];
    uint8_t nr51 = m->io[GB_IO_NR51];

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

void apu_mix_sample(gb_apu_t *apu, bitemu_audio_t *audio)
{
    if(!audio->enabled) return;

    int16_t ch1 = apu_square1_output(apu);
    int16_t ch2 = apu_square2_output(apu);
    int16_t ch3 = apu_wave_output(apu);
    int16_t ch4 = apu_noise_output(apu);

    int16_t left =  apply_volume_and_pan(ch1, ch2, ch3, ch4, apu->GB_IO_NR50, apu->GB_IO_NR51, 0);
    int16_t right = apply_volume_and_pan(ch1, ch2, ch3, ch4, apu->GB_IO_NR50, apu->GB_IO_NR51, 1);

    SDL_LockMutex(audio->mutex);
    if (audio->channels == 2)
    {
            audio->buffer[audio->write_pos++] = left;
            audio->buffer[audio->write_pos++] = right;
    } else
    {
            audio->buffer[audio->write_pos++] = (left + right) / 2;  // mono
    }

    audio->write_pos %= audio->buffer_size;

    SDL_UnlockMutex(audio->mutex);
}
