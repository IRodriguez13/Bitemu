#include "../../include/bitemu.h"

typedef struct
{
    bitemu_audio_backend_t backend;
    bool enabled;

    int16_t *buffer;
    size_t buffer_size;
    size_t read_pos;
    size_t write_pos;

    int sample_rate;
    int channels;

    union
    {
        SDL_AudioDeviceID sdl_dev;
        ma_device miniaudio_device;

        struct
        {
            bitemu_audio_callback_t callback;
            void *userdata;
        } custom;

    }back_data;

    SDL_mutex *mutex;

}bitemu_audio_t;
