# Bitemu Frontend - Diseño (estilo PCSX2)

Frontend genérico en Python (PySide6/Qt), desacoplado del core, reutilizable para BE1 (Game Boy) y futuras consolas (BE2, BE3).

## Objetivos

- **Portabilidad:** Python + Qt en Linux, Windows, macOS.
- **Desacoplamiento:** El frontend solo usa la API C (`bitemu.h`); no conoce CPU/PPU ni consola concreta.
- **Adaptable por consola:** Perfil (resolución FB, teclas, nombre) por edición; primera versión con perfil GB.
- **Primera beta:** Abrir ROM, escalado correcto, controles, menús básicos. Save states y CI más adelante.

## Wireframe de la ventana

```
┌─────────────────────────────────────────────────────────────────────────┐
│  Archivo    Emulación    Opciones    Ayuda                               │
├─────────────────────────────────────────────────────────────────────────┤
│  ▼ Archivo                                                               │
│     Abrir ROM...        Ctrl+O                                          │
│     ROMs recientes  ▶   (submenú 1..10)                                 │
│     ─────────────────────────────────                                   │
│     Salir               Ctrl+Q                                          │
│  ▼ Emulación                                                            │
│     Pausar / Reanudar   F3                                              │
│     Reiniciar           F4                                              │
│  ▼ Opciones          (fase 2: escala, teclas, audio)                   │
│  ▼ Ayuda                                                                 │
│     Controles...                                                         │
│     Acerca de                                                            │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│                    ┌────────────────────────────┐                        │
│                    │                            │                        │
│                    │     Zona de juego           │                        │
│                    │     (framebuffer 160×144   │                        │
│                    │      escalado, relación   │                        │
│                    │      de aspecto fija)     │                        │
│                    │                            │                        │
│                    └────────────────────────────┘                        │
│                                                                          │
├─────────────────────────────────────────────────────────────────────────┤
│  [Sin ROM]  |  Pausado  |  FPS: 59.7                    (barra estado)  │
└─────────────────────────────────────────────────────────────────────────┘
```

- **Barra de menú** arriba (Archivo, Emulación, Opciones, Ayuda).
- **Área central:** solo el juego; escala entera (p. ej. 4x) manteniendo relación de aspecto; centrado si sobra espacio.
- **Barra de estado** abajo (opcional en v1): nombre de ROM, estado Pausado/Corriendo, FPS.

## Flujo de guardado de partida (batería)

- El **core** ya maneja `.sav`: al cargar ROM carga el `.sav` si existe; al descargar ROM guarda el `.sav`.
- El frontend **no** tiene botón “Guardar partida”: es automático al cerrar la ROM o la aplicación.
- Save states (instantáneos) quedarían para una fase 2 (API en el core).

## Opciones recomendadas (primera versión beta)

| Menú    | Acción              | Atajo   | Notas                          |
|---------|---------------------|---------|---------------------------------|
| Archivo | Abrir ROM          | Ctrl+O  | Diálogo nativo Qt              |
| Archivo | Recientes (1..N)    | —        | Submenú con rutas guardadas    |
| Archivo | Salir              | Ctrl+Q  | Cierra ventana                 |
| Emulación | Pausar/Reanudar  | F3      | Alterna pausa                   |
| Emulación | Reiniciar        | F4      | bitemu_reset()                 |
| Ayuda   | Controles          | —        | Cuadro con mapeo teclado        |
| Ayuda   | Acerca de          | —        | Nombre, versión, consola       |

Opciones avanzadas (fase 2): factor de escala (2x, 4x, 8x), rebindings, filtro de imagen, audio.

## Perfil por consola (desacoplamiento)

El frontend puede cargar un “perfil” por consola para no hardcodear solo Game Boy:

- **Resolución:** `fb_width`, `fb_height` (GB: 160×144).
- **Título:** “Bitemu - Game Boy” / “Bitemu - Genesis” …
- **Teclas:** mapeo por defecto (D-pad, A, B, Select, Start, etc.).
- **Extensiones de ROM:** `["gb", "gbc"]` para el diálogo “Abrir”.

En beta usamos un único perfil GB; la estructura de código permite añadir `frontend/profiles/gb.yaml` (o similar) más adelante.

## Dependencias

- **Python 3.10+**
- **PySide6** (Qt para Python, LGPL)
- **libbitemu** compartida: `libbitemu.so` (Linux), `bitemu.dll` (Windows), `libbitemu.dylib` (macOS), generada por el Makefile del proyecto.

## Estructura del frontend

```
frontend/
├── DESIGN.md           # Este documento
├── README.md           # Cómo ejecutar y requisitos
├── requirements.txt    # PySide6
├── main.py             # Punto de entrada: lanza la app Qt
├── bitemu_gui/
│   ├── __init__.py
│   ├── core.py         # ctypes: carga libbitemu, wrappers de la API
│   ├── main_window.py  # Ventana principal, menús, barra estado
│   ├── game_widget.py  # QWidget que pinta el FB y captura teclas
│   ├── profile.py      # Perfil consola (GB por defecto)
│   └── keys.py         # Mapeo teclado → joypad
```

El core C no cambia; el frontend solo depende de `include/bitemu.h` y de la librería compartida.
