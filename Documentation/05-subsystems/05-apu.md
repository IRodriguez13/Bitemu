# APU (Audio Processing Unit)

## English

### 9.1 Overview

The Game Boy APU has four sound channels:

1. **Square 1**: Tone with sweep (frequency can change over time).
2. **Square 2**: Tone, no sweep.
3. **Wave**: 4-bit sample playback from 16-byte wave RAM.
4. **Noise**: Pseudo-random noise (LFSR).

### 9.2 Registers (NR10–NR52)

| Register | Range | Purpose |
|----------|-------|---------|
| NR52 | 0xFF26 | Master enable (bit 7). Bits 0–3: channel on/off. |
| NR10–NR14 | Square 1 | Sweep, duty, length, envelope, frequency |
| NR21–NR24 | Square 2 | Duty, length, envelope, frequency |
| NR30–NR34 | Wave | Enable, length, volume, frequency |
| NR41–NR44 | Noise | Length, envelope, clock, trigger |
| NR50, NR51 | Output | Volume, panning |

### 9.3 Implementation

- `gb_apu_step(apu, mem, cycles)` advances the APU by `cycles` T-cycles.
- Each channel has internal state (duty position, wave position, LFSR).
- When NR52 bit 7 = 0, APU is off; no processing.
- **Current status**: The APU reads registers and advances internal counters. No audio output is generated yet (no callback to host or sample buffer). The structure is in place for future integration.

### 9.4 Channel Logic (simplified)

- **Square**: Duty cycle (4 patterns: 12.5%, 25%, 50%, 75%). Frequency from NR13/NR14.
- **Wave**: 32 samples (4-bit each) from wave RAM. Volume from NR32.
- **Noise**: 15-bit LFSR. Clock divisor from NR43.

---

## Español

### 9.1 Resumen

El APU del Game Boy tiene cuatro canales:

1. **Square 1**: Tono con sweep.
2. **Square 2**: Tono sin sweep.
3. **Wave**: Reproducción de muestras de 4 bits desde wave RAM.
4. **Noise**: Ruido pseudoaleatorio (LFSR).

### 9.2 Registros

- NR52: Master enable, estado de canales.
- NR10–NR14: Square 1.
- NR21–NR24: Square 2.
- NR30–NR34: Wave.
- NR41–NR44: Noise.
- NR50, NR51: Volumen y panning.

### 9.3 Implementación

- `gb_apu_step(apu, mem, cycles)` avanza el APU.
- No hay salida de audio aún; la estructura está preparada para integración futura.
