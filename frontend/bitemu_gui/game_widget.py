"""
Widget central: dibuja el framebuffer del core y captura teclas.
Escala el juego para llenar la ventana (relación de aspecto 160:144).
"""
from PySide6.QtCore import Qt, QTimer, QSize
from PySide6.QtGui import QImage, QPainter, QColor
from PySide6.QtWidgets import QWidget, QSizePolicy

from .core import Emu, FB_WIDTH, FB_HEIGHT
from .keys import build_joypad_state
from .profile import ConsoleProfile, DEFAULT_PROFILE


class GameWidget(QWidget):
    def __init__(self, profile: ConsoleProfile | None = None, parent=None):
        super().__init__(parent)
        self._profile = profile or DEFAULT_PROFILE
        self._emu: Emu | None = None
        self._pressed_keys: set = set()
        self._paused = False
        self.setFocusPolicy(Qt.FocusPolicy.StrongFocus)
        self.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        self.setMinimumSize(
            self._profile.fb_width * 2,
            self._profile.fb_height * 2,
        )
        self._timer = QTimer(self)
        self._timer.timeout.connect(self._on_tick)
        self._timer.start(int(1000 / 59.73))  # ~60 FPS GB

    def set_emu(self, emu: Emu | None):
        self._emu = emu

    def set_paused(self, paused: bool):
        self._paused = paused

    def _on_tick(self):
        if not self._emu or not self._emu.is_valid or self._paused:
            return
        self._emu.set_input(build_joypad_state(self._pressed_keys))
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
        if not self._emu or not self._emu.is_valid:
            return
        fb = self._emu.get_framebuffer()
        w, h = self._profile.fb_width, self._profile.fb_height
        if not fb or len(fb) < w * h:
            return
        img = QImage(w, h, QImage.Format.Format_RGB32)
        for y in range(h):
            for x in range(w):
                g = fb[y * w + x]
                img.setPixel(x, y, (g << 16) | (g << 8) | g | 0xFF000000)
        # Escalar para llenar el widget (mínimo márgenes; relación de aspecto fija)
        rw, rh = self.width(), self.height()
        scale = min(rw / w, rh / h) if rw and rh else 1
        sw, sh = int(w * scale), int(h * scale)
        ox = (rw - sw) // 2
        oy = (rh - sh) // 2
        painter = QPainter(self)
        painter.fillRect(0, 0, rw, rh, QColor(0, 0, 0))
        scaled = img.scaled(sw, sh, Qt.AspectRatioMode.IgnoreAspectRatio, Qt.TransformationMode.SmoothTransformation)
        painter.drawImage(ox, oy, scaled)
        painter.end()

    def sizeHint(self):
        return QSize(self._profile.fb_width * 4, self._profile.fb_height * 4)
