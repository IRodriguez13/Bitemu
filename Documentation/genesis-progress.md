# Progreso de emulación Genesis / Mega Drive

Estimación aproximada del estado de implementación del core Genesis (BE2).

## Resumen: **~30–35%**

---

## Implementado ✓

### CPU 68000
- Opcodes principales (MOVE, ADD, SUB, CMP, JMP, JSR, LEA, etc.)
- Interrupciones (VBlank nivel 6, HBlank nivel 4)
- Vectores de excepción
- STOP, RESET, TRAP
- ~70–80% de los opcodes más usados

### VDP
- VRAM, CRAM, VSRAM
- Registros de control
- Render básico de tiles (plano A)
- Timing: ciclos por línea, scanlines
- Status register (VBlank, HBlank)
- HV counter (0xC00008)
- Interrupciones VBlank/HBlank

### Memoria
- ROM (con SMD/MD deinterleave)
- RAM 64KB
- Mapeo completo (VDP, IO, Z80 space)
- Joypad (2 puertos)

### Z80 (básico)
- RAM 8KB
- Bus request / reset (0xA11100, 0xA11200)
- Acceso 68k al espacio Z80

### PSG (SN76489)
- Escrituras en 0xC00011 aceptadas
- Registros de tono y volumen parseados
- Síntesis de audio pendiente

### YM2612 (stub)
- Registros y puertos (0xA04000–0xA04003)
- Sin síntesis FM (audio silencio)

### Save states
- Serialización de CPU, RAM, VDP, joypad

---

## Pendiente / Parcial

### Audio
- **YM2612**: síntesis FM completa (6 canales)
- **PSG**: SN76489 (3 canales de tono + ruido)
- **Z80**: ejecución para driver de sonido

### VDP avanzado
- Sprites
- Scroll (H/V)
- Prioridad de planos
- DMA (VRAM fill, copy)
- Modos 32/40 columnas

### Z80 completo
- Emulación de instrucciones
- ROM bank
- Acceso a YM2612 desde Z80

### Otros
- TMSS (licencia SEGA en Model 1)
- SRAM (batería)
- Lock-on (Sonic & Knuckles, etc.)

---

## Referencia para "100%"

Un Genesis completo requeriría:
- 68000: ~95%+ opcodes, timing exacto
- VDP: sprites, DMA, scroll, todos los modos
- YM2612: emulación FM fiel
- PSG: emulación completa
- Z80: emulación completa
- SRAM, TMSS, periféricos

**Juegos jugables**: muchos títulos simples pueden arrancar; la mayoría tendrá problemas de audio, sprites o timing. Juegos complejos (Sonic, etc.) necesitan más trabajo.
