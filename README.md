# Bitemu

Game Boy emulator with a generic engine designed for future console editions (Genesis, GBA, etc.).

**Author:** [Iván Ezequiel Rodriguez](https://github.com/IRodriguez13)

**Stack:** C (backend + CLI) and Python (GUI frontend). No Rust.

## Features

- Full LR35902 CPU (all opcodes including CB prefix, interrupts, HALT)
- PPU with backgrounds, sprites (8x8 / 8x16), Window layer, scroll, palettes, 10 sprites per scanline limit
- APU with 4 channels (Square1, Square2, Wave, Noise), frame sequencer, length counters, volume envelopes
- Memory: MBC1, MBC3 (with RTC registers), MBC5
- Battery saves (.sav) -- automatic load/save on ROM open/close
- **Save states** (F6/F7) -- save and restore full emulator state at any point (.bst format with CRC32 validation)
- Audio output via sounddevice (toggle in Options menu)
- Splash screen with boot sound
- Cross-platform GUI (PySide6) with menu bar, status bar, recent ROMs
- CI/CD pipeline for automated builds on Windows, macOS, and Linux

## Architecture

The core defines a generic `console_ops_t` interface. Each console (Game Boy, future Genesis, GBA) implements this contract. The frontend and public API never touch hardware-specific code.

```
┌──────────────────────┐
│      Bitemu Core     │  core/
│  Engine, Timing,     │  console.h defines the contract
│  Input, Utils        │
└─────────┬────────────┘
          │ console_ops_t
          ▼
┌─────────────────┐     ┌─────────────────┐
│   BE1 - GB      │     │  Future: BE2,   │
│   CPU, PPU,     │     │  BE_GBA, etc.   │
│   APU, Memory   │     │                 │
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
├── include/           # Public API (bitemu.h)
├── src/               # API implementation (bitemu.c)
├── frontend/          # GUI frontend (Python + PySide6)
├── tests/             # Test suites (core/ for C, frontend/ for Python)
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
```

## Usage

```bash
make run
```

1. **Ctrl+O** to open a ROM file (.gb / .gbc)
2. Play using keyboard (see Controls below)
3. **F6** to save state, **F7** to load state
4. Battery saves (.sav) are automatic

### Controls

| Action  | Keys                  |
|---------|-----------------------|
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

## Save states

Press **F6** to save the full emulator state to a `.bst` file (saved next to the ROM). Press **F7** to restore it. The state file includes a CRC32 of the ROM to prevent loading a state against the wrong game. The ROM must be loaded first before restoring a state.

## Tests

```bash
make test          # all tests
make test-core     # C core tests only (50 tests)
make test-frontend # Python frontend tests only (28 tests)
```

## Limitations

| Area | Status |
|------|--------|
| **PPU** | OBJ/BG priority edge cases may have minor issues |
| **APU** | Functional with some glitches in complex games |
| **RTC** | Registers read/write and latch work; time does not advance in real-time |
| **Precision** | Not cycle-accurate; timing-sensitive demos may have issues |
| **Boot ROM** | Not emulated; starts at 0x0100 (post-boot state) |

## License

GNU Lesser General Public License v3.0. See [LICENSE](LICENSE).
