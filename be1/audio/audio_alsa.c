/**
 * Bitemu - Audio backend ALSA (Linux)
 * Sin PortAudio. Usa ALSA directamente.
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#ifdef __linux__

#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <time.h>
#include <unistd.h>
#include "audio.h"
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <string.h>

#define ALSA_DEVICE "default"
#define ALSA_PERIOD_US 20000  /* ~20ms latency */

typedef struct
{
    snd_pcm_t *pcm;
    pthread_t thread;
    volatile int running;
    pthread_mutex_t mutex;
} alsa_ctx_t;

static void *alsa_thread(void *arg)
{
    bitemu_audio_t *a = (bitemu_audio_t *)arg;
    alsa_ctx_t *ctx = (alsa_ctx_t *)a->userdata;
    int16_t buf[1024];
    const int max = (int)(sizeof(buf) / sizeof(buf[0]));

    while (ctx->running)
    {
        int got = 0;
        pthread_mutex_lock(&ctx->mutex);
        if (a->buffer && a->buffer_size > 0)
        {
            size_t wr = a->write_pos;
            size_t rd = a->read_pos;
            size_t size = a->buffer_size;
            size_t avail = (wr >= rd) ? (wr - rd) : (size - rd + wr);
            if (avail > (size_t)max)
                avail = (size_t)max;
            for (size_t i = 0; i < avail; i++)
            {
                buf[i] = a->buffer[rd];
                rd = (rd + 1) % size;
            }
            a->read_pos = rd;
            got = (int)avail;
        }
        pthread_mutex_unlock(&ctx->mutex);

        if (got > 0 && ctx->pcm)
        {
            snd_pcm_sframes_t n = snd_pcm_writei(ctx->pcm, buf, (snd_pcm_uframes_t)got);
            if (n == -EPIPE)
            {
                snd_pcm_prepare(ctx->pcm);
            }
            else if (n < 0)
            {
                snd_pcm_recover(ctx->pcm, (int)n, 0);
            }
        }
        usleep(2000);  /* 2ms poll */
    }
    return NULL;
}

int bitemu_audio_alsa_start(bitemu_audio_t *a)
{
    if (!a || a->channels <= 0 || a->sample_rate <= 0)
        return -1;

    alsa_ctx_t *ctx = calloc(1, sizeof(alsa_ctx_t));
    if (!ctx)
        return -1;
    a->userdata = ctx;

    int err = snd_pcm_open(&ctx->pcm, ALSA_DEVICE, SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0)
    {
        free(ctx);
        a->userdata = NULL;
        return -1;
    }

    snd_pcm_format_t fmt = SND_PCM_FORMAT_S16_LE;
    err = snd_pcm_set_params(ctx->pcm, fmt, SND_PCM_ACCESS_RW_INTERLEAVED,
                             (unsigned)a->channels, (unsigned)a->sample_rate,
                             1, ALSA_PERIOD_US);
    if (err < 0)
    {
        snd_pcm_close(ctx->pcm);
        free(ctx);
        a->userdata = NULL;
        return -1;
    }

    pthread_mutex_init(&ctx->mutex, NULL);
    ctx->running = 1;
    if (pthread_create(&ctx->thread, NULL, alsa_thread, a) != 0)
    {
        pthread_mutex_destroy(&ctx->mutex);
        snd_pcm_close(ctx->pcm);
        free(ctx);
        a->userdata = NULL;
        return -1;
    }
    return 0;
}

void bitemu_audio_alsa_stop(bitemu_audio_t *a)
{
    if (!a || !a->userdata)
        return;
    alsa_ctx_t *ctx = (alsa_ctx_t *)a->userdata;
    ctx->running = 0;
    pthread_join(ctx->thread, NULL);
    snd_pcm_drain(ctx->pcm);
    snd_pcm_close(ctx->pcm);
    pthread_mutex_destroy(&ctx->mutex);
    free(ctx);
    a->userdata = NULL;
}

#endif /* __linux__ */
