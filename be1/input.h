/**
 * Bitemu BE1 - Game Boy Input
 *
 * gb_input_poll: lee teclado (WASD, J/K, U/I, Enter) y actualiza joypad_state.
 * gb_input_set_state: permite inyectar estado desde fuera (p. ej. GUI).
 * Convención: bit 1 = tecla pulsada; 0-3 = D-pad (R,L,U,D), 4-7 = A,B,Select,Start.
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
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
