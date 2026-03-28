# Bitemu Documentation

Comprehensive technical documentation of the Bitemu Game Boy emulator architecture.  
Documentación técnica detallada de la arquitectura del emulador Bitemu para Game Boy.

---

## Part I: Core Architecture / Arquitectura Core

The reusable engine that can drive multiple console emulators (GB, Genesis, PS1).

| Document | English | Español |
|----------|---------|---------|
| [01 - Overview](01-core-overview.md) | Core concepts, design goals | Conceptos, objetivos de diseño |
| [02 - Engine](02-engine.md) | Main loop, initialization | Loop principal, inicialización |
| [03 - Console Interface](03-console-interface.md) | `console_ops_t` contract | Contrato `console_ops_t` |
| [04 - Public API](04-public-api.md) | `bitemu.h` for frontends | API para frontends |

---

## Part II: BE1 Subsystems / Subsistemas BE1

Game Boy–specific implementation details.

| Document | English | Español |
|----------|---------|---------|
| [05 - CPU (LR35902)](05-subsystems/01-cpu.md) | Instructions, interrupts, HALT | Instrucciones, interrupciones |
| [06 - Memory](05-subsystems/02-memory.md) | Map, MBC1, I/O, DMA | Mapa, MBC1, I/O, DMA |
| [07 - PPU](05-subsystems/03-ppu.md) | Modes, tiles, sprites, framebuffer | Modos, tiles, sprites |
| [08 - Timer](05-subsystems/04-timer.md) | DIV, TIMA, TAC, overflow | DIV, TIMA, TAC |
| [09 - APU](05-subsystems/05-apu.md) | Square, Wave, Noise channels | Canales de sonido |
| [10 - Input](05-subsystems/06-input.md) | Joypad, polling, injection | Joypad, polling |

---

## Part III: Features / Funcionalidades

| Feature | Description |
|---------|-------------|
| [Profiling (GNU gprof)](07-gprof.md) | Flujo `profile-cli` / `profile-report`, lectura de flat profile y call graph |
| [Audio Implementation](audio-implementation.md) | APU architecture, buffer circular, frontend playback |
| [Audio Improvements](audio-improvements.md) | Known issues and fixes applied |
| [Branding System](05-branding.md) | Adaptive visual identity per console edition |
| [Save States](06-save-states.md) | BST v2 format, ABI guard, compatibility rules |

---

## File Structure / Estructura de Archivos

```
bitemu/
├── core/           # Reusable engine (no console-specific code)
├── be1/            # Game Boy implementation
├── include/        # Public API (bitemu.h)
├── src/            # API wrapper (bitemu.c)
├── frontend/       # GUI frontend (Python + PySide6)
├── tests/          # Test suites (core/ and frontend/)
├── installer/      # Packaging (Inno Setup template)
└── Documentation/  # This directory
```
