/**
 * Bitemu - WASAPI backend (Windows)
 * Sin PortAudio. Usa WASAPI directamente.
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#if defined(_WIN32) || defined(__MINGW32__)

#define WIN32_LEAN_AND_MEAN
#include "audio.h"
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>

#ifdef __MINGW32__
/* MinGW no tiene __uuidof; definimos los GUIDs manualmente */
const CLSID CLSID_MMDeviceEnumerator = {
    0xbcde0395, 0xe52f, 0x467c, {0x8e, 0x3d, 0xc4, 0x57, 0x92, 0x91, 0x69, 0x2e}
};
const IID IID_IMMDeviceEnumerator = {
    0xa95664d2, 0x9614, 0x4f35, {0xa7, 0x46, 0xde, 0x8d, 0xb6, 0x36, 0x17, 0xe6}
};
const IID IID_IAudioClient = {
    0x1cb9ad4c, 0xdbfa, 0x4c32, {0xb1, 0x78, 0xc2, 0xf5, 0x68, 0xa7, 0x03, 0xb2}
};
const IID IID_IAudioRenderClient = {
    0xf294acfc, 0x3146, 0x4483, {0xa7, 0xbf, 0xad, 0xdc, 0xa7, 0xc2, 0x60, 0xe2}
};
#endif
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    IAudioClient *client;
    IAudioRenderClient *render;
    pthread_t thread;
    volatile int running;
    pthread_mutex_t mutex;
} wasapi_ctx_t;

static void *wasapi_thread(void *arg)
{
    bitemu_audio_t *a = (bitemu_audio_t *)arg;
    wasapi_ctx_t *ctx = (wasapi_ctx_t *)a->userdata;
    int16_t buf[1024];
    const int max = (int)(sizeof(buf) / sizeof(buf[0]));

    while (ctx->running)
    {
        UINT32 padding = 0;
        ctx->client->lpVtbl->GetCurrentPadding(ctx->client, &padding);

        UINT32 buf_frames = 0;
        ctx->client->lpVtbl->GetBufferSize(ctx->client, &buf_frames);
        UINT32 avail_frames = buf_frames - padding;

        if (avail_frames > 0)
        {
            int to_write = (int)avail_frames;
            if (to_write > max)
                to_write = max;

            int got = 0;
            pthread_mutex_lock(&ctx->mutex);
            if (a->buffer && a->buffer_size > 0)
            {
                size_t wr = a->write_pos;
                size_t rd = a->read_pos;
                size_t size = a->buffer_size;
                size_t avail = (wr >= rd) ? (wr - rd) : (size - rd + wr);
                if (avail > (size_t)to_write)
                    avail = (size_t)to_write;
                for (size_t i = 0; i < avail; i++)
                {
                    buf[i] = a->buffer[rd];
                    rd = (rd + 1) % size;
                }
                a->read_pos = rd;
                got = (int)avail;
            }
            pthread_mutex_unlock(&ctx->mutex);

            if (got > 0)
            {
                BYTE *wasapi_buf = NULL;
                HRESULT hr = ctx->render->lpVtbl->GetBuffer(ctx->render, (UINT32)got, &wasapi_buf);
                if (SUCCEEDED(hr) && wasapi_buf)
                {
                    memcpy(wasapi_buf, buf, (size_t)got * sizeof(int16_t) * a->channels);
                    ctx->render->lpVtbl->ReleaseBuffer(ctx->render, (UINT32)got, 0);
                }
            }
            else if (avail_frames > 0)
            {
                BYTE *wasapi_buf = NULL;
                UINT32 frames = (avail_frames > (UINT32)max) ? (UINT32)max : avail_frames;
                HRESULT hr = ctx->render->lpVtbl->GetBuffer(ctx->render, frames, &wasapi_buf);
                if (SUCCEEDED(hr) && wasapi_buf)
                {
                    memset(wasapi_buf, 0, (size_t)frames * sizeof(int16_t) * a->channels);
                    ctx->render->lpVtbl->ReleaseBuffer(ctx->render, frames, 0);
                }
            }
        }
        Sleep(2);
    }
    return NULL;
}

