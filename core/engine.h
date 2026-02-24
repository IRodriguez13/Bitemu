/**
 * Bitemu - Core Engine
 * Motor genérico que usa la interfaz de consola.
 * Reutilizable por bitemu1 (GB), bitemu2 (Genesis), bitemu3 (PS1).
 */

#ifndef BITEMU_ENGINE_H
#define BITEMU_ENGINE_H

#include "console.h"

typedef struct engine engine_t;

struct engine {
    console_t console;
    int running;
};

void engine_init(engine_t *engine, const console_ops_t *ops, void *impl);
void engine_run(engine_t *engine);
void engine_stop(engine_t *engine);

#endif /* BITEMU_ENGINE_H */
