"""
Ventana principal: navega entre Splash (menú), Library (grilla de ROMs) y Game (emulación).
Audio: core en C (ALSA en Linux). Sin PortAudio/sounddevice.
"""
import os
import sys

from PySide6.QtCore import Qt, QSettings, QUrl, QEvent
from PySide6.QtGui import QAction, QKeySequence, QCloseEvent, QIcon, QPixmap
from PySide6.QtWidgets import (
    QMainWindow,
    QWidget,
    QVBoxLayout,
    QStackedWidget,
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
from .splash_widget import SplashWidget
from .lobby_widget import LobbyWidget
from .library_widget import LibraryWidget
from .profile import DEFAULT_PROFILE, ConsoleProfile, ensure_rom_folder_structure
from .input_config import InputConfig
from .keys import key_to_joypad
from .gamepad import GamepadPoller
from .input_dialog import InputSettingsDialog
from .options_dialog import OptionsDialog
from .update_checker import UpdateChecker
from .i18n import t

RECENT_KEY = "bitemu/recent_roms"
RECENT_MAX = 10


def _asset_path(filename: str) -> str:
    """Resolve asset path for both dev and PyInstaller bundle."""
    bundle = getattr(sys, "_MEIPASS", None)
    if bundle:
        return os.path.join(bundle, "assets", filename)
    return os.path.join(os.path.dirname(__file__), "..", "assets", filename)
AUDIO_KEY = "bitemu/audio_enabled"
AUDIO_VOLUME_KEY = "bitemu/audio_volume"
ROM_FOLDER_KEY = "bitemu/rom_folder"

_PAGE_SPLASH = 0
_PAGE_LOBBY = 1
_PAGE_LIBRARY = 2
_PAGE_GAME = 3


class MainWindow(QMainWindow):
    def __init__(self, profile: ConsoleProfile | None = None):
        super().__init__()
        self._profile = profile or DEFAULT_PROFILE
        self._emu = Emu()
        self._rom_path: str | None = None
        self._paused = False
        self._input_config = InputConfig()
        self._gamepad = GamepadPoller(self._input_config, profile=self._profile, parent=self)
        self._input_dialog: InputSettingsDialog | None = None
        self.setWindowTitle(self._profile.window_title)
        icon_path = _asset_path("bitemu-web.png")
        if os.path.isfile(icon_path):
            self.setWindowIcon(QIcon(icon_path))
        self.setMinimumSize(500, 450)
        self.resize(
            self._profile.fb_width * 4 + 60,
            self._profile.fb_height * 4 + 120,
        )

        self._stack = QStackedWidget()
        self.setCentralWidget(self._stack)

        self._splash = SplashWidget(
            profile=self._profile,
            on_show_sound=lambda: self._emu.play_ding(),
        )
        self._lobby = LobbyWidget(
            on_show_sound=lambda: self._emu.play_ding(),
        )
        self._library = LibraryWidget(
            profile=self._profile,
            input_config=self._input_config,
        )
        self._game = GameWidget(profile=self._profile, input_config=self._input_config)

        self._stack.addWidget(self._splash)   # 0
        self._stack.addWidget(self._lobby)    # 1
        self._stack.addWidget(self._library)  # 2
        self._stack.addWidget(self._game)     # 3

        self._splash.start_clicked.connect(self._show_lobby)
        self._splash.options_clicked.connect(self._open_options)
        self._splash.quit_clicked.connect(self.close)

        self._lobby.edition_selected.connect(self._on_edition_selected)
        self._lobby.back_clicked.connect(self._show_splash)

        self._library.rom_selected.connect(self._load_rom_path)
        self._library.back_clicked.connect(self._show_lobby)
        self._library.open_rom_clicked.connect(self._open_rom)
        self._library.change_folder_clicked.connect(self._pick_rom_folder)

        self._gamepad.state_changed.connect(self._game.set_gamepad_state)
        self._gamepad.gamepad_connected.connect(self._on_gamepad_connected)
        self._gamepad.gamepad_disconnected.connect(self._on_gamepad_disconnected)

        self._create_menus()
        self._status = QStatusBar()
        self.setStatusBar(self._status)
        self._status.setMinimumHeight(28)
        self._status.setStyleSheet("QStatusBar { padding: 4px 8px; background: palette(mid); }")
        self._status.showMessage("Bitemu — Menú principal")

        if not self._emu.create():
            QMessageBox.critical(self, "Error", "No se pudo crear el emulador.")
            QApplication.quit()
            return
        if not self._emu.init_audio():
            pass
        s = QSettings()
        self._emu.set_audio_enabled(s.value(AUDIO_KEY, False, type=bool))
        vol = s.value(AUDIO_VOLUME_KEY, 100, type=int)
        self._emu.set_audio_volume(max(0, min(100, vol)) / 100.0)
        self._game.set_emu(self._emu)

        from . import get_version
        self._update_checker = UpdateChecker(get_version(), parent=self)
        self._update_checker.update_available.connect(self._on_update_available)
        self._update_checker.check()

        QApplication.instance().installEventFilter(self)

    def eventFilter(self, obj, event):
        """Intercepta J/K en biblioteca para cargar ROM y volver."""
        if event.type() == QEvent.Type.KeyPress and self._stack.currentIndex() == _PAGE_LIBRARY:
            # Solo si el evento es para esta ventana o sus hijos (no diálogos)
            w = obj
            while w:
                if w == self:
                    break
                w = w.parent() if hasattr(w, "parent") else None
            else:
                return super().eventFilter(obj, event)
            key = event.key()
            km = self._input_config.get_keyboard_map(self._profile) if self._input_config else None
            bit = key_to_joypad(key, km)
            if bit == 4:  # A = cargar ROM
                entry = self._library.current_entry()
                if entry:
                    self._load_rom_path(entry.path)
                return True
            if bit == 5:  # B = volver
                self._show_lobby()
                return True
        return super().eventFilter(obj, event)

    # ── Update notification ──────────────────────────────────

    def _on_update_available(self, tag: str, url: str):
        from PySide6.QtGui import QDesktopServices
        msg = QMessageBox(self)
        msg.setIcon(QMessageBox.Icon.Information)
        msg.setWindowTitle("Actualización disponible")
        msg.setText(f"Hay una nueva versión de Bitemu: <b>{tag}</b>")
        msg.setInformativeText("¿Querés ir a la página de descarga?")
        msg.setStandardButtons(QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No)
        msg.button(QMessageBox.StandardButton.Yes).setText("Descargar")
        msg.button(QMessageBox.StandardButton.No).setText("Ahora no")
        if msg.exec() == QMessageBox.StandardButton.Yes:
            QDesktopServices.openUrl(QUrl(url))

    # ── Navigation ─────────────────────────────────────────

    def _show_splash(self):
        self._stack.setCurrentIndex(_PAGE_SPLASH)
        self.setWindowTitle(self._profile.window_title)
        self._status.showMessage("Bitemu — Menú principal")

    def _show_lobby(self):
        self._stack.setCurrentIndex(_PAGE_LOBBY)
        self.setWindowTitle("Bitemu")
        self._status.showMessage(f"Bitemu — {t('lobby.title')}")

    def _on_edition_selected(self, profile: ConsoleProfile):
        self._profile = profile
        self._library.set_profile(profile)
        self._game.set_profile(profile)
        self._gamepad.set_profile(profile)
        self._show_library()

    def _show_library(self):
        folder = QSettings().value(ROM_FOLDER_KEY, "", type=str)
        if not folder or not os.path.isdir(folder):
            self._pick_rom_folder()
            folder = QSettings().value(ROM_FOLDER_KEY, "", type=str)
        self._library.load_roms(folder if folder else None)
        self.setWindowTitle(self._profile.window_title)
        self._stack.setCurrentIndex(_PAGE_LIBRARY)
        self._status.showMessage(
            f"Biblioteca — {self._profile.name} — {folder}" if folder
            else f"Biblioteca — {self._profile.name} — sin carpeta configurada"
        )

    def _show_game(self):
        self._stack.setCurrentIndex(_PAGE_GAME)
        self._game.setFocus()

    # ── Menus ──────────────────────────────────────────────

    def _create_menus(self):
        menubar = self.menuBar()
        menubar.clear()

        file_menu = menubar.addMenu(t("menu.file"))
        open_act = QAction(t("menu.open_rom"), self)
        open_act.setShortcut(QKeySequence.StandardKey.Open)
        open_act.triggered.connect(self._open_rom)
        file_menu.addAction(open_act)
        recent_menu = file_menu.addMenu(t("menu.recent"))
        self._recent_menu = recent_menu
        self._refresh_recent_menu()
        file_menu.addSeparator()
        quit_act = QAction(t("menu.quit"), self)
        quit_act.setShortcut(QKeySequence.StandardKey.Quit)
        quit_act.triggered.connect(self.close)
        file_menu.addAction(quit_act)

        emu_menu = menubar.addMenu(t("menu.emulation"))
        self._pause_act = QAction(t("menu.pause"), self)
        self._pause_act.setShortcut(QKeySequence("F3"))
        self._pause_act.triggered.connect(self._toggle_pause)
        emu_menu.addAction(self._pause_act)
        reset_act = QAction(t("menu.reset"), self)
        reset_act.setShortcut(QKeySequence("F4"))
        reset_act.triggered.connect(self._reset)
        emu_menu.addAction(reset_act)
        close_rom_act = QAction(t("menu.close_rom"), self)
        close_rom_act.setShortcut(QKeySequence("F5"))
        close_rom_act.triggered.connect(self._close_rom)
        emu_menu.addAction(close_rom_act)
        emu_menu.addSeparator()
        save_state_act = QAction(t("menu.save_state"), self)
        save_state_act.setShortcut(QKeySequence("F6"))
        save_state_act.triggered.connect(self._save_state)
        emu_menu.addAction(save_state_act)
        load_state_act = QAction(t("menu.load_state"), self)
        load_state_act.setShortcut(QKeySequence("F7"))
        load_state_act.triggered.connect(self._load_state)
        emu_menu.addAction(load_state_act)

        opts_menu = menubar.addMenu(t("menu.options"))
        opts_act = QAction(t("menu.options_dlg"), self)
        opts_act.triggered.connect(self._open_options)
        opts_menu.addAction(opts_act)

        help_menu = menubar.addMenu(t("menu.help"))
        controls_act = QAction(t("menu.controls"), self)
        controls_act.triggered.connect(self._show_controls)
        help_menu.addAction(controls_act)
        about_act = QAction(t("menu.about"), self)
        about_act.triggered.connect(self._show_about)
        help_menu.addAction(about_act)

    # ── ROM loading ────────────────────────────────────────

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
            self, "Abrir ROM", "",
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
        self._show_game()
        self._game.show_loading(name)

    # ── ROM folder ─────────────────────────────────────────

    def _pick_rom_folder(self):
        current = QSettings().value(ROM_FOLDER_KEY, "", type=str)
        folder = QFileDialog.getExistingDirectory(
            self, "Elegir carpeta de ROMs", current or "",
        )
        if folder:
            QSettings().setValue(ROM_FOLDER_KEY, folder)
            ensure_rom_folder_structure(folder)
            self._status.showMessage(f"Carpeta de ROMs: {folder}")
            if self._stack.currentIndex() == _PAGE_LIBRARY:
                self._library.load_roms(folder)

    # ── Emulation controls ─────────────────────────────────

    def _toggle_pause(self):
        self._paused = not self._paused
        self._game.set_paused(self._paused)
        self._pause_act.setText(t("menu.resume") if self._paused else t("menu.pause"))
        self._status.showMessage("Pausado" if self._paused else self._rom_path or "")

    def _reset(self):
        if self._emu.is_valid:
            self._emu.reset()
        self._status.showMessage("Reiniciado")

    def _close_rom(self):
        if not self._emu.is_valid or self._rom_path is None:
            return
        reply = QMessageBox.question(
            self, "Cerrar juego",
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
        self._pause_act.setText("Pausar")
        self._show_library()

    # ── Save states ────────────────────────────────────────

    def _state_path(self) -> str | None:
        if not self._rom_path:
            return None
        rom_dir = os.path.dirname(self._rom_path)
        base_name = os.path.splitext(os.path.basename(self._rom_path))[0]
        saves_dir = os.path.join(rom_dir, "saves")
        return os.path.join(saves_dir, base_name + ".bst")

    def _save_state(self):
        path = self._state_path()
        if not path or not self._emu.is_valid:
            self._status.showMessage("No se puede guardar: sin ROM cargada")
            return
        saves_dir = os.path.dirname(path)
        os.makedirs(saves_dir, exist_ok=True)
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

    # ── Options ─────────────────────────────────────────────

    def _open_options(self):
        dlg = OptionsDialog(
            self._emu,
            self._input_config,
            self._gamepad,
            parent=self,
        )
        dlg.rom_folder_changed.connect(self._on_rom_folder_changed)
        dlg.language_changed.connect(self._on_language_changed)
        dlg.exec()
        if dlg.result() == QDialog.DialogCode.Accepted and self._stack.currentIndex() == _PAGE_LIBRARY:
            folder = QSettings().value(ROM_FOLDER_KEY, "", type=str)
            self._library.load_roms(folder if folder else None)

    def _on_language_changed(self):
        self._retranslate_ui()

    def _retranslate_ui(self):
        self._splash.retranslate()
        self._refresh_recent_menu()
        # Rebuild menus, etc.
        self._create_menus()

    def _on_rom_folder_changed(self):
        if self._stack.currentIndex() == _PAGE_LIBRARY:
            folder = QSettings().value(ROM_FOLDER_KEY, "", type=str)
            self._library.load_roms(folder if folder else None)

    # ── Gamepad ────────────────────────────────────────────

    def _on_gamepad_connected(self, name: str):
        self._status.showMessage(f"Gamepad detectado: {name}")
        if self._input_dialog is not None:
            self._input_dialog.select_tab_for_profile(self._profile)
            self._input_dialog.activateWindow()
            self._input_dialog.raise_()
            return
        reply = QMessageBox.question(
            self, "Gamepad detectado",
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
                self._input_dialog.select_tab_for_profile(self._profile)
            self._input_dialog.activateWindow()
            self._input_dialog.raise_()
            return
        dlg = InputSettingsDialog(self._input_config, self._gamepad, parent=self)
        self._input_dialog = dlg
        if gamepad_tab:
            dlg.select_tab_for_profile(self._profile)
        dlg.exec()
        self._input_dialog = None
        self._status.showMessage("Controles actualizados")

    # ── Dialogs ────────────────────────────────────────────

    def _show_controls(self):
        QMessageBox.information(
            self, "Controles",
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

        logo_path = _asset_path("bitemu-web.png")
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

    # ── Window close ───────────────────────────────────────

    def closeEvent(self, event: QCloseEvent):
        if self._rom_path is not None:
            reply = QMessageBox.question(
                self, "Salir de Bitemu",
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
        self._gamepad.shutdown()
        self._emu.destroy()
        event.accept()
