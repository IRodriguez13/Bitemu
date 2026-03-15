"""
Cache de metadata en JSON — persistencia local para uso offline.

Estructura: ~/.cache/bitemu/metadata/{system}.json
Cada sistema tiene un JSON con metadata por rom_name.
Las fuentes de imágenes (libretro-thumbnails) no proveen metadata;
esta cache se llena con: parse del filename, API futura, o edición manual.
"""
import json
import os
import platform
import re
from typing import Any


def _cache_root() -> str:
    if platform.system() == "Windows":
        base = os.environ.get("LOCALAPPDATA", os.path.expanduser("~"))
        return os.path.join(base, "Bitemu", "metadata")
    elif platform.system() == "Darwin":
        return os.path.expanduser("~/Library/Caches/Bitemu/metadata")
    xdg = os.environ.get("XDG_CACHE_HOME", os.path.expanduser("~/.cache"))
    return os.path.join(xdg, "bitemu", "metadata")


def _safe_system(system: str) -> str:
    return system.replace("/", "_").replace("\\", "_").replace(":", "_")


# Patrones comunes en nombres No-Intro/Redump para extraer región
_REGION_PATTERNS = [
    (r"\(USA\)", "USA"),
    (r"\(Europe\)", "Europe"),
    (r"\(World\)", "World"),
    (r"\(Japan\)", "Japan"),
    (r"\(JP\)", "Japan"),
    (r"\(UK\)", "UK"),
    (r"\(Germany\)", "Germany"),
    (r"\(France\)", "France"),
    (r"\(Spain\)", "Spain"),
    (r"\(Italy\)", "Italy"),
    (r"\(Brazil\)", "Brazil"),
    (r"\(Asia\)", "Asia"),
    (r"\(Korea\)", "Korea"),
    (r"\(Australia\)", "Australia"),
]


def parse_metadata_from_filename(raw_name: str) -> dict[str, str]:
    """
    Extrae metadata del nombre del archivo (convención No-Intro/Redump).
    Retorna dict con keys: region, title (limpio).
    """
    result: dict[str, str] = {}
    for pattern, region in _REGION_PATTERNS:
        if re.search(pattern, raw_name, re.IGNORECASE):
            result["region"] = region
            break
    # Título sin tags (USA), [Europe], etc.
    clean = re.sub(r"\s*\([^)]*\)|\s*\[[^\]]*\]", "", raw_name).strip()
    if clean:
        result["title"] = clean
    return result


def _path(system: str) -> str:
    return os.path.join(_cache_root(), _safe_system(system) + ".json")


def load(system: str) -> dict[str, dict[str, Any]]:
    """Carga el cache de metadata para un sistema. rom_name -> {developer, year, ...}."""
    p = _path(system)
    if not os.path.isfile(p):
        return {}
    try:
        with open(p, "r", encoding="utf-8") as f:
            return json.load(f)
    except (json.JSONDecodeError, OSError):
        return {}


def save(system: str, data: dict[str, dict[str, Any]]) -> None:
    """Guarda el cache de metadata para un sistema."""
    p = _path(system)
    os.makedirs(os.path.dirname(p), exist_ok=True)
    with open(p, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)


def get(system: str, rom_name: str) -> dict[str, Any] | None:
    """Obtiene metadata de un ROM desde cache. None si no existe."""
    cache = load(system)
    return cache.get(rom_name)


def put(system: str, rom_name: str, meta: dict[str, Any]) -> None:
    """Guarda metadata de un ROM en cache (merge con existente)."""
    cache = load(system)
    existing = cache.get(rom_name, {})
    for k, v in meta.items():
        if v is not None and (isinstance(v, str) and v.strip()) or (not isinstance(v, str) and v):
            existing[k] = v
    cache[rom_name] = existing
    save(system, cache)
