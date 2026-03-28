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

- **`test_genesis_cpu.c`**: RTD, TRAPV, LEA (PC rel), CHK, DBcc/DBRA, LINE F.
- **`test_genesis_vdp.c`**: modo PAL (313 líneas), NTSC (262), **H-int aproximada vía reg 10**.
- **`test_genesis_memory.c`** / **`test_genesis_ym2612.c`**: SRAM A130F1, lock-on, ROM impar, lectura YM2612 (MVP).

Los test ROMs de la comunidad siguen siendo la referencia final para VDP cycle-exact, DMA speed y audio FM.

## Notas

- Las ROMs comerciales están protegidas por copyright; usa solo dumps propios o software con licencia explícita.
- Los homebrew y ROMs de test de la comunidad suelen ser de dominio público o licencias permisivas.
