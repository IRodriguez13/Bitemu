"""
Diálogo de configuración de controles: teclado y gamepad.
"""
from PySide6.QtCore import Qt, QTimer
from PySide6.QtWidgets import (
    QDialog,
    QVBoxLayout,
    QHBoxLayout,
    QTabWidget,
    QWidget,
    QGridLayout,
    QLabel,
    QPushButton,
    QDialogButtonBox,
    QGroupBox,
)
from PySide6.QtGui import QKeyEvent

from .keys import ACTION_NAMES, key_name, DEFAULT_MAP
from .input_config import InputConfig, DEFAULT_GAMEPAD_BUTTONS, DEFAULT_GAMEPAD_AXES
from .gamepad import GamepadPoller

GP_BTN_NAMES = {
    0: "Botón Sur (A/X)",
    1: "Botón Este (B/O)",
    2: "Botón Oeste (X/□)",
    3: "Botón Norte (Y/△)",
    4: "L1/LB",
    5: "R1/RB",
    6: "Back/Select",
    7: "Start",
    8: "L3",
    9: "R3",
}


def _gp_btn_name(btn_idx: int) -> str:
    return GP_BTN_NAMES.get(btn_idx, f"Botón {btn_idx}")


class _KeyCaptureButton(QPushButton):
    """Button that captures the next keypress for rebinding."""

    def __init__(self, qt_key: int, parent=None):
        super().__init__(key_name(qt_key), parent)
        self.qt_key = qt_key
        self._listening = False
        self.setMinimumWidth(100)
        self.clicked.connect(self._start_listen)

    def _start_listen(self):
        self._listening = True
        self.setText("Presiona una tecla...")
        self.setFocus()

    def keyPressEvent(self, event: QKeyEvent):
        if self._listening:
            self.qt_key = event.key()
            self.setText(key_name(self.qt_key))
            self._listening = False
            event.accept()
            return
        super().keyPressEvent(event)

    def focusOutEvent(self, event):
        if self._listening:
            self._listening = False
            self.setText(key_name(self.qt_key))
        super().focusOutEvent(event)


