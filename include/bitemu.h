/**
 * Bitemu - API pública para frontends (CLI, GUI)
 * Usado por bitemu (CLI) y bitemu-gui (Rust)
 */

#ifndef BITEMU_H
#define BITEMU_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BITEMU_FB_WIDTH  160
#define BITEMU_FB_HEIGHT 144
#define BITEMU_FB_SIZE   (BITEMU_FB_WIDTH * BITEMU_FB_HEIGHT)

typedef struct bitemu bitemu_t;

/* Crear/destruir instancia */
bitemu_t *bitemu_create(void);
void bitemu_destroy(bitemu_t *emu);

/* Cargar ROM desde archivo. Retorna true si OK */
bool bitemu_load_rom(bitemu_t *emu, const char *path);

/* Ejecutar un frame (70224 ciclos). Retorna false si debe parar */
bool bitemu_run_frame(bitemu_t *emu);

/* Framebuffer: 160x144, 1 byte por pixel (0=blanco, 0xFF=negro) */
const uint8_t *bitemu_get_framebuffer(const bitemu_t *emu);

/* Joypad: bits 0-3=D-pad(R,L,U,D), 4-7=botones(A,B,Select,Start); 1=presionado */
void bitemu_set_input(bitemu_t *emu, uint8_t state);

/* Solo para CLI: lee teclado (stdin) y actualiza el joypad. La GUI usa bitemu_set_input. */
void bitemu_poll_input(bitemu_t *emu);

/* Reset de la consola (estado post-carga, sin recargar la ROM) */
void bitemu_reset(bitemu_t *emu);

/* Señal de parada (para el loop) */
void bitemu_stop(bitemu_t *emu);
bool bitemu_is_running(const bitemu_t *emu);

#ifdef __cplusplus
}
#endif

#endif /* BITEMU_H */
