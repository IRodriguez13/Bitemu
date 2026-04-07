/**
 * Debug test para VDP wrapping
 */

#include <stdio.h>
#include "be2/vdp/vdp.h"
#include "be2/genesis_constants.h"

int main(void) {
    gen_vdp_t vdp;
    gen_vdp_init(&vdp);
    gen_vdp_reset(&vdp);
    gen_vdp_set_pal(&vdp, 0);  // NTSC
    
    printf("Initial state:\n");
    printf("  line_counter: %d\n", vdp.line_counter);
    printf("  vcounter: %d\n", vdp.vcounter);
    printf("  cycle_counter: %d\n", vdp.cycle_counter);
    
    // Ejecutar exactamente un frame
    int total_cycles = GEN_SCANLINES_TOTAL * GEN_CYCLES_PER_LINE;
    printf("\nEjecutando %d ciclos (%d líneas * %d ciclos/línea)\n", 
           total_cycles, GEN_SCANLINES_TOTAL, GEN_CYCLES_PER_LINE);
    
    gen_vdp_step(&vdp, total_cycles);
    
    printf("\nEstado después de un frame:\n");
    printf("  line_counter: %d (esperado: 0)\n", vdp.line_counter);
    printf("  vcounter: %d (esperado: 0)\n", vdp.vcounter);
    printf("  cycle_counter: %d\n", vdp.cycle_counter);
    
    // Ejecutar otro frame
    printf("\nEjecutando segundo frame...\n");
    gen_vdp_step(&vdp, total_cycles);
    
    printf("\nEstado después de dos frames:\n");
    printf("  line_counter: %d (esperado: 0)\n", vdp.line_counter);
    printf("  vcounter: %d (esperado: 0)\n", vdp.vcounter);
    printf("  cycle_counter: %d\n", vdp.cycle_counter);
    
    return 0;
}
