"""Tests for InputConfig persistence and defaults."""

import sys
import os
import json

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "frontend"))

from unittest.mock import patch, MagicMock
from PySide6.QtCore import Qt
from bitemu_gui.input_config import (
    InputConfig,
    DEFAULT_GAMEPAD_BUTTONS_GB,
    DEFAULT_GAMEPAD_AXES,
    SETTINGS_KEYBOARD_GB_KEY,
    SETTINGS_KEYBOARD_GENESIS_KEY,
    SETTINGS_GAMEPAD_GB_KEY,
    SETTINGS_GAMEPAD_GENESIS_KEY,
    SETTINGS_GAMEPAD_AXIS_KEY,
)
from bitemu_gui.keys import DEFAULT_MAP, GENESIS_6_MAP
from bitemu_gui.profile import PROFILE_GB, PROFILE_GENESIS


def _mock_settings(stored: dict | None = None):
    """Return a MagicMock QSettings that serves stored values."""
    data = stored or {}
    mock = MagicMock()
    mock.value = lambda key, default=None, **kw: data.get(key, default)
    mock.setValue = MagicMock()
    mock.remove = MagicMock()
    return mock


def test_defaults_loaded():
    with patch("bitemu_gui.input_config.QSettings", return_value=_mock_settings()):
        cfg = InputConfig()
    assert cfg.keyboard_map_gb == dict(DEFAULT_MAP)
    assert cfg.keyboard_map_genesis == dict(GENESIS_6_MAP)
    assert cfg.gamepad_buttons_gb == dict(DEFAULT_GAMEPAD_BUTTONS_GB)
    assert cfg.gamepad_axes == dict(DEFAULT_GAMEPAD_AXES)


def test_get_keyboard_map_by_profile():
    with patch("bitemu_gui.input_config.QSettings", return_value=_mock_settings()):
        cfg = InputConfig()
    assert cfg.get_keyboard_map(PROFILE_GB) == cfg.keyboard_map_gb
    assert cfg.get_keyboard_map(PROFILE_GENESIS) == cfg.keyboard_map_genesis


def test_keyboard_gb_persists():
    custom = {int(Qt.Key.Key_P): 4, int(Qt.Key.Key_Q): 5}
    stored = {SETTINGS_KEYBOARD_GB_KEY: json.dumps({str(k): v for k, v in custom.items()})}
    with patch("bitemu_gui.input_config.QSettings", return_value=_mock_settings(stored)):
        cfg = InputConfig()
    assert cfg.keyboard_map_gb == custom


def test_keyboard_genesis_persists():
    custom = {int(Qt.Key.Key_Q): 8, int(Qt.Key.Key_E): 9}
    stored = {SETTINGS_KEYBOARD_GENESIS_KEY: json.dumps({str(k): v for k, v in custom.items()})}
    with patch("bitemu_gui.input_config.QSettings", return_value=_mock_settings(stored)):
        cfg = InputConfig()
    assert cfg.keyboard_map_genesis == custom


def test_gamepad_gb_persists():
    custom_btns = {2: 4, 3: 5}
    custom_axes = {0: [1, 0]}
    stored = {
        SETTINGS_GAMEPAD_GB_KEY: json.dumps({str(k): v for k, v in custom_btns.items()}),
        SETTINGS_GAMEPAD_AXIS_KEY: json.dumps({str(k): v for k, v in custom_axes.items()}),
    }
    with patch("bitemu_gui.input_config.QSettings", return_value=_mock_settings(stored)):
        cfg = InputConfig()
    assert cfg.gamepad_buttons_gb == custom_btns
    assert cfg.gamepad_axes == {0: (1, 0)}


def test_reset_keyboard_gb():
    with patch("bitemu_gui.input_config.QSettings", return_value=_mock_settings()):
        cfg = InputConfig()
    cfg.keyboard_map_gb = {99: 0}
    cfg.reset_keyboard_gb()
    assert cfg.keyboard_map_gb == dict(DEFAULT_MAP)


def test_reset_keyboard_genesis():
    with patch("bitemu_gui.input_config.QSettings", return_value=_mock_settings()):
        cfg = InputConfig()
    cfg.keyboard_map_genesis = {99: 0}
    cfg.reset_keyboard_genesis()
    assert cfg.keyboard_map_genesis == dict(GENESIS_6_MAP)


def test_reset_gamepad_gb():
    with patch("bitemu_gui.input_config.QSettings", return_value=_mock_settings()):
        cfg = InputConfig()
    cfg.gamepad_buttons_gb = {99: 0}
    cfg.reset_gamepad_gb()
    assert cfg.gamepad_buttons_gb == dict(DEFAULT_GAMEPAD_BUTTONS_GB)
    assert cfg.gamepad_axes == dict(DEFAULT_GAMEPAD_AXES)


def test_save_calls_qsettings():
    mock_qs = _mock_settings()
    with patch("bitemu_gui.input_config.QSettings", return_value=mock_qs):
        cfg = InputConfig()
        cfg.save()
    assert mock_qs.setValue.call_count >= 5
    keys_called = [call[0][0] for call in mock_qs.setValue.call_args_list]
    assert SETTINGS_KEYBOARD_GB_KEY in keys_called
    assert SETTINGS_KEYBOARD_GENESIS_KEY in keys_called
    assert SETTINGS_GAMEPAD_GB_KEY in keys_called
    assert SETTINGS_GAMEPAD_GENESIS_KEY in keys_called
    assert SETTINGS_GAMEPAD_AXIS_KEY in keys_called


def test_corrupt_json_falls_back():
    stored = {SETTINGS_KEYBOARD_GB_KEY: "not-json{{{"}
    with patch("bitemu_gui.input_config.QSettings", return_value=_mock_settings(stored)):
        cfg = InputConfig()
    assert cfg.keyboard_map_gb == dict(DEFAULT_MAP)
