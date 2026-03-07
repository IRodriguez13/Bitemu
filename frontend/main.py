#!/usr/bin/env python3
"""
Punto de entrada del frontend Bitemu (Python + PySide6).
Requisito: haber compilado la librería con 'make lib' en la raíz del proyecto.
"""
from PySide6.QtWidgets import QApplication
from bitemu_gui.main_window import MainWindow
from bitemu_gui.profile import DEFAULT_PROFILE
import sys
from pathlib import Path

# Asegurar que el frontend está en el path
frontend_dir = Path(__file__).resolve().parent
if str(frontend_dir) not in sys.path:
    sys.path.insert(0, str(frontend_dir))




BANNER = r"""
  ____  _ _                       
 | __ )(_) |_ ___ _ __ ___  _   _ 
 |  _ \| | __/ _ \ '_ ` _ \| | | |
 | |_) | | ||  __/ | | | | | |_| |
 |____/|_|\__\___|_| |_| |_|\__,_|
  Game Boy Emulator
"""


def main():
    print(BANNER, file=sys.stderr)
    app = QApplication(sys.argv)
    app.setApplicationName("Bitemu 1")
    win = MainWindow(profile=DEFAULT_PROFILE)
    win.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
