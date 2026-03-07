"""
Widget central: dibuja el framebuffer del core y captura teclas.
Escala el juego para llenar la ventana (relación de aspecto 160:144).
Muestra un splash "BITEMU" al estilo del boot de la Game Boy original
mientras no haya ROM cargada.
"""
import array
import math
import threading

try:
    import sounddevice as sd
    _SOUNDDEVICE_AVAILABLE = True
except ImportError:
    sd = None
    _SOUNDDEVICE_AVAILABLE = False

from PySide6.QtCore import Qt, QTimer, QSize
from PySide6.QtGui import QImage, QPainter, QColor, QFont
from PySide6.QtWidgets import QWidget, QSizePolicy

from .core import Emu, FB_WIDTH, FB_HEIGHT
from .keys import build_joypad_state
from .profile import ConsoleProfile, DEFAULT_PROFILE

_GB_DARKEST  = QColor(0x0F, 0x38, 0x0F)
_GB_DARK     = QColor(0x30, 0x62, 0x30)
_GB_LIGHT    = QColor(0x8B, 0xAC, 0x0F)
_GB_LIGHTEST = QColor(0x9B, 0xBC, 0x0F)

_SPLASH_CHARS = {
    'B': [0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110],
    'I': [0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b11111],
    'T': [0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100],
    'E': [0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111],
    'M': [0b10001, 0b11011, 0b10101, 0b10101, 0b10001, 0b10001, 0b10001],
    'U': [0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110],
}
_CHAR_W, _CHAR_H, _CHAR_GAP = 5, 7, 1

_DING_SAMPLE_RATE = 44100
_DING_FREQ = 1046.5       # C6
_DING_DURATION = 0.3      # seconds
_DING_DECAY = 12.0        # exponential decay rate
_DING_AMPLITUDE = 8000    # peak int16 amplitude


def _generate_ding() -> bytes:
    """Programmatic Game-Boy-style boot ding: square wave with exponential decay."""
    n = int(_DING_SAMPLE_RATE * _DING_DURATION)
    samples = array.array("h")
    for i in range(n):
        t = i / _DING_SAMPLE_RATE
        phase = (_DING_FREQ * t) % 1.0
        wave = 1.0 if phase < 0.5 else -1.0
        envelope = math.exp(-t * _DING_DECAY)
        sample = int(wave * envelope * _DING_AMPLITUDE)
        samples.append(max(-32768, min(32767, sample)))
    return samples.tobytes()


def _play_ding_async():
    if not _SOUNDDEVICE_AVAILABLE:
        return

    def _worker():
        try:
            data = _generate_ding()
            stream = sd.RawOutputStream(
                samplerate=_DING_SAMPLE_RATE, channels=1, dtype="int16",
            )
            stream.start()
            stream.write(data)
            sd.sleep(80)
            stream.stop()
            stream.close()
        except Exception:
            pass

    threading.Thread(target=_worker, daemon=True).start()


class GameWidget(QWidget):
    def __init__(self, profile: ConsoleProfile | None = None, parent=None):
        super().__init__(parent)
        self._profile = profile or DEFAULT_PROFILE
        self._emu: Emu | None = None
        self._pressed_keys: set = set()
        self._paused = False
        self._show_splash = True
        self.setFocusPolicy(Qt.FocusPolicy.StrongFocus)
        self.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        self.setMinimumSize(
            self._profile.fb_width * 2,
            self._profile.fb_height * 2,
        )
        self._timer = QTimer(self)
        self._timer.timeout.connect(self._on_tick)
        self._timer.start(int(1000 / 59.73))
        QTimer.singleShot(300, _play_ding_async)

    def set_emu(self, emu: Emu | None):
        self._emu = emu

    def set_paused(self, paused: bool):
        self._paused = paused

    def hide_splash(self):
        self._show_splash = False
        self.update()

    def show_splash(self):
        self._show_splash = True
        self.update()
        _play_ding_async()

    def _on_tick(self):
        if not self._emu or not self._emu.is_valid or self._paused:
            return
        if self._show_splash:
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
        rw, rh = self.width(), self.height()
        painter = QPainter(self)

        if self._show_splash or not self._emu or not self._emu.is_valid:
            self._draw_splash(painter, rw, rh)
            painter.end()
            return

        fb = self._emu.get_framebuffer()
        w, h = self._profile.fb_width, self._profile.fb_height
        if not fb or len(fb) < w * h:
            self._draw_splash(painter, rw, rh)
            painter.end()
            return

        img = QImage(w, h, QImage.Format.Format_RGB32)
        for y in range(h):
            for x in range(w):
                g = fb[y * w + x]
                img.setPixel(x, y, (g << 16) | (g << 8) | g | 0xFF000000)
        scale = min(rw / w, rh / h) if rw and rh else 1
        sw, sh = int(w * scale), int(h * scale)
        ox = (rw - sw) // 2
        oy = (rh - sh) // 2
        painter.fillRect(0, 0, rw, rh, QColor(0, 0, 0))
        scaled = img.scaled(
            sw, sh,
            Qt.AspectRatioMode.IgnoreAspectRatio,
            Qt.TransformationMode.SmoothTransformation,
        )
        painter.drawImage(ox, oy, scaled)
        painter.end()

    def _draw_splash(self, painter, rw, rh):
        painter.fillRect(0, 0, rw, rh, _GB_DARKEST)

        text = "BITEMU"
        native_w = len(text) * _CHAR_W + (len(text) - 1) * _CHAR_GAP
        px = max(1, int(rw * 0.55 / native_w))

        logo_w = native_w * px
        logo_h = _CHAR_H * px
        x0 = (rw - logo_w) // 2
        y0 = rh // 2 - logo_h

        for ci, ch in enumerate(text):
            rows = _SPLASH_CHARS.get(ch)
            if not rows:
                continue
            cx = x0 + ci * (_CHAR_W + _CHAR_GAP) * px
            for r in range(_CHAR_H):
                for c in range(_CHAR_W):
                    if rows[r] & (1 << (_CHAR_W - 1 - c)):
                        painter.fillRect(
                            cx + c * px, y0 + r * px, px, px, _GB_LIGHTEST,
                        )

        line_y = y0 + logo_h + px * 2
        line_w = int(logo_w * 0.7)
        line_h = max(1, px // 3)
        painter.fillRect((rw - line_w) // 2, line_y, line_w, line_h, _GB_DARK)

        font = QFont("monospace")
        font.setPixelSize(max(10, px * 2))
        painter.setFont(font)
        painter.setPen(_GB_LIGHT)
        sub_y = line_y + px * 2
        painter.drawText(
            0, sub_y, rw, px * 4,
            Qt.AlignmentFlag.AlignHCenter | Qt.AlignmentFlag.AlignTop,
            "Game Boy Emulator",
        )

    def sizeHint(self):
        return QSize(self._profile.fb_width * 4, self._profile.fb_height * 4)
