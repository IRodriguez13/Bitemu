"""
Ventana principal: barra de menús (Archivo, Emulación, Opciones, Ayuda), widget de juego, barra de estado.
"""
import os

try:
    import sounddevice as sd
    _SOUNDDEVICE_AVAILABLE = True
except ImportError:
    sd = None
    _SOUNDDEVICE_AVAILABLE = False

from PySide6.QtCore import Qt, QSettings
from PySide6.QtGui import QAction, QKeySequence, QCloseEvent, QIcon, QPixmap
from PySide6.QtWidgets import (
    QMainWindow,
    QWidget,
    QVBoxLayout,
    QFileDialog,
    QMessageBox,
    QStatusBar,
    QApplication,
    QDialog,
    QLabel,
    QDialogButtonBox,
)

from .core import Emu
from .game_widget import GameWidget
from .profile import DEFAULT_PROFILE, ConsoleProfile
from .input_config import InputConfig
from .gamepad import GamepadPoller
from .input_dialog import InputSettingsDialog

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
        self._audio_stream = None
        self._input_config = InputConfig()
        self._gamepad = GamepadPoller(self._input_config, parent=self)
        self._input_dialog: InputSettingsDialog | None = None
        self.setWindowTitle(self._profile.window_title)
        icon_path = os.path.join(os.path.dirname(__file__), "..", "assets", "icon.png")
        if os.path.isfile(icon_path):
            self.setWindowIcon(QIcon(icon_path))
        self.setMinimumSize(400, 380)
        self.resize(
            self._profile.fb_width * 4 + 40,
            self._profile.fb_height * 4 + 80,
        )
        central = QWidget()
        self.setCentralWidget(central)
        layout = QVBoxLayout(central)
        layout.setContentsMargins(4, 4, 4, 4)
        self._game = GameWidget(profile=self._profile, input_config=self._input_config)
        self._game.set_emu(self._emu)
        self._gamepad.state_changed.connect(self._game.set_gamepad_state)
        self._gamepad.gamepad_connected.connect(self._on_gamepad_connected)
        self._gamepad.gamepad_disconnected.connect(self._on_gamepad_disconnected)
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
        if not self._emu.init_audio():
            pass
        self._emu.set_audio_enabled(QSettings().value(AUDIO_KEY, False, type=bool))
        self._apply_audio_stream()
        self._audio_chans = 1

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
        close_rom_act = QAction("Cerrar juego", self)
        close_rom_act.setShortcut(QKeySequence("F5"))
        close_rom_act.triggered.connect(self._close_rom)
        emu_menu.addAction(close_rom_act)
        emu_menu.addSeparator()
        save_state_act = QAction("Guardar estado", self)
        save_state_act.setShortcut(QKeySequence("F6"))
        save_state_act.triggered.connect(self._save_state)
        emu_menu.addAction(save_state_act)
        load_state_act = QAction("Cargar estado", self)
        load_state_act.setShortcut(QKeySequence("F7"))
        load_state_act.triggered.connect(self._load_state)
        emu_menu.addAction(load_state_act)
        opts_menu = menubar.addMenu("&Opciones")
        self._audio_act = QAction("Activar audio", self)
        self._audio_act.setCheckable(True)
        self._audio_act.setChecked(QSettings().value(AUDIO_KEY, False, type=bool))
        self._audio_act.triggered.connect(self._toggle_audio)
        opts_menu.addAction(self._audio_act)
        opts_menu.addSeparator()
        input_act = QAction("Configurar controles...", self)
        input_act.triggered.connect(self._open_input_settings)
        opts_menu.addAction(input_act)
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
        self._status.showMessage(f"Cargando {name}...")
        self._game.show_loading(name)

    def _toggle_pause(self):
        self._paused = not self._paused
        self._game.set_paused(self._paused)
        self._pause_act.setText("Reanudar" if self._paused else "Pausar")
        self._status.showMessage("Pausado" if self._paused else self._rom_path or "")

    def _reset(self):
        if self._emu.is_valid:
            self._emu.reset()
        self._status.showMessage("Reiniciado")

    def _close_rom(self):
        if not self._emu.is_valid or self._rom_path is None:
            return
        reply = QMessageBox.question(
            self,
            "Cerrar juego",
            "¿Seguro que querés cerrar el juego?\n\n"
            "Si no guardaste el estado, se perderá el progreso.",
            QMessageBox.StandardButton.Save
            | QMessageBox.StandardButton.Discard
            | QMessageBox.StandardButton.Cancel,
            QMessageBox.StandardButton.Save,
        )
        if reply == QMessageBox.StandardButton.Cancel:
            return
        if reply == QMessageBox.StandardButton.Save:
            self._save_state()
        self._emu.unload_rom()
        self._rom_path = None
        self._paused = False
        self._game.set_paused(False)
        self._game.show_splash()
        self._pause_act.setText("Pausar")
        self.setWindowTitle(self._profile.window_title)
        self._status.showMessage("Juego cerrado")

    def _state_path(self) -> str | None:
        if not self._rom_path:
            return None
        base, _ = os.path.splitext(self._rom_path)
        return base + ".bst"

    def _save_state(self):
        path = self._state_path()
        if not path or not self._emu.is_valid:
            self._status.showMessage("No se puede guardar: sin ROM cargada")
            return
        ok = self._emu.save_state(path)
        if ok:
            size = os.path.getsize(path) if os.path.isfile(path) else 0
            self._status.showMessage(f"Estado guardado: {os.path.basename(path)} ({size} bytes)")
        else:
            self._status.showMessage("Error al guardar estado")

    def _load_state(self):
        path = self._state_path()
        if not path or not self._emu.is_valid:
            self._status.showMessage("No se puede cargar: sin ROM cargada")
            return
        if not os.path.isfile(path):
            self._status.showMessage("No hay estado guardado para esta ROM")
            return
        size = os.path.getsize(path)
        ok = self._emu.load_state(path)
        if ok:
            self._game.update()
            self._status.showMessage(f"Estado cargado: {os.path.basename(path)} ({size} bytes)")
        else:
            self._status.showMessage(f"Error al cargar estado — archivo existe ({size} bytes) pero falló la carga")

    def _toggle_audio(self, checked: bool):
        QSettings().setValue(AUDIO_KEY, checked)
        self._emu.set_audio_enabled(checked)
        self._apply_audio_stream()

    def _apply_audio_stream(self):
        """Abre o cierra el stream de salida según la opción y disponibilidad de sounddevice."""
        enabled = QSettings().value(AUDIO_KEY, False, type=bool)
        if self._audio_stream is not None:
            try:
                self._audio_stream.stop()
                self._audio_stream.close()
            except Exception:
                pass
            self._audio_stream = None
        if not enabled:
            return
        if not _SOUNDDEVICE_AVAILABLE:
            QMessageBox.information(
                self,
                "Audio",
                "Para reproducir audio instala: pip install sounddevice",
            )
            self._audio_act.setChecked(False)
            QSettings().setValue(AUDIO_KEY, False)
            self._emu.set_audio_enabled(False)
            return
        rate, chans = self._emu.get_audio_spec()
        self._audio_chans = chans
        try:
            self._audio_stream = sd.RawOutputStream(
                samplerate=rate,
                channels=chans,
                dtype="int16",
                blocksize=1024,
                latency="high",
                callback=self._audio_callback,
            )
            self._audio_stream.start()
        except Exception as e:
            QMessageBox.warning(
                self,
                "Audio",
                f"No se pudo abrir el dispositivo de audio: {e}",
            )
            self._audio_act.setChecked(False)
            QSettings().setValue(AUDIO_KEY, False)
            self._emu.set_audio_enabled(False)

    def _audio_callback(self, outdata, frames, time_info, status):
        """sounddevice callback (audio thread): lee del buffer circular del core."""
        bytes_needed = frames * self._audio_chans * 2
        data = b""
        if self._emu and self._emu.is_valid:
            data = self._emu.read_audio(frames * self._audio_chans)
        n = len(data)
        if n > 0:
            outdata[:n] = data
        if n < bytes_needed:
            outdata[n:bytes_needed] = b'\x00' * (bytes_needed - n)

    def _on_gamepad_connected(self, name: str):
        self._status.showMessage(f"Gamepad detectado: {name}")
        if self._input_dialog is not None:
            self._input_dialog.select_gamepad_tab()
            self._input_dialog.activateWindow()
            self._input_dialog.raise_()
            return
        reply = QMessageBox.question(
            self,
            "Gamepad detectado",
            f"Se detectó: {name}\n\n¿Querés configurarlo?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            QMessageBox.StandardButton.Yes,
        )
        if reply == QMessageBox.StandardButton.Yes:
            self._open_input_settings(gamepad_tab=True)

    def _on_gamepad_disconnected(self):
        self._status.showMessage("Gamepad desconectado")

    def _open_input_settings(self, gamepad_tab: bool = False):
        if self._input_dialog is not None:
            if gamepad_tab:
                self._input_dialog.select_gamepad_tab()
            self._input_dialog.activateWindow()
            self._input_dialog.raise_()
            return
        dlg = InputSettingsDialog(self._input_config, self._gamepad, parent=self)
        self._input_dialog = dlg
        if gamepad_tab:
            dlg.select_gamepad_tab()
        dlg.exec()
        self._input_dialog = None
        self._status.showMessage("Controles actualizados")

    def _show_controls(self):
        QMessageBox.information(
            self,
            "Controles",
            "D-pad: W A S D o flechas\n"
            "A: J o Z\nB: K o X\nSelect: U\nStart: I o Enter\n\n"
            "F3: Pausar / Reanudar\nF4: Reiniciar\nF5: Cerrar juego\n"
            "F6: Guardar estado\nF7: Cargar estado\n\n"
            "Opciones → Configurar controles para editar teclas y gamepad.",
        )

    def _show_about(self):
        from . import get_version
        dlg = QDialog(self)
        dlg.setWindowTitle("Acerca de Bitemu")
        dlg.setFixedSize(380, 340)
        layout = QVBoxLayout(dlg)
        layout.setContentsMargins(20, 16, 20, 12)

        logo_path = os.path.join(os.path.dirname(__file__), "..", "assets", "logo.png")
        if os.path.isfile(logo_path):
            pix = QPixmap(logo_path).scaled(
                200, 100,
                Qt.AspectRatioMode.KeepAspectRatio,
                Qt.TransformationMode.SmoothTransformation,
            )
            logo_lbl = QLabel()
            logo_lbl.setPixmap(pix)
            logo_lbl.setAlignment(Qt.AlignmentFlag.AlignCenter)
            layout.addWidget(logo_lbl)

        info = QLabel(
            f"<h3>Bitemu v{get_version()}</h3>"
            f"<p>{self._profile.name} Emulator</p>"
            "<p>Core en C &middot; Frontend en Python (PySide6)</p>"
            "<hr>"
            "<p><b>Autor:</b> Iván Ezequiel Rodriguez</p>"
            '<p><b>GitHub:</b> <a href="https://github.com/IRodriguez13">'
            "github.com/IRodriguez13</a></p>"
            "<hr>"
            "<p><small>Licencia: LGPL v3.0</small></p>"
        )
        info.setAlignment(Qt.AlignmentFlag.AlignCenter)
        info.setOpenExternalLinks(True)
        info.setWordWrap(True)
        layout.addWidget(info)

        btn_box = QDialogButtonBox(QDialogButtonBox.StandardButton.Ok)
        btn_box.accepted.connect(dlg.accept)
        layout.addWidget(btn_box)
        dlg.exec()

    def closeEvent(self, event: QCloseEvent):
        if self._rom_path is not None:
            reply = QMessageBox.question(
                self,
                "Salir de Bitemu",
                "Hay un juego en ejecución.\n\n"
                "¿Querés guardar el estado antes de salir?",
                QMessageBox.StandardButton.Save
                | QMessageBox.StandardButton.Discard
                | QMessageBox.StandardButton.Cancel,
                QMessageBox.StandardButton.Save,
            )
            if reply == QMessageBox.StandardButton.Cancel:
                event.ignore()
                return
            if reply == QMessageBox.StandardButton.Save:
                self._save_state()
        if self._audio_stream is not None:
            try:
                self._audio_stream.stop()
                self._audio_stream.close()
            except Exception:
                pass
            self._audio_stream = None
        self._gamepad.shutdown()
        self._emu.destroy()
        event.accept()
