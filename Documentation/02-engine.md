# Engine

## English

### 2.1 Role

The engine (`core/engine.c`) is the **main loop driver**. It holds a `console_t` and invokes its operations. It does not know what hardware the console emulates.

### 2.2 Structure

```c
struct engine {
    console_t console;   /* ops + impl */
    int running;         /* 0 = stop, 1 = run */
};
```

### 2.3 Functions

#### `engine_init(engine_t *engine, const console_ops_t *ops, void *impl)`

- Binds `ops` and `impl` to `engine->console`.
- Calls `console_init()` (i.e. `ops->init(ctx)`).
- Logs the console name.

#### `engine_run(engine_t *engine)`

- Sets `engine->running = 1`.
- Loop: while running, call `cycles_per_frame()` (or default 70224), then `console_step(cycles)`.
- Used by the legacy `engine_run()` path; the public API uses `bitemu_run_frame()` instead.

#### `engine_stop(engine_t *engine)`

- Sets `engine->running = 0`.

### 2.4 Integration

The public API (`src/bitemu.c`) creates a `bitemu_t` that embeds `engine_t` and `gb_impl_t`. `bitemu_run_frame()` calls `console_step()` directly with `GB_DOTS_PER_FRAME` (70224), bypassing `engine_run()` for finer control.

---

## Español

### 2.1 Rol

El engine (`core/engine.c`) es el **controlador del loop principal**. Mantiene un `console_t` e invoca sus operaciones. No sabe qué hardware emula la consola.

### 2.2 Estructura

```c
struct engine {
    console_t console;   /* ops + impl */
    int running;        /* 0 = parar, 1 = ejecutar */
};
```

### 2.3 Funciones

#### `engine_init(engine_t *engine, const console_ops_t *ops, void *impl)`

- Asocia `ops` e `impl` a `engine->console`.
- Llama a `console_init()` (es decir, `ops->init(ctx)`).
- Registra el nombre de la consola.

#### `engine_run(engine_t *engine)`

- Pone `engine->running = 1`.
- Bucle: mientras `running`, llama a `cycles_per_frame()` (o 70224 por defecto) y luego `console_step(cycles)`.
- Se usa en el flujo legacy; la API pública usa `bitemu_run_frame()` en su lugar.

#### `engine_stop(engine_t *engine)`

- Pone `engine->running = 0`.

### 2.4 Integración

La API pública (`src/bitemu.c`) crea un `bitemu_t` que contiene `engine_t` y `gb_impl_t`. `bitemu_run_frame()` llama a `console_step()` directamente con `GB_DOTS_PER_FRAME` (70224), evitando `engine_run()` para un control más fino.
