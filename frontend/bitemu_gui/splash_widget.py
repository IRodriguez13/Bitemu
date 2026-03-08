"""
Splash / Main Menu — BITEMU pixel-art logo with Start, Options, and Quit buttons.
Replaces the old in-GameWidget splash; acts as the app's home screen.
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

from PySide6.QtCore import Qt, Signal, QTimer
from PySide6.QtGui import QPainter, QColor, QFont
from PySide6.QtWidgets import (
    QWidget,
    QPushButton,
    QVBoxLayout,
    QHBoxLayout,
    QSizePolicy,
    QSpacerItem,
)

from .profile import ConsoleProfile, DEFAULT_PROFILE
from . import get_version

_SPLASH_CHARS = {
    'B': [0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110],
    'I': [0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b11111],
    'T': [0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100],
    'E': [0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111],
    'M': [0b10001, 0b11011, 0b10101, 0b10101, 0b10001, 0b10001, 0b10001],
    'U': [0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110],
}
_CHAR_W, _CHAR_H, _CHAR_GAP = 5, 7, 1
_DING_SR = 44100
_DING_AMP = 8000


def _generate_ding() -> bytes:
    tones = [(523.25, 0.12, 10.0), (1046.5, 0.22, 8.0)]
    gap = int(_DING_SR * 0.04)
    samples = array.array("h")
    for freq, dur, decay in tones:
        n = int(_DING_SR * dur)
        for i in range(n):
            t = i / _DING_SR
            phase = (freq * t) % 1.0
            wave = 1.0 if phase < 0.5 else -1.0
            env = math.exp(-t * decay)
            samples.append(max(-32768, min(32767, int(wave * env * _DING_AMP))))
        for _ in range(gap):
            samples.append(0)
    return samples.tobytes()


def _play_ding():
    if not _SOUNDDEVICE_AVAILABLE:
        return

    def _worker():
        try:
            data = _generate_ding()
            stream = sd.RawOutputStream(samplerate=_DING_SR, channels=1, dtype="int16")
            stream.start()
            stream.write(data)
            sd.sleep(80)
            stream.stop()
            stream.close()
        except Exception:
            pass

    threading.Thread(target=_worker, daemon=True).start()


def _qcolor(rgb: tuple[int, int, int]) -> QColor:
    return QColor(rgb[0], rgb[1], rgb[2])


def _btn_style(bg: tuple[int, int, int], fg: tuple[int, int, int], accent: tuple[int, int, int]) -> str:
    bg_css = f"rgb({bg[0]},{bg[1]},{bg[2]})"
    fg_css = f"rgb({fg[0]},{fg[1]},{fg[2]})"
    acc_css = f"rgb({accent[0]},{accent[1]},{accent[2]})"
    return (
        f"QPushButton {{ background: {bg_css}; color: {fg_css}; border: 2px solid {acc_css};"
        f" border-radius: 6px; padding: 10px 32px; font-family: monospace;"
        f" font-size: 16px; font-weight: bold; }}"
        f" QPushButton:hover {{ background: {acc_css}; }}"
    )


class SplashWidget(QWidget):
    """Main menu screen with BITEMU logo and navigation buttons."""

    start_clicked = Signal()
    options_clicked = Signal()
    quit_clicked = Signal()

    def __init__(self, profile: ConsoleProfile | None = None, parent=None):
        super().__init__(parent)
        self._profile = profile or DEFAULT_PROFILE
        self.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)

        style = _btn_style(self._profile.splash_bg, self._profile.splash_fg, self._profile.splash_accent)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)

        layout.addSpacerItem(QSpacerItem(0, 0, QSizePolicy.Policy.Minimum, QSizePolicy.Policy.Expanding))

        btn_layout = QVBoxLayout()
        btn_layout.setSpacing(12)
        btn_layout.setAlignment(Qt.AlignmentFlag.AlignHCenter)

        self._btn_start = QPushButton("Start")
        self._btn_options = QPushButton("Opciones")
        self._btn_quit = QPushButton("Cerrar Bitemu")

        for btn in (self._btn_start, self._btn_options, self._btn_quit):
            btn.setMinimumWidth(220)
            btn.setCursor(Qt.CursorShape.PointingHandCursor)
            btn.setStyleSheet(style)
            btn_layout.addWidget(btn, alignment=Qt.AlignmentFlag.AlignHCenter)

        layout.addLayout(btn_layout)
        layout.addSpacerItem(QSpacerItem(0, 40, QSizePolicy.Policy.Minimum, QSizePolicy.Policy.Fixed))

        self._btn_start.clicked.connect(self.start_clicked)
        self._btn_options.clicked.connect(self.options_clicked)
        self._btn_quit.clicked.connect(self.quit_clicked)

        QTimer.singleShot(300, _play_ding)

    def paintEvent(self, event):
        super().paintEvent(event)
        rw, rh = self.width(), self.height()
        painter = QPainter(self)

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
        y0 = int(rh * 0.18)

        for ci, ch in enumerate(text):
            rows = _SPLASH_CHARS.get(ch)
            if not rows:
                continue
            cx = x0 + ci * (_CHAR_W + _CHAR_GAP) * px
            for r in range(_CHAR_H):
                for c in range(_CHAR_W):
                    if rows[r] & (1 << (_CHAR_W - 1 - c)):
                        painter.fillRect(cx + c * px, y0 + r * px, px, px, col_fg)

        line_y = y0 + logo_h + px * 2
        line_w = int(logo_w * 0.7)
        line_h = max(1, px // 3)
        painter.fillRect((rw - line_w) // 2, line_y, line_w, line_h, col_accent)

        font = QFont("monospace")
        font.setPixelSize(max(10, px * 2))
        painter.setFont(font)
        painter.setPen(col_sub)
        sub_y = line_y + px * 2
        painter.drawText(
            0, sub_y, rw, px * 4,
            Qt.AlignmentFlag.AlignHCenter | Qt.AlignmentFlag.AlignTop,
            f"-v{get_version()}-",
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

        painter.end()
