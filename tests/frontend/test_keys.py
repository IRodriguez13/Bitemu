"""Tests for joypad key mapping logic."""

import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "frontend"))

from PySide6.QtCore import Qt
from bitemu_gui.keys import (
    key_to_joypad,
    build_joypad_state,
    build_joypad_state_genesis,
    DEFAULT_MAP,
    GENESIS_6_MAP,
    GENESIS_6_ACTION_NAMES,
    key_name,
    ACTION_NAMES,
)


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


def test_action_names_count():
    assert len(ACTION_NAMES) == 8


def test_key_name_readable():
    name = key_name(Qt.Key.Key_W)
    assert isinstance(name, str)
    assert len(name) > 0


# --- Genesis 6-button ---


def test_genesis_dpad_mapping():
    """Genesis raw: 0=R,1=L,2=D,3=U. W=Up(3), S=Down(2)."""
    state = build_joypad_state_genesis({Qt.Key.Key_W}, GENESIS_6_MAP)
    assert state & (1 << 3)  # Up
    state = build_joypad_state_genesis({Qt.Key.Key_S}, GENESIS_6_MAP)
    assert state & (1 << 2)  # Down
    state = build_joypad_state_genesis({Qt.Key.Key_D}, GENESIS_6_MAP)
    assert state & (1 << 0)  # Right
    state = build_joypad_state_genesis({Qt.Key.Key_A}, GENESIS_6_MAP)
    assert state & (1 << 1)  # Left


def test_genesis_xyz_buttons():
    """Q=X(8), E=Y(9), R=Z(10), Tab=Mode(11)."""
    state = build_joypad_state_genesis({Qt.Key.Key_Q}, GENESIS_6_MAP)
    assert state & (1 << 8)
    state = build_joypad_state_genesis({Qt.Key.Key_E}, GENESIS_6_MAP)
    assert state & (1 << 9)
    state = build_joypad_state_genesis({Qt.Key.Key_R}, GENESIS_6_MAP)
    assert state & (1 << 10)
    state = build_joypad_state_genesis({Qt.Key.Key_Tab}, GENESIS_6_MAP)
    assert state & (1 << 11)


def test_genesis_12bit_mask():
    state = build_joypad_state_genesis(set(GENESIS_6_MAP.keys()), GENESIS_6_MAP)
    assert 0 <= state <= 0x0FFF


def test_genesis_action_names():
    assert len(GENESIS_6_ACTION_NAMES) == 12
