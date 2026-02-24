# Core Architecture Overview

## English

### 1.1 Purpose

Bitemu is designed as a **multi-console emulator framework**. The core (`core/`) is a generic engine that knows nothing about specific hardware (Game Boy, Genesis, PS1). Each console lives in its own module (BE1 = Game Boy, BE2 = Genesis, BE3 = PS1) and implements a common interface.

### 1.2 Design Principles

- **Separation of concerns**: Core only defines the contract; console-specific code stays in `be1/`, `be2/`, etc.
- **Dependency injection**: The engine receives function pointers (`console_ops_t`) and an opaque `impl` pointer. It never dereferences hardware structures.
- **Frame-based execution**: The engine runs in units of "cycles per frame" (e.g. 70224 for Game Boy). Each console decides how to consume those cycles.
- **Frontend-agnostic**: The public API (`bitemu.h`) exposes create/destroy, load ROM, run frame, get framebuffer, set input. CLI and GUI both use this API.

### 1.3 Data Flow

```
Frontend (CLI/GUI)
       │
       ▼
  bitemu.h API
       │
       ▼
  engine_t + console_ops_t
       │
       ▼
  console_step(cycles)
       │
       ├── CPU step
       ├── Timer step
       ├── PPU step
       └── APU step
```

### 1.4 Key Types

| Type | Location | Purpose |
|------|----------|---------|
| `engine_t` | `core/engine.h` | Holds `console_t` and `running` flag |
| `console_t` | `core/console.h` | `ops` (function table) + `impl` (opaque state) |
| `console_ops_t` | `core/console.h` | `init`, `reset`, `step`, `load_rom`, `unload_rom`, `cycles_per_frame` |
| `bitemu_t` | `include/bitemu.h` | Public handle; contains `gb_impl_t` + `engine_t` |

---

## Español

### 1.1 Propósito

Bitemu está pensado como un **framework de emulación multi-consola**. El core (`core/`) es un motor genérico que no conoce el hardware concreto (Game Boy, Genesis, PS1). Cada consola vive en su propio módulo (BE1 = Game Boy, BE2 = Genesis, BE3 = PS1) e implementa una interfaz común.

### 1.2 Principios de diseño

- **Separación de responsabilidades**: El core solo define el contrato; el código específico de cada consola permanece en `be1/`, `be2/`, etc.
- **Inyección de dependencias**: El engine recibe punteros a función (`console_ops_t`) y un puntero opaco `impl`. Nunca desreferencia estructuras de hardware.
- **Ejecución por frames**: El engine corre en unidades de "ciclos por frame" (ej. 70224 para Game Boy). Cada consola decide cómo consumir esos ciclos.
- **Independiente del frontend**: La API pública (`bitemu.h`) expone crear/destruir, cargar ROM, ejecutar frame, obtener framebuffer, establecer input. CLI y GUI usan esta API.

### 1.3 Flujo de datos

```
Frontend (CLI/GUI)
       │
       ▼
  API bitemu.h
       │
       ▼
  engine_t + console_ops_t
       │
       ▼
  console_step(cycles)
       │
       ├── CPU step
       ├── Timer step
       ├── PPU step
       └── APU step
```

### 1.4 Tipos principales

| Tipo | Ubicación | Propósito |
|------|-----------|-----------|
| `engine_t` | `core/engine.h` | Contiene `console_t` y flag `running` |
| `console_t` | `core/console.h` | `ops` (tabla de funciones) + `impl` (estado opaco) |
| `console_ops_t` | `core/console.h` | `init`, `reset`, `step`, `load_rom`, `unload_rom`, `cycles_per_frame` |
| `bitemu_t` | `include/bitemu.h` | Handle público; contiene `gb_impl_t` + `engine_t` |