int bitemu_audio_wasapi_start(bitemu_audio_t *a)
{
    if (!a || a->channels <= 0 || a->sample_rate <= 0)
        return -1;

    wasapi_ctx_t *ctx = calloc(1, sizeof(wasapi_ctx_t));
    if (!ctx)
        return -1;
    a->userdata = ctx;

    CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);

    IMMDeviceEnumerator *enumerator = NULL;
    HRESULT hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL,
                                  &IID_IMMDeviceEnumerator, (void **)&enumerator);
    if (FAILED(hr) || !enumerator)
    {
        free(ctx);
        a->userdata = NULL;
        return -1;
    }

    IMMDevice *endpoint = NULL;
    hr = enumerator->lpVtbl->GetDefaultAudioEndpoint(enumerator, eRender, eMultimedia, &endpoint);
    enumerator->lpVtbl->Release(enumerator);
    if (FAILED(hr) || !endpoint)
    {
        free(ctx);
        a->userdata = NULL;
        return -1;
    }

    hr = endpoint->lpVtbl->Activate(endpoint, &IID_IAudioClient, CLSCTX_ALL, NULL, (void **)&ctx->client);
    endpoint->lpVtbl->Release(endpoint);
    if (FAILED(hr) || !ctx->client)
    {
        free(ctx);
        a->userdata = NULL;
        return -1;
    }

    REFERENCE_TIME period = 0, min_period = 0;
    ctx->client->lpVtbl->GetDevicePeriod(ctx->client, &period, &min_period);

    WAVEFORMATEX fmt = {0};
    fmt.wFormatTag = WAVE_FORMAT_PCM;
    fmt.nChannels = (WORD)a->channels;
    fmt.nSamplesPerSec = (DWORD)a->sample_rate;
    fmt.wBitsPerSample = 16;
    fmt.nBlockAlign = (WORD)(a->channels * 2);
    fmt.nAvgBytesPerSec = (DWORD)(a->sample_rate * fmt.nBlockAlign);

    hr = ctx->client->lpVtbl->Initialize(ctx->client, AUDCLNT_SHAREMODE_SHARED,
                                         AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM,
                                         period, 0, &fmt, NULL);
    if (FAILED(hr))
    {
        ctx->client->lpVtbl->Release(ctx->client);
        free(ctx);
        a->userdata = NULL;
        return -1;
    }

    hr = ctx->client->lpVtbl->GetService(ctx->client, &IID_IAudioRenderClient, (void **)&ctx->render);
    if (FAILED(hr) || !ctx->render)
    {
        ctx->client->lpVtbl->Release(ctx->client);
        free(ctx);
        a->userdata = NULL;
        return -1;
    }

    hr = ctx->client->lpVtbl->Start(ctx->client);
    if (FAILED(hr))
    {
        ctx->render->lpVtbl->Release(ctx->render);
        ctx->client->lpVtbl->Release(ctx->client);
        free(ctx);
        a->userdata = NULL;
        return -1;
    }

    pthread_mutex_init(&ctx->mutex, NULL);
    ctx->running = 1;
    if (pthread_create(&ctx->thread, NULL, wasapi_thread, a) != 0)
    {
        ctx->running = 0;
        ctx->client->lpVtbl->Stop(ctx->client);
        ctx->render->lpVtbl->Release(ctx->render);
        ctx->client->lpVtbl->Release(ctx->client);
        pthread_mutex_destroy(&ctx->mutex);
        free(ctx);
        a->userdata = NULL;
        return -1;
    }
    return 0;
}

void bitemu_audio_wasapi_stop(bitemu_audio_t *a)
{
    if (!a || !a->userdata)
        return;
    wasapi_ctx_t *ctx = (wasapi_ctx_t *)a->userdata;
    ctx->running = 0;
    pthread_join(ctx->thread, NULL);
    ctx->client->lpVtbl->Stop(ctx->client);
    ctx->render->lpVtbl->Release(ctx->render);
    ctx->client->lpVtbl->Release(ctx->client);
    pthread_mutex_destroy(&ctx->mutex);
    free(ctx);
    a->userdata = NULL;
}

#endif /* _WIN32 || __MINGW32__ */
