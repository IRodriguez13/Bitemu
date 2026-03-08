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

    /* path may be NULL; used for battery save (.sav) path derivation */
    bool (*load_rom)(console_t *ctx, const char *path, const uint8_t *data, size_t size);
    void (*unload_rom)(console_t *ctx);

    /* Ciclos por frame (NULL = 70224). Usado por engine_run. */
    int (*cycles_per_frame)(console_t *ctx);

    /* Save state: serializar/deserializar estado completo a archivo. 0=OK, -1=error. */
    int (*save_state)(console_t *ctx, const char *path);
    int (*load_state)(console_t *ctx, const char *path);
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

static inline bool console_load_rom(console_t *c, const char *path, const uint8_t *data, size_t size) {
    if (c && c->ops && c->ops->load_rom)
        return c->ops->load_rom(c, path, data, size);
    return false;
}

static inline void console_unload_rom(console_t *c) {
    if (c && c->ops && c->ops->unload_rom)
        c->ops->unload_rom(c);
}

static inline int console_save_state(console_t *c, const char *path) {
    if (c && c->ops && c->ops->save_state)
        return c->ops->save_state(c, path);
    return -1;
}

static inline int console_load_state(console_t *c, const char *path) {
    if (c && c->ops && c->ops->load_state)
        return c->ops->load_state(c, path);
    return -1;
}

#endif /* BITEMU_CONSOLE_H */
