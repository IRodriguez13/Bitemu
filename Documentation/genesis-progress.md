# Progreso de emulación Genesis / Mega Drive

Estimación aproximada del estado de implementación del core Genesis (BE2).

## Resumen: **~75–90%**

### Estructura modular
- **genesis_constants.h**: todas las constantes (sin magic numbers)
- **be2/vdp/**: VDP (comandos, render, timing)
- **be2/cpu/**: 68000 (cpu.c + cpu_handlers.c)
- **be2/ym2612/**, **be2/psg/**: audio

---

## Implementado ✓

### CPU 68000
- Opcodes principales (MOVE, ADD, SUB, CMP, JMP, JSR, LEA, etc.)
- Interrupciones (VBlank nivel 6, HBlank nivel 4)
- Vectores de excepción
- STOP, RESET, TRAP, RTD, TRAPV
- Modos EA: (d16,PC), (d8,PC,Xn), (d8,An,Xn) — direccionamiento indexado y PC-relativo
- ~85–90% de los opcodes más usados

### VDP
- VRAM, CRAM, VSRAM
- Registros de control
- Render: planos A y B, sprites (SAT, tamaños 8×8 a 8×32)
- Scroll vertical (VSRAM) y horizontal (tabla en VRAM)
- Prioridad de planos (bit 15 en tile map) y sprites (bit 15 en word 2)
- Modos H32 (256 px) / H40 (320 px) — reg 12
- Window plane (reg 3 base, reg 18 WX, reg 19 WY)
- Color de fondo (reg 7)
- Timing: ciclos por línea, scanlines
- Status register (VBlank, HBlank)
- HV counter (0xC00008)
- Interrupciones VBlank/HBlank

### Memoria
- ROM (con SMD/MD deinterleave)
- RAM 64KB
- SRAM (batería): detección "RA" en header 0x1B0, carga/guardado .sav en `saves/`
- Mapeo completo (VDP, IO, Z80 space)
- Joypad (2 puertos)
- **TMSS**: write "SEGA" a 0xA14000 desbloquea VDP (Model 1)
- **Lock-on**: Sonic & Knuckles, Sonic 2 & Knuckles, Sonic 3 & Knuckles (ROM ≥4MB, A130F1 patch/SRAM)

### Z80
- RAM 8KB, ejecución cuando tiene el bus
- Bus request / reset (0xA11100, 0xA11200)
- Mapa: 0x0000 RAM, 0x4000 YM2612, 0x6000 bank, 0x7F11 PSG, 0x8000-0xFFFF ROM
- Prefijos CB (RLC, RRC, RL, RR, SLA, SRA, SRL, BIT, SET, RES), DD (IX), FD (IY)

### PSG (SN76489)
- Escrituras en 0xC00011 aceptadas
- Síntesis completa: 3 tonos (square) + ruido
- Tabla de volumen 2dB, LFSR 16-bit (white/periodic)

### YM2612
- Registros y puertos (0xA04000–0xA04003)
- Canal DAC (0x2A/0x2B): samples PCM 8-bit
- FM 6 canales: fase, key on/off, MUL, DT, TL, pan
- **8 algoritmos FM** (M1→M2→M3→C, M1→M2→C+M3→C, etc.)
- **Envolventes ADSR** (AR, D1R, D2R, RR, SL por operador)

### Save states
- **Módulo universal** (`core/savestate.h`, `core/savestate.c`): header común, secciones con tamaño prefijado, validación (magic, console_id, version, CRC)
- Cada consola implementa save/load vía `console_ops_t` usando `bst_*`
- Política: enforcing (version), retrocompatibilidad (secciones), ABI Guard en tests
- Serialización Genesis: CPU, RAM, VDP, joypad, PSG, YM2612, Z80

---

## Pendiente / Parcial

### Audio
- ✓ PSG, YM2612 (FM 8 algoritmos + DAC + ADSR), Z80 (CB/DD/FD)

### VDP avanzado
- ✓ Prioridad, H32/H40, window plane (implementados)

### DMA ✓
- 68k → VRAM/CRAM/VSRAM
- VRAM fill (trigger en data port write)
- VRAM copy

### Z80
- ✓ CB/DD/FD prefijos implementados

### Referencia
- **Genesis-Plus-GX**: ver [genesis-plus-gx-reference.md](genesis-plus-gx-reference.md) para mapeo core/frontend e ideas a copiar.
- **ROMs de test**: ver [genesis-test-roms.md](genesis-test-roms.md) para evaluar el emulador.

---

### Input
- Joypad 3-button (D-pad, A, B, C, Start)
- **Joypad 6-button**: protocolo TH (0xA10003/05), ciclos 1-9, X/Y/Z/Mode en ciclo 7
- API: `bitemu_set_input` (8-bit), `bitemu_set_input_genesis` (12-bit)
- GUI: teclas Q/E/R para X, Y, Z

## Referencia para "100%"

Un Genesis completo requeriría:
- 68000: ~95%+ opcodes, timing exacto
- VDP: sprites, DMA, scroll, todos los modos
- YM2612: emulación FM fiel
- PSG: emulación completa
- Z80: emulación completa
- SRAM, TMSS, periféricos
- Lightgun (Menacer, etc.)

**Juegos jugables**: muchos títulos simples pueden arrancar; la mayoría tendrá problemas de audio, sprites o timing. Juegos complejos (Sonic, etc.) necesitan más trabajo.
