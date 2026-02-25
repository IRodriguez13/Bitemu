/**
 * Bitemu - API pública
 * Wrapper sobre el engine y la consola BE1
 */

#include "bitemu.h"
#include "core/engine.h"
#include "core/console.h"
#include "core/utils/log.h"
#include "be1/gb_impl.h"
#include "be1/input.h"
#include "be1/ppu.h"
#include "be1/gb_constants.h"

extern const console_ops_t gb_console_ops;
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct bitemu {
    gb_impl_t impl;
    engine_t engine;
    int running;
    unsigned frame_count;
};

bitemu_t *bitemu_create(void) {
    bitemu_t *emu = calloc(1, sizeof(bitemu_t));
    if (!emu) return NULL;
    engine_init(&emu->engine, &gb_console_ops, &emu->impl);
    emu->running = 1;
    log_info("Emulador creado");
    return emu;
}

void bitemu_destroy(bitemu_t *emu) {
    if (emu) {
        console_unload_rom(&emu->engine.console);
        free(emu);
    }
}

bool bitemu_load_rom(bitemu_t *emu, const char *path) {
    if (!emu || !path) return false;

    FILE *f = fopen(path, "rb");
    if (!f) return false;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size <= 0 || size > 8 * 1024 * 1024) {
        fclose(f);
        return false;
    }

    uint8_t *data = malloc((size_t)size);
    if (!data) {
        fclose(f);
        return false;
    }
    size_t read = fread(data, 1, (size_t)size, f);
    fclose(f);
    if (read != (size_t)size) {
        free(data);
        return false;
    }

    bool ok = console_load_rom(&emu->engine.console, path, data, (size_t)size);
    free(data);
    if (ok)
        log_info("ROM cargada: %s (%ld bytes)", path, (long)size);
    else
        log_error("Error al cargar ROM: %s", path);
    return ok;
}

bool bitemu_run_frame(bitemu_t *emu) {
    if (!emu || !emu->running) return false;
    console_step(&emu->engine.console, GB_DOTS_PER_FRAME);
    emu->frame_count++;
    if (getenv("BITEMU_VERBOSE") && emu->frame_count % 60 == 0)
        log_info("Frame %u (~%.1f FPS)", emu->frame_count, 60.0);
    return emu->running;
}

const uint8_t *bitemu_get_framebuffer(const bitemu_t *emu) {
    if (!emu) return NULL;
    return emu->impl.ppu.framebuffer;
}

void bitemu_set_input(bitemu_t *emu, uint8_t state) {
    if (emu) emu->impl.mem.joypad_state = state;
}

void bitemu_poll_input(bitemu_t *emu) {
    if (emu) gb_input_poll(&emu->impl.mem);
}

void bitemu_reset(bitemu_t *emu) {
    if (emu)
        console_reset(&emu->engine.console);
}

void bitemu_stop(bitemu_t *emu) {
    if (emu) emu->running = 0;
}

bool bitemu_is_running(const bitemu_t *emu) {
    return emu && emu->running;
}
