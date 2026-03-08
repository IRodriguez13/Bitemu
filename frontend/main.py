#!/usr/bin/env python3
"""
Punto de entrada del frontend Bitemu (Python + PySide6).
Requisito: haber compilado la librería con 'make lib' en la raíz del proyecto.
"""
from PySide6.QtWidgets import QApplication
from bitemu_gui.main_window import MainWindow
from bitemu_gui.profile import DEFAULT_PROFILE
from bitemu_gui import get_version
import sys
from pathlib import Path

frontend_dir = Path(__file__).resolve().parent
if str(frontend_dir) not in sys.path:
    sys.path.insert(0, str(frontend_dir))


_BANNER_ART = r"""
  ____  _ _                       
 | __ )(_) |_ ___ _ __ ___  _   _ 
 |  _ \| | __/ _ \ '_ ` _ \| | | |
 | |_) | | ||  __/ | | | | | |_| |
 |____/|_|\__\___|_| |_| |_|\__,_|
"""


def _colored_banner(profile, version: str) -> str:
    c = profile.ansi_color
    reset = "\033[0m"
    bold = "\033[1m"
    art = f"{bold}\033[{c}m{_BANNER_ART.rstrip()}{reset}"
    subtitle = f"  \033[{c}m{profile.name} Emulator{reset}  v{version}"
    return f"{art}\n{subtitle}\n"


def main():
    version = get_version()
    print(_colored_banner(DEFAULT_PROFILE, version), file=sys.stderr)
    app = QApplication(sys.argv)
    app.setApplicationName("Bitemu 1")
    win = MainWindow(profile=DEFAULT_PROFILE)
    win.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
