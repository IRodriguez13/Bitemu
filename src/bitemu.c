/**
 * Bitemu - API pública
 * Wrapper sobre el engine y la consola BE1
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "bitemu.h"
#include "core/engine.h"
#include <math.h>
#include "core/console.h"
#include "core/utils/log.h"
#include "be1/gb_impl.h"
#include "be1/input.h"
#include "be1/ppu.h"
#include "be1/gb_constants.h"
#include "be1/audio/audio.h"
#if defined(__linux__)
#include "be1/audio/audio_alsa.h"
#elif defined(__APPLE__)
#include "be1/audio/audio_coreaudio.h"
#elif defined(_WIN32) || defined(__MINGW32__)
#include "be1/audio/audio_wasapi.h"
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern const console_ops_t gb_console_ops;

#define BITEMU_AUDIO_DEFAULT_RATE   44100
#define BITEMU_AUDIO_DEFAULT_CHANS  1
#define BITEMU_AUDIO_DEFAULT_SAMPLES 32768  /* suficiente para chirp+ding (~17k muestras) */

struct bitemu
{
    gb_impl_t impl;
    engine_t engine;
    int running;
    unsigned frame_count;
    bitemu_audio_t audio;
};

bitemu_t *bitemu_create(void)
{
    bitemu_t *emu = calloc(1, sizeof(bitemu_t));
    if (!emu)
        return NULL;
    engine_init(&emu->engine, &gb_console_ops, &emu->impl);
    emu->running = 1;
    log_info("Emulador creado");
    return emu;
}

void bitemu_destroy(bitemu_t *emu)
{
    if (emu)
    {
        bitemu_audio_shutdown(emu);
        console_unload_rom(&emu->engine.console);
        free(emu);
    }
}

bool bitemu_load_rom(bitemu_t *emu, const char *path)
{
    if (!emu || !path)
        return false;

    FILE *f = fopen(path, "rb");
    if (!f)
        return false;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size <= 0 || size > 8 * 1024 * 1024)
    {
        fclose(f);
        return false;
    }

    uint8_t *data = malloc((size_t)size);
    if (!data)
    {
        fclose(f);
        return false;
    }
    
    size_t read = fread(data, 1, (size_t)size, f);
    fclose(f);
    if (read != (size_t)size)
    {
        free(data);
        return false;
    }

    bool ok = console_load_rom(&emu->engine.console, path, data, (size_t)size);
    free(data);
    if (!ok)
    {
        log_error("Error al cargar ROM: %s", path);
        return false;
    }
    
    log_info("ROM cargada: %s (%ld bytes)", path, (long)size);
    return ok;
}

void bitemu_unload_rom(bitemu_t *emu)
{
    if (emu)
    {
        console_unload_rom(&emu->engine.console);
        console_reset(&emu->engine.console);
        emu->frame_count = 0;
        log_info("ROM descargada");
    }
}

bool bitemu_run_frame(bitemu_t *emu)
{
    if (!emu || !emu->running)
        return false;
    console_step(&emu->engine.console, GB_DOTS_PER_FRAME);
    emu->frame_count++;
    
    if (getenv("BITEMU_VERBOSE") && emu->frame_count % 60 == 0)
        log_info("Frame %u (~%.1f FPS)", emu->frame_count, 60.0);
    return emu->running;
}

const uint8_t *bitemu_get_framebuffer(const bitemu_t *emu)
{
    if (!emu)
        return NULL;

    return emu->impl.ppu.framebuffer;
}

void bitemu_get_video_size(const bitemu_t *emu, int *width, int *height)
{
    if (!emu)
        return;
    console_get_video_info(&emu->engine.console, width, height);
}

void bitemu_set_input(bitemu_t *emu, uint8_t state)
{
    if (emu)
        emu->impl.mem.joypad_state = state;
}

void bitemu_poll_input(bitemu_t *emu)
{
    if (emu)
        gb_input_poll(&emu->impl.mem);
}

