"""Tests for update_checker version comparison logic."""
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "frontend"))
import pytest

from bitemu_gui.update_checker import parse_version, is_newer


class TestParseVersion:
    def test_semver(self):
        assert parse_version("v1.2.3") == (1, 2, 3)

    def test_without_v(self):
        assert parse_version("0.2.1-beta") == (0, 2, 1)

    def test_plain_numbers(self):
        assert parse_version("2.0.0") == (2, 0, 0)

    def test_with_suffix(self):
        assert parse_version("v0.3.0-rc1") == (0, 3, 0, 1)

    def test_dev(self):
        assert parse_version("dev") == ()

    def test_empty(self):
        assert parse_version("") == ()


class TestIsNewer:
    def test_newer_patch(self):
        assert is_newer("v0.2.2", "0.2.1-beta") is True

    def test_newer_minor(self):
        assert is_newer("v0.3.0", "0.2.1-beta") is True

    def test_newer_major(self):
        assert is_newer("v1.0.0", "0.2.1-beta") is True

    def test_same_version(self):
        assert is_newer("v0.2.1", "0.2.1-beta") is False

    def test_older_version(self):
        assert is_newer("v0.1.0", "0.2.1-beta") is False

    def test_local_dev(self):
        assert is_newer("v0.1.0", "dev") is True

    def test_both_dev(self):
        assert is_newer("dev", "dev") is False
