"""
PyInstaller runtime hook: ensure bundled native libraries are discoverable.
Runs before any application code so sounddevice/pygame find their libs.
Covers Linux, macOS and Windows.

PyInstaller 6+ onedir: binaries go in _internal/ (sys._MEIPASS).
We search both parent (executable dir) and bundle (_internal).
"""
import os
import sys
import glob
import ctypes

if getattr(sys, "frozen", False):
    bundle = sys._MEIPASS if hasattr(sys, "_MEIPASS") else os.path.dirname(sys.executable)
    parent = os.path.dirname(sys.executable)
    # linuxdeploy puts libs in usr/lib; PyInstaller 6+ uses _internal
    usr_lib = os.path.join(os.path.dirname(parent), "lib")  # sibling of usr/bin
    search_dirs = [usr_lib, parent, bundle]
    search_dirs = [d for d in search_dirs if os.path.isdir(d)]

    if sys.platform.startswith("linux"):
        lib_path = ":".join(search_dirs + [os.environ.get("LD_LIBRARY_PATH", "")])
        os.environ["LD_LIBRARY_PATH"] = lib_path
        for search_dir in search_dirs:
            for pattern in ("libportaudio*", "libSDL2*"):
                for lib in glob.glob(os.path.join(search_dir, pattern)):
                    try:
                        ctypes.cdll.LoadLibrary(lib)
                    except OSError:
                        continue

    elif sys.platform == "darwin":
        dyld_path = ":".join(search_dirs + [os.environ.get("DYLD_LIBRARY_PATH", "")])
        os.environ["DYLD_LIBRARY_PATH"] = dyld_path
        for search_dir in search_dirs:
            for pattern in ("libportaudio*", "libSDL2*"):
                for lib in glob.glob(os.path.join(search_dir, pattern)):
                    try:
                        ctypes.cdll.LoadLibrary(lib)
                    except OSError:
                        continue

    elif sys.platform == "win32":
        for d in search_dirs:
            if os.path.isdir(d):
                os.add_dll_directory(d)
        os.environ["PATH"] = ";".join(search_dirs + [os.environ.get("PATH", "")])
        for search_dir in search_dirs:
            for pattern in ("portaudio*", "SDL2*"):
                for lib in glob.glob(os.path.join(search_dir, pattern)):
                    try:
                        ctypes.cdll.LoadLibrary(lib)
                    except OSError:
                        continue
