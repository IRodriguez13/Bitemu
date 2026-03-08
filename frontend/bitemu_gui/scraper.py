"""
Cover Scraper — downloads boxart from the libretro-thumbnails CDN.

Tries multiple image categories in order (Boxarts -> Title screens ->
In-game snaps) so coverage is maximised even when boxart is missing.

No API key required.  Images are cached locally under
~/.cache/bitemu/covers/{system}/ so each cover is fetched at most once.
"""
import os
import platform
from urllib.parse import quote

from PySide6.QtCore import QObject, Signal, QUrl
from PySide6.QtGui import QPixmap
from PySide6.QtNetwork import QNetworkAccessManager, QNetworkRequest, QNetworkReply

_CDN_BASE = "https://thumbnails.libretro.com"

_IMAGE_CATEGORIES = [
    "Named_Boxarts",
    "Named_Titles",
    "Named_Snaps",
]


def _cache_root() -> str:
    if platform.system() == "Windows":
        base = os.environ.get("LOCALAPPDATA", os.path.expanduser("~"))
        return os.path.join(base, "Bitemu", "covers")
    elif platform.system() == "Darwin":
        return os.path.expanduser("~/Library/Caches/Bitemu/covers")
    xdg = os.environ.get("XDG_CACHE_HOME", os.path.expanduser("~/.cache"))
    return os.path.join(xdg, "bitemu", "covers")


class CoverCache:
    """Disk-backed QPixmap cache keyed by (system, rom_name)."""

    def __init__(self):
        self._root = _cache_root()

    def _path(self, system: str, rom_name: str) -> str:
        safe_sys = system.replace("/", "_").replace("\\", "_")
        safe_name = rom_name.replace("/", "_").replace("\\", "_")
        return os.path.join(self._root, safe_sys, safe_name + ".png")

    def get(self, system: str, rom_name: str) -> QPixmap | None:
        p = self._path(system, rom_name)
        if os.path.isfile(p):
            pix = QPixmap(p)
            if not pix.isNull():
                return pix
        return None

    def store(self, system: str, rom_name: str, data: bytes) -> QPixmap | None:
        p = self._path(system, rom_name)
        os.makedirs(os.path.dirname(p), exist_ok=True)
        with open(p, "wb") as f:
            f.write(data)
        pix = QPixmap(p)
        return pix if not pix.isNull() else None


class CoverScraper(QObject):
    """Async cover downloader — tries Boxarts, then Titles, then Snaps."""

    cover_ready = Signal(str, QPixmap)  # (rom_name, pixmap)
    cover_failed = Signal(str)          # (rom_name)

    def __init__(self, system: str, parent=None):
        super().__init__(parent)
        self._system = system
        self._cache = CoverCache()
        self._nam = QNetworkAccessManager(self)
        self._pending: dict[QNetworkReply, tuple[str, int]] = {}

    def fetch(self, rom_name: str):
        """Request a cover.  Emits cover_ready or cover_failed."""
        cached = self._cache.get(self._system, rom_name)
        if cached is not None:
            self.cover_ready.emit(rom_name, cached)
            return
        self._try_category(rom_name, 0)

    def _try_category(self, rom_name: str, cat_idx: int):
        if cat_idx >= len(_IMAGE_CATEGORIES):
            self.cover_failed.emit(rom_name)
            return

        encoded_sys = quote(self._system)
        encoded_name = quote(rom_name)
        category = _IMAGE_CATEGORIES[cat_idx]
        url = f"{_CDN_BASE}/{encoded_sys}/{category}/{encoded_name}.png"

        req = QNetworkRequest(QUrl(url))
        reply = self._nam.get(req)
        self._pending[reply] = (rom_name, cat_idx)
        reply.finished.connect(lambda r=reply: self._on_finished(r))

    def _on_finished(self, reply: QNetworkReply):
        entry = self._pending.pop(reply, None)
        if entry is None:
            reply.deleteLater()
            return

        rom_name, cat_idx = entry

        if reply.error() == QNetworkReply.NetworkError.NoError:
            data = bytes(reply.readAll())
            pix = self._cache.store(self._system, rom_name, data)
            if pix and not pix.isNull():
                self.cover_ready.emit(rom_name, pix)
                reply.deleteLater()
                return

        reply.deleteLater()
        self._try_category(rom_name, cat_idx + 1)
