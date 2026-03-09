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
    c_int,
    c_short,
    c_char,
    c_char_p,
    POINTER,
    cast,
    byref,
)

# Ruta a la librería: env override > PyInstaller bundle > repo root
_ROOT = os.path.realpath(os.path.join(os.path.dirname(__file__), "..", ".."))
_BUNDLE_DIR = getattr(sys, "_MEIPASS", None) or os.path.dirname(sys.executable)

_LIB_NAME = "libbitemu.so"

if sys.platform == "win32":
    _LIB_NAME = "bitemu.dll"
elif sys.platform == "darwin":
    _LIB_NAME = "libbitemu.dylib"

def _find_lib():
    env = os.environ.get("BITEMU_LIB_PATH")
    if env:
        return env
    for base in (_BUNDLE_DIR, _ROOT):
        p = os.path.join(base, _LIB_NAME)
        if os.path.isfile(p):
            return p
    return os.path.join(_ROOT, _LIB_NAME)

_LIB_PATH = _find_lib()

FB_WIDTH = 160
FB_HEIGHT = 144
FB_SIZE = FB_WIDTH * FB_HEIGHT

# Audio backends
AUDIO_BACKEND_NULL = 2    # Buffer only (legacy)
AUDIO_BACKEND_NATIVE = 4  # ALSA/WASAPI/CoreAudio según plataforma


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
    lib.bitemu_unload_rom.argtypes = [c_void_p]
    # Save state
    lib.bitemu_save_state.argtypes = [c_void_p, c_char_p]
    lib.bitemu_save_state.restype = c_int
    lib.bitemu_load_state.argtypes = [c_void_p, c_char_p]
    lib.bitemu_load_state.restype = c_int    # Audio (buffer circular)
    lib.bitemu_audio_init.argtypes = [c_void_p, c_int, c_void_p]
    lib.bitemu_audio_init.restype = c_int
    lib.bitemu_audio_set_enabled.argtypes = [c_void_p, c_bool]
    lib.bitemu_audio_play_chirp.argtypes = [c_void_p]
    lib.bitemu_audio_play_chirp.restype = None
    lib.bitemu_audio_play_ding.argtypes = [c_void_p]
    lib.bitemu_audio_play_ding.restype = None
    lib.bitemu_audio_shutdown.argtypes = [c_void_p]
    lib.bitemu_read_audio.argtypes = [c_void_p, POINTER(c_short), c_int]
    lib.bitemu_read_audio.restype = c_int
    lib.bitemu_get_audio_spec.argtypes = [POINTER(c_int), POINTER(c_int)]
    lib.bitemu_get_audio_spec.restype = None
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
        return (c_uint8 * FB_SIZE).from_address(addr)

    def set_input(self, state: int):
        if self._handle is not None:
            self._lib.bitemu_set_input(self._handle, state & 0xFF)

    def reset(self):
        if self._handle is not None:
            self._lib.bitemu_reset(self._handle)

    def unload_rom(self):
        if self._handle is not None:
            self._lib.bitemu_unload_rom(self._handle)

    def save_state(self, path: str) -> bool:
        if self._handle is None:
            return False
        return self._lib.bitemu_save_state(self._handle, path.encode("utf-8")) == 0

    def load_state(self, path: str) -> bool:
        if self._handle is None:
            return False
        return self._lib.bitemu_load_state(self._handle, path.encode("utf-8")) == 0

    def init_audio(self) -> bool:
        """Inicializa audio nativo (ALSA/CoreAudio/WASAPI según plataforma). Sin PortAudio."""
        if self._handle is None:
            return False
        return self._lib.bitemu_audio_init(self._handle, AUDIO_BACKEND_NATIVE, None) == 0

    def set_audio_enabled(self, enabled: bool):
        if self._handle is not None:
            self._lib.bitemu_audio_set_enabled(self._handle, enabled)

    def play_chirp(self):
        """Bip al cargar ROM (reproducido por el core)."""
        if self._handle is not None:
            self._lib.bitemu_audio_play_chirp(self._handle)

    def play_ding(self):
        """Doble bip al mostrar splash (reproducido por el core)."""
        if self._handle is not None:
            self._lib.bitemu_audio_play_ding(self._handle)

    def read_audio(self, max_samples: int = 1024) -> bytes:
        """Lee hasta max_samples muestras S16 del buffer. Vacío si no hay audio inicializado."""
        if self._handle is None or max_samples <= 0:
            return b""
        buf = (c_short * max_samples)()
        n = self._lib.bitemu_read_audio(self._handle, buf, max_samples)
        if n <= 0:
            return b""
        # int16 → bytes crudos (little-endian, 2 bytes por muestra); bytes() no acepta int16
        view = (c_char * (n * 2)).from_buffer(buf)
        return bytes(view)

    def get_audio_spec(self) -> tuple[int, int]:
        """(sample_rate, channels)"""
        rate = c_int()
        chans = c_int()
        self._lib.bitemu_get_audio_spec(byref(rate), byref(chans))
        return rate.value, chans.value

    @property
    def is_valid(self):
        return self._handle is not None
