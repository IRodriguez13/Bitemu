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
    c_float,
    c_double,
    c_uint8,
    c_uint16,
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
FB_SIZE = FB_WIDTH * FB_HEIGHT  # Default GB; use get_video_size() after load_rom for Genesis

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
    lib.bitemu_get_video_size.argtypes = [c_void_p, POINTER(c_int), POINTER(c_int)]
    lib.bitemu_get_video_size.restype = None
    lib.bitemu_get_frame_hz.argtypes = [c_void_p]
    lib.bitemu_get_frame_hz.restype = c_double
    lib.bitemu_set_input.argtypes = [c_void_p, c_uint8]
    lib.bitemu_set_input_genesis.argtypes = [c_void_p, c_uint16]
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
    lib.bitemu_audio_set_volume.argtypes = [c_void_p, c_float]
    lib.bitemu_audio_set_volume.restype = None
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

    def get_video_size(self) -> tuple[int, int]:
        """(width, height) — 160×144 GB, 320×224 Genesis."""
        if self._handle is None:
            return FB_WIDTH, FB_HEIGHT
        w, h = c_int(), c_int()
        self._lib.bitemu_get_video_size(self._handle, byref(w), byref(h))
        return w.value or FB_WIDTH, h.value or FB_HEIGHT

    def get_frame_hz(self) -> float:
        """Frecuencia de frame del core (~59.73 GB; 60/50 Genesis según PAL)."""
        if self._handle is None:
            return 59.73
        return float(self._lib.bitemu_get_frame_hz(self._handle))

    def get_framebuffer(self):
        if self._handle is None:
            return None
        ptr = self._lib.bitemu_get_framebuffer(self._handle)
        if not ptr:
            return None
        w, h = self.get_video_size()
        bpp = 3 if w == 320 else 1
        size = w * h * bpp
        addr = cast(ptr, c_void_p).value
        return (c_uint8 * size).from_address(addr)

    def set_input(self, state: int):
        if self._handle is not None:
            self._lib.bitemu_set_input(self._handle, state & 0xFF)

    def set_input_genesis(self, state: int):
        """Genesis 6-button: bits 0-7 = D-pad,A,B,C,Start; 8-11 = X,Y,Z,Mode."""
        if self._handle is not None:
            self._lib.bitemu_set_input_genesis(self._handle, state & 0x0FFF)

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

    def set_audio_volume(self, volume: float):
        """Volumen 0.0–1.0. Se aplica al leer del buffer."""
        if self._handle is not None:
            v = max(0.0, min(1.0, float(volume)))
            self._lib.bitemu_audio_set_volume(self._handle, v)

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
