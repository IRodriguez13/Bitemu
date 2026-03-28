# Bitemu

Multi-console emulator: **Game Boy (BE1)** and **Sega Genesis / Mega Drive (BE2)**. Núcleo genérico pensado para futuras ediciones (GBA, etc.).

**Author:** [Iván Ezequiel Rodriguez](https://github.com/IRodriguez13)

**License:** GNU Lesser General Public License v3.0 — see [LICENSE](LICENSE).

**Stack:** C (backend + CLI) · Python/PySide6 (GUI frontend).

## Features

### BE1 — Game Boy (core emulation)
- Full LR35902 CPU — all 256 opcodes + CB prefix, interrupts, HALT bug
- PPU — backgrounds, window (internal line counter), sprites (8x8/8x16 with X-priority), DMG palettes, STAT edge detection, SCY/SCX latching
- APU — 4 channels (Square1, Square2, Wave, Noise), frame sequencer, length counters, volume envelopes, sweep
- Memory — ROM-only, MBC1 (with mode select), MBC2, MBC3 (with RTC), MBC5
- Timer — DIV, TIMA/TMA/TAC with overflow and interrupt

### BE2 — Genesis (en desarrollo activo)
- 68000: fetch/decode/ejecutar; IRQ desde VDP; líneas de instrucción reservadas (LINE A / LINE F) → excepción ilegal
- VDP: VRAM/CRAM/VSRAM, planos A/B, ventana, sprites, DMA, HV/status aproximados; framebuffer **RGB888** (3 bytes/píxel) en host
- Audio: YM2612 (FM), SN76489 PSG, mezcla con sample timing NTSC; Z80 para bus de sonido cuando tiene el bus
- Memoria: ROM/SRAM (header `RA`), lock-on Sonic & Knuckles, TMSS/VDP/joypad 3 y 6 botones, `.smd`/`.md` vía API

#### Genesis MVP (seguimiento)
- **Hecho reciente:** modo **PAL** (~50 Hz) vía región cartucho (`E` sin `J`/`U` en el header), ciclos por frame/audio/ ratio Z80 alineados; API **`bitemu_get_frame_hz`** para GUIs; extensión de save state **GEN1** (Z80 RAM, timing VDP parcial, framebuffer, `is_pal`, etc.); GUI sincroniza QTimer y poll de gamepad al Hz del core. **VDP:** ventana derecha (reg **17** bit 7), bit **COL** por solape de sprites opacos + lectura de status; **YM2612:** bit **busy** aproximado en lectura de status; **SRAM:** tamaño y espejo desde cabecera **0x1B4–0x1BB** cuando hay `RA`.
- **Pendiente / conocido:** precisión VDP línea a línea e IRQ, DMA vs 68k, YM2612 vs test ROMs. **Sync 68000↔Z80:** módulo [`be2/cpu_sync`](be2/cpu_sync) + [Documentation/08-cpu-sync-genesis.md](Documentation/08-cpu-sync-genesis.md) (RAM Z80 con BUSREQ; proporción de relojes). Checklist: [Documentation/genesis-test-roms.md](Documentation/genesis-test-roms.md).
- **Perfilado tiempo por función (gprof):** guía en [Documentation/07-gprof.md](Documentation/07-gprof.md), resumen con `make profile-help`. **Linux `perf`:** `make perf-help`, `make perf-cli-ready`, `make perf-record-cli PERF_ROM=...`. **Tablas gperf** (ciclos nominales, no perfilado): `make gperf-help`.

### Save system
- Battery saves (.sav) — automatic load/save on ROM open/close (GB + Genesis SRAM)
- Save states (F6/F7) — BST v2 format with section-based forward/backward compatibility and CRC32 ROM validation

### Frontend
- Game library with cover art grid (fetched from libretro-thumbnails CDN with multi-source fallback)
- Main menu splash screen with console-adaptive branding
- Configurable keyboard mappings and gamepad support (pygame joystick; hotplug vía eventos SDL)
- Native core audio path (ALSA / CoreAudio / WASAPI; toggle in Options)
- Cross-platform GUI (PySide6) with menu bar, status bar, ROM folder config

### Infrastructure
- Automatic update notifications — checks GitHub Releases API on startup, prompts download if newer version exists
- CI/CD pipeline — automated builds on Windows, macOS, and Linux with native packaging (AppImage, DMG, Inno Setup)
- Automated tests (C core + Python frontend) including ABI guard for save state binary compatibility (GB + Genesis)
- Version injection from git tags

## Architecture

The core defines a generic `console_ops_t` interface. Each console (Game Boy, Genesis, future GBA) implements this contract. The frontend uses the public C API only.

```
┌──────────────────────┐
│      Bitemu Core     │  core/
│  Engine, Timing,     │  console.h defines the contract
│  Input, Utils        │
└─────────┬────────────┘
          │ console_ops_t
          ▼
┌─────────────────┐     ┌─────────────────┐
│   BE1 - GB      │     │  BE2 - Genesis  │
│   CPU, PPU,     │     │  68000, VDP,    │
│   APU, Memory   │     │  YM2612, PSG,   │
│                 │     │  Z80, mem map   │
└─────────────────┘     └─────────────────┘
          │
          ▼
┌─────────────────┐     ┌─────────────────┐
│  libbitemu.so   │     │  Frontend GUI   │
│  Public C API   │────▶│  Python/PySide6 │
│  (bitemu.h)     │     │  (ctypes)       │
└─────────────────┘     └─────────────────┘
```

## Project structure

```
bitemu/
├── core/              # Generic engine (console.h, engine, timing, input, utils, crc32)
├── be1/               # Game Boy backend (cpu, ppu, apu, memory, timer)
├── be2/               # Genesis backend (68000, vdp/rgb framebuffer, fm, psg, z80, memory)
├── include/           # Public API (bitemu.h)
├── src/               # API implementation (bitemu.c)
├── frontend/          # GUI frontend (Python + PySide6)
├── tests/             # Test suites (core/ for C, frontend/ for Python, be2/ for Genesis)
├── installer/         # Packaging scripts (Inno Setup)
├── .github/workflows/ # CI/CD pipeline
├── Documentation/     # Technical docs
├── Makefile
└── VERSION
```

## Build

Requirements: C compiler (gcc/clang), Python 3.10+, PySide6.

```bash
make all      # compile + test + run
make lib      # libbitemu.so only
make cli      # bitemu-cli (headless)
make run      # launch GUI frontend
make test     # run all tests (C core + Python frontend)
make help     # list all targets
profile-help  # GNU gprof → ver Documentation/07-gprof.md
perf-help     # Linux perf (perf-cli-ready, perf-record-cli, perf-report)
gperf-help    # gperf por backend (ciclos/tablas; no es gprof)
```

Linux: el hilo ALSA acepta `BITEMU_ALSA_POLL_US` (microsegundos entre polls, rango 100–50000; por defecto 2000) para equilibrio latencia vs uso de CPU.

Compilación: por defecto el `Makefile` usa **`PROFILE=1`** (GCC `-pg` para gprof, sin LTO). Para el binario optimizado habitual: `PROFILE=0 make lib cli`.

## Usage

```bash
make run
```

1. Click **Start** on the splash screen to open the game library
2. Set your ROM folder when prompted (or via Options > ROM folder)
3. Click a game to play, or **Ctrl+O** to open a ROM directly
4. **F6** to save state, **F7** to load state
5. Battery saves (.sav) are automatic

### Controls (Game Boy default)

| Action  | Keys                  |
|---------|----------------------|
| D-pad   | W A S D / Arrow keys  |
| A       | J / Z                 |
| B       | K / X                 |
| Select  | U                     |
| Start   | I / Enter             |
| Pause   | F3                    |
| Reset   | F4                    |
| Close ROM | F5                  |
| Save state | F6                 |
| Load state | F7                 |

Genesis usa el mismo esquema de teclas ampliado (6 botones) según `frontend/bitemu_gui/keys.py` y opciones de entrada.

## Tests

```bash
make test          # all tests
make test-core     # C core (GB + Genesis, ABI, API, etc.)
make test-frontend # Python frontend (bindings, keys, input, etc.)
```

### Regresión manual (Genesis)

Ver [Documentation/genesis-test-roms.md](Documentation/genesis-test-roms.md): ROMs de test recomendadas, emuladores de referencia y flujo sugerido (reproducir en bitemu, anotar fallos, acotar en tests C cuando sea posible sin binarios en el repo).

## Known limitations

| Area | Status |
|------|--------|
| **GB PPU** | Mode-level (not dot-accurate); LYC escrito actualiza STAT al instante; HUD sensible al timing posible |
| **GB APU** | Functional with occasional glitches in complex games |
| **Genesis VDP** | Timing/IRQ aproximados; SOVR por línea (>20 sprites); DMA vs 68k simplificado en partes |
| **Genesis audio** | YM2612 (TL ~0.75 dB/step), PSG y Z80; afinación fina vs hardware según juego |
| **Gamepad** | pygame opcional; teclado completo; mapeo por consola en Options |
| **GB RTC** | Real-time clock advances; save/load in .sav (MBC3) |
| **Boot ROM** | Not emulated; GB starts at 0x0100 with post-boot register state |
| **Serial (GB)** | Stub only — no link cable (single-player unaffected) |

## License

GNU Lesser General Public License v3.0. See [LICENSE](LICENSE).
