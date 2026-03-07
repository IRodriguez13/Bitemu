# Frontend Bitemu GUI (desacoplado del core)
from pathlib import Path as _Path

_VERSION_FILE = _Path(__file__).resolve().parent.parent.parent / "VERSION"


def get_version() -> str:
    try:
        return _VERSION_FILE.read_text().strip()
    except FileNotFoundError:
        return "dev"
