# Bitemu

Emulador de Game Boy. Motor genérico con interfaz reutilizable para futuras ediciones (Genesis, PS1, etc.).

**Stack:** solo **C** (backend + CLI) y **Python** (frontend gráfico). Sin Rust.

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

El engine recibe `ops` + `impl` (void*) y nunca conoce el hardware. La inyección se hace en el frontend (Python GUI o CLI en C).

## Estructura del proyecto

```
bitemu/
├── core/           # Motor genérico (Engine, Timing, Input, Utils)
├── be1/            # Game Boy (cpu, ppu, apu, memory, timer, console)
├── include/        # API pública (bitemu.h)
├── src/            # Wrapper API (bitemu.c)
├── main_cli.c      # Frontend CLI (C, headless)
├── Makefile
├── frontend/       # Frontend GUI (Python + PySide6)
└── Documentation/  # Documentación técnica (EN/ES)
```

Ver [Documentation/README.md](Documentation/README.md) para la documentación detallada de la arquitectura y subsistemas.

## Build

Solo **C** (backend y CLI) y **Python** (ventana). Sin Rust.

```bash
make          # compila lib + cli y ejecuta frontend Python
make lib      # libbitemu.so (para el frontend)
make cli      # bitemu-cli (terminal)
make run      # solo ejecuta la ventana Python
make help     # ver todos los objetivos
```

Requisitos: compilador C (gcc), Python 3.10+, PySide6. Ver `frontend/README.md`.

## Cómo lanzar la GUI

```bash
make run
```

O tras `make lib`: `cd frontend && python3 main.py`. Controles: WASD/flechas (D-pad), J/Z (A), K/X (B), U (Select), I/Enter (Start), F3=Pausa, F4=Reiniciar, Archivo → Abrir ROM.

## Uso

```bash
# Ventana (frontend Python)
make run

# Terminal (CLI en C)
./bitemu-cli -rom juego.gb
```

Controles en la GUI: WASD o flechas (D-pad), J/Z (A), K/X (B), U (Select), I/Enter (Start).

Los juegos con batería (p. ej. Pokémon Red/Blue) guardan la partida en un archivo `.sav` junto a la ROM; se carga al abrir la ROM y se guarda al cerrar el emulador.

## Licencia

Bitemu se distribuye bajo la **GNU Lesser General Public License v3.0** (o posterior). Ver [LICENSE](LICENSE) para el texto completo. Permite usar la biblioteca (core C y `libbitemu`) en programas propietarios siempre que se cumplan los términos de la LGPL (p. ej. enlazado dinámico y entrega del código fuente de la biblioteca si se modifica).

## Limitaciones y qué falta para juegos complejos

Lo que **ya está** implementado:

- CPU LR35902: juego de instrucciones completo (incl. prefijo CB), interrupciones, HALT, EI con retardo.
- Memoria: mapa completo, **MBC1, MBC3 y MBC5**, RAM de batería (.sav), **MBC3 RTC** (registros y latch 0x01/0x00), JOYP, DIV, DMA a OAM, IF/IE.
- PPU: fondos, sprites (8×8 y 8×16), **capa Window** (WY, WX, tile map), scroll, paletas BGP/OBP, **límite de 10 sprites por línea**, STAT y VBLANK.
- Timer: DIV, TIMA, TAC, interrupción de timer.
- Input: joypad vía API (GUI inyecta estado con teclado).

Para **juegos más complejos** puede seguir faltando o estar simplificado:

| Área | Qué falta o está incompleto |
|------|----------------------------|
| **PPU** | Prioridad exacta OBJ/BG en algunos edge cases; posible bug de OAM cuando la PPU lee durante modo 2/3. |
| **APU** | El sonido está esqueleto: se leen los registros NRxx pero **no hay salida de audio** hacia el sistema. |
| **RTC** | Los registros RTC (S, M, H, D) se leen/escriben y el latch funciona; **el tiempo no avanza en tiempo real** (no se actualiza con el reloj del sistema). |
| **Precisión** | No es cycle-accurate; pequeños desfases pueden afectar a demos o juegos que dependen de timing muy exacto. |
| **Boot ROM** | Se arranca en 0x0100 (estado post-boot); no se emula la boot ROM. |

Resumen: la **capa Window**, **MBC5**, **10 sprites por línea** y **RTC básico (MBC3)** ya están. Para la mayoría de juegos comerciales lo más notable que falta es **salida de audio** y, si el juego usa RTC real, **avance del tiempo** en los registros RTC.
