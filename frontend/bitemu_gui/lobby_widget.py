"""
Lobby — selector de consola (Game Boy, Genesis, GBA).
Estilo retro 80s/90s: CRT, neón, tarjetas tipo arcade.
Imágenes de consolas opcionales en assets/consoles/.
"""
import os
import sys
from typing import Callable

from PySide6.QtCore import Qt, Signal
from PySide6.QtGui import QPainter, QColor, QLinearGradient, QPixmap
from PySide6.QtWidgets import (
    QWidget,
    QPushButton,
    QVBoxLayout,
    QHBoxLayout,
    QSizePolicy,
    QSpacerItem,
    QFrame,
    QLabel,
)

from .profile import ConsoleProfile, AVAILABLE_EDITIONS, EDITIONS_WITH_BACKEND
from .i18n import t


def _asset_path(filename: str) -> str:
    bundle = getattr(sys, "_MEIPASS", None)
    if bundle:
        return os.path.join(bundle, "assets", filename)
    return os.path.join(os.path.dirname(__file__), "..", "assets", filename)


class EditionCard(QFrame):
    """Tarjeta de consola estilo arcade/retro. Emite clicked(profile)."""

    clicked = Signal(object)  # ConsoleProfile

    def __init__(self, profile: ConsoleProfile, available: bool, parent=None):
        super().__init__(parent)
        self._profile = profile
        self._available = available
        self.setCursor(Qt.CursorShape.PointingHandCursor if available else Qt.CursorShape.ForbiddenCursor)
        self.setMinimumSize(200, 160)
        self.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        self.setFrameShape(QFrame.Shape.StyledPanel)

        bg = profile.splash_bg
        accent = profile.splash_accent
        fg = profile.splash_fg
        # Bordes gruesos tipo plástico 80s, gradiente sutil
        self.setStyleSheet(
            f"EditionCard {{"
            f" background: qlineargradient(x1:0,y1:0,x2:1,y2:1,"
            f" stop:0 rgb({min(255,bg[0]+20)},{min(255,bg[1]+20)},{min(255,bg[2]+20)}),"
            f" stop:1 rgb({bg[0]},{bg[1]},{bg[2]}));"
            f" border: 3px solid rgb({accent[0]},{accent[1]},{accent[2]});"
            f" border-radius: 12px;"
            f" }}"
            f" EditionCard:hover {{"
            f" border: 3px solid rgb({fg[0]},{fg[1]},{fg[2]});"
            f" }}"
        )

        layout = QVBoxLayout(self)
        layout.setContentsMargins(20, 20, 20, 20)
        layout.setSpacing(10)

        # Imagen de consola (opcional): si existe assets/consoles/xxx.png
        self._img_label = None
        if getattr(profile, "console_image", None):
            img_path = _asset_path(profile.console_image)
            if os.path.isfile(img_path):
                pix = QPixmap(img_path)
                if not pix.isNull():
                    self._img_label = QLabel()
                    self._img_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
                    self._img_label.setFixedHeight(80)
                    self._img_label.setStyleSheet("background: transparent;")
                    scaled = pix.scaled(
                        120, 80,
                        Qt.AspectRatioMode.KeepAspectRatio,
                        Qt.TransformationMode.SmoothTransformation,
                    )
                    self._img_label.setPixmap(scaled)
                    layout.addWidget(self._img_label)

        name = QLabel(profile.name)
        name.setAlignment(Qt.AlignmentFlag.AlignCenter)
        name.setWordWrap(True)
        name.setStyleSheet(
            f"color: rgb({fg[0]},{fg[1]},{fg[2]}); font-size: 16px;"
            f" font-family: 'Courier New', monospace; font-weight: bold; background: transparent;"
        )
        layout.addWidget(name)

        self._soon_label = None
        if not available:
            sub = QLabel(t("lobby.soon"))
            sub.setAlignment(Qt.AlignmentFlag.AlignCenter)
            sub.setStyleSheet(
                f"color: rgb({accent[0]},{accent[1]},{accent[2]}); font-size: 12px;"
                f" font-family: monospace; background: transparent;"
            )
            layout.addWidget(sub)
            self._soon_label = sub

        layout.addStretch()

    def retranslate(self):
        if self._soon_label is not None:
            self._soon_label.setText(t("lobby.soon"))

    def mousePressEvent(self, event):
        if event.button() == Qt.MouseButton.LeftButton and self._available:
            self.clicked.emit(self._profile)
        super().mousePressEvent(event)


