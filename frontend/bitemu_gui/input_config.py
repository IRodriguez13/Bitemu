"""
Configuración de entrada: mapeo de teclado y gamepad por consola, persistido con QSettings.
Cada consola (Game Boy, Genesis) tiene su propio mapeo de teclado y gamepad.
"""
import json
from PySide6.QtCore import QSettings

from .keys import DEFAULT_MAP, GENESIS_6_MAP

SETTINGS_KEYBOARD_GB_KEY = "bitemu/input/keyboard_map_gb"
SETTINGS_KEYBOARD_GENESIS_KEY = "bitemu/input/keyboard_map_genesis"
SETTINGS_GAMEPAD_GB_KEY = "bitemu/input/gamepad_buttons_gb"
SETTINGS_GAMEPAD_GENESIS_KEY = "bitemu/input/gamepad_buttons_genesis"
SETTINGS_GAMEPAD_AXIS_KEY = "bitemu/input/gamepad_axes"
# Retrocompatibilidad: migrar desde formato antiguo
SETTINGS_KEYBOARD_KEY_LEGACY = "bitemu/input/keyboard_map"
SETTINGS_GAMEPAD_BTN_KEY_LEGACY = "bitemu/input/gamepad_buttons"

DEFAULT_GAMEPAD_BUTTONS_GB = {
    0: 4,   # Face South (A on Xbox) -> A
    1: 5,   # Face East  (B on Xbox) -> B
    6: 6,   # Back/Select -> Select
    7: 7,   # Start       -> Start
}

# Genesis 6-button: bits 0-3 D-pad, 4=A, 5=B, 6=C, 7=Start, 8=X, 9=Y, 10=Z, 11=Mode
DEFAULT_GAMEPAD_BUTTONS_GENESIS = {
    0: 4,   # Face South -> A
    1: 5,   # Face East  -> B
    2: 6,   # Face West  -> C
    7: 7,   # Start      -> Start
    3: 8,   # Face North -> X
    4: 9,   # L1         -> Y
    5: 10,  # R1         -> Z
    6: 11,  # Back       -> Mode
}

DEFAULT_GAMEPAD_AXES = {
    0: (1, 0),  # Left stick X: negative=Left(1), positive=Right(0)
    1: (2, 3),  # Left stick Y: negative=Up(2),   positive=Down(3)
}

# Alias para compatibilidad
DEFAULT_GAMEPAD_BUTTONS = DEFAULT_GAMEPAD_BUTTONS_GB

AXIS_DEADZONE = 0.5


def _is_genesis_profile(profile) -> bool:
    """True si el perfil es Genesis (fb_width 320)."""
    return profile is not None and getattr(profile, "fb_width", 0) == 320


