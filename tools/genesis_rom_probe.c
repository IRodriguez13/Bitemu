/**
 * Headless: carga ROM Genesis, corre N frames, imprime stats del core y checksum del FB.
 * Uso: LD_LIBRARY_PATH=.. ./genesis_rom_probe ../test_roms/simple_test.bin [frames]
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#include "bitemu.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <rom.bin> [frames]\n", argv[0]);
        return 1;
    }
    int frames = (argc >= 3) ? atoi(argv[2]) : 60;
    if (frames < 1)
        frames = 1;

    bitemu_t *emu = bitemu_create();
    if (!emu || !bitemu_load_rom(emu, argv[1])) {
        fprintf(stderr, "Error cargando ROM: %s\n", argv[1]);
        bitemu_destroy(emu);
        return 1;
    }

    for (int f = 0; f < frames; f++) {
        if (!bitemu_run_frame(emu))
            break;
    }

    uint64_t cpu = 0, z80 = 0, dma = 0;
    if (bitemu_genesis_get_core_stats(emu, &cpu, &z80, &dma) != 0) {
        fprintf(stderr, "No es consola Genesis o stats no disponibles.\n");
        bitemu_destroy(emu);
        return 1;
    }

    int w = 0, h = 0;
    bitemu_get_video_size(emu, &w, &h);
    const uint8_t *fb = bitemu_get_framebuffer(emu);
    uint64_t s0 = 0, s1 = 0;
    size_t nbytes = (size_t)w * (size_t)h * 3u;
    for (size_t i = 0; i < nbytes; i++) {
        s0 += fb[i];
        s1 += (uint64_t)fb[i] * (uint64_t)(i & 255);
    }

    printf("frames=%d  video=%dx%d  hz=%.4f\n", frames, w, h, bitemu_get_frame_hz(emu));
    printf("stats: cpu_68k_cyc=%" PRIu64 " z80_cyc=%" PRIu64 " dma_stall_68k=%" PRIu64 "\n",
           cpu, z80, dma);
    printf("fb_rgb888: sum_bytes=%" PRIu64 " weighted=%" PRIu64 "\n", s0, s1);

    bitemu_destroy(emu);
    return 0;
}
