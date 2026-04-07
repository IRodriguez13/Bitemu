/**
 * Debug del estado VDP después de cargar ROM
 */

#include <stdio.h>
#include <stdlib.h>
#include "bitemu.h"
#include "be2/vdp/vdp.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <rom_path>\n", argv[0]);
        return 1;
    }
    
    const char *rom_path = argv[1];
    printf("=== DEBUG ESTADO VDP ===\n");
    printf("ROM: %s\n", rom_path);
    
    // Crear emulador
    bitemu_t *emu = bitemu_create();
    if (!emu) {
        printf("❌ Error creando emulador\n");
        return 1;
    }
    
    // Cargar ROM
    if (!bitemu_load_rom(emu, rom_path)) {
        printf("❌ Error cargando ROM\n");
        bitemu_destroy(emu);
        return 1;
    }
    printf("✅ ROM cargada\n");
    
    // Ejecutar un frame
    if (!bitemu_run_frame(emu)) {
        printf("❌ Error ejecutando frame\n");
    }
    printf("✅ Frame ejecutado\n");
    
    // Obtener acceso al VDP (necesitamos acceder a la implementación interna)
    // Esto es solo para debugging - en producción no se debería hacer
    printf("\\n=== ESTADO VDP (POST-CARGA) ===\\n");
    
    // Verificar framebuffer
    const uint8_t *fb = bitemu_get_framebuffer(emu);
    if (fb) {
        // Verificar si hay colores diferentes de negro
        int color_count = 0;
        for (int i = 0; i < 320*224*3; i += 3) {
            if (fb[i] != 0 || fb[i+1] != 0 || fb[i+2] != 0) {
                color_count++;
                if (color_count <= 10) {
                    printf("Color %d: RGB(%02X,%02X,%02X)\\n", 
                           color_count, fb[i], fb[i+1], fb[i+2]);
                }
            }
        }
        printf("Total píxeles con color: %d\\n", color_count);
    }
    
    bitemu_destroy(emu);
    printf("\\n✅ Debug VDP completado\\n");
    return 0;
}
