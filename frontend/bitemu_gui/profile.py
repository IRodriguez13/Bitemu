"""
Perfil de consola: resolución, nombre, extensiones de ROM.
Desacopla el frontend de una consola concreta; en beta solo Game Boy.
"""
from dataclasses import dataclass
from .core import FB_WIDTH, FB_HEIGHT


@dataclass
class ConsoleProfile:
    name: str
    fb_width: int
    fb_height: int
    rom_extensions: list[str]
    window_title: str


PROFILE_GB: ConsoleProfile = ConsoleProfile(
    name="Game Boy",
    fb_width=FB_WIDTH,
    fb_height=FB_HEIGHT,
    rom_extensions=["gb", "gbc"],
    window_title="Bitemu - Game Boy",
)

# Perfil por defecto para esta edición (BE1)
DEFAULT_PROFILE = PROFILE_GB
