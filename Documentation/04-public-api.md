# Public API (bitemu.h)

## English

### 4.1 Purpose

`include/bitemu.h` defines the **public interface** for all frontends (CLI, Rust GUI). It hides the internal `engine_t` and `gb_impl_t` behind an opaque `bitemu_t` handle.

### 4.2 Types and Constants

```c
#define BITEMU_FB_WIDTH  160
#define BITEMU_FB_HEIGHT 144
#define BITEMU_FB_SIZE   (BITEMU_FB_WIDTH * BITEMU_FB_HEIGHT)

typedef struct bitemu bitemu_t;  /* Opaque */
```

### 4.3 Functions

| Function | Description |
|----------|-------------|
| `bitemu_create()` | Allocate emulator, init engine with `gb_console_ops`, return handle. NULL on failure. |
| `bitemu_destroy(emu)` | Unload ROM, free handle. |
| `bitemu_load_rom(emu, path)` | Open file, read, call `console_load_rom`. Returns true on success. |
| `bitemu_run_frame(emu)` | Run `GB_DOTS_PER_FRAME` (70224) cycles via `console_step`. Returns `emu->running`. |
| `bitemu_get_framebuffer(emu)` | Return pointer to PPU framebuffer (160×144, 1 byte/pixel, 0=white, 0xFF=black). |
| `bitemu_set_input(emu, state)` | Set joypad state. Bits 0–3: D-pad (R,L,U,D), 4–7: A,B,Select,Start. 1 = pressed. |
| `bitemu_stop(emu)` | Set `emu->running = 0`. |
| `bitemu_is_running(emu)` | Return `emu->running`. |

### 4.4 Joypad Encoding

```
Bit 0: Right (D-pad)
Bit 1: Left
Bit 2: Up
Bit 3: Down
Bit 4: A
Bit 5: B
Bit 6: Select
Bit 7: Start

1 = pressed, 0 = released
Default (no keys): 0xFF
```

### 4.5 Implementation (src/bitemu.c)

`bitemu_t` is defined as:

```c
struct bitemu {
    gb_impl_t impl;      /* mem, cpu, ppu, apu */
    engine_t engine;
    int running;
    unsigned frame_count;
};
```

---

## Español

### 4.1 Propósito

`include/bitemu.h` define la **interfaz pública** para todos los frontends (CLI, GUI Rust). Oculta el `engine_t` y `gb_impl_t` internos detrás de un handle opaco `bitemu_t`.

### 4.2 Tipos y constantes

```c
#define BITEMU_FB_WIDTH  160
#define BITEMU_FB_HEIGHT 144
#define BITEMU_FB_SIZE   (BITEMU_FB_WIDTH * BITEMU_FB_HEIGHT)

typedef struct bitemu bitemu_t;  /* Opaco */
```

### 4.3 Funciones

| Función | Descripción |
|---------|-------------|
| `bitemu_create()` | Asignar emulador, init engine con `gb_console_ops`, devolver handle. NULL si falla. |
| `bitemu_destroy(emu)` | Descargar ROM, liberar handle. |
| `bitemu_load_rom(emu, path)` | Abrir archivo, leer, llamar `console_load_rom`. Devuelve true si OK. |
| `bitemu_run_frame(emu)` | Ejecutar 70224 ciclos vía `console_step`. Devuelve `emu->running`. |
| `bitemu_get_framebuffer(emu)` | Puntero al framebuffer PPU (160×144, 1 byte/píxel). |
| `bitemu_set_input(emu, state)` | Estado del joypad. Bits 0–3: D-pad, 4–7: A,B,Select,Start. 1 = pulsado. |
| `bitemu_stop(emu)` | Pone `emu->running = 0`. |
| `bitemu_is_running(emu)` | Devuelve `emu->running`. |

### 4.4 Codificación del Joypad

```
Bit 0: Derecha (D-pad)
Bit 1: Izquierda
Bit 2: Arriba
Bit 3: Abajo
Bit 4: A
Bit 5: B
Bit 6: Select
Bit 7: Start

1 = pulsado, 0 = soltado
Por defecto (sin teclas): 0xFF
```
