/**
 * Bitemu - CoreAudio backend (macOS)
 * Sin PortAudio. Usa AudioQueue directamente.
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#ifdef __APPLE__

#define NUM_BUFFERS 3
#define BUFFER_FRAMES 1024

#include "audio.h"
#include <AudioToolbox/AudioToolbox.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "core/simd/simd.h"

typedef struct
{
    AudioQueueRef queue;
    AudioQueueBufferRef buffers[NUM_BUFFERS];
    volatile int running;
    pthread_mutex_t mutex;
} coreaudio_ctx_t;

static void coreaudio_callback(void *userdata, AudioQueueRef queue, AudioQueueBufferRef buf_ref)
{
    bitemu_audio_t *a = (bitemu_audio_t *)userdata;
    coreaudio_ctx_t *ctx = (coreaudio_ctx_t *)a->userdata;
    if (!ctx || !ctx->running || !a->buffer || a->buffer_size == 0)
    {
        AudioQueueEnqueueBuffer(queue, buf_ref, 0, NULL);
        return;
    }

    AudioQueueBuffer *buf = buf_ref;
    int16_t *out = (int16_t *)buf->mAudioData;
    const int max_frames = (int)(buf->mAudioDataByteSize / (sizeof(int16_t) * a->channels));
    int got = 0;

    pthread_mutex_lock(&ctx->mutex);
    if (a->buffer && a->buffer_size > 0)
    {
        size_t wr = a->write_pos;
        size_t rd = a->read_pos;
        size_t size = a->buffer_size;
        size_t avail = (wr >= rd) ? (wr - rd) : (size - rd + wr);
        if (avail > (size_t)max_frames)
            avail = (size_t)max_frames;
        a->read_pos = bitemu_audio_ring_pull_scaled_clip_i16(
            a->buffer, size, rd, a->volume, avail, out);
        got = (int)avail;
    }
    pthread_mutex_unlock(&ctx->mutex);

    if (got < max_frames)
        memset(out + got, 0, (size_t)(max_frames - got) * sizeof(int16_t) * a->channels);
    buf->mAudioDataByteSize = (UInt32)(max_frames * sizeof(int16_t) * a->channels);
    buf->mPacketDescriptionCount = 0;
    AudioQueueEnqueueBuffer(queue, buf_ref, 0, NULL);
}

int bitemu_audio_coreaudio_start(bitemu_audio_t *a)
{
    if (!a || a->channels <= 0 || a->sample_rate <= 0)
        return -1;

    coreaudio_ctx_t *ctx = calloc(1, sizeof(coreaudio_ctx_t));
    if (!ctx)
        return -1;
    a->userdata = ctx;

    AudioStreamBasicDescription fmt = {0};
    fmt.mSampleRate = (Float64)a->sample_rate;
    fmt.mFormatID = kAudioFormatLinearPCM;
    fmt.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    fmt.mChannelsPerFrame = (UInt32)a->channels;
    fmt.mBitsPerChannel = 16;
    fmt.mFramesPerPacket = 1;
    fmt.mBytesPerFrame = (UInt32)(sizeof(int16_t) * a->channels);
    fmt.mBytesPerPacket = fmt.mBytesPerFrame;

    OSStatus err = AudioQueueNewOutput(&fmt, coreaudio_callback, a, NULL, NULL, 0, &ctx->queue);
    if (err != noErr)
    {
        free(ctx);
        a->userdata = NULL;
        return -1;
    }

    pthread_mutex_init(&ctx->mutex, NULL);
    ctx->running = 1;

    UInt32 buf_size = (UInt32)(BUFFER_FRAMES * fmt.mBytesPerFrame);
    for (int i = 0; i < NUM_BUFFERS; i++)
    {
        err = AudioQueueAllocateBuffer(ctx->queue, buf_size, &ctx->buffers[i]);
        if (err != noErr)
        {
            for (int j = 0; j < i; j++)
                AudioQueueFreeBuffer(ctx->queue, ctx->buffers[j]);
            AudioQueueDispose(ctx->queue, 0);
            pthread_mutex_destroy(&ctx->mutex);
            free(ctx);
            a->userdata = NULL;
            return -1;
        }
        ctx->buffers[i]->mAudioDataByteSize = buf_size;
        coreaudio_callback(a, ctx->queue, ctx->buffers[i]);
    }

    err = AudioQueueStart(ctx->queue, NULL);
    if (err != noErr)
    {
        for (int i = 0; i < NUM_BUFFERS; i++)
            AudioQueueFreeBuffer(ctx->queue, ctx->buffers[i]);
        AudioQueueDispose(ctx->queue, 0);
        pthread_mutex_destroy(&ctx->mutex);
        free(ctx);
        a->userdata = NULL;
        return -1;
    }
    return 0;
}

void bitemu_audio_coreaudio_stop(bitemu_audio_t *a)
{
    if (!a || !a->userdata)
        return;
    coreaudio_ctx_t *ctx = (coreaudio_ctx_t *)a->userdata;
    ctx->running = 0;
    AudioQueueStop(ctx->queue, 1);
    for (int i = 0; i < NUM_BUFFERS; i++)
        AudioQueueFreeBuffer(ctx->queue, ctx->buffers[i]);
    AudioQueueDispose(ctx->queue, 0);
    pthread_mutex_destroy(&ctx->mutex);
    free(ctx);
    a->userdata = NULL;
}

#endif /* __APPLE__ */
