# ROMs de test para Genesis / Mega Drive

ROMs útiles para evaluar la precisión del emulador bitemu. **No se distribuyen aquí**; hay que obtenerlas por tu cuenta (dumps propios, homebrew público, etc.).

---

## ROMs de test (homebrew / comunidad)

| ROM | Propósito | Fuente |
|-----|-----------|--------|
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
./bitemu_cli ruta/a/test.bin

# O con el frontend (cargar ROM desde el menú)
```

---

## Emuladores de referencia

Para comparar comportamiento:

- **BlastEm** – Alta precisión
- **Genesis Plus GX** – Muy usado, buena compatibilidad
- **Kega Fusion** – Clásico
- **PicoDrive** – Rápido, portable

---

## Notas

- Las ROMs comerciales están protegidas por copyright; usa solo dumps propios o software con licencia explícita.
- Los homebrew y ROMs de test de la comunidad suelen ser de dominio público o licencias permisivas.
