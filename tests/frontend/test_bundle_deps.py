"""
Tests that verify native dependencies are loadable at runtime.
Sin PortAudio: audio en core (ALSA en Linux).
"""
import os
import sys
import importlib

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "frontend"))


class TestPygameAvailable:
    def test_pygame_imports(self):
        """pygame must import (bundles its own SDL2)."""
        pg = importlib.import_module("pygame")
        assert hasattr(pg, "joystick")

    def test_pygame_sdl2_version(self):
        """pygame must report a valid SDL version."""
        pg = importlib.import_module("pygame")
        ver = pg.get_sdl_version()
        assert len(ver) == 3
        assert ver[0] >= 2


class TestCoreLibrary:
    def test_libbitemu_loadable(self):
        """libbitemu must be loadable via the frontend wrapper."""
        from bitemu_gui.core import get_lib
        lib = get_lib()
        assert lib is not None

    def test_libbitemu_has_api(self):
        """libbitemu must expose the expected API symbols."""
        from bitemu_gui.core import get_lib
        lib = get_lib()
        assert hasattr(lib, "bitemu_create")
        assert hasattr(lib, "bitemu_destroy")
        assert hasattr(lib, "bitemu_run_frame")
        assert hasattr(lib, "bitemu_get_frame_hz")
        assert hasattr(lib, "bitemu_audio_init")


class TestRuntimeHook:
    def test_hook_file_exists(self):
        """The runtime hook must exist for PyInstaller to use it."""
        hook = os.path.join(
            os.path.dirname(__file__), "..", "..", "frontend", "hooks", "rthook_portaudio.py"
        )
        assert os.path.isfile(hook), f"Runtime hook not found at {hook}"

    def test_hook_is_valid_python(self):
        """The runtime hook must be parseable Python."""
        hook = os.path.join(
            os.path.dirname(__file__), "..", "..", "frontend", "hooks", "rthook_portaudio.py"
        )
        with open(hook) as f:
            compile(f.read(), hook, "exec")
