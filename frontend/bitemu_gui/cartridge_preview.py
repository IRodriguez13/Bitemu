"""
Plantilla de cartucho para preview en biblioteca.
Genesis: forma rectangular con zona de etiqueta para boxart.
"""
from PySide6.QtCore import Qt, QRect
from PySide6.QtGui import QPainter, QColor, QPixmap, QPen, QBrush
from PySide6.QtWidgets import QWidget, QLabel, QVBoxLayout, QFrame

from .profile import ConsoleProfile


class CartridgePreview(QFrame):
    """
    Preview tipo cartucho: fondo con forma de cartucho Genesis,
    boxart superpuesto en la zona de la etiqueta.
    """

    def __init__(self, profile: ConsoleProfile, size: int = 200, parent=None):
        super().__init__(parent)
        self._profile = profile
        self._size = size
        self._boxart: QPixmap | None = None
        self._title = ""
        self.setFixedSize(size, int(size * 1.4))
        self.setFrameShape(QFrame.Shape.NoFrame)

    def set_boxart(self, pixmap: QPixmap | None):
        self._boxart = pixmap
        self.update()

    def set_title(self, title: str):
        self._title = title
        self.update()

    def paintEvent(self, event):
        super().paintEvent(event)
        w, h = self.width(), self.height()
        painter = QPainter(self)

        # Fondo: cartucho oscuro (plástico)
        cart_color = QColor(
            self._profile.splash_bg[0] + 20,
            self._profile.splash_bg[1] + 20,
            self._profile.splash_bg[2] + 20,
        )
        painter.fillRect(0, 0, w, h, cart_color)

        # Borde tipo plástico
        accent = self._profile.splash_accent
        painter.setPen(QPen(QColor(accent[0], accent[1], accent[2]), 2))
        painter.setBrush(Qt.BrushStyle.NoBrush)
        painter.drawRoundedRect(2, 2, w - 4, h - 4, 8, 8)

        # Zona etiqueta: rectángulo centrado (~60% ancho, ~45% alto)
        label_margin = int(w * 0.12)
        label_x = label_margin
        label_y = int(h * 0.15)
        label_w = w - 2 * label_margin
        label_h = int(h * 0.55)
        label_rect = QRect(label_x, label_y, label_w, label_h)

        if self._boxart and not self._boxart.isNull():
            scaled = self._boxart.scaled(
                label_w, label_h,
                Qt.AspectRatioMode.KeepAspectRatio,
                Qt.TransformationMode.SmoothTransformation,
            )
            px = label_x + (label_w - scaled.width()) // 2
            py = label_y + (label_h - scaled.height()) // 2
            painter.drawPixmap(px, py, scaled)
        else:
            # Placeholder: texto del juego
            painter.setPen(QColor(accent[0], accent[1], accent[2]))
            font = painter.font()
            font.setPixelSize(min(14, label_h // 8))
            font.setBold(True)
            painter.setFont(font)
            painter.drawText(
                label_rect,
                Qt.AlignmentFlag.AlignCenter | Qt.TextFlag.TextWordWrap,
                self._title or "Seleccioná un juego",
            )

        painter.end()
