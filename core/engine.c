/**
 * Bitemu - Core Engine
 * Motor genérico que delega en la consola vía interfaz.
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "engine.h"
#include "utils/log.h"

void engine_init(engine_t *engine, const console_ops_t *ops, void *impl)
{
    engine->console.ops = ops;
    engine->console.impl = impl;
    engine->running = 0;
    console_init(&engine->console);
    log_info("Engine: %s", ops ? ops->name : "none");
}

void engine_run(engine_t *engine) 
{
    engine->running = 1;

    while (engine->running) {
        int cycles = engine->console.ops->cycles_per_frame
            ? engine->console.ops->cycles_per_frame(&engine->console)
            : 70224;
        console_step(&engine->console, cycles);
    }
}

void engine_stop(engine_t *engine) 
{
    engine->running = 0;
}
