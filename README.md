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

**Multiplataforma:** Linux (make), macOS y Windows: `cd rust && cargo build --release`. Config/recientes: Linux `~/.config/bitemu/`, macOS `~/Library/Application Support/bitemu/`, Windows `%APPDATA%\bitemu\`. Se necesita Rust y un C compiler (Xcode CLI en macOS, VS Build Tools o MinGW en Windows).

## Cómo lanzar la GUI

Tras compilar con `make`:

```bash
./bitemu -rom /ruta/a/tu/juego.gb
```

- **Sin `--cli`** → se abre la ventana (GUI con minifb).
- **Con `--cli`** → modo headless (sin ventana, corre en terminal).
- **Controles:** D-pad: WASD o flechas. A=J o Z, B=K o X. Select=U, Start=I o Enter. Escape = cerrar. F3=Pausa, F4=Reset, Ctrl+O=Abrir ROM.

El ejecutable `bitemu` está en la raíz del proyecto. Si lo quieres en el PATH, puedes hacer `sudo cp bitemu /usr/local/bin/` o añadir el directorio al PATH.

## Uso

```bash
# GUI (ventana) — por defecto
./bitemu -rom juego.gb

# Headless (sin ventana)
./bitemu -rom juego.gb --cli

# Alternativa C (solo headless)
./bitemu-cli -rom juego.gb
```

Controles: WASD o flechas (D-pad), J/Z (A), K/X (B), U (Select), I/Enter (Start), Escape (salir). Ventana 4x (640×576).

Los juegos con batería (p. ej. Pokémon Red/Blue) guardan la partida en un archivo `.sav` junto a la ROM; se carga al abrir la ROM y se guarda al cerrar el emulador.

## Limitaciones y qué falta para juegos complejos

Lo que **ya está** implementado:

- CPU LR35902: juego de instrucciones completo (incl. prefijo CB), interrupciones, HALT, EI con retardo.
- Memoria: mapa completo, **MBC1, MBC3 y MBC5**, RAM de bater2ía (.sav), **MBC3 RTC** (registros y latch 0x01/0x00), JOYP, DIV, DMA a OAM, IF/IE.
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
