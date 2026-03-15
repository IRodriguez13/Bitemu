"""
Widget central: dibuja el framebuffer del core y captura teclas.
Escala el juego para llenar la ventana (relación de aspecto 160:144).
Muestra una pantalla de carga breve al cargar una ROM.
"""
from PySide6.QtCore import Qt, QTimer, QSize
from PySide6.QtGui import QImage, QPainter, QColor, QFont
from PySide6.QtWidgets import QWidget, QSizePolicy

from .core import Emu, FB_WIDTH, FB_HEIGHT
from .keys import build_joypad_state
from .profile import ConsoleProfile, DEFAULT_PROFILE
from .input_config import InputConfig


def _qcolor(rgb: tuple[int, int, int]) -> QColor:
    return QColor(rgb[0], rgb[1], rgb[2])


class GameWidget(QWidget):
    def __init__(self, profile: ConsoleProfile | None = None, input_config: InputConfig | None = None, parent=None):
        super().__init__(parent)
        self._profile = profile or DEFAULT_PROFILE
        self._input_config = input_config
        self._emu: Emu | None = None
        self._pressed_keys: set = set()
        self._gamepad_state: int = 0
        self._paused = False
        self._loading_rom_name: str | None = None
        self.setFocusPolicy(Qt.FocusPolicy.StrongFocus)
        self.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        self.setMinimumSize(
            self._profile.fb_width * 2,
            self._profile.fb_height * 2,
        )
        self._timer = QTimer(self)
        self._timer.timeout.connect(self._on_tick)
        self._timer.start(int(1000 / 59.73))

    def set_emu(self, emu: Emu | None):
        self._emu = emu

    def set_profile(self, profile: ConsoleProfile):
        """Cambiar perfil (ej. al elegir otra edición en el lobby)."""
        self._profile = profile
        self.setMinimumSize(
            self._profile.fb_width * 2,
            self._profile.fb_height * 2,
        )

    def set_paused(self, paused: bool):
        self._paused = paused

    def show_loading(self, rom_name: str, duration_ms: int = 1000):
        self._loading_rom_name = rom_name
        self.update()
        if self._emu:
            self._emu.play_chirp()
        QTimer.singleShot(duration_ms, self._finish_loading)

    def _finish_loading(self):
        self._loading_rom_name = None
        self.update()

    def set_gamepad_state(self, state: int):
        self._gamepad_state = state & 0xFF

    def _on_tick(self):
        if not self._emu or not self._emu.is_valid or self._paused:
            return
        if self._loading_rom_name:
            return
        km = self._input_config.keyboard_map if self._input_config else None
        kb_state = build_joypad_state(self._pressed_keys, km)
        combined = (kb_state | self._gamepad_state) & 0xFF
        self._emu.set_input(combined)
        self._emu.run_frame()
        self.update()

    def keyPressEvent(self, event):
        key = event.key()
        self._pressed_keys.add(key)
        event.accept()

    def keyReleaseEvent(self, event):
        self._pressed_keys.discard(event.key())
        event.accept()

    def paintEvent(self, event):
        super().paintEvent(event)
        rw, rh = self.width(), self.height()
        painter = QPainter(self)

        if self._loading_rom_name:
            self._draw_loading(painter, rw, rh, self._loading_rom_name)
            painter.end()
            return

        if not self._emu or not self._emu.is_valid:
            painter.fillRect(0, 0, rw, rh, QColor(0, 0, 0))
            painter.end()
            return

        fb = self._emu.get_framebuffer()
        w, h = self._emu.get_video_size()
        if fb is None or len(fb) < w * h:
            painter.fillRect(0, 0, rw, rh, QColor(0, 0, 0))
            painter.end()
            return

        raw = bytes(fb)
        img = QImage(raw, w, h, w, QImage.Format.Format_Grayscale8)

        scale = min(rw / w, rh / h) if rw and rh else 1
        sw, sh = int(w * scale), int(h * scale)
        ox = (rw - sw) // 2
        oy = (rh - sh) // 2
        painter.fillRect(0, 0, rw, rh, QColor(0, 0, 0))
        scaled = img.scaled(
            sw, sh,
            Qt.AspectRatioMode.KeepAspectRatio,
            Qt.TransformationMode.FastTransformation,
        )
        painter.drawImage(ox, oy, scaled)
        painter.end()

    def _draw_loading(self, painter, rw, rh, rom_name: str):
        p = self._profile
        col_bg = _qcolor(p.splash_bg)
        col_fg = _qcolor(p.splash_fg)
        col_accent = _qcolor(p.splash_accent)

        painter.fillRect(0, 0, rw, rh, col_bg)

        title_font = QFont("monospace")
        title_size = max(10, min(rh // 20, rw // 18))
        title_font.setPixelSize(title_size)
        title_font.setBold(True)
        painter.setFont(title_font)
        painter.setPen(col_fg)

        title_y = rh // 2 - title_size
        painter.drawText(
            0, title_y, rw, title_size + 4,
            Qt.AlignmentFlag.AlignHCenter | Qt.AlignmentFlag.AlignVCenter,
            "Cargando...",
        )

        bar_w = int(rw * 0.45)
        bar_h = max(2, title_size // 5)
        bar_x = (rw - bar_w) // 2
        bar_y = title_y + title_size + title_size // 3
        painter.fillRect(bar_x, bar_y, bar_w, bar_h, col_accent)

        name_font = QFont("monospace")
        name_size = max(8, title_size * 2 // 5)
        name_font.setPixelSize(name_size)
        painter.setFont(name_font)
        painter.setPen(col_accent)
        name_y = bar_y + bar_h + name_size // 3
        max_chars = max(20, rw // (name_size * 2 // 3 + 1))
        display_name = rom_name if len(rom_name) <= max_chars else rom_name[:max_chars - 3] + "..."
        painter.drawText(
            0, name_y, rw, name_size + 4,
            Qt.AlignmentFlag.AlignHCenter | Qt.AlignmentFlag.AlignVCenter,
            display_name,
        )

    def sizeHint(self):
        return QSize(self._profile.fb_width * 4, self._profile.fb_height * 4)
