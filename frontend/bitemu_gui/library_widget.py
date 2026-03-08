"""
Game Library — scrollable grid of ROM cards with cover art.
Covers are fetched asynchronously from the libretro-thumbnails CDN
and cached on disk.  ROMs without covers show a styled placeholder.
"""
import os

from PySide6.QtCore import Qt, Signal
from PySide6.QtGui import QPixmap, QColor, QPainter, QFont, QMouseEvent
from PySide6.QtWidgets import (
    QWidget,
    QVBoxLayout,
    QHBoxLayout,
    QGridLayout,
    QScrollArea,
    QPushButton,
    QLineEdit,
    QLabel,
    QFrame,
    QSizePolicy,
    QFileDialog,
)

from .profile import ConsoleProfile, DEFAULT_PROFILE
from .rom_scanner import RomEntry, scan_folder
from .scraper import CoverScraper

_CARD_W = 160
_CARD_H = 210
_COVER_H = 160


class GameCard(QFrame):
    """Single card: cover image + game name. Emits clicked(path)."""

    clicked = Signal(str)

    def __init__(self, entry: RomEntry, profile: ConsoleProfile, parent=None):
        super().__init__(parent)
        self._entry = entry
        self._profile = profile
        self.setFixedSize(_CARD_W, _CARD_H)
        self.setCursor(Qt.CursorShape.PointingHandCursor)
        self.setFrameShape(QFrame.Shape.StyledPanel)
        bg = profile.splash_bg
        accent = profile.splash_accent
        self.setStyleSheet(
            f"GameCard {{ background: rgb({bg[0]},{bg[1]},{bg[2]});"
            f" border: 2px solid rgb({accent[0]},{accent[1]},{accent[2]});"
            f" border-radius: 8px; }}"
            f" GameCard:hover {{ border: 2px solid rgb({profile.splash_fg[0]},"
            f"{profile.splash_fg[1]},{profile.splash_fg[2]}); }}"
        )

        layout = QVBoxLayout(self)
        layout.setContentsMargins(4, 4, 4, 4)
        layout.setSpacing(4)

        self._cover_label = QLabel()
        self._cover_label.setFixedSize(_CARD_W - 8, _COVER_H)
        self._cover_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self._set_placeholder()
        layout.addWidget(self._cover_label)

        self._name_label = QLabel(entry.display_name)
        self._name_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self._name_label.setWordWrap(True)
        fg = profile.splash_fg
        self._name_label.setStyleSheet(
            f"color: rgb({fg[0]},{fg[1]},{fg[2]}); font-size: 11px;"
            f" font-family: monospace; background: transparent; border: none;"
        )
        self._name_label.setMaximumHeight(36)
        layout.addWidget(self._name_label)

    def set_cover(self, pixmap: QPixmap):
        scaled = pixmap.scaled(
            _CARD_W - 8, _COVER_H,
            Qt.AspectRatioMode.KeepAspectRatio,
            Qt.TransformationMode.SmoothTransformation,
        )
        self._cover_label.setPixmap(scaled)

    def _set_placeholder(self):
        pix = QPixmap(_CARD_W - 8, _COVER_H)
        bg = self._profile.splash_bg
        fg = self._profile.splash_accent
        pix.fill(QColor(bg[0], bg[1], bg[2]))
        painter = QPainter(pix)
        font = QFont("monospace", 10)
        font.setBold(True)
        painter.setFont(font)
        painter.setPen(QColor(fg[0], fg[1], fg[2]))
        painter.drawText(pix.rect(), Qt.AlignmentFlag.AlignCenter, self._entry.display_name)
        painter.end()
        self._cover_label.setPixmap(pix)

    def mousePressEvent(self, event: QMouseEvent):
        if event.button() == Qt.MouseButton.LeftButton:
            self.clicked.emit(self._entry.path)
        super().mousePressEvent(event)


