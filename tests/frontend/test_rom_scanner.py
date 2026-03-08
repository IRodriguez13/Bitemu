"""Tests for ROM scanner module."""
import os
import sys
import tempfile

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "frontend"))

from bitemu_gui.rom_scanner import scan_folder, RomEntry, _clean_display_name


def test_clean_display_name_removes_region():
    assert _clean_display_name("Tetris (World)") == "Tetris"


def test_clean_display_name_removes_multiple_tags():
    raw = "Pokemon - Yellow Version (USA, Europe) (GBC,SGB Enhanced)"
    result = _clean_display_name(raw)
    assert result == "Pokemon - Yellow Version"


def test_clean_display_name_removes_square_brackets():
    assert _clean_display_name("Zelda (USA) [!]") == "Zelda"


def test_clean_display_name_preserves_plain_name():
    assert _clean_display_name("Super Mario Land") == "Super Mario Land"


def test_clean_display_name_empty_after_strip_returns_raw():
    assert _clean_display_name("(USA)") == "(USA)"


def test_rom_entry_dataclass():
    e = RomEntry(path="/roms/test.gb", raw_name="test", display_name="Test")
    assert e.path == "/roms/test.gb"
    assert e.raw_name == "test"
    assert e.display_name == "Test"


def test_scan_folder_finds_gb_files():
    with tempfile.TemporaryDirectory() as d:
        for name in ["game1.gb", "game2.gbc", "readme.txt", "image.png"]:
            open(os.path.join(d, name), "w").close()
        entries = scan_folder(d, ["gb", "gbc"])
        names = {e.raw_name for e in entries}
        assert names == {"game1", "game2"}


def test_scan_folder_sorted_by_display_name():
    with tempfile.TemporaryDirectory() as d:
        for name in ["Zelda.gb", "Mario.gb", "Asteroids.gb"]:
            open(os.path.join(d, name), "w").close()
        entries = scan_folder(d, ["gb"])
        display_names = [e.display_name for e in entries]
        assert display_names == sorted(display_names, key=str.lower)


def test_scan_folder_empty_dir():
    with tempfile.TemporaryDirectory() as d:
        entries = scan_folder(d, ["gb"])
        assert entries == []


def test_scan_folder_nonexistent_path():
    entries = scan_folder("/nonexistent/path/foo/bar", ["gb"])
    assert entries == []


def test_scan_folder_none_path():
    entries = scan_folder(None, ["gb"])
    assert entries == []


def test_scan_folder_ignores_directories():
    with tempfile.TemporaryDirectory() as d:
        os.makedirs(os.path.join(d, "subdir.gb"))
        open(os.path.join(d, "real.gb"), "w").close()
        entries = scan_folder(d, ["gb"])
        assert len(entries) == 1
        assert entries[0].raw_name == "real"


def test_scan_folder_case_insensitive_extensions():
    with tempfile.TemporaryDirectory() as d:
        for name in ["game.GB", "game2.Gb", "game3.gb"]:
            open(os.path.join(d, name), "w").close()
        entries = scan_folder(d, ["gb"])
        assert len(entries) == 3
