"""
Perfil de consola: resolución, nombre, extensiones de ROM y branding.

Cada edición de Bitemu define un ConsoleProfile con su propia paleta
de colores para splash, banner y UI.  El frontend lee estos campos en
vez de usar constantes hardcodeadas, de modo que agregar una nueva
consola solo requiere crear un nuevo profile — sin tocar el código
de renderizado ni el banner.

Campos de branding:
    splash_bg     — fondo del splash screen (color más oscuro)
    splash_fg     — texto/logo del splash (color más claro)
    splash_accent — línea divisoria y elementos secundarios
    splash_sub    — subtítulo de versión
    ansi_color    — código ANSI para colorear el banner de terminal
                    (ej: "32" = verde, "34" = azul, "35" = magenta)
"""
from dataclasses import dataclass, field
from .core import FB_WIDTH, FB_HEIGHT


@dataclass
class ConsoleProfile:
    name: str
    fb_width: int
    fb_height: int
    rom_extensions: list[str]
    window_title: str
    splash_bg: tuple[int, int, int] = (0x0F, 0x38, 0x0F)
    splash_fg: tuple[int, int, int] = (0x9B, 0xBC, 0x0F)
    splash_accent: tuple[int, int, int] = (0x30, 0x62, 0x30)
    splash_sub: tuple[int, int, int] = (0x8B, 0xAC, 0x0F)
    ansi_color: str = "32"


PROFILE_GB: ConsoleProfile = ConsoleProfile(
    name="Game Boy",
    fb_width=FB_WIDTH,
    fb_height=FB_HEIGHT,
    rom_extensions=["gb", "gbc"],
    window_title="Bitemu - Game Boy",
    splash_bg=(0x0F, 0x38, 0x0F),
    splash_fg=(0x9B, 0xBC, 0x0F),
    splash_accent=(0x30, 0x62, 0x30),
    splash_sub=(0x8B, 0xAC, 0x0F),
    ansi_color="32",
)

# --- Future editions (reference only; uncomment when backend is ready) ---
#
# PROFILE_GBA: ConsoleProfile = ConsoleProfile(
#     name="Game Boy Advance",
#     fb_width=240,
#     fb_height=160,
#     rom_extensions=["gba"],
#     window_title="Bitemu+ — Game Boy Advance",
#     splash_bg=(0x1B, 0x05, 0x33),
#     splash_fg=(0x5B, 0x3E, 0x96),
#     splash_accent=(0x3A, 0x20, 0x6A),
#     splash_sub=(0xC8, 0xC8, 0xD4),
#     ansi_color="35",
# )
#
# PROFILE_GENESIS: ConsoleProfile = ConsoleProfile(
#     name="Sega Genesis / Mega Drive",
#     fb_width=320,
#     fb_height=224,
#     rom_extensions=["md", "bin", "gen", "smd"],
#     window_title="Bitemu 2 — Sega Genesis",
#     splash_bg=(0x0C, 0x1A, 0x3C),
#     splash_fg=(0x21, 0x76, 0xFF),
#     splash_accent=(0xD4, 0xA0, 0x17),
#     splash_sub=(0x7E, 0xB8, 0xFF),
#     ansi_color="34",
# )

DEFAULT_PROFILE = PROFILE_GB