class LibraryWidget(QWidget):
    """Scrollable grid of GameCard widgets with a toolbar."""

    rom_selected = Signal(str)
    back_clicked = Signal()
    open_rom_clicked = Signal()
    change_folder_clicked = Signal()

    def __init__(self, profile: ConsoleProfile | None = None, parent=None):
        super().__init__(parent)
        self._profile = profile or DEFAULT_PROFILE
        self._cards: dict[str, GameCard] = {}
        self._all_entries: list[RomEntry] = []
        self._scraper: CoverScraper | None = None

        main_layout = QVBoxLayout(self)
        main_layout.setContentsMargins(8, 8, 8, 8)

        toolbar = QHBoxLayout()
        p = self._profile

        btn_style = (
            f"QPushButton {{ background: rgb({p.splash_bg[0]},{p.splash_bg[1]},{p.splash_bg[2]});"
            f" color: rgb({p.splash_fg[0]},{p.splash_fg[1]},{p.splash_fg[2]});"
            f" border: 1px solid rgb({p.splash_accent[0]},{p.splash_accent[1]},{p.splash_accent[2]});"
            f" border-radius: 4px; padding: 6px 14px; font-family: monospace; font-weight: bold; }}"
            f" QPushButton:hover {{ background: rgb({p.splash_accent[0]},{p.splash_accent[1]},{p.splash_accent[2]}); }}"
        )

        self._btn_back = QPushButton("← Volver")
        self._btn_open = QPushButton("Abrir ROM...")
        self._btn_folder = QPushButton("Carpeta de ROMs...")

        for btn in (self._btn_back, self._btn_open, self._btn_folder):
            btn.setStyleSheet(btn_style)
            btn.setCursor(Qt.CursorShape.PointingHandCursor)

        toolbar.addWidget(self._btn_back)
        toolbar.addWidget(self._btn_open)
        toolbar.addWidget(self._btn_folder)
        toolbar.addStretch()

        self._search = QLineEdit()
        self._search.setPlaceholderText("Buscar...")
        self._search.setMaximumWidth(250)
        self._search.setStyleSheet(
            f"QLineEdit {{ background: rgb({p.splash_bg[0]},{p.splash_bg[1]},{p.splash_bg[2]});"
            f" color: rgb({p.splash_fg[0]},{p.splash_fg[1]},{p.splash_fg[2]});"
            f" border: 1px solid rgb({p.splash_accent[0]},{p.splash_accent[1]},{p.splash_accent[2]});"
            f" border-radius: 4px; padding: 6px; font-family: monospace; }}"
        )
        toolbar.addWidget(self._search)

        main_layout.addLayout(toolbar)

        self._scroll = QScrollArea()
        self._scroll.setWidgetResizable(True)
        self._scroll.setStyleSheet("QScrollArea { border: none; background: transparent; }")
        self._grid_widget = QWidget()
        bg = self._profile.splash_bg
        self._grid_widget.setStyleSheet(
            f"background: rgb({bg[0]},{bg[1]},{bg[2]});"
        )
        self._grid_layout = QGridLayout(self._grid_widget)
        self._grid_layout.setSpacing(12)
        self._grid_layout.setAlignment(Qt.AlignmentFlag.AlignTop | Qt.AlignmentFlag.AlignLeft)
        self._scroll.setWidget(self._grid_widget)
        main_layout.addWidget(self._scroll)

        self._empty_label = QLabel("No hay ROMs. Elegí una carpeta de ROMs para comenzar.")
        fg = self._profile.splash_fg
        self._empty_label.setStyleSheet(
            f"color: rgb({fg[0]},{fg[1]},{fg[2]}); font-size: 14px;"
            f" font-family: monospace; padding: 40px;"
        )
        self._empty_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self._empty_label.hide()
        main_layout.addWidget(self._empty_label)

        self._btn_back.clicked.connect(self.back_clicked)
        self._btn_open.clicked.connect(self.open_rom_clicked)
        self._btn_folder.clicked.connect(self.change_folder_clicked)
        self._search.textChanged.connect(self._filter_cards)

        bg_css = f"rgb({bg[0]},{bg[1]},{bg[2]})"
        self.setStyleSheet(f"LibraryWidget {{ background: {bg_css}; }}")

    def load_roms(self, folder: str | None):
        """Scan folder and populate the grid."""
        self._clear_grid()
        if not folder or not os.path.isdir(folder):
            self._empty_label.show()
            self._scroll.hide()
            return

        entries = scan_folder(folder, self._profile.rom_extensions)
        self._all_entries = entries

        if not entries:
            self._empty_label.show()
            self._scroll.hide()
            return

        self._empty_label.hide()
        self._scroll.show()
        self._populate_grid(entries)
        self._start_scraping()

    def _populate_grid(self, entries: list[RomEntry]):
        cols = max(1, (self._scroll.viewport().width() - 24) // (_CARD_W + 12))
        for i, entry in enumerate(entries):
            card = GameCard(entry, self._profile)
            card.clicked.connect(self.rom_selected)
            self._cards[entry.raw_name] = card
            self._grid_layout.addWidget(card, i // cols, i % cols)

    def _clear_grid(self):
        self._cards.clear()
        while self._grid_layout.count():
            item = self._grid_layout.takeAt(0)
            w = item.widget()
            if w:
                w.deleteLater()

    def _start_scraping(self):
        if self._scraper is not None:
            self._scraper.deleteLater()
        self._scraper = CoverScraper(self._profile.thumbnail_system, parent=self)
        self._scraper.cover_ready.connect(self._on_cover_ready)
        for entry in self._all_entries:
            self._scraper.fetch(entry.raw_name)

    def _on_cover_ready(self, rom_name: str, pixmap: QPixmap):
        card = self._cards.get(rom_name)
        if card:
            card.set_cover(pixmap)

    def _filter_cards(self, text: str):
        query = text.lower().strip()
        for raw_name, card in self._cards.items():
            visible = not query or query in card._entry.display_name.lower()
            card.setVisible(visible)

    def resizeEvent(self, event):
        super().resizeEvent(event)
        if self._all_entries and self._cards:
            self._reflow_grid()

    def _reflow_grid(self):
        visible = [c for c in self._cards.values() if not c.isHidden()]
        cols = max(1, (self._scroll.viewport().width() - 24) // (_CARD_W + 12))
        for i, card in enumerate(visible):
            self._grid_layout.addWidget(card, i // cols, i % cols)
