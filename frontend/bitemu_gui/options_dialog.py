"""
Modal de opciones con pestañas: Audio, Controles, General.
Centraliza la configuración del sistema.
"""
from PySide6.QtCore import Qt, QSettings, Signal
from PySide6.QtWidgets import (
    QDialog,
    QVBoxLayout,
    QHBoxLayout,
    QTabWidget,
    QWidget,
    QLabel,
    QSlider,
    QCheckBox,
    QPushButton,
    QDialogButtonBox,
    QGroupBox,
    QFileDialog,
)

from .input_dialog import InputSettingsDialog
from .input_config import InputConfig
from .gamepad import GamepadPoller

AUDIO_KEY = "bitemu/audio_enabled"
AUDIO_VOLUME_KEY = "bitemu/audio_volume"
ROM_FOLDER_KEY = "bitemu/rom_folder"


class OptionsDialog(QDialog):
    """Modal de opciones con pestañas."""

    rom_folder_changed = Signal()
    language_changed = Signal()

    def __init__(
        self,
        emu,
        input_config: InputConfig,
        gamepad: GamepadPoller | None,
        parent=None,
    ):
        super().__init__(parent)
        self.setWindowTitle("Opciones")
        self.setMinimumSize(480, 420)
        self._emu = emu
        self._input_config = input_config
        self._gamepad = gamepad

        layout = QVBoxLayout(self)
        self._tabs = QTabWidget()
        layout.addWidget(self._tabs)

        from .i18n import t
        self._tabs.addTab(self._build_audio_tab(), t("options.audio"))
        self._tabs.addTab(self._build_controls_tab(), t("options.controls"))
        self._tabs.addTab(self._build_general_tab(), t("options.general"))

        buttons = QDialogButtonBox(
            QDialogButtonBox.StandardButton.Ok | QDialogButtonBox.StandardButton.Cancel
        )
        buttons.accepted.connect(self._apply_and_accept)
        buttons.rejected.connect(self.reject)
        layout.addWidget(buttons)

    def _build_audio_tab(self) -> QWidget:
        widget = QWidget()
        layout = QVBoxLayout(widget)

        from .i18n import t
        group = QGroupBox(t("options.audio"))
        group_layout = QVBoxLayout(group)

        self._audio_enabled = QCheckBox(t("options.audio_enable"))
        s = QSettings()
        self._audio_enabled.setChecked(s.value(AUDIO_KEY, False, type=bool))
        group_layout.addWidget(self._audio_enabled)

        vol_layout = QHBoxLayout()
        vol_layout.addWidget(QLabel(t("options.volume")))
        self._volume_slider = QSlider(Qt.Orientation.Horizontal)
        self._volume_slider.setMinimum(0)
        self._volume_slider.setMaximum(100)
        vol = int(s.value(AUDIO_VOLUME_KEY, 100, type=int))
        self._volume_slider.setValue(max(0, min(100, vol)))
        self._volume_slider.setTickPosition(QSlider.TickPosition.TicksBelow)
        self._volume_slider.setTickInterval(25)
        vol_layout.addWidget(self._volume_slider)
        self._volume_label = QLabel(f"{self._volume_slider.value()}%")
        self._volume_slider.valueChanged.connect(
            lambda v: self._volume_label.setText(f"{v}%")
        )
        vol_layout.addWidget(self._volume_label)
        group_layout.addLayout(vol_layout)

        layout.addWidget(group)
        layout.addStretch()
        return widget

    def _build_controls_tab(self) -> QWidget:
        widget = QWidget()
        layout = QVBoxLayout(widget)
        from .i18n import t
        layout.addWidget(QLabel(t("options.controls_desc")))
        self._open_controls_btn = QPushButton(t("options.configure"))
        self._open_controls_btn.clicked.connect(self._open_input_settings)
        layout.addWidget(self._open_controls_btn)
        layout.addStretch()
        return widget

    def _open_input_settings(self):
        dlg = InputSettingsDialog(self._input_config, self._gamepad, parent=self)
        dlg.exec()

    def _build_general_tab(self) -> QWidget:
        from .i18n import t, set_language, current_language, language_name, SUPPORTED
        widget = QWidget()
        layout = QVBoxLayout(widget)

        # Idioma
        lang_group = QGroupBox(t("options.language"))
        lang_layout = QVBoxLayout(lang_group)
        lang_layout.addWidget(QLabel(t("options.language_desc")))
        self._lang_buttons = {}
        lang_btn_layout = QHBoxLayout()
        for code in SUPPORTED:
            btn = QPushButton(language_name(code))
            btn.setCheckable(True)
            btn.setChecked(code == current_language())
            btn.clicked.connect(lambda checked, c=code: self._set_lang(c))
            self._lang_buttons[code] = btn
            lang_btn_layout.addWidget(btn)
        lang_layout.addLayout(lang_btn_layout)
        layout.addWidget(lang_group)

        group = QGroupBox(t("options.library"))
        group_layout = QVBoxLayout(group)
        self._rom_folder_label = QLabel()
        s = QSettings()
        folder = s.value(ROM_FOLDER_KEY, "", type=str)
        self._rom_folder_label.setText(folder or "(sin configurar)")
        self._rom_folder_label.setWordWrap(True)
        group_layout.addWidget(self._rom_folder_label)

        btn_layout = QHBoxLayout()
        pick_btn = QPushButton(t("options.rom_folder"))
        pick_btn.clicked.connect(self._pick_rom_folder)
        btn_layout.addWidget(pick_btn)
        group_layout.addLayout(btn_layout)

        layout.addWidget(group)
        layout.addStretch()
        return widget

    def _set_lang(self, code: str):
        from .i18n import set_language
        set_language(code)
        if hasattr(self, "_lang_buttons"):
            for c, btn in self._lang_buttons.items():
                btn.setChecked(c == code)
        self.language_changed.emit()

    def _pick_rom_folder(self):
        current = QSettings().value(ROM_FOLDER_KEY, "", type=str)
        folder = QFileDialog.getExistingDirectory(
            self, "Elegir carpeta de ROMs", current or ""
        )
        if folder:
            QSettings().setValue(ROM_FOLDER_KEY, folder)
            self._rom_folder_label.setText(folder)
            self.rom_folder_changed.emit()

    def _apply_and_accept(self):
        s = QSettings()
        s.setValue(AUDIO_KEY, self._audio_enabled.isChecked())
        vol = self._volume_slider.value()
        s.setValue(AUDIO_VOLUME_KEY, vol)

        if self._emu and self._emu.is_valid:
            self._emu.set_audio_enabled(self._audio_enabled.isChecked())
            self._emu.set_audio_volume(vol / 100.0)

        self.accept()
