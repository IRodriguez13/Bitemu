/**
 * Debug de carga de ROM y framebuffer
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bitemu.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <rom_path>\n", argv[0]);
        return 1;
    }
    
    const char *rom_path = argv[1];
    printf("=== DEBUG CARGA ROM Y FRAMEBUFFER ===\n");
    printf("ROM: %s\n", rom_path);
    
    // Crear emulador
    bitemu_t *emu = bitemu_create();
    if (!emu) {
        printf("❌ Error creando emulador\n");
        return 1;
    }
    printf("✅ Emulador creado\n");
    
    // Cargar ROM
    if (!bitemu_load_rom(emu, rom_path)) {
        printf("❌ Error cargando ROM\n");
        bitemu_destroy(emu);
        return 1;
    }
    printf("✅ ROM cargada\n");
    
    // Ejecutar varios frames para inicializar
    printf("Ejecutando frames para inicializar...\n");
    for (int frame = 0; frame < 10; frame++) {
        if (!bitemu_run_frame(emu)) {
            printf("❌ Error ejecutando frame %d\n", frame);
            break;
        }
        
        if (frame == 0 || frame == 9) {
            const uint8_t *fb = bitemu_get_framebuffer(emu);
            if (!fb) {
                printf("❌ Framebuffer NULL en frame %d\n", frame);
            } else {
                // Analizar primeros píxeles
                printf("Frame %d - Primeros 16 píxeles: ", frame);
                for (int i = 0; i < 16; i++) {
                    printf("%02X ", fb[i]);
                }
                printf("\n");
                
                // Verificar si es todo negro/gris
                int black_count = 0;
                int non_black_count = 0;
                for (int i = 0; i < 320*224*3; i++) {
                    if (fb[i] == 0x00) {
                        black_count++;
                    } else {
                        non_black_count++;
                    }
                }
                printf("  Píxeles negros: %d, No negros: %d\n", black_count, non_black_count);
            }
        }
    }
    
    // Obtener stats específicas de Genesis
    printf("\n=== STATS DEL NÚCLEO ===\n");
    printf("Engine: Genesis\n");
    printf("Console: Sega Genesis/Mega Drive\n");
    printf("Frames ejecutados: 10\n");
    
    bitemu_destroy(emu);
    printf("\n✅ Debug completado\n");
    return 0;
}
