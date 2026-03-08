"""
Configuración de entrada: mapeo de teclado y gamepad, persistido con QSettings.
Reutilizable para todas las ediciones de Bitemu.
"""
import json
from PySide6.QtCore import QSettings

from .keys import DEFAULT_MAP

SETTINGS_KEYBOARD_KEY = "bitemu/input/keyboard_map"
SETTINGS_GAMEPAD_BTN_KEY = "bitemu/input/gamepad_buttons"
SETTINGS_GAMEPAD_AXIS_KEY = "bitemu/input/gamepad_axes"

DEFAULT_GAMEPAD_BUTTONS = {
    0: 4,   # Face South (A on Xbox) -> A
    1: 5,   # Face East  (B on Xbox) -> B
    6: 6,   # Back/Select -> Select
    7: 7,   # Start       -> Start
}

DEFAULT_GAMEPAD_AXES = {
    0: (1, 0),  # Left stick X: negative=Left(1), positive=Right(0)
    1: (2, 3),  # Left stick Y: negative=Up(2),   positive=Down(3)
}

AXIS_DEADZONE = 0.5


class InputConfig:
    """Holds keyboard and gamepad mappings with QSettings persistence."""

    def __init__(self):
        self.keyboard_map: dict[int, int] = dict(DEFAULT_MAP)
        self.gamepad_buttons: dict[int, int] = dict(DEFAULT_GAMEPAD_BUTTONS)
        self.gamepad_axes: dict[int, tuple[int, int]] = dict(DEFAULT_GAMEPAD_AXES)
        self.load()

    def load(self):
        s = QSettings()
        raw = s.value(SETTINGS_KEYBOARD_KEY, None)
        if raw:
            try:
                self.keyboard_map = {int(k): int(v) for k, v in json.loads(raw).items()}
            except (json.JSONDecodeError, ValueError):
                self.keyboard_map = dict(DEFAULT_MAP)

        raw = s.value(SETTINGS_GAMEPAD_BTN_KEY, None)
        if raw:
            try:
                self.gamepad_buttons = {int(k): int(v) for k, v in json.loads(raw).items()}
            except (json.JSONDecodeError, ValueError):
                self.gamepad_buttons = dict(DEFAULT_GAMEPAD_BUTTONS)

        raw = s.value(SETTINGS_GAMEPAD_AXIS_KEY, None)
        if raw:
            try:
                parsed = json.loads(raw)
                self.gamepad_axes = {int(k): (int(v[0]), int(v[1])) for k, v in parsed.items()}
            except (json.JSONDecodeError, ValueError, IndexError):
                self.gamepad_axes = dict(DEFAULT_GAMEPAD_AXES)

    def save(self):
        s = QSettings()
        s.setValue(SETTINGS_KEYBOARD_KEY, json.dumps({str(k): v for k, v in self.keyboard_map.items()}))
        s.setValue(SETTINGS_GAMEPAD_BTN_KEY, json.dumps({str(k): v for k, v in self.gamepad_buttons.items()}))
        axes_ser = {str(k): list(v) for k, v in self.gamepad_axes.items()}
        s.setValue(SETTINGS_GAMEPAD_AXIS_KEY, json.dumps(axes_ser))

    def reset_keyboard(self):
        self.keyboard_map = dict(DEFAULT_MAP)

    def reset_gamepad(self):
        self.gamepad_buttons = dict(DEFAULT_GAMEPAD_BUTTONS)
        self.gamepad_axes = dict(DEFAULT_GAMEPAD_AXES)