class InputSettingsDialog(QDialog):
    def __init__(self, config: InputConfig, gamepad: GamepadPoller | None = None, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Configurar controles")
        self.setMinimumWidth(420)
        self._config = config
        self._gamepad = gamepad

        layout = QVBoxLayout(self)
        self._tabs = QTabWidget()
        layout.addWidget(self._tabs)

        self._tabs.addTab(self._build_keyboard_tab(), "Teclado")
        self._tabs.addTab(self._build_gamepad_tab(), "Gamepad")

        buttons = QDialogButtonBox(QDialogButtonBox.StandardButton.Ok | QDialogButtonBox.StandardButton.Cancel)
        buttons.accepted.connect(self._apply_and_accept)
        buttons.rejected.connect(self.reject)
        layout.addWidget(buttons)

    def select_gamepad_tab(self):
        self._tabs.setCurrentIndex(1)

    def _build_keyboard_tab(self) -> QWidget:
        widget = QWidget()
        outer = QVBoxLayout(widget)

        group = QGroupBox("Asignación de teclas")
        grid = QGridLayout(group)
        grid.setColumnStretch(1, 1)

        self._key_buttons: dict[int, list[_KeyCaptureButton]] = {}
        current_keys = self._keys_by_bit(self._config.keyboard_map)

        for bit in range(8):
            label = QLabel(f"{ACTION_NAMES[bit]}:")
            label.setAlignment(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)
            grid.addWidget(label, bit, 0)

            row_layout = QHBoxLayout()
            btns = []
            assigned = current_keys.get(bit, [])
            for qt_key in assigned:
                btn = _KeyCaptureButton(qt_key)
                row_layout.addWidget(btn)
                btns.append(btn)
            if not assigned:
                btn = _KeyCaptureButton(0)
                btn.setText("(sin asignar)")
                row_layout.addWidget(btn)
                btns.append(btn)

            add_btn = QPushButton("+")
            add_btn.setFixedWidth(30)
            add_btn.setToolTip("Agregar tecla alternativa")
            bit_capture = bit
            add_btn.clicked.connect(lambda checked=False, b=bit_capture, rl=row_layout, bl=btns: self._add_key_slot(b, rl, bl))
            row_layout.addWidget(add_btn)

            row_layout.addStretch()
            grid.addLayout(row_layout, bit, 1)
            self._key_buttons[bit] = btns

        outer.addWidget(group)

        reset_btn = QPushButton("Restaurar teclas por defecto")
        reset_btn.clicked.connect(self._reset_keyboard)
        outer.addWidget(reset_btn)
        outer.addStretch()
        return widget

    def _add_key_slot(self, bit: int, row_layout: QHBoxLayout, btns: list):
        btn = _KeyCaptureButton(0)
        btn.setText("Presiona una tecla...")
        btn._listening = True
        insert_pos = row_layout.count() - 2
        row_layout.insertWidget(insert_pos, btn)
        btns.append(btn)
        btn.setFocus()

    def _build_gamepad_tab(self) -> QWidget:
        widget = QWidget()
        outer = QVBoxLayout(widget)

        status_layout = QHBoxLayout()
        self._gp_status = QLabel()
        self._update_gamepad_status()
        status_layout.addWidget(self._gp_status)
        detect_btn = QPushButton("Detectar")
        detect_btn.clicked.connect(self._detect_gamepad)
        status_layout.addWidget(detect_btn)
        status_layout.addStretch()
        outer.addLayout(status_layout)

        group = QGroupBox("Botones del gamepad")
        grid = QGridLayout(group)

        self._gp_capture_buttons: dict[int, QPushButton] = {}
        self._gp_pending_bit: int | None = None
        self._gp_pending_map: dict[int, int] = dict(self._config.gamepad_buttons)

        for bit in range(8):
            label = QLabel(f"{ACTION_NAMES[bit]}:")
            label.setAlignment(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)
            grid.addWidget(label, bit, 0)

            btn = QPushButton(self._gp_current_label(bit))
            btn.setMinimumWidth(160)
            btn.clicked.connect(lambda checked=False, b=bit: self._start_gp_capture(b))
            grid.addWidget(btn, bit, 1)
            self._gp_capture_buttons[bit] = btn

        outer.addWidget(group)

        axis_group = QGroupBox("Ejes analógicos")
        axis_info = QLabel(
            "El stick izquierdo y el D-pad del gamepad\n"
            "se mapean automáticamente a las direcciones."
        )
        axis_layout = QVBoxLayout(axis_group)
        axis_layout.addWidget(axis_info)
        outer.addWidget(axis_group)

        reset_btn = QPushButton("Restaurar gamepad por defecto")
        reset_btn.clicked.connect(self._reset_gamepad)
        outer.addWidget(reset_btn)
        outer.addStretch()

        if self._gamepad:
            self._gamepad.button_pressed.connect(self._on_gp_button_pressed)

        return widget

    def _gp_current_label(self, bit: int) -> str:
        mapped = [bi for bi, b in self._gp_pending_map.items() if b == bit]
        if mapped:
            return ", ".join(_gp_btn_name(b) for b in mapped)
        return "(sin asignar) — clic para asignar"

    def _start_gp_capture(self, bit: int):
        if self._gp_pending_bit is not None:
            old_btn = self._gp_capture_buttons.get(self._gp_pending_bit)
            if old_btn:
                old_btn.setText(self._gp_current_label(self._gp_pending_bit))

        self._gp_pending_bit = bit
        self._gp_capture_buttons[bit].setText("Presiona un botón del gamepad...")

        self._gp_capture_timer = QTimer(self)
        self._gp_capture_timer.setSingleShot(True)
        self._gp_capture_timer.timeout.connect(self._gp_capture_timeout)
        self._gp_capture_timer.start(5000)

    def _on_gp_button_pressed(self, btn_idx: int):
        if self._gp_pending_bit is None:
            return

        bit = self._gp_pending_bit
        self._gp_pending_bit = None
        if hasattr(self, "_gp_capture_timer"):
            self._gp_capture_timer.stop()

        self._gp_pending_map = {k: v for k, v in self._gp_pending_map.items() if v != bit}
        self._gp_pending_map[btn_idx] = bit

        for b in range(8):
            self._gp_capture_buttons[b].setText(self._gp_current_label(b))

    def _gp_capture_timeout(self):
        if self._gp_pending_bit is not None:
            btn = self._gp_capture_buttons.get(self._gp_pending_bit)
            if btn:
                btn.setText(self._gp_current_label(self._gp_pending_bit))
            self._gp_pending_bit = None

    def _update_gamepad_status(self):
        if self._gamepad and self._gamepad.available:
            if self._gamepad.connected:
                name = self._gamepad.joystick_name or "Gamepad"
                self._gp_status.setText(f"Conectado: {name}")
            else:
                self._gp_status.setText("No hay gamepad conectado — conecta uno y presiona Detectar")
        else:
            self._gp_status.setText("pygame no disponible — instalar con: pip install pygame")

    def _detect_gamepad(self):
        if self._gamepad:
            self._gamepad.refresh()
        self._update_gamepad_status()

    def _reset_keyboard(self):
        current_keys = self._keys_by_bit(DEFAULT_MAP)
        for bit, btns in self._key_buttons.items():
            assigned = current_keys.get(bit, [])
            for i, btn in enumerate(btns):
                if i < len(assigned):
                    btn.qt_key = assigned[i]
                    btn.setText(key_name(assigned[i]))
                else:
                    btn.qt_key = 0
                    btn.setText("(sin asignar)")

    def _reset_gamepad(self):
        self._gp_pending_map = dict(DEFAULT_GAMEPAD_BUTTONS)
        self._gp_pending_bit = None
        for bit in range(8):
            self._gp_capture_buttons[bit].setText(self._gp_current_label(bit))

    def _apply_and_accept(self):
        new_map = {}
        for bit, btns in self._key_buttons.items():
            for btn in btns:
                if btn.qt_key and btn.qt_key != 0:
                    new_map[btn.qt_key] = bit
        self._config.keyboard_map = new_map
        self._config.gamepad_buttons = dict(self._gp_pending_map)
        self._config.save()
        self.accept()

    @staticmethod
    def _keys_by_bit(key_map: dict[int, int]) -> dict[int, list[int]]:
        result: dict[int, list[int]] = {}
        for qt_key, bit in key_map.items():
            result.setdefault(bit, []).append(qt_key)
        return result
