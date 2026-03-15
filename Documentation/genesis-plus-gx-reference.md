# Genesis-Plus-GX como recurso de referencia

Referencia para copiar/adaptar lo que falta en bitemu. [Genesis-Plus-GX](https://github.com/ekeeke/Genesis-Plus-GX) es un emulador Sega 8/16-bit preciso y portable (Genesis, Mega CD, Master System, Game Gear, SG-1000).

---

## Mapeo Core: GPGX → bitemu

| GPGX (core/) | bitemu | Estado |
|--------------|--------|--------|
| `genesis.c` / `genesis.h` | `be2/console.c`, `genesis_impl.h` | ✓ Orquestación |
| `system.c` / `system.h` | `be2/console.c`, `core/engine.c` | ✓ Frame loop |
| `mem68k.c` / `mem68k.h` | `be2/memory.c` | ✓ Mapeo 68k |
| `memz80.c` / `memz80.h` | `be2/z80/z80.c`, `be2/memory.c` | ✓ Mapa Z80 |
| `m68k/` (Musashi) | `be2/cpu/` | ✓ 68000 |
| `z80/` (MAME Z80) | `be2/z80/` | Parcial (menos opcodes) |
| `vdp_ctrl.c` / `vdp_render.c` | `be2/vdp/vdp.c` | Parcial (falta sprites, DMA, scroll) |
| `sound/` (YM2612, SN76489) | `be2/ym2612/`, `be2/psg/` | ✓ Básico |
| `loadrom.c` / `loadrom.h` | `be2/console.c` (load_rom) | Parcial (falta SRAM, mappers) |
| `cart_hw/` | — | Pendiente (SRAM, mappers) |
| `state.c` / `state.h` | `core/savestate.c` | ✓ Universal |
| `io_ctrl.c` | `be2/input.c`, `be2/memory.c` | ✓ Joypad |
| `input_hw/` | — | Pendiente (6-button, lightgun) |
| `cd_hw/` | — | No planeado (Mega CD) |
| `ntsc/` (filtro NTSC) | — | Opcional |
| `debug/` | — | Opcional |

---

## Qué copiar / adaptar del Core

### 1. VDP (prioridad alta)

- **`vdp_ctrl.c`** (~95 KB): comandos VDP, DMA, HV counter, registros.
- **`vdp_render.c`** (~131 KB): render de sprites, planos, scroll H/V, prioridad, modos 32/40 columnas.

**Archivos clave:**
- https://github.com/ekeeke/Genesis-Plus-GX/blob/master/core/vdp_ctrl.c
- https://github.com/ekeeke/Genesis-Plus-GX/blob/master/core/vdp_render.c
- https://github.com/ekeeke/Genesis-Plus-GX/blob/master/core/vdp_ctrl.h
- https://github.com/ekeeke/Genesis-Plus-GX/blob/master/core/vdp_render.h

### 2. Cart hardware (SRAM, mappers)

- **`loadrom.c`**: detección de cabecera, SRAM, mappers (Sega, Codemasters, etc.).
- **`cart_hw/`**: `md_cart.c`, manejo de SRAM con batería.

**Archivos clave:**
- https://github.com/ekeeke/Genesis-Plus-GX/blob/master/core/loadrom.c
- https://github.com/ekeeke/Genesis-Plus-GX/blob/master/core/cart_hw/

### 3. Z80 (más opcodes)

- **`core/z80/`**: Z80 completo (MAME). bitemu tiene subset mínimo; añadir CB/DD/FD si drivers fallan.

### 4. YM2612 (mejoras)

- **`core/sound/`**: emulación FM más fiel (ADSR, algoritmos 1–7, moduladores).

### 5. TMSS (licencia SEGA)

- **`gen_tmss_w`** en genesis.c: boot ROM, chequeo de "SEGA" en cabecera.

### 6. Input avanzado

- **`input_hw/`**: 6-button pad, Team Player, lightgun, paddle.

---

## Ideas del Frontend (GPGX)

### Frontend GX (Wii/GameCube)

Estructura en `gx/`:

- **`main.c`**: loop principal, `system_frame_gen()`, sync audio/video, menú.
- **`config.c`** / **`config.h`**: opciones persistentes (vsync, overclock, region).
- **`gx_video.c`**: render a framebuffer, escalado, filtros.
- **`gx_audio.c`**: buffer de audio, sync.
- **`gx_input.c`**: mapeo Wiimote/GC pad.
- **`gui/`**: menús, file browser, slots de save state.
- **`fileio/`**: carga de ROM, SRAM, BIOS, cheats.
- **`utils/`**: helpers.

**Ideas aplicables a bitemu:**

| GPGX GX | bitemu frontend | Acción |
|---------|-----------------|--------|
| Hot swap (cambiar ROM sin reiniciar) | — | Añadir "Cambiar ROM" en menú |
| Slots de save state (1–9) | Save/Load en API | Exponer slots en UI |
| Auto-load/save state al abrir/cerrar | — | Opción "Cargar último estado" |
| Directorios: saves/, snaps/, cheats/, bios/ | — | Estructura de carpetas por consola |
| Config persistente (config_save) | QSettings | Ya existe |
| Cheats (Game Genie, Action Replay) | — | Fase futura |
| Overclock 68k/Z80 | — | Opción avanzada |
| Region (NTSC/PAL) | — | Opción |
| Filtro NTSC | — | Opcional |

### Frontend Libretro

Estructura en `libretro/`:

- **`libretro.c`**: API libretro (retro_init, retro_run, retro_serialize, etc.).
- Soporta: 3/6-button pad, lightgun, paddle, cheats, save states, core options.
- **Core options**: region, overclock, filtros, etc.

**Ideas aplicables:**

- **Core options** como modelo para "Opciones avanzadas" en bitemu.
- **Serialización**: `retro_serialize` / `retro_unserialize` → equivalente a `bst_write_all` / `bst_read_all`.
- **Input**: subclases de joypad (3-button, 6-button) → perfil Genesis 6-button en bitemu.

### Frontend SDL

- `sdl/`: port genérico con SDL. Más portable que GX.
- Útil para ver cómo abstraer video/audio/input sin Qt.

---

## Estructura de carpetas sugerida (estilo GPGX)

```
~/.config/bitemu/           # o equivalente
├── saves/
│   ├── gb/                 # SRAM Game Boy
│   └── md/                 # SRAM Genesis
├── states/                 # Save states
│   ├── gb/
│   └── md/
├── cheats/                 # (futuro)
│   └── md/
├── bios/                   # (futuro) TMSS, Mega CD
└── config.ini              # QSettings o similar
```

---

## Licencia GPGX

Genesis-Plus-GX usa una licencia **no comercial** (similar a BSD pero prohíbe uso comercial). Al copiar código:

1. **Mantener copyright** original (Charles MacDonald, Eke-Eke).
2. **Incluir aviso de licencia** en archivos derivados.
3. **No usar en producto comercial** sin acuerdo.

Para bitemu (LGPL): el core puede ser LGPL; si se incorpora código GPGX, habría que aclarar compatibilidad o reimplementar desde especificaciones.

---

## Enlaces útiles

- [Genesis-Plus-GX GitHub](https://github.com/ekeeke/Genesis-Plus-GX)
- [Core structure](https://github.com/ekeeke/Genesis-Plus-GX/tree/master/core)
- [Libretro port](https://github.com/ekeeke/Genesis-Plus-GX/tree/master/libretro)
- [SDL port](https://github.com/ekeeke/Genesis-Plus-GX/tree/master/sdl)
- [Wiki GPGX](https://github.com/ekeeke/Genesis-Plus-GX/wiki) (si existe)
