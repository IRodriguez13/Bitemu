/**
 * Bitemu - Audio interno (buffer circular)
 * Sin dependencias de SDL/miniaudio. El frontend (p. ej. Python) lee
 * con bitemu_read_audio() y reproduce con su propia librería (sounddevice, etc.).
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
} bitemu_audio_t;