class LobbyWidget(QWidget):
    """Pantalla lobby: elegir consola. Estilo retro 80s/90s."""

    edition_selected = Signal(object)  # ConsoleProfile
    back_clicked = Signal()

    def __init__(
        self,
        on_show_sound: Callable[[], None] | None = None,
        parent=None,
    ):
        super().__init__(parent)
        self._on_show_sound = on_show_sound
        self.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        self.setAutoFillBackground(True)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(32, 32, 32, 32)

        # Título estilo neón / arcade
        self._title = QLabel(t("lobby.title"))
        self._title.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self._title.setStyleSheet(
            "color: rgb(255, 220, 100); font-size: 28px;"
            " font-family: 'Courier New', monospace; font-weight: bold;"
        )
        layout.addWidget(self._title)

        layout.addSpacerItem(QSpacerItem(0, 28, QSizePolicy.Policy.Minimum, QSizePolicy.Policy.Fixed))

        # Grid de tarjetas de consolas
        cards_layout = QHBoxLayout()
        cards_layout.setSpacing(28)
        cards_layout.setAlignment(Qt.AlignmentFlag.AlignCenter)

        self._cards = []
        for profile in AVAILABLE_EDITIONS:
            available = profile.name in EDITIONS_WITH_BACKEND
            card = EditionCard(profile, available=available)
            card.clicked.connect(self.edition_selected.emit)
            cards_layout.addWidget(card)
            self._cards.append(card)

        layout.addLayout(cards_layout)
        layout.addStretch()

        # Botón volver
        btn_layout = QHBoxLayout()
        btn_layout.setAlignment(Qt.AlignmentFlag.AlignLeft)
        self._btn_back = QPushButton(t("lobby.back"))
        self._btn_back.setStyleSheet(
            "QPushButton { background: rgb(30, 30, 45); color: rgb(180, 180, 200);"
            " border: 2px solid rgb(80, 80, 120); border-radius: 8px;"
            " padding: 10px 24px; font-family: monospace; font-size: 14px; font-weight: bold; }"
            " QPushButton:hover { background: rgb(50, 50, 80); border-color: rgb(120, 120, 180); }"
        )
        self._btn_back.setCursor(Qt.CursorShape.PointingHandCursor)
        self._btn_back.clicked.connect(self.back_clicked)
        btn_layout.addWidget(self._btn_back)

        layout.addLayout(btn_layout)

    def paintEvent(self, event):
        super().paintEvent(event)
        rw, rh = self.width(), self.height()
        painter = QPainter(self)

        # Fondo oscuro tipo CRT / arcade (azul oscuro con gradiente)
        grad = QLinearGradient(0, 0, rw, rh)
        grad.setColorAt(0, QColor(12, 15, 35))
        grad.setColorAt(0.5, QColor(18, 22, 50))
        grad.setColorAt(1, QColor(8, 10, 25))
        painter.fillRect(0, 0, rw, rh, grad)

        # Grilla noventera (líneas sutiles tipo retícula)
        grid_color = QColor(30, 35, 70, 60)
        painter.setPen(grid_color)
        step = 40
        for x in range(0, rw + 1, step):
            painter.drawLine(x, 0, x, rh)
        for y in range(0, rh + 1, step):
            painter.drawLine(0, y, rw, y)

        # Efecto CRT: scanlines sutiles
        painter.setCompositionMode(QPainter.CompositionMode.CompositionMode_Multiply)
        scan_color = QColor(0, 0, 0, 8)
        for y in range(0, rh, 4):
            painter.fillRect(0, y, rw, 1, scan_color)
        painter.setCompositionMode(QPainter.CompositionMode.CompositionMode_SourceOver)

        painter.end()

    def retranslate(self):
        """Actualiza textos al cambiar idioma."""
        self._title.setText(t("lobby.title"))
        self._btn_back.setText(t("lobby.back"))
        for card in self._cards:
            card.retranslate()
