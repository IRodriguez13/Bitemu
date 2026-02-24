/**
 * Bitemu BE1 - Game Boy Input
 * Joypad: D-pad + A, B, Select, Start
 */

#ifndef BITEMU_GB_INPUT_H
#define BITEMU_GB_INPUT_H

#include "memory.h"
#include "gb_constants.h"

/* Actualiza joypad_state desde teclado. Sin display: WASD + IJKL+Enter+Space */
void gb_input_poll(gb_mem_t *mem);

/* Permite inyectar estado externamente (ej. desde SDL). bits: 1=presionado */
void gb_input_set_state(gb_mem_t *mem, uint8_t state);

#endif /* BITEMU_GB_INPUT_H */