void bitemu_reset(bitemu_t *emu)
{
    if (emu)
        console_reset(&emu->engine.console);
}

int bitemu_save_state(bitemu_t *emu, const char *path)
{
    if (!emu || !path)
        return -1;
    return console_save_state(&emu->engine.console, path);
}

int bitemu_load_state(bitemu_t *emu, const char *path)
{
    if (!emu || !path)
        return -1;
    return console_load_state(&emu->engine.console, path);
}

void bitemu_stop(bitemu_t *emu)
{
    if (emu)
        emu->running = 0;
}

bool bitemu_is_running(const bitemu_t *emu)
{
    return emu && emu->running;
}

/* ----- Audio (buffer circular; sin SDL/miniaudio) ----- */

int bitemu_audio_init(bitemu_t *emu, bitemu_audio_backend_t back, const bitemu_audio_config_t *config)
{
    if (!emu)
        return -1;
    bitemu_audio_shutdown(emu);
    int rate = BITEMU_AUDIO_DEFAULT_RATE;
    int chans = BITEMU_AUDIO_DEFAULT_CHANS;
    size_t samples = BITEMU_AUDIO_DEFAULT_SAMPLES;
    if (config)
    {
        if (config->sample_rate > 0) rate = config->sample_rate;
        if (config->channels > 0 && config->channels <= 2) chans = config->channels;
        if (config->buffer_samples > 0) samples = (size_t)config->buffer_samples;
    }
    emu->audio.buffer = calloc(samples, sizeof(int16_t));
    if (!emu->audio.buffer)
        return -1;
    emu->audio.buffer_size = samples;
    emu->audio.write_pos = 0;
    emu->audio.read_pos = 0;
    emu->audio.sample_rate = rate;
    emu->audio.channels = chans;
    emu->audio.enabled = false;
    emu->audio.userdata = NULL;
    emu->impl.audio_output = &emu->audio;

#if defined(__linux__)
    if (back == BITEMU_AUDIO_BACKEND_NATIVE)
    {
        if (bitemu_audio_alsa_start(&emu->audio) != 0)
            emu->audio.userdata = NULL;  /* fallback: buffer only */
    }
#elif defined(__APPLE__)
    if (back == BITEMU_AUDIO_BACKEND_NATIVE)
    {
        if (bitemu_audio_coreaudio_start(&emu->audio) != 0)
            emu->audio.userdata = NULL;
    }
#elif defined(_WIN32) || defined(__MINGW32__)
    if (back == BITEMU_AUDIO_BACKEND_NATIVE)
    {
        if (bitemu_audio_wasapi_start(&emu->audio) != 0)
            emu->audio.userdata = NULL;
    }
#else
    (void)back;
#endif
    return 0;
}

void bitemu_audio_set_enabled(bitemu_t *emu, bool enabled)
{
    if (emu)
        emu->audio.enabled = enabled;
}

/* Onda cuadrada con decay exponencial (chirp y ding originales). */
static int audio_inject_square_decay(bitemu_audio_t *a, double freq, double dur_sec, double decay)
{
    if (!a || !a->buffer || a->buffer_size == 0 || a->sample_rate <= 0)
        return -1;
    int n = (int)(a->sample_rate * dur_sec);
    if (n <= 0 || (size_t)n > a->buffer_size - 256)
        return -1;
    size_t size = a->buffer_size;
    size_t wr = a->write_pos;
    size_t rd = a->read_pos;
    size_t used = (wr >= rd) ? (wr - rd) : (size - rd + wr);
    if (used + (size_t)n >= size)
        return -1;
    const int16_t amp = 8000;
    for (int i = 0; i < n; i++)
    {
        double t = (double)i / (double)a->sample_rate;
        double phase = fmod(freq * t, 1.0);
        double wave = (phase < 0.5) ? 1.0 : -1.0;
        double env = exp(-t * decay);
        int32_t v = (int32_t)(wave * env * amp);
        if (v > 32767) v = 32767;
        if (v < -32768) v = -32768;
        a->buffer[wr] = (int16_t)v;
        wr = (wr + 1) % size;
    }
    a->write_pos = wr;
    return 0;
}

