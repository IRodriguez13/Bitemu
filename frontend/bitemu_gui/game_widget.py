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
from .input_config import InputConfig
from . import get_version

def _qcolor(rgb: tuple[int, int, int]) -> QColor:
    return QColor(rgb[0], rgb[1], rgb[2])

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
_DING_AMPLITUDE = 8000


def _generate_ding() -> bytes:
    """Two-tone 'ding-ding' C5→C6, Nintendo-style boot sound."""
    tones = [
        (523.25, 0.12, 10.0),   # C5 — short "ding"
        (1046.5, 0.22, 8.0),    # C6 — longer "ding"
    ]
    gap = int(_DING_SAMPLE_RATE * 0.04)
    samples = array.array("h")
    for freq, duration, decay in tones:
        n = int(_DING_SAMPLE_RATE * duration)
        for i in range(n):
            t = i / _DING_SAMPLE_RATE
            phase = (freq * t) % 1.0
            wave = 1.0 if phase < 0.5 else -1.0
            envelope = math.exp(-t * decay)
            sample = int(wave * envelope * _DING_AMPLITUDE)
            samples.append(max(-32768, min(32767, sample)))
        for _ in range(gap):
            samples.append(0)
    return samples.tobytes()


def _generate_chirp() -> bytes:
    """Single short chirp for ROM loading transition."""
    freq, duration, decay = 880.0, 0.08, 14.0
    n = int(_DING_SAMPLE_RATE * duration)
    samples = array.array("h")
    for i in range(n):
        t = i / _DING_SAMPLE_RATE
        phase = (freq * t) % 1.0
        wave = 1.0 if phase < 0.5 else -1.0
        envelope = math.exp(-t * decay)
        sample = int(wave * envelope * _DING_AMPLITUDE)
        samples.append(max(-32768, min(32767, sample)))
    return samples.tobytes()


def _play_sound_async(generator):
    if not _SOUNDDEVICE_AVAILABLE:
        return

    def _worker():
        try:
            data = generator()
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
    def __init__(self, profile: ConsoleProfile | None = None, input_config: InputConfig | None = None, parent=None):
        super().__init__(parent)
        self._profile = profile or DEFAULT_PROFILE
        self._input_config = input_config
        self._emu: Emu | None = None
        self._pressed_keys: set = set()
        self._gamepad_state: int = 0
        self._paused = False
        self._show_splash = True
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
        QTimer.singleShot(300, lambda: _play_sound_async(_generate_ding))

    def set_emu(self, emu: Emu | None):
        self._emu = emu

    def set_paused(self, paused: bool):
        self._paused = paused

    def show_loading(self, rom_name: str, duration_ms: int = 1000):
        """Show a mini loading splash with the ROM name, then start gameplay."""
        self._loading_rom_name = rom_name
        self._show_splash = False
        self.update()
        _play_sound_async(_generate_chirp)
        QTimer.singleShot(duration_ms, self._finish_loading)

    def _finish_loading(self):
        self._loading_rom_name = None
        self.update()

    def hide_splash(self):
        self._show_splash = False
        self._loading_rom_name = None
        self.update()

    def show_splash(self):
        self._show_splash = True
        self._loading_rom_name = None
        self.update()
        _play_sound_async(_generate_ding)

    def set_gamepad_state(self, state: int):
        self._gamepad_state = state & 0xFF

    def _on_tick(self):
        if not self._emu or not self._emu.is_valid or self._paused:
            return
        if self._show_splash or self._loading_rom_name:
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

        if self._show_splash or not self._emu or not self._emu.is_valid:
            self._draw_splash(painter, rw, rh)
            painter.end()
            return

        fb = self._emu.get_framebuffer()
        w, h = self._profile.fb_width, self._profile.fb_height
        if fb is None or len(fb) < w * h:
            self._draw_splash(painter, rw, rh)
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

    def _draw_splash(self, painter, rw, rh):
        p = self._profile
        col_bg = _qcolor(p.splash_bg)
        col_fg = _qcolor(p.splash_fg)
        col_accent = _qcolor(p.splash_accent)
        col_sub = _qcolor(p.splash_sub)

        painter.fillRect(0, 0, rw, rh, col_bg)

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
                            cx + c * px, y0 + r * px, px, px, col_fg,
                        )

        line_y = y0 + logo_h + px * 2
        line_w = int(logo_w * 0.7)
        line_h = max(1, px // 3)
        painter.fillRect((rw - line_w) // 2, line_y, line_w, line_h, col_accent)

        font = QFont("monospace")
        font.setPixelSize(max(10, px * 2))
        painter.setFont(font)
        painter.setPen(col_sub)
        sub_y = line_y + px * 2
        subtitle = f"-v{get_version()}-"
        painter.drawText(
            0, sub_y, rw, px * 4,
            Qt.AlignmentFlag.AlignHCenter | Qt.AlignmentFlag.AlignTop,
            subtitle,
        )

        credit_font = QFont("monospace")
        credit_font.setPixelSize(max(8, px))
        painter.setFont(credit_font)
        painter.setPen(col_accent)
        margin = max(4, px)
        painter.drawText(
            margin, rh - credit_font.pixelSize() - margin, rw, credit_font.pixelSize(),
            Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignTop,
            "Created by: Iván Ezequiel Rodriguez",
        )

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
