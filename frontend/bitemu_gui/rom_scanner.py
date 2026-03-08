"""
ROM Scanner — scans a folder for ROM files and builds a list of entries.
The raw_name (filename without extension) is kept intact for cover lookup
against the libretro-thumbnails CDN which uses the No-Intro naming convention.
"""
import os
import re
from dataclasses import dataclass

_REGION_RE = re.compile(r"\s*\([^)]*\)|\s*\[[^\]]*\]")


@dataclass
class RomEntry:
    path: str
    raw_name: str
    display_name: str


def _clean_display_name(raw: str) -> str:
    """Strip region/version tags for a human-readable name."""
    name = _REGION_RE.sub("", raw).strip()
    return name if name else raw


def scan_folder(folder: str, extensions: list[str]) -> list[RomEntry]:
    """Return sorted list of RomEntry for every matching file in *folder*."""
    if not folder or not os.path.isdir(folder):
        return []
    ext_set = {e.lower().lstrip(".") for e in extensions}
    entries: list[RomEntry] = []
    for fname in os.listdir(folder):
        fpath = os.path.join(folder, fname)
        if not os.path.isfile(fpath):
            continue
        base, ext = os.path.splitext(fname)
        if ext.lower().lstrip(".") not in ext_set:
            continue
        entries.append(RomEntry(
            path=fpath,
            raw_name=base,
            display_name=_clean_display_name(base),
        ))
    entries.sort(key=lambda e: e.display_name.lower())
    return entries
