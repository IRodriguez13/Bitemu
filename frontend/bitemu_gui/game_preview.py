"""
Preview de juego: cover/cartucho + metadata (desarrollador, año, etc.).
"""
from PySide6.QtCore import Qt
from PySide6.QtGui import QPixmap
from PySide6.QtWidgets import QWidget, QVBoxLayout, QLabel, QFrame, QScrollArea

from .profile import ConsoleProfile, PROFILE_GENESIS
from .cartridge_preview import CartridgePreview
from .metadata_service import MetadataResult
from .i18n import t


def _meta_row(label: str, value: str, accent: tuple) -> str:
    if not value or value.strip() == "":
        value = "—"
    return f"<tr><td style='color:rgb({accent[0]},{accent[1]},{accent[2]});padding-right:8px;'>{label}:</td><td>{value}</td></tr>"


class GamePreviewWithMetadata(QWidget):
    """
    Cover/cartucho arriba + panel de metadata abajo.
    Muestra developer, year, genre, etc. y badge de fuente (API/Local).
    """

    def __init__(self, profile: ConsoleProfile, size: int = 200, parent=None):
        super().__init__(parent)
        self._profile = profile
        self._size = size
        self._metadata: MetadataResult | None = None
        self._placeholder_title = ""

        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(12)

        if profile.name == PROFILE_GENESIS.name:
            self._cover = CartridgePreview(profile, size=size)
        else:
            self._cover = QLabel()
            self._cover.setFixedSize(size, size)
            self._cover.setAlignment(Qt.AlignmentFlag.AlignCenter)
            self._cover.setStyleSheet(
                f"background: rgb({profile.splash_bg[0]},{profile.splash_bg[1]},{profile.splash_bg[2]});"
                " border-radius: 8px;"
            )
            self._cover.setScaledContents(False)
        layout.addWidget(self._cover, alignment=Qt.AlignmentFlag.AlignHCenter)

        self._meta_frame = QFrame()
        r, g, b = profile.splash_bg
        self._meta_frame.setStyleSheet(
            f"QFrame {{ background: rgb({max(0,r-20)},{max(0,g-20)},{max(0,b-20)}); "
            f"border-radius: 6px; border: 1px solid rgb({profile.splash_accent[0]},{profile.splash_accent[1]},{profile.splash_accent[2]}); "
            "padding: 8px; }"
        )
        meta_layout = QVBoxLayout(self._meta_frame)
        meta_layout.setContentsMargins(12, 8, 12, 8)

        self._meta_label = QLabel()
        self._meta_label.setWordWrap(True)
        self._meta_label.setStyleSheet(
            f"color: rgb({profile.splash_fg[0]},{profile.splash_fg[1]},{profile.splash_fg[2]});"
            " font-size: 11px; font-family: monospace; background: transparent;"
        )
        self._meta_label.setTextFormat(Qt.TextFormat.RichText)
        meta_layout.addWidget(self._meta_label)

        self._source_badge = QLabel()
        self._source_badge.setStyleSheet(
            f"color: rgb({profile.splash_accent[0]},{profile.splash_accent[1]},{profile.splash_accent[2]});"
            " font-size: 9px; font-family: monospace; background: transparent;"
        )
        meta_layout.addWidget(self._source_badge)

        layout.addWidget(self._meta_frame)
        self._meta_frame.setMaximumHeight(140)
        self._meta_frame.hide()  # Oculto por defecto; se muestra solo si hay metadata

    def set_cover(self, pixmap: QPixmap | None):
        if isinstance(self._cover, CartridgePreview):
            self._cover.set_boxart(pixmap)
        elif pixmap and not pixmap.isNull():
            scaled = pixmap.scaled(
                self._size, self._size,
                Qt.AspectRatioMode.KeepAspectRatio,
                Qt.TransformationMode.SmoothTransformation,
            )
            self._cover.setPixmap(scaled)
            self._cover.setText("")
        else:
            self._cover.clear()
            self._cover.setText(self._placeholder_title or "")

    def set_title(self, title: str):
        self._placeholder_title = title
        if isinstance(self._cover, CartridgePreview):
            self._cover.set_title(title)
        else:
            p = self._cover.pixmap()
            if not p or p.isNull():
                self._cover.setText(title)

    def set_metadata(self, meta: MetadataResult | None):
        self._metadata = meta
        self._refresh_meta_display()

    def _has_metadata(self, m: MetadataResult) -> bool:
        """True si hay al menos un campo de metadata útil (API o local)."""
        return bool(
            (m.developer or "").strip()
            or (m.year or "").strip()
            or (m.genre or "").strip()
            or (m.players or "").strip()
            or (m.region or "").strip()
        )

    def _refresh_meta_display(self):
        acc = self._profile.splash_accent
        if not self._metadata or not self._has_metadata(self._metadata):
            self._meta_label.setText("")
            self._source_badge.setText("")
            self._meta_frame.hide()
            return

        m = self._metadata
        rows = []
        if (m.developer or "").strip():
            rows.append(_meta_row(t("metadata.developer"), m.developer, acc))
        if (m.year or "").strip():
            rows.append(_meta_row(t("metadata.year"), m.year, acc))
        if (m.genre or "").strip():
            rows.append(_meta_row(t("metadata.genre"), m.genre, acc))
        if (m.players or "").strip():
            rows.append(_meta_row(t("metadata.players"), m.players, acc))
        if (m.region or "").strip():
            rows.append(_meta_row(t("metadata.region"), m.region, acc))

        html = f"<table cellspacing='0' cellpadding='2'>{''.join(rows)}</table>"
        self._meta_label.setText(html)

        if m.from_api:
            self._source_badge.setText(f"✓ {t('metadata.source')}: {t('metadata.api')}")
        else:
            self._source_badge.setText(f"  {t('metadata.source')}: {t('metadata.local')}")

        self._meta_frame.show()
