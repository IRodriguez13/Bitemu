"""
PyInstaller runtime hook: ensure bundled native libraries are discoverable.
Runs before any application code so sounddevice/pygame find their libs.
Covers Linux, macOS and Windows.
"""
import os
import sys
import glob
import ctypes

if getattr(sys, "frozen", False):
    bundle = sys._MEIPASS if hasattr(sys, "_MEIPASS") else os.path.dirname(sys.executable)

    if sys.platform.startswith("linux"):
        os.environ["LD_LIBRARY_PATH"] = bundle + ":" + os.environ.get("LD_LIBRARY_PATH", "")
        for pattern in ("libportaudio*", "libSDL2*"):
            for lib in glob.glob(os.path.join(bundle, pattern)):
                try:
                    ctypes.cdll.LoadLibrary(lib)
                except OSError:
                    continue

    elif sys.platform == "darwin":
        os.environ["DYLD_LIBRARY_PATH"] = bundle + ":" + os.environ.get("DYLD_LIBRARY_PATH", "")
        for pattern in ("libportaudio*", "libSDL2*"):
            for lib in glob.glob(os.path.join(bundle, pattern)):
                try:
                    ctypes.cdll.LoadLibrary(lib)
                except OSError:
                    continue

    elif sys.platform == "win32":
        os.add_dll_directory(bundle)
        os.environ["PATH"] = bundle + ";" + os.environ.get("PATH", "")
        for pattern in ("portaudio*", "SDL2*"):
            for lib in glob.glob(os.path.join(bundle, pattern)):
                try:
                    ctypes.cdll.LoadLibrary(lib)
                except OSError:
                    continue
