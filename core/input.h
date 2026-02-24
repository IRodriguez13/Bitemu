/**
 * Bitemu - Input
 * Manejo de entrada del usuario
 */

#ifndef BITEMU_INPUT_H
#define BITEMU_INPUT_H

typedef struct input input_t;

void input_init(input_t *input);
void input_poll(input_t *input);

#endif /* BITEMU_INPUT_H */
