# Input

## English

### 10.1 Overview

The Game Boy has a simple joypad: 4-direction D-pad + 4 buttons (A, B, Select, Start). The CPU reads the state via the JOYP register (0xFF00).

### 10.2 JOYP Register

- **Write**: Bits 4–5 select which group to read.
  - Bit 5 = 0: D-pad (R, L, U, D)
  - Bit 4 = 0: Buttons (A, B, Select, Start)
- **Read**: Returns `0xC0 | selection_bits | (~pressed_bits)`.
  - Selected bits are 0 when pressed, 1 when released.

### 10.3 Internal State

`gb_mem_t.joypad_state` stores the raw state:

- Bit 0: Right
- Bit 1: Left
- Bit 2: Up
- Bit 3: Down
- Bit 4: A
- Bit 5: B
- Bit 6: Select
- Bit 7: Start

**1 = pressed, 0 = released.** Default (no keys): 0xFF.

### 10.4 Polling vs Injection

- **`gb_input_poll(mem)`**: Called from `gb_step`. On Linux, reads from stdin (raw mode): WASD, IJKL, U, I, Enter. Updates `joypad_state`.
- **`gb_input_set_state(mem, state)`**: Allows external injection (e.g. from GUI). Used by `bitemu_set_input()`.

### 10.5 Joypad Interrupt

- Writing to JOYP can trigger the joypad interrupt (IF bit 4).
- Not implemented in current version; polling only.

### 10.6 Key Mapping (CLI)

| Key | Action |
|-----|--------|
| W, A, S, D | Up, Left, Down, Right |
| J, K | A, B |
| U, I | Select, Start |
| Enter | Start |

---

## Español

### 10.1 Resumen

El Game Boy tiene un joypad: D-pad de 4 direcciones + 4 botones (A, B, Select, Start). La CPU lee el estado vía el registro JOYP (0xFF00).

### 10.2 Registro JOYP

- **Escritura**: Bits 4–5 seleccionan qué grupo leer (D-pad o botones).
- **Lectura**: Bits seleccionados son 0 cuando están pulsados.

### 10.3 Estado interno

`joypad_state`: 1 = pulsado, 0 = soltado. Por defecto: 0xFF.

### 10.4 Polling vs inyección

- **`gb_input_poll`**: Lee desde stdin (modo raw) en CLI.
- **`gb_input_set_state`**: Inyección externa (GUI). Usado por `bitemu_set_input()`.

### 10.5 Mapeo de teclas (CLI)

| Tecla | Acción |
|-------|--------|
| W, A, S, D | Arriba, Izq, Abajo, Der |
| J, K | A, B |
| U, I | Select, Start |
| Enter | Start |
