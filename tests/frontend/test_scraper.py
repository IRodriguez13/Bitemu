"""Tests for scraper cache logic (no network, no Qt widgets needed)."""
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "frontend"))

from bitemu_gui.scraper import _cache_root


def test_cache_root_returns_string():
    root = _cache_root()
    assert isinstance(root, str)
    assert len(root) > 0


def test_cache_root_contains_bitemu():
    root = _cache_root()
    assert "bitemu" in root.lower() or "Bitemu" in root


def test_cache_root_contains_covers():
    root = _cache_root()
    assert root.endswith("covers")
