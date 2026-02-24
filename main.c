/**
 * Bitemu BE1 - Game Boy
 * Punto de entrada
 */

#include "core/engine.h"
#include "core/console.h"
#include "core/utils/log.h"
#include <stdio.h>

extern const console_ops_t gb_console_ops;

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    log_info("Bitemu BE1 - %s", gb_console_ops.name);

    static char impl_buf[4096];
    engine_t engine;

    engine_init(&engine, &gb_console_ops, impl_buf);
    printf("Engine listo.\n");
    return 0;
}
