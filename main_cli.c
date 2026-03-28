/**
 * Bitemu - Frontend CLI
 * bitemu -rom juego.gb [-frames N] [--cli]
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "bitemu.h"
#include "core/utils/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char *prog) 
{
    fprintf(stderr, "Uso: %s -rom <archivo> [opciones]\n", prog);
    fprintf(stderr, "  -rom <fichero>   ROM .gb / .md / .gen\n");
    fprintf(stderr, "  -frames N        Salir tras N frames (útil con gprof: salida limpia escribe gmon.out)\n");
    fprintf(stderr, "  --cli            Modo headless (sin ventana)\n");
}

int main(int argc, char *argv[]) 
{
    const char *rom_path = NULL;
    long max_frames = 0;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-rom") == 0 && i + 1 < argc)
        {
            rom_path = argv[++i];
            continue;
        }
        if (strcmp(argv[i], "-frames") == 0 && i + 1 < argc)
        {
            char *end = NULL;
            max_frames = strtol(argv[++i], &end, 10);
            if (end == argv[i] || max_frames <= 0)
            {
                fprintf(stderr, "Error: -frames requiere un entero > 0\n");
                return 1;
            }
            continue;
        }
        if (strcmp(argv[i], "--cli") == 0)
            continue;
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
        {
            usage(argv[0]);
            return 0;
        }
    }

    if (!rom_path) 
    {
        fprintf(stderr, "Error: se requiere -rom <archivo>\n");
        usage(argv[0]);
        return 1;
    }

    log_info("Bitemu CLI - %s", rom_path);

    bitemu_t *emu = bitemu_create();
    if (!emu) 
    {
        fprintf(stderr, "Error: no se pudo crear emulador\n");
        return 1;
    }

    if (!bitemu_load_rom(emu, rom_path)) {
        fprintf(stderr, "Error: no se pudo cargar ROM: %s\n", rom_path);
        bitemu_destroy(emu);
        return 1;
    }

    if (max_frames > 0)
        log_info("ROM cargada. Ejecutando %ld frames...", max_frames);
    else
        log_info("ROM cargada. Ejecutando (Ctrl+C para salir)...");

    for (long f = 0;; f++)
    {
        bitemu_poll_input(emu);  /* CLI: leer teclado antes de cada frame */
        if (!bitemu_run_frame(emu)) break;
        if (max_frames > 0 && f + 1 >= max_frames) break;
    }

    bitemu_destroy(emu);
    return 0;
}
