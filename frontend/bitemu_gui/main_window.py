"""
Ventana principal: barra de menús (Archivo, Emulación, Opciones, Ayuda), widget de juego, barra de estado.
"""
import os

from PySide6.QtCore import QSettings
from PySide6.QtGui import QAction, QKeySequence, QCloseEvent
from PySide6.QtWidgets import (
    QMainWindow,
    QWidget,
    QVBoxLayout,
    QFileDialog,
    QMessageBox,
    QStatusBar,
    QApplication,
)

from .core import Emu
from .game_widget import GameWidget
from .profile import DEFAULT_PROFILE, ConsoleProfile

RECENT_KEY = "bitemu/recent_roms"
RECENT_MAX = 10
AUDIO_KEY = "bitemu/audio_enabled"


class MainWindow(QMainWindow):
    def __init__(self, profile: ConsoleProfile | None = None):
        super().__init__()
        self._profile = profile or DEFAULT_PROFILE
        self._emu = Emu()
        self._rom_path: str | None = None
        self._paused = False
        self.setWindowTitle(self._profile.window_title)
        self.setMinimumSize(400, 380)
        self.resize(
            self._profile.fb_width * 4 + 40,
            self._profile.fb_height * 4 + 80,
        )
        central = QWidget()
        self.setCentralWidget(central)
        layout = QVBoxLayout(central)
        layout.setContentsMargins(4, 4, 4, 4)
        self._game = GameWidget(profile=self._profile)
        self._game.set_emu(self._emu)
        layout.addWidget(self._game)
        self._create_menus()
        self._status = QStatusBar()
        self.setStatusBar(self._status)
        self._status.setMinimumHeight(28)
        self._status.setStyleSheet("QStatusBar { padding: 4px 8px; background: palette(mid); }")
        self._status.showMessage("Sin ROM — Archivo → Abrir ROM para comenzar")
        if not self._emu.create():
            QMessageBox.critical(self, "Error", "No se pudo crear el emulador.")
            QApplication.quit()
            return

    def _create_menus(self):
        menubar = self.menuBar()
        # Archivo
        file_menu = menubar.addMenu("&Archivo")
        open_act = QAction("Abrir ROM...", self)
        open_act.setShortcut(QKeySequence.StandardKey.Open)
        open_act.triggered.connect(self._open_rom)
        file_menu.addAction(open_act)
        recent_menu = file_menu.addMenu("ROMs recientes")
        self._recent_menu = recent_menu
        self._refresh_recent_menu()
        file_menu.addSeparator()
        quit_act = QAction("Salir", self)
        quit_act.setShortcut(QKeySequence.StandardKey.Quit)
        quit_act.triggered.connect(self.close)
        file_menu.addAction(quit_act)
        # Emulación
        emu_menu = menubar.addMenu("&Emulación")
        self._pause_act = QAction("Pausar", self)
        self._pause_act.setShortcut(QKeySequence("F3"))
        self._pause_act.triggered.connect(self._toggle_pause)
        emu_menu.addAction(self._pause_act)
        reset_act = QAction("Reiniciar", self)
        reset_act.setShortcut(QKeySequence("F4"))
        reset_act.triggered.connect(self._reset)
        emu_menu.addAction(reset_act)
        # Opciones (para futuras releases: audio, teclas, etc.)
        opts_menu = menubar.addMenu("&Opciones")
        self._audio_act = QAction("Activar audio", self)
        self._audio_act.setCheckable(True)
        self._audio_act.setChecked(QSettings().value(AUDIO_KEY, False, type=bool))
        self._audio_act.triggered.connect(self._toggle_audio)
        opts_menu.addAction(self._audio_act)
        # Ayuda
        help_menu = menubar.addMenu("A&yuda")
        controls_act = QAction("Controles...", self)
        controls_act.triggered.connect(self._show_controls)
        help_menu.addAction(controls_act)
        about_act = QAction("Acerca de", self)
        about_act.triggered.connect(self._show_about)
        help_menu.addAction(about_act)

    def _refresh_recent_menu(self):
        self._recent_menu.clear()
        paths = self._recent_paths()
        for path in paths:
            if not path or not os.path.isfile(path):
                continue
            name = os.path.basename(path)
            act = QAction(name, self)
            act.triggered.connect(lambda checked=False, p=path: self._load_rom_path(p))
            self._recent_menu.addAction(act)
        if not self._recent_menu.actions():
            act = QAction("(ninguna)", self)
            act.setEnabled(False)
            self._recent_menu.addAction(act)

    def _recent_paths(self) -> list[str]:
        s = QSettings()
        return s.value(RECENT_KEY, [], type=list)[:RECENT_MAX]

    def _add_recent(self, path: str):
        s = QSettings()
        paths = s.value(RECENT_KEY, [], type=list)
        if path in paths:
            paths.remove(path)
        paths.insert(0, path)
        s.setValue(RECENT_KEY, paths[:RECENT_MAX])

    def _open_rom(self):
        exts = " ".join(f"*.{e}" for e in self._profile.rom_extensions)
        path, _ = QFileDialog.getOpenFileName(
            self,
            "Abrir ROM",
            "",
            f"ROMs ({exts});;Todos los archivos (*)",
        )
        if path:
            self._load_rom_path(path)

    def _load_rom_path(self, path: str):
        if not self._emu.is_valid:
            return
        if not self._emu.load_rom(path):
            QMessageBox.warning(self, "Error", f"No se pudo cargar la ROM:\n{path}")
            return
        self._rom_path = path
        self._add_recent(path)
        self._refresh_recent_menu()
        self._paused = False
        self._game.set_paused(False)
        self._pause_act.setText("Pausar")
        name = os.path.basename(path)
        self.setWindowTitle(f"{self._profile.window_title} — {name}")
        self._status.showMessage(path)

    def _toggle_pause(self):
        self._paused = not self._paused
        self._game.set_paused(self._paused)
        self._pause_act.setText("Reanudar" if self._paused else "Pausar")
        self._status.showMessage("Pausado" if self._paused else self._rom_path or "")

    def _reset(self):
        if self._emu.is_valid:
            self._emu.reset()
        self._status.showMessage("Reiniciado")

    def _toggle_audio(self, checked: bool):
        QSettings().setValue(AUDIO_KEY, checked)
        # Placeholder: el core aún no tiene salida de audio; esta opción se usará cuando se implemente

    def _show_controls(self):
        QMessageBox.information(
            self,
            "Controles",
            "D-pad: W A S D o flechas\n"
            "A: J o Z\nB: K o X\nSelect: U\nStart: I o Enter\n"
            "F3: Pausar\nF4: Reiniciar",
        )

    def _show_about(self):
        QMessageBox.about(
            self,
            "Acerca de",
            f"{self._profile.window_title}\n\n"
            "Frontend Python (PySide6). Core en C.\n"
            "Audio: en desarrollo para una próxima release.",
        )

    def closeEvent(self, event: QCloseEvent):
        self._emu.destroy()
        event.accept()
