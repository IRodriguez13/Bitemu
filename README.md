# Bitemu

Emulador de Game Boy. Motor genérico con interfaz reutilizable para futuras ediciones (Genesis, PS1, etc.).

## Arquitectura

El **core** es una interfaz que controla todo y no sabe nada del hardware concreto. Cada consola implementa el contrato vía un adaptador.

```
                    ┌──────────────────────┐
                    │      Bitemu Core     │
                    │──────────────────────│
                    │ - Engine             │
                    │ - Timing             │
                    │ - Input              │
                    │ - Utils / Logging    │
                    └─────────┬────────────┘
                              │
                              ▼
                  ┌────────────────────────┐
                  │  Console Interface     │
                  │ (Adaptador Core ↔ BE)  │
                  └─────────┬──────────────┘
                            │
       ┌────────────────────┼────────────────────┐
       ▼                    ▼                    ▼
┌─────────────┐      ┌───────────────┐      ┌───────────────┐
│   BE1 - GB  │      │  BE2 - Genesis │      │   BE3 - PS1   │
│─────────────│      │───────────────│      │───────────────│
│ CPU: LR35902│      │ CPU: 68000    │      │ CPU: MIPS R3000│
│ PPU: LCD    │      │ VDP: Video     │      │ GPU: Graphics │
│ APU: Sound  │      │ YM2612/PSG     │      │ SPU: Sound    │
│ Memory & Reg│      │ Memory & Reg   │      │ Memory & Reg  │
└─────────────┘      └───────────────┘      └───────────────┘
```

**Este repo:** solo BE1 (Game Boy). BE2 y BE3 viven en repos separados y reutilizan el mismo core.

## Cómo funciona

- **core/** — No incluye nada de `be1/`. Solo define `console_ops_t` (init, reset, step, load_rom, unload_rom) y delega vía punteros a función.
- **core/console.h** — Contrato que toda consola debe cumplir.
- **be1/console.c** — Adaptador: implementa la interfaz y traduce las llamadas genéricas a CPU, PPU, APU concretos.
- **be1/** — Emulación del hardware: LR35902, PPU, APU, memoria.

El engine recibe `ops` + `impl` (void*) y nunca conoce el hardware. La inyección se hace en el frontend (Rust GUI o CLI).

## Estructura del proyecto

```
bitemu/
├── core/           # Motor genérico (Engine, Timing, Input, Utils)
├── be1/            # Game Boy (cpu, ppu, apu, memory, timer, console)
├── include/        # API pública (bitemu.h)
├── src/            # Wrapper API (bitemu.c)
├── main_cli.c      # Frontend CLI (headless)
├── Makefile
├── rust/           # Frontend GUI (ventana minifb)
└── Documentation/  # Documentación técnica (EN/ES)
```

Ver [Documentation/README.md](Documentation/README.md) para la documentación detallada de la arquitectura y subsistemas.

## Build

```bash
make          # o make gui → bitemu (Rust GUI)
make cli      # → bitemu-cli (C headless)
```

## Uso

```bash
# GUI (ventana) — por defecto
./bitemu -rom juego.gb

# Headless (sin ventana)
./bitemu -rom juego.gb --cli

# Alternativa C (solo headless)
./bitemu-cli -rom juego.gb
```

Controles: WASD (D-pad), J/K (A/B), U/I (Select/Start), Escape.

Los juegos con batería (p. ej. Pokémon Red/Blue) guardan la partida en un archivo `.sav` junto a la ROM; se carga al abrir la ROM y se guarda al cerrar el emulador.
