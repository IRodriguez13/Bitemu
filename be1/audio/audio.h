/**
 * Bitemu - Audio interno (buffer circular + backends nativos)
 * Sin PortAudio. Backends: ALSA (Linux), NULL (buffer only).
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct bitemu_audio
{
    bool enabled;
    int sample_rate;
    int channels;
    int16_t *buffer;
    size_t buffer_size;
    size_t write_pos;
    size_t read_pos;
    void *userdata;  /* backend-specific (e.g. alsa_ctx_t) */
} bitemu_audio_t;
