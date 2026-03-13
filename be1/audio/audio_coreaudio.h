/**
 * Bitemu - CoreAudio backend (macOS)
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#ifndef BITEMU_AUDIO_COREAUDIO_H
#define BITEMU_AUDIO_COREAUDIO_H

#include "audio.h"

#ifdef __APPLE__
int bitemu_audio_coreaudio_start(bitemu_audio_t *a);
void bitemu_audio_coreaudio_stop(bitemu_audio_t *a);
#else
static inline int bitemu_audio_coreaudio_start(bitemu_audio_t *a) { (void)a; return -1; }
static inline void bitemu_audio_coreaudio_stop(bitemu_audio_t *a) { (void)a; }
#endif

#endif
