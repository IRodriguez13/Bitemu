# Frontend Bitemu (Python + PySide6)

Frontend gráfico desacoplado del core. Solo depende de la API C (`bitemu.h`) y de la librería compartida `libbitemu.so` / `bitemu.dll` / `libbitemu.dylib`.

## Requisitos

- **Python 3.10+**
- **PySide6 y sounddevice:** `pip install -r requirements.txt` (sounddevice para Opciones → Activar audio).
- **Librería del core:** en la raíz del repo ejecuta `make lib` para generar `libbitemu.so` (Linux).

## Cómo ejecutar

1. En la **raíz del proyecto**: `make lib`
2. Desde frontend/: usar venv (python3 -m venv venv; ./venv/bin/pip install -r requirements.txt; ./venv/bin/python main.py). En Windows: venv\Scripts\python main.py
3. Opcional: `BITEMU_LIB_PATH=/ruta/a/libbitemu.so python main.py`

## Estructura

- `DESIGN.md` — Diseño ventana y menús
- `bitemu_gui/core.py` — ctypes sobre libbitemu
- `bitemu_gui/profile.py` — Perfil consola (GB por defecto)
- `bitemu_gui/keys.py` — Mapeo teclado → joypad
- `bitemu_gui/game_widget.py` — Widget juego (FB + input)
- `bitemu_gui/main_window.py` — Ventana principal y menús

Controles: WASD/flechas, J/Z=A, K/X=B, U=Select, I/Enter=Start, F3=Pausa, F4=Reset.
