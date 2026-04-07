/**
 * Test de carga de paleta con más frames
 */

#include <stdio.h>
#include <stdlib.h>
#include "bitemu.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <rom_path>\n", argv[0]);
        return 1;
    }
    
    const char *rom_path = argv[1];
    printf("=== TEST DE CARGA DE PALETA ===\n");
    printf("ROM: %s\n", rom_path);
    
    bitemu_t *emu = bitemu_create();
    if (!bitemu_load_rom(emu, rom_path)) {
        printf("❌ Error cargando ROM\n");
        return 1;
    }
    
    // Ejecutar más frames para permitir inicialización completa
    printf("Ejecutando 60 frames (1 segundo a 60fps)...\n");
    for (int frame = 0; frame < 60; frame++) {
        if (!bitemu_run_frame(emu)) {
            printf("❌ Error ejecutando frame %d\n", frame);
            break;
        }
        
        // Analizar cada 10 frames
        if (frame % 10 == 0) {
            const uint8_t *fb = bitemu_get_framebuffer(emu);
            if (fb) {
                // Buscar colores diferentes de negro
                int color_count = 0;
                int max_r = 0, max_g = 0, max_b = 0;
                
                for (int i = 0; i < 320*224*3; i += 3) {
                    if (fb[i] > 0 || fb[i+1] > 0 || fb[i+2] > 0) {
                        color_count++;
                        if (fb[i] > max_r) max_r = fb[i];
                        if (fb[i+1] > max_g) max_g = fb[i+1];
                        if (fb[i+2] > max_b) max_b = fb[i+2];
                    }
                }
                
                printf("Frame %2d: %6d píxeles con color, Max RGB(%3d,%3d,%3d)\n", 
                       frame, color_count, max_r, max_g, max_b);
            }
        }
    }
    
    printf("\n=== ANÁLISIS FINAL ===\n");
    const uint8_t *fb = bitemu_get_framebuffer(emu);
    if (fb) {
        // Mostrar primeros 32 colores encontrados
        printf("Primeros 32 colores únicos:\n");
        int unique_colors = 0;
        for (int i = 0; i < 320*224*3 && unique_colors < 32; i += 3) {
            if (fb[i] > 0 || fb[i+1] > 0 || fb[i+2] > 0) {
                printf("  RGB(%3d,%3d,%3d)\n", fb[i], fb[i+1], fb[i+2]);
                unique_colors++;
            }
        }
    }
    
    bitemu_destroy(emu);
    printf("\n✅ Test completado\n");
    return 0;
}
