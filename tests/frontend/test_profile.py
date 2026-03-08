"""Tests for console profile definitions."""

import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "frontend"))

from bitemu_gui.profile import PROFILE_GB, DEFAULT_PROFILE, ConsoleProfile


def test_gb_profile_dimensions():
    assert PROFILE_GB.fb_width == 160
    assert PROFILE_GB.fb_height == 144


def test_gb_profile_name():
    assert PROFILE_GB.name == "Game Boy"


def test_gb_profile_extensions():
    assert "gb" in PROFILE_GB.rom_extensions
    assert "gbc" in PROFILE_GB.rom_extensions


def test_gb_profile_window_title():
    assert "Bitemu" in PROFILE_GB.window_title


def test_default_is_gb():
    assert DEFAULT_PROFILE is PROFILE_GB


def test_console_profile_dataclass():
    p = ConsoleProfile(
        name="Test",
        fb_width=320,
        fb_height=240,
        rom_extensions=["bin"],
        window_title="Test Emu",
    )
    assert p.fb_width == 320
    assert p.rom_extensions == ["bin"]


def test_gb_profile_branding_colors():
    assert len(PROFILE_GB.splash_bg) == 3
    assert len(PROFILE_GB.splash_fg) == 3
    assert len(PROFILE_GB.splash_accent) == 3
    assert len(PROFILE_GB.splash_sub) == 3
    assert all(0 <= c <= 255 for c in PROFILE_GB.splash_bg)
    assert all(0 <= c <= 255 for c in PROFILE_GB.splash_fg)


def test_gb_profile_ansi_color():
    assert PROFILE_GB.ansi_color == "32"


def test_profile_defaults_match_gb():
    p = ConsoleProfile(
        name="Defaults",
        fb_width=160,
        fb_height=144,
        rom_extensions=["gb"],
        window_title="Test",
    )
    assert p.splash_bg == PROFILE_GB.splash_bg
    assert p.splash_fg == PROFILE_GB.splash_fg
    assert p.ansi_color == "32"


def test_custom_branding():
    p = ConsoleProfile(
        name="Genesis",
        fb_width=320,
        fb_height=224,
        rom_extensions=["md"],
        window_title="Bitemu 2",
        thumbnail_system="Sega - Mega Drive - Genesis",
        splash_bg=(0x0C, 0x1A, 0x3C),
        splash_fg=(0x21, 0x76, 0xFF),
        splash_accent=(0xD4, 0xA0, 0x17),
        splash_sub=(0x7E, 0xB8, 0xFF),
        ansi_color="34",
    )
    assert p.splash_fg == (0x21, 0x76, 0xFF)
    assert p.ansi_color == "34"
    assert p.thumbnail_system == "Sega - Mega Drive - Genesis"


def test_gb_profile_thumbnail_system():
    assert PROFILE_GB.thumbnail_system == "Nintendo - Game Boy"


def test_thumbnail_system_default():
    p = ConsoleProfile(
        name="Test",
        fb_width=160,
        fb_height=144,
        rom_extensions=["gb"],
        window_title="Test",
    )
    assert p.thumbnail_system == "Nintendo - Game Boy"
