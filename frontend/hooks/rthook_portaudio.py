"""
PyInstaller runtime hook: ensure bundled native libs (pygame/SDL2) are discoverable.
Sin PortAudio: audio en core (ALSA en Linux).
"""
import os
import sys
import glob
import ctypes

if getattr(sys, "frozen", False):
    bundle = sys._MEIPASS if hasattr(sys, "_MEIPASS") else os.path.dirname(sys.executable)
    parent = os.path.dirname(sys.executable)
    search_dirs = [parent, bundle] if parent != bundle else [bundle]

    if sys.platform.startswith("linux"):
        lib_path = ":".join(search_dirs + [os.environ.get("LD_LIBRARY_PATH", "")])
        os.environ["LD_LIBRARY_PATH"] = lib_path
        for search_dir in search_dirs:
            for lib in glob.glob(os.path.join(search_dir, "libSDL2*")):
                try:
                    ctypes.cdll.LoadLibrary(lib)
                except OSError:
                    pass

    elif sys.platform == "darwin":
        dyld_path = ":".join(search_dirs + [os.environ.get("DYLD_LIBRARY_PATH", "")])
        os.environ["DYLD_LIBRARY_PATH"] = dyld_path
        for search_dir in search_dirs:
            for lib in glob.glob(os.path.join(search_dir, "libSDL2*")):
                try:
                    ctypes.cdll.LoadLibrary(lib)
                except OSError:
                    pass

    elif sys.platform == "win32":
        for d in search_dirs:
            if os.path.isdir(d):
                os.add_dll_directory(d)
        os.environ["PATH"] = ";".join(search_dirs + [os.environ.get("PATH", "")])