/* Chirp original: 880 Hz, 80ms, square wave, decay 14 */
void bitemu_audio_play_chirp(bitemu_t *emu)
{
    if (emu && emu->audio.buffer)
        audio_inject_square_decay(&emu->audio, 880.0, 0.08, 14.0);
}

static int audio_inject_silence(bitemu_audio_t *a, int duration_ms)
{
    if (!a || !a->buffer || a->buffer_size == 0 || a->sample_rate <= 0)
        return -1;
    int samples = (int)((long)a->sample_rate * duration_ms / 1000);
    if (samples <= 0 || (size_t)samples > a->buffer_size - 256)
        return -1;
    size_t size = a->buffer_size;
    size_t wr = a->write_pos;
    size_t rd = a->read_pos;
    size_t used = (wr >= rd) ? (wr - rd) : (size - rd + wr);
    if (used + (size_t)samples >= size)
        return -1;
    for (int i = 0; i < samples; i++)
    {
        a->buffer[wr] = 0;
        wr = (wr + 1) % size;
    }
    a->write_pos = wr;
    return 0;
}

/* Ding original: 523.25 Hz 120ms decay=10, gap 40ms, 1046.5 Hz 220ms decay=8 */
void bitemu_audio_play_ding(bitemu_t *emu)
{
    if (!emu || !emu->audio.buffer)
        return;
    audio_inject_square_decay(&emu->audio, 523.25, 0.12, 10.0);
    audio_inject_silence(&emu->audio, 40);
    audio_inject_square_decay(&emu->audio, 1046.5, 0.22, 8.0);
}

void bitemu_audio_set_callback(bitemu_t *emu, bitemu_audio_callback_t cb, void *userdata)
{
    (void)emu;
    (void)cb;
    (void)userdata;
    /* No usado en el modelo buffer; el frontend usa bitemu_read_audio. */
}

void bitemu_audio_shutdown(bitemu_t *emu)
{
    if (!emu)
        return;
#if defined(__linux__)
    if (emu->audio.userdata)
        bitemu_audio_alsa_stop(&emu->audio);
#elif defined(__APPLE__)
    if (emu->audio.userdata)
        bitemu_audio_coreaudio_stop(&emu->audio);
#elif defined(_WIN32) || defined(__MINGW32__)
    if (emu->audio.userdata)
        bitemu_audio_wasapi_stop(&emu->audio);
#endif
    free(emu->audio.buffer);
    emu->audio.buffer = NULL;
    emu->audio.buffer_size = 0;
    emu->audio.write_pos = 0;
    emu->audio.read_pos = 0;
    emu->audio.userdata = NULL;
    emu->impl.audio_output = NULL;
}

int bitemu_read_audio(bitemu_t *emu, int16_t *buf, int max_samples)
{
    if (!emu || !buf || max_samples <= 0 || !emu->audio.buffer)
        return 0;
    bitemu_audio_t *a = &emu->audio;

    size_t wr = a->write_pos;
    size_t rd = a->read_pos;
    size_t size = a->buffer_size;
    size_t avail = (wr >= rd) ? (wr - rd) : (size - rd + wr);
    
    if (avail > (size_t)max_samples)
        avail = (size_t)max_samples;
    
    if (avail == 0)
        return 0;
    
    for (size_t i = 0; i < avail; i++)
    {
        buf[i] = a->buffer[rd];
        rd = (rd + 1) % size;
    }
    a->read_pos = rd;
    return (int)avail;
}

void bitemu_get_audio_spec(int *sample_rate, int *channels)
{
    if (sample_rate)
        *sample_rate = BITEMU_AUDIO_DEFAULT_RATE;
    if (channels)
        *channels = BITEMU_AUDIO_DEFAULT_CHANS;
}
