/**
 * Bitemu - Audio interno (buffer circular + backends nativos)
 * Sin PortAudio. Backends: ALSA (Linux), CoreAudio (macOS), WASAPI (Windows).
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct bitemu_audio
{
    bool enabled;
    float volume;    /* 0.0–1.0, aplicado al leer/escribir a dispositivo */
    int sample_rate;
    int channels;
    int16_t *buffer;
    size_t buffer_size;
    size_t write_pos;
    size_t read_pos;
    void *userdata;  /* backend-specific (e.g. alsa_ctx_t) */
} bitemu_audio_t;
