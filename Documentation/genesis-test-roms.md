# ROMs de test para Genesis / Mega Drive

ROMs útiles para evaluar la precisión del emulador bitemu. **No se distribuyen aquí**; hay que obtenerlas por tu cuenta (dumps propios, homebrew público, etc.).

---

## ROMs de test (homebrew / comunidad)

| ROM | Propósito | Fuente |
|-----|-----------|--------|
| **gentest** (`game.bin.xz`) | Ejemplo con audio; útil para **gprof** en CLI (`scripts/fetch_gentest_rom.sh`) | [clbr/gentest](https://github.com/clbr/gentest) |
| **Misc Test ROM** | Sprite collision, VCounter, >64 sprites | [SpritesMind.Net](https://gendev.spritesmind.net/forum/viewtopic.php?t=2565) |
| **DMA Speed Test** | Velocidad y timing de DMA | [SpritesMind.Net](https://gendev.spritesmind.net/forum/viewtopic.php?t=2565) |
| **VDP tests** | Comportamiento del VDP | [SpritesMind.Net](https://gendev.spritesmind.net/forum/viewtopic.php?t=3329) |
| **test_inst_speed** | Comparación de ciclos de instrucciones | Referenciado en compatibilidad BlastEm |

---

## Dónde buscar

- **[SpritesMind.Net / GenDev](https://gendev.spritesmind.net/forum/)** – Foro de desarrollo Genesis, ROMs de test
- **[SMS Power](https://www.smspower.org/)** – Tests para Master System/Game Gear (compatibles en modo backwards)
- **[Plutiedev](https://www.plutiedev.com/)** – Documentación y ejemplos
- **GitHub** – Buscar "Genesis test rom" o "Mega Drive test"

---

## Cómo probar en bitemu

```bash
# CLI
./bitemu-cli -rom ruta/a/test.bin

# Perfilado host (gprof), sin GUI: ver Documentation/07-gprof.md y scripts/profile_genesis_gprof.sh
```

### Perfilado rápido (gprof + ROM gentest)

```bash
bash scripts/fetch_gentest_rom.sh
bash scripts/profile_genesis_gprof.sh   # o: ./bitemu-cli -rom tools/gentest_game.bin -frames 8000
```
Usá **`-frames N`** para salir con normalidad y que se escriba `gmon.out` (evitá solo `timeout` con SIGTERM).

---

## Hitos MVP (seguimiento en core)

| Área | Objetivo | Anclaje en repo |
|------|-----------|------------------|
| 68000 | Instrucciones / borde de excepciones | `tests/be2/test_genesis_cpu.c` |
| VDP / IRQ | H-int / V-int vs máscara 68k | `gen_vdp_pending_irq_level`, tests `gen_vdp_pending_irq_*` |
| DMA | Stall 68k durante transferencia VDP (distinto en activo vs vblank) | `GEN_VDP_DMA_STALL_*_ACTIVE` / `*_VB`, `test_genesis_vdp`, `console.c` |
| YM2612 | Busy y timing de escritura | `GEN_YM2612_WRITE_BUSY_CYCLES_68K`, `test_genesis_ym2612` (`busy_clears`, `read_port_mvp`) |
| Integración | ROMs comunitarias | Tablas de arriba + BlastEm / GPGX |

### Siguiente hito (post-MVP núcleo)

1. **Acceso a bus / precisión**: open bus, tamaño de instrucción y alineación en excepciones; ciclos por instrucción vs BlastEm/gentest.  
2. **VDP**: píxel-exact HV, orden sprite/flag en línea, DMA slot vs display.  
3. **Cartridge / edge**: SSF2 bancos raros, mappers homebrew, EEPROM.  
4. **Z80 + FM**: corrida larga con gentest + comparación de espectrograma o checksum de buffer.

#### Progreso fase “m6 / bus” (repo)

- **Open bus MVP**: direcciones sin chip (p. ej. `0x500000`) → `0xFF` por byte vía `genesis_open_bus_read_u8`; test `gen_mem_unmapped_open_bus`.  
- **Documentado** en `cpu_sync.c` el siguiente salto de precisión (BUSACK, contienda con DMA).  
- **Aún no**: prefetch visible en lecturas, tablas de ciclos cycle-exact vs gentest.  
- **Parcial**: word/long en dirección impar → vector 3 (Address Error) en EA, PC impar y pila; `GEN_CYCLES_STOP` 8; tests `gen_cpu_odd_pc_address_error`, `gen_cpu_move_w_odd_ea_address_error`.

#### Qué queda **después** de m6 (visión corta)

| Orden sugerido | Qué es |
|----------------|--------|
| 1 | **Ciclos 68k** alineados a tabla/gentest (y STOP/wait states). |
| 2 | **VDP** HV en resolución de píxel; INT al ciclo correcto dentro de la línea. |
| 3 | **DMA** frente a 68k (slots, no solo stall agregado). |
| 4 | **FM** YM2612 vs medición en hardware o ROM de test. |
| 5 | **Perifericos** raros, Teamplayer, multitap, SVP, Sega CD (otro MVP). |

**Estado reciente (post‑m6, aproximación):** `gen_vdp_step` avanza hasta el siguiente evento (entrada a hblank / fin de línea): bit **HB** y **H‑int** (reg 10) alineados al paso a hblank; **HV** leído por tramos (activo `0x00–0x93` vs `0xE9–0xFF` en blank). **DMA:** stall por palabra mayor en pantalla activa que en vblank (`*_ACTIVE` vs `*_VBLANK`). **YM2612:** busy tras escritura en `GEN_YM2612_WRITE_BUSY_CYCLES_68K` (~88 ciclos bus, objetivo medición/test ROMs). Sigue siendo **no** cycle‑exact a nivel slot VDP ni bus compartido fino.

---

### Hardware límite y periféricos (alcance / roadmap)

No hay que implementarlo todo en el core actual; sirve como lista de **extensiones** y ROMs que suelen romper emuladores “solo cartucho”:

| Área | Notas |
|------|--------|
| **SVP (Virtua Processor)** | Cartucho con coprocesador; mapa memoria y timing aparte (Virtua Racing). MVP posible: detectar y fallar claro o stub mínimo. |
| **Mega CD / Sega CD** | Subsistema 68000 + PCM + gráficos; **otro proyecto** respecto al Genesis cartucho (BIOS, prefijo `0xA12000`, CD‑ROM). |
| **32X** | Dual SH2 + VDP adicional; mismo orden de magnitud que Sega CD en complejidad. |
| **Mappers / banco** | Más allá de SSF2: J‑Cart (EEPROM + dos puertos extra), SVP, algunos homebrew. EEPROM I²C en cartucho es otro eje (tamaño, ACK). |
| **Pads / multitaps** | Team Player, EA 4‑way, XE‑1AP: protocolo serie en líneas TH/TR; el core ya tiene base 3/6 botones — enlazar documentación de `input.c` y tests cuando se amplíe. |
| **Menudería** | Activisionmapper raro, daiku sosho, algunas protecciones anti‑copia; priorizar según ROMs objetivo. |

Para validación: cuando se toque una categoría, añadir ROM de test o homebrew conocido en la tabla superior y un caso sintético en `tests/be2/` si aplica.

---

## Flujo de regresión sugerido (auditoría / CI manual)

1. **Inventario**: elige 2–4 ROMs de la tabla (p. ej. test de VDP + test de sprites).
2. **Referencia**: ejecuta la misma ROM en BlastEm o Genesis Plus GX y captura pantalla o log.
3. **Bitemu**: `make cli` o GUI; anota desviaciones (video, audio, cuelgues).
4. **Acotar**: si el fallo es VDP/CPU/memoria, reducir a un comportamiento observable (registro HV, un solo frame) y valora añadir un test en `tests/be2/*.c` **sin** incrustar el `.bin` (solo constantes o estados sintéticos).
5. **Seguimiento**: referenciar en el PR/commit qué ROM y qué build de referencia usaste (fecha/emulador).

---

## Emuladores de referencia

Para comparar comportamiento:

- **BlastEm** – Alta precisión
- **Genesis Plus GX** – Muy usado, buena compatibilidad
- **Kega Fusion** – Clásico
- **PicoDrive** – Rápido, portable

---

## Tests automáticos en el repo (sin ROM externa)

`make test-core` incluye:

- **`test_genesis_cpu.c`**: RTD, TRAPV, LEA (PC rel), CHK, DBcc/DBRA, LINE F, tabla `gen_cpu_line_cycles_ref`, Address Error impar.
- **`test_genesis_vdp.c`**: PAL/NTSC, **H-int** al entrar en hblank (reg 10), lectura **HV** por tramo activo/blank, DMA stall activo vs vblank.
- **`test_genesis_memory.c`** / **`test_genesis_ym2612.c`**: SRAM A130F1, lock-on, ROM impar, lectura YM2612 (MVP).

Los test ROMs de la comunidad siguen siendo la referencia final para VDP cycle-exact, DMA speed y audio FM.

## Notas

- Las ROMs comerciales están protegidas por copyright; usa solo dumps propios o software con licencia explícita.
- Los homebrew y ROMs de test de la comunidad suelen ser de dominio público o licencias permisivas.
