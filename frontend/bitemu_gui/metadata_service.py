"""
MetadataService centralizado: boxart, título, cartucho, etc.
Backends: Libretro (actual), Screenscraper (futuro), LocalCache.
Desacoplado para reutilizar en GB, Genesis, GBA.
"""
from dataclasses import dataclass, field
from typing import Protocol
from PySide6.QtGui import QPixmap


@dataclass
class MetadataResult:
    """Resultado de metadata para un juego."""
    boxart: QPixmap | None = None
    title: str = ""
    developer: str = ""
    year: str = ""
    genre: str = ""
    players: str = ""
    region: str = ""
    cartridge_path: str | None = None
    raw_name: str = ""
    from_api: bool = False  # True si viene de API (Screenscraper, etc.)


class MetadataBackend(Protocol):
    """Interfaz de backend de metadata."""

    def fetch_boxart(self, system: str, rom_name: str, callback) -> None:
        """Solicita boxart. callback(rom_name, QPixmap | None)."""
        ...

    def get_title(self, system: str, rom_name: str) -> str:
        """Título del juego si está en cache. Vacío si no."""
        ...


class LibretroBackend:
    """
    Backend libretro-thumbnails: Boxarts, Titles, Snaps.
    Usa CoverScraper. Una instancia por (system, parent).
    """

    def __init__(self, parent=None):
        self._parent = parent
        self._scrapers: dict[str, "CoverScraper"] = {}
        self._callbacks: dict[str, list] = {}
        self._titles: dict[tuple[str, str], str] = {}

    def _get_scraper(self, system: str) -> "CoverScraper":
        from .scraper import CoverScraper
        if system not in self._scrapers:
            s = CoverScraper(system, parent=self._parent)
            s.cover_ready.connect(self._on_ready)
            s.cover_failed.connect(self._on_failed)
            self._scrapers[system] = s
        return self._scrapers[system]

    def _on_ready(self, rom_name: str, pixmap: QPixmap):
        cbs = self._callbacks.pop(rom_name, [])
        for cb in cbs:
            cb(rom_name, pixmap)

    def _on_failed(self, rom_name: str):
        cbs = self._callbacks.pop(rom_name, [])
        for cb in cbs:
            cb(rom_name, None)

    def get_cached_boxart(self, system: str, rom_name: str) -> QPixmap | None:
        s = self._get_scraper(system)
        return s._cache.get(system, rom_name)

    def fetch_boxart(self, system: str, rom_name: str, callback) -> None:
        """callback(rom_name, QPixmap | None)."""
        self._callbacks.setdefault(rom_name, []).append(callback)
        self._get_scraper(system).fetch(rom_name)

    def get_title(self, system: str, rom_name: str) -> str:
        return self._titles.get((system, rom_name), "")


class MetadataService:
    """
    Servicio centralizado: agrega backends y expone get(system, rom_id).
    """

    def __init__(self, parent=None):
        self._parent = parent
        self._backends: list[MetadataBackend] = [LibretroBackend(parent=parent)]
        self._cache: dict[tuple[str, str], MetadataResult] = {}

    def get(self, system: str, rom_name: str) -> MetadataResult:
        """Obtiene metadata desde cache. Si no hay, retorna vacío."""
        key = (system, rom_name)
        if key in self._cache:
            return self._cache[key]
        result = MetadataResult(raw_name=rom_name, title=rom_name)
        for backend in self._backends:
            if isinstance(backend, LibretroBackend):
                cached = backend.get_cached_boxart(system, rom_name)
                if cached:
                    result.boxart = cached
                    self._cache[key] = result
                    return result
        self._cache[key] = result
        return result

    def fetch_boxart(self, system: str, rom_name: str, on_ready) -> None:
        """Solicita boxart de forma asíncrona. on_ready(rom_name, QPixmap | None)."""
        self._backends[0].fetch_boxart(system, rom_name, on_ready)

    def fetch_metadata(self, system: str, rom_name: str, on_ready) -> None:
        """Solicita metadata. on_ready(rom_name, MetadataResult). Por ahora local."""
        from PySide6.QtCore import QTimer
        def _emit():
            result = MetadataResult(
                raw_name=rom_name,
                title=rom_name,
                from_api=False,
            )
            key = (system, rom_name)
            if key in self._cache:
                result.boxart = self._cache[key].boxart
            self._cache[key] = result
            if on_ready:
                on_ready(rom_name, result)
        QTimer.singleShot(0, _emit)

    def add_backend(self, backend: MetadataBackend):
        self._backends.append(backend)
