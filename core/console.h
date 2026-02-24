/**
 * Bitemu - Interfaz genérica de consola
 *
 * Contrato que debe implementar cada emulador:
 * - bitemu1 (Game Boy)
 * - bitemu2 (Genesis)
 * - bitemu3 (PS1)
 *
 * El core usa esta interfaz; cada repo aporta su implementación.
 */

#ifndef BITEMU_CONSOLE_H
#define BITEMU_CONSOLE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct console console_t;

/* Operaciones que cada consola implementa */
typedef struct 
{
    const char *name;

    void (*init)(console_t *ctx);
    void (*reset)(console_t *ctx);
    void (*step)(console_t *ctx, int cycles);

    bool (*load_rom)(console_t *ctx, const uint8_t *data, size_t size);
    void (*unload_rom)(console_t *ctx);

    /* Ciclos por frame (NULL = 70224). Usado por engine_run. */
    int (*cycles_per_frame)(console_t *ctx);
} console_ops_t;

struct console 
{
    const console_ops_t *ops;
    void *impl;  /* Estado interno de la consola (cpu, ppu, etc.) */
};

/* Helpers para el engine */
static inline void console_init(console_t *c) 
{
    if (c && c->ops && c->ops->init)
        c->ops->init(c);
}

static inline void console_reset(console_t *c) 
{
    if (c && c->ops && c->ops->reset)
        c->ops->reset(c);
}

static inline void console_step(console_t *c, int cycles) 
{
    if (c && c->ops && c->ops->step)
        c->ops->step(c, cycles);
}

static inline bool console_load_rom(console_t *c, const uint8_t *data, size_t size) {
    if (c && c->ops && c->ops->load_rom)
        return c->ops->load_rom(c, data, size);
    return false;
}

static inline void console_unload_rom(console_t *c) {
    if (c && c->ops && c->ops->unload_rom)
        c->ops->unload_rom(c);
}

#endif /* BITEMU_CONSOLE_H */
