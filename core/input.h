/**
 * Bitemu - Input
 * Manejo de entrada del usuario
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef BITEMU_INPUT_H
#define BITEMU_INPUT_H

typedef struct input input_t;

void input_init(input_t *input);
void input_poll(input_t *input);

#endif /* BITEMU_INPUT_H */
