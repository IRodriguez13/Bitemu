# Console Interface

## English

### 3.1 Contract

Every console (BE1, BE2, BE3) must provide a `console_ops_t`:

```c
typedef struct {
    const char *name;

    void (*init)(console_t *ctx);
    void (*reset)(console_t *ctx);
    void (*step)(console_t *ctx, int cycles);

    bool (*load_rom)(console_t *ctx, const uint8_t *data, size_t size);
    void (*unload_rom)(console_t *ctx);

    int (*cycles_per_frame)(console_t *ctx);  /* NULL = 70224 */
} console_ops_t;
```

### 3.2 Console Structure

```c
struct console {
    const console_ops_t *ops;
    void *impl;   /* Opaque: gb_impl_t, genesis_impl_t, etc. */
};
```

### 3.3 Operation Semantics

| Operation | When called | Responsibility |
|-----------|-------------|----------------|
| `init` | Once, at engine init | Initialize all subsystems (CPU, PPU, APU, memory, etc.) |
| `reset` | After load_rom or explicit reset | Reset state to power-on / post-boot |
| `step(cycles)` | Every frame (or sub-frame) | Consume `cycles` T-cycles: run CPU, timer, PPU, APU |
| `load_rom(data, size)` | When user loads a ROM | Copy ROM, reset, return true on success |
| `unload_rom` | When destroying emulator | Free ROM buffer, clear references |
| `cycles_per_frame` | Optional; used by engine_run | Return cycles per frame (e.g. 70224 for GB) |

### 3.4 BE1 Implementation

`be1/console.c` implements `gb_console_ops`:

- `gb_init`: `gb_mem_init`, `gb_cpu_init`, `gb_ppu_init`, `gb_apu_init`
- `gb_reset`: `gb_mem_reset`, `gb_cpu_reset`
- `gb_step`: Loop: `gb_input_poll` → `gb_cpu_step` → `gb_timer_step` → `gb_ppu_step` → `gb_apu_step` until `cycles` consumed
- `gb_load_rom`: Allocate, copy, reset
- `gb_unload_rom`: Free ROM
- `gb_cycles_per_frame`: Return `GB_DOTS_PER_FRAME` (70224)

---

## Español

### 3.1 Contrato

Cada consola (BE1, BE2, BE3) debe proporcionar un `console_ops_t`:

```c
typedef struct {
    const char *name;

    void (*init)(console_t *ctx);
    void (*reset)(console_t *ctx);
    void (*step)(console_t *ctx, int cycles);

    bool (*load_rom)(console_t *ctx, const uint8_t *data, size_t size);
    void (*unload_rom)(console_t *ctx);

    int (*cycles_per_frame)(console_t *ctx);  /* NULL = 70224 */
} console_ops_t;
```

### 3.2 Estructura Console

```c
struct console {
    const console_ops_t *ops;
    void *impl;   /* Opaco: gb_impl_t, genesis_impl_t, etc. */
};
```

### 3.3 Semántica de las operaciones

| Operación | Cuándo se llama | Responsabilidad |
|-----------|-----------------|-----------------|
| `init` | Una vez, al iniciar el engine | Inicializar todos los subsistemas |
| `reset` | Tras load_rom o reset explícito | Restaurar estado de encendido |
| `step(cycles)` | Cada frame (o sub-frame) | Consumir `cycles` T-ciclos: CPU, timer, PPU, APU |
| `load_rom(data, size)` | Al cargar una ROM | Copiar ROM, reset, devolver true si OK |
| `unload_rom` | Al destruir el emulador | Liberar buffer ROM |
| `cycles_per_frame` | Opcional; usado por engine_run | Devolver ciclos por frame (ej. 70224 para GB) |

### 3.4 Implementación BE1

`be1/console.c` implementa `gb_console_ops`:

- `gb_init`: `gb_mem_init`, `gb_cpu_init`, `gb_ppu_init`, `gb_apu_init`
- `gb_reset`: `gb_mem_reset`, `gb_cpu_reset`
- `gb_step`: Bucle: `gb_input_poll` → `gb_cpu_step` → `gb_timer_step` → `gb_ppu_step` → `gb_apu_step` hasta consumir `cycles`
- `gb_load_rom`: Allocar, copiar, reset
- `gb_unload_rom`: Liberar ROM
- `gb_cycles_per_frame`: Devolver `GB_DOTS_PER_FRAME` (70224)
