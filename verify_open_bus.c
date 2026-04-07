/**
 * Verificación de comportamiento Open Bus vs especificaciones Genesis
 */

#include <stdio.h>
#include "be2/memory.h"

// Simulación de memoria para testing
struct test_mem {
    genesis_mem_t mem;
};

int main(void) {
    struct test_mem impl;
    genesis_mem_init(&impl.mem);
    
    printf("=== VERIFICACIÓN OPEN BUS GENESIS ===\n");
    printf("Según especificaciones de hardware Genesis:\n");
    printf("- Bit 7 siempre debe estar set (0x80)\n");
    printf("- Para direcciones no mapeadas, debe retornar 0x40 base\n");
    printf("- Para 0x500000 específicamente, test espera 0x40\n\n");
    
    // Test varias direcciones
    uint8_t b1 = genesis_mem_read8(&impl.mem, 0x500000);
    uint8_t b2 = genesis_mem_read8(&impl.mem, 0x500001);
    uint8_t b3 = genesis_mem_read8(&impl.mem, 0x500002);
    uint8_t b4 = genesis_mem_read8(&impl.mem, 0x500003);
    
    printf("Resultados actuales:\n");
    printf("0x500000: 0x%02X (bits 6-7: 0x%02X) %s\n", 
           b1, (b1 & 0xC0), (b1 & 0xC0) == 0x40 ? "✅" : "❌");
    printf("0x500001: 0x%02X (bits 6-7: 0x%02X) %s\n", 
           b2, (b2 & 0xC0), (b2 & 0xC0) == 0x40 ? "✅" : "❌");
    printf("0x500002: 0x%02X (bits 6-7: 0x%02X) %s\n", 
           b3, (b3 & 0xC0), (b3 & 0xC0) == 0x40 ? "✅" : "❌");
    printf("0x500003: 0x%02X (bits 6-7: 0x%02X) %s\n", 
           b4, (b4 & 0xC0), (b4 & 0xC0) == 0x40 ? "✅" : "❌");
    
    printf("\nAnálisis:\n");
    if ((b1 & 0xC0) == 0x40 && (b2 & 0xC0) == 0x40 && 
        (b3 & 0xC0) == 0x40 && (b4 & 0xC0) == 0x40) {
        printf("✅ Comportamiento CORRECTO según especificaciones Genesis\n");
    } else {
        printf("❌ Comportamiento INCORRECTO - necesita corrección\n");
    }
    
    return 0;
}
