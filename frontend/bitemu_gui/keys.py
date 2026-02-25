"""
Mapeo teclado Qt → estado joypad (1 = pulsado).
Bits: 0–3 D-pad (R,L,U,D), 4–7 A,B,Select,Start.
"""
from PySide6.QtCore import Qt

# Por defecto: WASD + flechas, J/Z=A, K/X=B, U=Select, I/Enter=Start
DEFAULT_MAP = {
    Qt.Key.Key_W: 2,   # Up
    Qt.Key.Key_S: 3,   # Down
    Qt.Key.Key_A: 1,   # Left
    Qt.Key.Key_D: 0,   # Right
    Qt.Key.Key_Up: 2,
    Qt.Key.Key_Down: 3,
    Qt.Key.Key_Left: 1,
    Qt.Key.Key_Right: 0,
    Qt.Key.Key_J: 4,   # A
    Qt.Key.Key_Z: 4,
    Qt.Key.Key_K: 5,   # B
    Qt.Key.Key_X: 5,
    Qt.Key.Key_U: 6,   # Select
    Qt.Key.Key_I: 7,   # Start
    Qt.Key.Key_Return: 7,
    Qt.Key.Key_Enter: 7,
}


def key_to_joypad(key: int, key_map: dict | None = None) -> int | None:
    """Devuelve el bit del joypad (0–7) si la tecla está mapeada."""
    m = key_map or DEFAULT_MAP
    return m.get(key)


def build_joypad_state(pressed_keys: set, key_map: dict | None = None) -> int:
    """Estado joypad: bit N en 1 si la tecla correspondiente está pulsada."""
    m = key_map or DEFAULT_MAP
    state = 0
    for key, bit in m.items():
        if key in pressed_keys:
            state |= 1 << bit
    return state & 0xFF
