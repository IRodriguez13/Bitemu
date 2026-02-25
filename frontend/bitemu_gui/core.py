"""
Wrapper ctypes sobre libbitemu.
El frontend solo depende de esta API; no conoce el core ni la consola.
"""
import os
import sys
from ctypes import (
    CDLL,
    c_void_p,
    c_bool,
    c_uint8,
    c_char_p,
    POINTER,
    cast,
)

# Ruta a la librería: repo root (padre de frontend/) o BITEMU_LIB
_ROOT = os.path.realpath(os.path.join(os.path.dirname(__file__), "..", ".."))
_LIB_NAME = "libbitemu.so"
if sys.platform == "win32":
    _LIB_NAME = "bitemu.dll"
elif sys.platform == "darwin":
    _LIB_NAME = "libbitemu.dylib"
_LIB_PATH = os.environ.get("BITEMU_LIB_PATH") or os.path.join(_ROOT, _LIB_NAME)

FB_WIDTH = 160
FB_HEIGHT = 144
FB_SIZE = FB_WIDTH * FB_HEIGHT


def _load_lib():
    if not os.path.isfile(_LIB_PATH):
        raise FileNotFoundError(
            f"No se encontró {_LIB_NAME}. Ejecuta 'make lib' en la raíz del proyecto.\n"
            f"Buscado en: {_LIB_PATH}"
        )
    lib = CDLL(_LIB_PATH)
    # API
    lib.bitemu_create.restype = c_void_p
    lib.bitemu_create.argtypes = []
    lib.bitemu_destroy.argtypes = [c_void_p]
    lib.bitemu_load_rom.argtypes = [c_void_p, c_char_p]
    lib.bitemu_load_rom.restype = c_bool
    lib.bitemu_run_frame.argtypes = [c_void_p]
    lib.bitemu_run_frame.restype = c_bool
    lib.bitemu_get_framebuffer.argtypes = [c_void_p]
    lib.bitemu_get_framebuffer.restype = POINTER(c_uint8)
    lib.bitemu_set_input.argtypes = [c_void_p, c_uint8]
    lib.bitemu_reset.argtypes = [c_void_p]
    return lib


_lib = None


def get_lib():
    global _lib
    if _lib is None:
        _lib = _load_lib()
    return _lib


class Emu:
    """Manejador del emulador; encapsula el puntero y la API."""

    def __init__(self):
        self._handle = None
        self._lib = get_lib()

    def create(self):
        self._handle = self._lib.bitemu_create()
        return self._handle is not None

    def destroy(self):
        if self._handle is not None:
            self._lib.bitemu_destroy(self._handle)
            self._handle = None

    def load_rom(self, path: str) -> bool:
        if self._handle is None:
            return False
        return self._lib.bitemu_load_rom(self._handle, path.encode("utf-8"))

    def run_frame(self) -> bool:
        if self._handle is None:
            return False
        return self._lib.bitemu_run_frame(self._handle)

    def get_framebuffer(self):
        if self._handle is None:
            return None
        ptr = self._lib.bitemu_get_framebuffer(self._handle)
        if not ptr:
            return None
        addr = cast(ptr, c_void_p).value
        buf = (c_uint8 * FB_SIZE).from_address(addr)
        return bytes(buf)

    def set_input(self, state: int):
        if self._handle is not None:
            self._lib.bitemu_set_input(self._handle, state & 0xFF)

    def reset(self):
        if self._handle is not None:
            self._lib.bitemu_reset(self._handle)

    @property
    def is_valid(self):
        return self._handle is not None
