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
