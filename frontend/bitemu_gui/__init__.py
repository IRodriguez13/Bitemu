# Frontend Bitemu GUI (desacoplado del core)
import sys
from pathlib import Path as _Path

_VERSION_FILE = _Path(__file__).resolve().parent.parent.parent / "VERSION"


def get_version() -> str:
    candidates = [_VERSION_FILE]
    bundle = getattr(sys, "_MEIPASS", None)
    if bundle:
        candidates.insert(0, _Path(bundle) / "VERSION")
    for p in candidates:
        try:
            return p.read_text().strip()
        except FileNotFoundError:
            continue
    return "dev"
