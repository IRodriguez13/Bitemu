# Bitemu

Multi-console emulator: Game Boy (BE1) and Sega Genesis (BE2, stub). Generic engine designed for future editions (GBA, etc.).

**Author:** [Iván Ezequiel Rodriguez](https://github.com/IRodriguez13)

**License:** GNU Lesser General Public License v3.0 — see [LICENSE](LICENSE).

**Stack:** C (backend + CLI) · Python/PySide6 (GUI frontend).

## Features

### Core emulation
- Full LR35902 CPU — all 256 opcodes + CB prefix, interrupts, HALT bug
- PPU — backgrounds, window (internal line counter), sprites (8x8/8x16 with X-priority), DMG palettes, STAT edge detection, SCY/SCX latching
- APU — 4 channels (Square1, Square2, Wave, Noise), frame sequencer, length counters, volume envelopes, sweep
- Memory — ROM-only, MBC1 (with mode select), MBC2, MBC3 (with RTC), MBC5
- Timer — DIV, TIMA/TMA/TAC with overflow and interrupt

### Save system
- Battery saves (.sav) — automatic load/save on ROM open/close
- Save states (F6/F7) — BST v2 format with section-based forward/backward compatibility and CRC32 ROM validation

### Frontend
- Game library with cover art grid (fetched from libretro-thumbnails CDN with multi-source fallback)
- Main menu splash screen with console-adaptive branding
- Configurable keyboard mappings and gamepad support (WIP)
- Audio output via sounddevice (toggle in Options menu)
- Cross-platform GUI (PySide6) with menu bar, status bar, ROM folder config

### Infrastructure
- Automatic update notifications — checks GitHub Releases API on startup, prompts download if newer version exists
- CI/CD pipeline — automated builds on Windows, macOS, and Linux with native packaging (AppImage, DMG, Inno Setup)
- 157 automated tests (77 C core + 81 Python frontend) including ABI guard for save state binary compatibility (GB + Genesis)
- Version injection from git tags

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
│   BE1 - GB      │     │  BE2 - Genesis  │
│   CPU, PPU,     │     │  (stub, v1.0)   │
│   APU, Memory   │     │  Future: GBA    │
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
├── be2/               # Genesis backend (stub: ROM load, 320×224 framebuffer)
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

1. Click **Start** on the splash screen to open the game library
2. Set your ROM folder when prompted (or via Options > ROM folder)
3. Click a game to play, or **Ctrl+O** to open a ROM directly
4. **F6** to save state, **F7** to load state
5. Battery saves (.sav) are automatic

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

## Tests

```bash
make test          # all tests (158 total)
make test-core     # C core tests (77 tests: memory, APU, timer, API, PPU, MBC2, Genesis, ABI guard GB+Genesis)
make test-frontend # Python frontend tests (81 tests: bindings, keys, input config, profile, ROM scanner, scraper, update checker, bundle deps)
```

## Known limitations

| Area | Status |
|------|--------|
| **PPU** | Mode-level (not dot-accurate); timing-sensitive games may have minor HUD glitches |
| **APU** | Functional with occasional glitches in complex games |
| **Gamepad** | Detection and mapping WIP — keyboard fully functional |
| **RTC** | Real-time clock advances; save/load in .sav (MBC3) |
| **Boot ROM** | Not emulated; starts at 0x0100 with post-boot register state |
| **Serial** | Stub only — no link cable (single-player unaffected) |

## License

GNU Lesser General Public License v3.0. See [LICENSE](LICENSE).
