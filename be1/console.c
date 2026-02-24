/**
 * BE1 - Game Boy: implementación de la interfaz core
 */

#include "core/console.h"
#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    gb_cpu_t cpu;
    gb_ppu_t ppu;
    gb_apu_t apu;
    uint8_t *rom;
    size_t rom_size;
} gb_impl_t;

static void gb_init(console_t *ctx) 
{
    gb_impl_t *impl = ctx->impl;
    gb_cpu_init(&impl->cpu);
    gb_ppu_init(&impl->ppu);
    gb_apu_init(&impl->apu);
}

static void gb_reset(console_t *ctx) 
{
    (void)ctx;
}


static void gb_step(console_t *ctx, int cycles) 
{
    gb_impl_t *impl = ctx->impl;
    gb_cpu_step(&impl->cpu);
    gb_ppu_step(&impl->ppu, cycles);
    gb_apu_step(&impl->apu, cycles);
}

static bool gb_load_rom(console_t *ctx, const uint8_t *data, size_t size) 
{
    gb_impl_t *impl = ctx->impl;
    if (impl->rom) free(impl->rom);
    impl->rom = malloc(size);
    if (!impl->rom) return false;
    memcpy(impl->rom, data, size);
    impl->rom_size = size;
    return true;
}

static void gb_unload_rom(console_t *ctx) 
{
    gb_impl_t *impl = ctx->impl;
    free(impl->rom);
    impl->rom = NULL;
    impl->rom_size = 0;
}

const console_ops_t gb_console_ops = 
{
    .name = "Game Boy",
    .init = gb_init,
    .reset = gb_reset,
    .step = gb_step,
    .load_rom = gb_load_rom,
    .unload_rom = gb_unload_rom,
};
