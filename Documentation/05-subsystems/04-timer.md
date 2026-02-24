# Timer

## English

### 8.1 Overview

The Game Boy has a timer system used for real-time events (e.g. gameplay timing, interrupts). It consists of:

- **DIV** (0xFF04): Divider register. Upper 8 bits of a 16-bit internal counter.
- **TIMA** (0xFF05): Timer counter. Increments at a rate set by TAC.
- **TMA** (0xFF06): Timer modulo. When TIMA overflows, it is reloaded from TMA.
- **TAC** (0xFF07): Timer control (enable, clock select).

### 8.2 DIV Internal Counter

- 16-bit counter (`timer_div` in `gb_mem_t`).
- Increments every T-cycle.
- DIV = `timer_div >> 8` (upper byte).
- **Writing to DIV** (any value) resets the internal counter to 0.

### 8.3 TIMA

- When TAC bit 2 = 1, TIMA is enabled.
- TAC bits 0–1 select the clock divisor:

| TAC bits 0–1 | Divisor | Frequency |
|--------------|---------|-----------|
| 00 | 1024 | 4096 Hz |
| 01 | 16 | 262144 Hz |
| 10 | 64 | 65536 Hz |
| 11 | 256 | 16384 Hz |

- When the internal counter reaches the divisor boundary, TIMA increments.
- When TIMA overflows (0xFF → 0x00): TIMA is reloaded from TMA, and IF bit 2 (timer interrupt) is set.

### 8.4 Implementation

```c
gb_timer_step(mem, cycles);
```

- `mem->timer_div += cycles`
- `mem->io[GB_IO_DIV] = timer_div >> 8`
- If TAC enabled: compute TIMA increments from divisor; on overflow, reload TMA and set IF bit 2.

### 8.5 Usage

- Games use the timer for timing (e.g. 60 Hz logic).
- Blargg tests use it to test timer interrupt and HALT wake-up.

---

## Español

### 8.1 Resumen

El Game Boy tiene un sistema de timer para eventos en tiempo real:

- **DIV** (0xFF04): Registro divisor. Byte superior del contador interno de 16 bits.
- **TIMA** (0xFF05): Contador de timer.
- **TMA** (0xFF06): Módulo. Reload cuando TIMA hace overflow.
- **TAC** (0xFF07): Control (enable, selector de reloj).

### 8.2 Contador interno DIV

- Contador de 16 bits.
- Incrementa cada T-ciclo.
- DIV = `timer_div >> 8`.
- **Escribir en DIV** resetea el contador a 0.

### 8.3 TIMA

- TAC bit 2 = 1: habilita TIMA.
- TAC bits 0–1: divisor (1024, 16, 64, 256 ciclos).
- Overflow: TIMA = TMA, IF bit 2 = 1.

### 8.4 Implementación

`gb_timer_step(mem, cycles)` actualiza DIV y TIMA según el número de ciclos.
