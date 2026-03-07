"""Tests for joypad key mapping logic."""

import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "frontend"))

from PySide6.QtCore import Qt
from bitemu_gui.keys import key_to_joypad, build_joypad_state, DEFAULT_MAP


def test_wasd_mapped():
    assert key_to_joypad(Qt.Key.Key_W) == 2   # Up
    assert key_to_joypad(Qt.Key.Key_A) == 1   # Left
    assert key_to_joypad(Qt.Key.Key_S) == 3   # Down
    assert key_to_joypad(Qt.Key.Key_D) == 0   # Right


def test_arrows_mapped():
    assert key_to_joypad(Qt.Key.Key_Up) == 2
    assert key_to_joypad(Qt.Key.Key_Down) == 3
    assert key_to_joypad(Qt.Key.Key_Left) == 1
    assert key_to_joypad(Qt.Key.Key_Right) == 0


def test_buttons_mapped():
    assert key_to_joypad(Qt.Key.Key_J) == 4   # A
    assert key_to_joypad(Qt.Key.Key_Z) == 4   # A alt
    assert key_to_joypad(Qt.Key.Key_K) == 5   # B
    assert key_to_joypad(Qt.Key.Key_X) == 5   # B alt
    assert key_to_joypad(Qt.Key.Key_U) == 6   # Select
    assert key_to_joypad(Qt.Key.Key_I) == 7   # Start
    assert key_to_joypad(Qt.Key.Key_Return) == 7


def test_unmapped_returns_none():
    assert key_to_joypad(Qt.Key.Key_F12) is None
    assert key_to_joypad(Qt.Key.Key_Escape) is None


def test_build_empty_set():
    assert build_joypad_state(set()) == 0


def test_build_single_key():
    state = build_joypad_state({Qt.Key.Key_W})
    assert state & (1 << 2)  # Up bit set


def test_build_multiple_keys():
    pressed = {Qt.Key.Key_W, Qt.Key.Key_J}
    state = build_joypad_state(pressed)
    assert state & (1 << 2)  # Up
    assert state & (1 << 4)  # A


def test_build_fits_in_byte():
    all_keys = set(DEFAULT_MAP.keys())
    state = build_joypad_state(all_keys)
    assert 0 <= state <= 0xFF


def test_custom_keymap():
    custom = {Qt.Key.Key_P: 4}
    assert key_to_joypad(Qt.Key.Key_P, custom) == 4
    assert key_to_joypad(Qt.Key.Key_W, custom) is None

    state = build_joypad_state({Qt.Key.Key_P}, custom)
    assert state == (1 << 4)
