"""Tests for ctypes bindings to libbitemu."""

import sys
import os
import tempfile

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "frontend"))

from bitemu_gui.core import Emu


def test_create_and_destroy():
    emu = Emu()
    assert emu.create()
    assert emu.is_valid
    emu.destroy()
    assert not emu.is_valid


def test_double_destroy_safe():
    emu = Emu()
    emu.create()
    emu.destroy()
    emu.destroy()


def test_load_invalid_rom():
    emu = Emu()
    emu.create()
    assert not emu.load_rom("/nonexistent/path.gb")
    emu.destroy()


def test_audio_init():
    emu = Emu()
    emu.create()
    assert emu.init_audio()
    emu.destroy()


def test_audio_spec():
    emu = Emu()
    emu.create()
    rate, chans = emu.get_audio_spec()
    assert rate == 44100
    assert chans == 1
    emu.destroy()


def test_read_audio_empty():
    emu = Emu()
    emu.create()
    data = emu.read_audio(64)
    assert data == b""
    emu.destroy()


def test_unload_no_rom():
    emu = Emu()
    emu.create()
    emu.unload_rom()
    emu.destroy()


def test_reset_no_crash():
    emu = Emu()
    emu.create()
    emu.reset()
    emu.destroy()


def test_framebuffer_not_none():
    emu = Emu()
    emu.create()
    fb = emu.get_framebuffer()
    assert fb is not None
    assert len(fb) == 160 * 144
    emu.destroy()


def test_set_input():
    emu = Emu()
    emu.create()
    emu.set_input(0xFF)
    emu.set_input(0x00)
    emu.destroy()


def test_save_state_no_rom():
    emu = Emu()
    emu.create()
    assert not emu.save_state(os.path.join(tempfile.gettempdir(), "bitemu_py_test.bst"))
    emu.destroy()


def test_load_state_no_rom():
    emu = Emu()
    emu.create()
    assert not emu.load_state(os.path.join(tempfile.gettempdir(), "bitemu_py_test.bst"))
    emu.destroy()


def test_save_load_roundtrip():
    rom_path = os.path.join(tempfile.gettempdir(), "bitemu_py_test_rom.gb")
    state_path = os.path.join(tempfile.gettempdir(), "bitemu_py_test.bst")
    rom_data = bytearray(512)
    rom_data[0x147] = 0x00
    with open(rom_path, "wb") as f:
        f.write(rom_data)

    emu = Emu()
    emu.create()
    assert emu.load_rom(rom_path)
    assert emu.save_state(state_path)
    assert emu.load_state(state_path)
    emu.destroy()

    os.remove(rom_path)
    if os.path.exists(state_path):
        os.remove(state_path)
