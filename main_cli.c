/**
 * Bitemu - Frontend CLI
 * bitemu -rom juego.gb [--cli]
 */

#include "bitemu.h"
#include "core/utils/log.h"
#include <stdio.h>
#include <string.h>

static void usage(const char *prog) {
    fprintf(stderr, "Uso: %s -rom <archivo.gb> [--cli]\n", prog);
    fprintf(stderr, "  -rom    Cargar ROM\n");
    fprintf(stderr, "  --cli   Modo headless (sin ventana)\n");
}

int main(int argc, char *argv[]) {
    const char *rom_path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-rom") == 0 && i + 1 < argc) {
            rom_path = argv[++i];
        } else if (strcmp(argv[i], "--cli") == 0) {
            /* CLI mode - headless */
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        }
    }

    if (!rom_path) {
        fprintf(stderr, "Error: se requiere -rom <archivo>\n");
        usage(argv[0]);
        return 1;
    }

    log_info("Bitemu CLI - %s", rom_path);

    bitemu_t *emu = bitemu_create();
    if (!emu) {
        fprintf(stderr, "Error: no se pudo crear emulador\n");
        return 1;
    }

    if (!bitemu_load_rom(emu, rom_path)) {
        fprintf(stderr, "Error: no se pudo cargar ROM: %s\n", rom_path);
        bitemu_destroy(emu);
        return 1;
    }

    log_info("ROM cargada. Ejecutando (Ctrl+C para salir)...");

    while (bitemu_run_frame(emu)) {
        /* CLI: sin ventana, solo corre */
    }

    bitemu_destroy(emu);
    return 0;
}
