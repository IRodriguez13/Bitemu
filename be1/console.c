/**
 * BE1 - Game Boy: implementación de la interfaz core
 */

#include "core/console.h"
#include "gb_impl.h"
#include "cpu/cpu.h"
#include "ppu.h"
#include "apu.h"
#include "timer.h"
#include "memory.h"
#include "input.h"
#include "gb_constants.h"
#include <stdlib.h>
#include <string.h>

static void gb_init(console_t *ctx)
{
    gb_impl_t *impl = ctx->impl;
    gb_mem_init(&impl->mem);
    gb_cpu_init(&impl->cpu);
    gb_ppu_init(&impl->ppu);
    gb_apu_init(&impl->apu);
}

static void gb_reset(console_t *ctx)
{
    gb_impl_t *impl = ctx->impl;
    gb_mem_reset(&impl->mem);
    gb_cpu_reset(&impl->cpu);
}

static void gb_step(console_t *ctx, int cycles)
{
    gb_impl_t *impl = ctx->impl;
    gb_input_poll(&impl->mem);
    int consumed = 0;
    while (consumed < cycles)
    {
        int c = gb_cpu_step(&impl->cpu, &impl->mem);
        consumed += c;
        gb_timer_step(&impl->mem, c);
        gb_ppu_step(&impl->ppu, &impl->mem, c);
        gb_apu_step(&impl->apu, &impl->mem, c);
    }
}

static bool gb_load_rom(console_t *ctx, const char *path, const uint8_t *data, size_t size)
{
    gb_impl_t *impl = ctx->impl;
    if (impl->mem.rom)
        free(impl->mem.rom);
    impl->mem.rom = malloc(size);
    if (!impl->mem.rom)
        return false;
    memcpy(impl->mem.rom, data, size);
    impl->mem.rom_size = size;
    impl->mem.cart_type = (size > 0x148) ? data[0x147] : 0x01;
    impl->rom_path[0] = '\0';
    if (path) {
        size_t len = strlen(path);
        if (len >= GB_ROM_PATH_MAX)
            len = GB_ROM_PATH_MAX - 1;
        memcpy(impl->rom_path, path, len);
        impl->rom_path[len] = '\0';
    }
    gb_mem_reset(&impl->mem);
    gb_cpu_reset(&impl->cpu);
    if (path)
        gb_mem_load_sav(&impl->mem, path);
    return true;
}

static void gb_unload_rom(console_t *ctx)
{
    gb_impl_t *impl = ctx->impl;
    if (impl->rom_path[0])
        gb_mem_save_sav(&impl->mem, impl->rom_path);
    free(impl->mem.rom);
    impl->mem.rom = NULL;
    impl->mem.rom_size = 0;
    impl->rom_path[0] = '\0';
}

static int gb_cycles_per_frame(console_t *ctx)
{
    (void)ctx;
    return GB_DOTS_PER_FRAME;
}

const console_ops_t gb_console_ops = {
    .name = "Game Boy",
    .init = gb_init,
    .reset = gb_reset,
    .step = gb_step,
    .load_rom = gb_load_rom,
    .unload_rom = gb_unload_rom,
    .cycles_per_frame = gb_cycles_per_frame,
};