class InputConfig:
    """Mapeos de teclado y gamepad por consola, persistidos con QSettings."""

    def __init__(self):
        self.keyboard_map_gb: dict[int, int] = dict(DEFAULT_MAP)
        self.keyboard_map_genesis: dict[int, int] = dict(GENESIS_6_MAP)
        self.gamepad_buttons_gb: dict[int, int] = dict(DEFAULT_GAMEPAD_BUTTONS_GB)
        self.gamepad_buttons_genesis: dict[int, int] = dict(DEFAULT_GAMEPAD_BUTTONS_GENESIS)
        self.gamepad_axes: dict[int, tuple[int, int]] = dict(DEFAULT_GAMEPAD_AXES)
        self.load()

    def get_keyboard_map(self, profile) -> dict[int, int]:
        """Devuelve el mapa de teclado para la consola actual."""
        return self.keyboard_map_genesis if _is_genesis_profile(profile) else self.keyboard_map_gb

    def get_gamepad_buttons(self, profile) -> dict[int, int]:
        """Devuelve el mapa de botones del gamepad para la consola actual."""
        return self.gamepad_buttons_genesis if _is_genesis_profile(profile) else self.gamepad_buttons_gb

    def load(self):
        s = QSettings()
        # Migrar desde formato antiguo (un solo mapa)
        raw_legacy = s.value(SETTINGS_KEYBOARD_KEY_LEGACY, None)
        if raw_legacy:
            try:
                migrated = {int(k): int(v) for k, v in json.loads(raw_legacy).items()}
                self.keyboard_map_gb = migrated
                s.remove(SETTINGS_KEYBOARD_KEY_LEGACY)
            except (json.JSONDecodeError, ValueError):
                pass

        raw = s.value(SETTINGS_KEYBOARD_GB_KEY, None)
        if raw:
            try:
                self.keyboard_map_gb = {int(k): int(v) for k, v in json.loads(raw).items()}
            except (json.JSONDecodeError, ValueError):
                pass

        raw = s.value(SETTINGS_KEYBOARD_GENESIS_KEY, None)
        if raw:
            try:
                self.keyboard_map_genesis = {int(k): int(v) for k, v in json.loads(raw).items()}
            except (json.JSONDecodeError, ValueError):
                pass

        raw_legacy = s.value(SETTINGS_GAMEPAD_BTN_KEY_LEGACY, None)
        if raw_legacy:
            try:
                migrated = {int(k): int(v) for k, v in json.loads(raw_legacy).items()}
                self.gamepad_buttons_gb = migrated
                s.remove(SETTINGS_GAMEPAD_BTN_KEY_LEGACY)
            except (json.JSONDecodeError, ValueError):
                pass

        raw = s.value(SETTINGS_GAMEPAD_GB_KEY, None)
        if raw:
            try:
                self.gamepad_buttons_gb = {int(k): int(v) for k, v in json.loads(raw).items()}
            except (json.JSONDecodeError, ValueError):
                pass

        raw = s.value(SETTINGS_GAMEPAD_GENESIS_KEY, None)
        if raw:
            try:
                self.gamepad_buttons_genesis = {int(k): int(v) for k, v in json.loads(raw).items()}
            except (json.JSONDecodeError, ValueError):
                pass

        raw = s.value(SETTINGS_GAMEPAD_AXIS_KEY, None)
        if raw:
            try:
                parsed = json.loads(raw)
                self.gamepad_axes = {int(k): (int(v[0]), int(v[1])) for k, v in parsed.items()}
            except (json.JSONDecodeError, ValueError, IndexError):
                pass

    def save(self):
        s = QSettings()
        s.setValue(SETTINGS_KEYBOARD_GB_KEY, json.dumps({str(k): v for k, v in self.keyboard_map_gb.items()}))
        s.setValue(SETTINGS_KEYBOARD_GENESIS_KEY, json.dumps({str(k): v for k, v in self.keyboard_map_genesis.items()}))
        s.setValue(SETTINGS_GAMEPAD_GB_KEY, json.dumps({str(k): v for k, v in self.gamepad_buttons_gb.items()}))
        s.setValue(SETTINGS_GAMEPAD_GENESIS_KEY, json.dumps({str(k): v for k, v in self.gamepad_buttons_genesis.items()}))
        axes_ser = {str(k): list(v) for k, v in self.gamepad_axes.items()}
        s.setValue(SETTINGS_GAMEPAD_AXIS_KEY, json.dumps(axes_ser))

    def reset_keyboard_gb(self):
        self.keyboard_map_gb = dict(DEFAULT_MAP)

    def reset_keyboard_genesis(self):
        self.keyboard_map_genesis = dict(GENESIS_6_MAP)

    def reset_gamepad_gb(self):
        self.gamepad_buttons_gb = dict(DEFAULT_GAMEPAD_BUTTONS_GB)
        self.gamepad_axes = dict(DEFAULT_GAMEPAD_AXES)

    def reset_gamepad_genesis(self):
        self.gamepad_buttons_genesis = dict(DEFAULT_GAMEPAD_BUTTONS_GENESIS)
        self.gamepad_axes = dict(DEFAULT_GAMEPAD_AXES)
