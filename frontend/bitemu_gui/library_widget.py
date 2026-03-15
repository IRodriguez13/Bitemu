"""
Game Library — lista + preview (estilo Genesis Plus GX).
Lista de ROMs a la izquierda, portada/cartucho a la derecha.
Portadas desde libretro-thumbnails CDN.
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
)

from .profile import ConsoleProfile, DEFAULT_PROFILE, PROFILE_GENESIS, ensure_rom_folder_structure
from .rom_scanner import RomEntry, scan_folder
from .metadata_service import MetadataService
from .layout_components import AppHeader, AppFooter, ListWithPreviewPanel
from .game_preview import GamePreviewWithMetadata
from .i18n import t

_CARD_W = 160
_CARD_H = 210
_COVER_H = 160
_PREVIEW_SIZE = 200


def _asset_path(filename: str) -> str:
    import sys
    bundle = getattr(sys, "_MEIPASS", None)
    if bundle:
        return os.path.join(bundle, "assets", filename)
    return os.path.join(os.path.dirname(__file__), "..", "assets", filename)


class GameCard(QFrame):
    """Single card: cover image + game name. Emits clicked(path). Usado en modo grid (legacy)."""

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
    """Biblioteca con layout lista + preview (estilo Genesis Plus GX)."""

    rom_selected = Signal(str)
    back_clicked = Signal()
    open_rom_clicked = Signal()
    change_folder_clicked = Signal()

    def __init__(
        self,
        profile: ConsoleProfile | None = None,
        input_config=None,
        parent=None,
    ):
        super().__init__(parent)
        self._profile = profile or DEFAULT_PROFILE
        self._input_config = input_config
        self._cards: dict[str, GameCard] = {}
        self._all_entries: list[RomEntry] = []
        self._metadata = MetadataService(parent=self)

        main_layout = QVBoxLayout(self)
        main_layout.setContentsMargins(12, 12, 12, 12)
        main_layout.setSpacing(12)

        self._header = AppHeader(
            t("library.header"),
            profile=self._profile,
            logo_path=_asset_path("bitemu-web.png"),
        )
        main_layout.addWidget(self._header)

        toolbar = QHBoxLayout()
        p = self._profile
        btn_style = (
            f"QPushButton {{ background: rgb({p.splash_bg[0]},{p.splash_bg[1]},{p.splash_bg[2]});"
            f" color: rgb({p.splash_fg[0]},{p.splash_fg[1]},{p.splash_fg[2]});"
            f" border: 1px solid rgb({p.splash_accent[0]},{p.splash_accent[1]},{p.splash_accent[2]});"
            f" border-radius: 4px; padding: 6px 14px; font-family: monospace; font-weight: bold; }}"
            f" QPushButton:hover {{ background: rgb({p.splash_accent[0]},{p.splash_accent[1]},{p.splash_accent[2]}); }}"
        )
        self._btn_back = QPushButton(t("library.back"))
        self._btn_open = QPushButton(t("library.open_rom"))
        self._btn_folder = QPushButton(t("library.rom_folder"))
        for btn in (self._btn_back, self._btn_open, self._btn_folder):
            btn.setStyleSheet(btn_style)
            btn.setCursor(Qt.CursorShape.PointingHandCursor)
        toolbar.addWidget(self._btn_back)
        toolbar.addWidget(self._btn_open)
        toolbar.addWidget(self._btn_folder)
        toolbar.addStretch()

        self._search = QLineEdit()
        self._search.setPlaceholderText(t("library.search"))
        self._search.setMaximumWidth(250)
        self._search.setStyleSheet(
            f"QLineEdit {{ background: rgb({p.splash_bg[0]},{p.splash_bg[1]},{p.splash_bg[2]});"
            f" color: rgb({p.splash_fg[0]},{p.splash_fg[1]},{p.splash_fg[2]});"
            f" border: 1px solid rgb({p.splash_accent[0]},{p.splash_accent[1]},{p.splash_accent[2]});"
            f" border-radius: 4px; padding: 6px; font-family: monospace; }}"
        )
        toolbar.addWidget(self._search)
        main_layout.addLayout(toolbar)

        def make_preview(parent_widget):
            return GamePreviewWithMetadata(self._profile, size=_PREVIEW_SIZE)

        self._list_panel = ListWithPreviewPanel[RomEntry](
            profile=self._profile,
            list_width=300,
            item_to_display=lambda e: e.display_name,
            preview_widget_factory=make_preview,
        )
        self._list_panel.item_selected.connect(self._on_item_selected)
        self._list_panel.item_activated.connect(self._on_item_activated)
        self._list_panel.set_preview_updater(self._update_preview)
        main_layout.addWidget(self._list_panel, 1)

        self._footer = AppFooter(
            actions=[(5, t("footer.back")), (4, t("footer.load"))],
            input_config=input_config,
            profile=self._profile,
        )
        main_layout.addWidget(self._footer)

        self._empty_label = QLabel(t("library.empty"))
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
        self._search.textChanged.connect(self._filter_list)

        bg = self._profile.splash_bg
        self.setStyleSheet(f"LibraryWidget {{ background: rgb({bg[0]},{bg[1]},{bg[2]}); }}")

    def _update_preview(self, entry: RomEntry | None):
        preview = self._list_panel.get_preview_widget()
        if not preview or not isinstance(preview, GamePreviewWithMetadata):
            return
        if entry is None:
            preview.set_cover(None)
            preview.set_title(t("library.select_game"))
            preview.set_metadata(None)
            return
        card = self._cards.get(entry.raw_name)
        pix = card._cover_label.pixmap() if card else None
        preview.set_cover(pix if (pix and not pix.isNull()) else None)
        preview.set_title(entry.display_name or "—")
        self._metadata.fetch_metadata(
            self._profile.thumbnail_system,
            entry.raw_name,
            lambda rn, meta: self._on_metadata_ready(rn, meta, entry),
        )

    def _on_metadata_ready(self, rom_name: str, meta, current_entry: RomEntry | None):
        if current_entry and current_entry.raw_name == rom_name:
            preview = self._list_panel.get_preview_widget()
            if isinstance(preview, GamePreviewWithMetadata):
                preview.set_metadata(meta)

    def _on_item_selected(self, entry: RomEntry | None):
        if entry:
            self._update_preview(entry)

    def _on_item_activated(self, entry: RomEntry):
        """Doble clic o Enter en la lista → cargar ROM."""
        self.rom_selected.emit(entry.path)

    def set_profile(self, profile: ConsoleProfile):
        self._profile = profile
        self._header.set_profile(profile)
        self._list_panel.set_profile(profile)
        self._footer.set_profile(profile)
        self._footer.set_actions([(5, t("footer.back")), (4, t("footer.load"))])
        self._apply_profile_styles()

    def _apply_profile_styles(self):
        p = self._profile
        btn_style = (
            f"QPushButton {{ background: rgb({p.splash_bg[0]},{p.splash_bg[1]},{p.splash_bg[2]});"
            f" color: rgb({p.splash_fg[0]},{p.splash_fg[1]},{p.splash_fg[2]});"
            f" border: 1px solid rgb({p.splash_accent[0]},{p.splash_accent[1]},{p.splash_accent[2]});"
            f" border-radius: 4px; padding: 6px 14px; font-family: monospace; font-weight: bold; }}"
            f" QPushButton:hover {{ background: rgb({p.splash_accent[0]},{p.splash_accent[1]},{p.splash_accent[2]}); }}"
        )
        for btn in (self._btn_back, self._btn_open, self._btn_folder):
            btn.setStyleSheet(btn_style)
        self._search.setStyleSheet(
            f"QLineEdit {{ background: rgb({p.splash_bg[0]},{p.splash_bg[1]},{p.splash_bg[2]});"
            f" color: rgb({p.splash_fg[0]},{p.splash_fg[1]},{p.splash_fg[2]});"
            f" border: 1px solid rgb({p.splash_accent[0]},{p.splash_accent[1]},{p.splash_accent[2]});"
            f" border-radius: 4px; padding: 6px; font-family: monospace; }}"
        )
        self._empty_label.setStyleSheet(
            f"color: rgb({p.splash_fg[0]},{p.splash_fg[1]},{p.splash_fg[2]}); font-size: 14px;"
            f" font-family: monospace; padding: 40px;"
        )
        self.setStyleSheet(
            f"LibraryWidget {{ background: rgb({p.splash_bg[0]},{p.splash_bg[1]},{p.splash_bg[2]}); }}"
        )

    def load_roms(self, folder: str | None):
        self._cards.clear()
        self._all_entries = []
        if not folder or not os.path.isdir(folder):
            self._empty_label.show()
            self._list_panel.hide()
            return

        ensure_rom_folder_structure(folder)
        # Escanear carpeta raíz primero (compatibilidad); si está vacía, usar subcarpeta por consola
        entries = scan_folder(folder, self._profile.rom_extensions)
        if not entries:
            scan_folder_path = os.path.join(folder, self._profile.rom_subdir)
            if os.path.isdir(scan_folder_path):
                entries = scan_folder(scan_folder_path, self._profile.rom_extensions)
        self._all_entries = entries

        if not entries:
            self._empty_label.show()
            self._list_panel.hide()
            return

        self._empty_label.hide()
        self._list_panel.show()
        for entry in entries:
            card = GameCard(entry, self._profile)
            self._cards[entry.raw_name] = card
        self._list_panel.set_items(entries)
        self._start_scraping()
        if entries:
            self._update_preview(entries[0])

    def _start_scraping(self):
        for entry in self._all_entries:
            self._metadata.fetch_boxart(
                self._profile.thumbnail_system,
                entry.raw_name,
                self._on_cover_ready,
            )

    def _on_cover_ready(self, rom_name: str, pixmap: QPixmap | None):
        card = self._cards.get(rom_name)
        if card and pixmap and not pixmap.isNull():
            card.set_cover(pixmap)
        current = self._list_panel.current_item()
        if current and current.raw_name == rom_name:
            self._update_preview(current)

    def _filter_list(self, text: str):
        query = text.lower().strip()
        filtered = [e for e in self._all_entries if not query or query in e.display_name.lower()]
        self._list_panel.set_items(filtered)
        if filtered:
            self._update_preview(filtered[0])

    def current_entry(self) -> RomEntry | None:
        """ROM actualmente seleccionada en la lista (para J=cargar)."""
        return self._list_panel.current_item()
