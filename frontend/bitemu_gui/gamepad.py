"""
Gamepad polling via pygame.joystick (optional dependency).
Emits joypad state byte compatible with bitemu_set_input().

pygame.display.init() initialises SDL's video subsystem (needed by the
event pump that updates joystick state) but does NOT create a window.
A visible window only appears on pygame.display.set_mode(), which we
never call.  This coexists safely with PySide6 on Linux, Windows and
macOS because both Qt and SDL open independent connections to the
platform's display server.
"""
import os

try:
    os.environ.setdefault("PYGAME_HIDE_SUPPORT_PROMPT", "1")
    import pygame
    import pygame.joystick
    import pygame.event
    import pygame.display
    _PYGAME_AVAILABLE = True
except ImportError:
    _PYGAME_AVAILABLE = False

from PySide6.QtCore import QObject, QTimer, Signal

from .input_config import InputConfig, AXIS_DEADZONE


class GamepadPoller(QObject):
    """Polls the first connected gamepad at ~60 Hz and emits the joypad byte."""

    state_changed = Signal(int)
    gamepad_connected = Signal(str)
    gamepad_disconnected = Signal()
    button_pressed = Signal(int)

    def __init__(self, config: InputConfig, parent=None):
        super().__init__(parent)
        self._config = config
        self._joystick = None
        self._state: int = 0
        self._prev_buttons: set[int] = set()
        self._initialized = False

        if not _PYGAME_AVAILABLE:
            return

        try:
            pygame.display.init()
            pygame.joystick.init()
        except Exception:
            return
        pygame.event.set_blocked(None)
        pygame.event.set_allowed([
            pygame.JOYDEVICEADDED,
            pygame.JOYDEVICEREMOVED,
            pygame.JOYBUTTONDOWN,
            pygame.JOYBUTTONUP,
            pygame.JOYAXISMOTION,
            pygame.JOYHATMOTION,
        ])
        self._initialized = True
        self._try_connect()

        self._timer = QTimer(self)
        self._timer.timeout.connect(self._poll)
        self._timer.start(16)

    @property
    def available(self) -> bool:
        return _PYGAME_AVAILABLE

    @property
    def connected(self) -> bool:
        return self._joystick is not None

    @property
    def joystick_name(self) -> str:
        if self._joystick:
            try:
                return self._joystick.get_name()
            except Exception:
                pass
        return ""

    def refresh(self):
        if not self._initialized:
            return
        self._try_connect()

    def _try_connect(self):
        if not self._initialized:
            return
        count = pygame.joystick.get_count()
        was_connected = self._joystick is not None
        if count > 0 and self._joystick is None:
            try:
                self._joystick = pygame.joystick.Joystick(0)
                self._joystick.init()
                self.gamepad_connected.emit(self._joystick.get_name())
            except Exception:
                self._joystick = None
        elif count == 0 and was_connected:
            self._joystick = None
            self.gamepad_disconnected.emit()

    def _poll(self):
        if not self._initialized:
            return

        pygame.event.pump()

        prev_connected = self._joystick is not None
        cur_count = pygame.joystick.get_count()
        if cur_count > 0 and not prev_connected:
            self._try_connect()
        elif cur_count == 0 and prev_connected:
            self._try_connect()

        if not self._joystick:
            self._prev_buttons.clear()
            if self._state != 0:
                self._state = 0
                self.state_changed.emit(0)
            return

        cur_buttons: set[int] = set()
        try:
            num_buttons = self._joystick.get_numbuttons()
            for i in range(num_buttons):
                if self._joystick.get_button(i):
                    cur_buttons.add(i)
        except Exception:
            pass

        newly_pressed = cur_buttons - self._prev_buttons
        for btn_idx in sorted(newly_pressed):
            self.button_pressed.emit(btn_idx)
        self._prev_buttons = cur_buttons

        state = 0
        btn_map = self._config.gamepad_buttons
        for btn_idx, bit in btn_map.items():
            if btn_idx in cur_buttons:
                state |= 1 << bit

        axis_map = self._config.gamepad_axes
        for axis_idx, (neg_bit, pos_bit) in axis_map.items():
            try:
                val = self._joystick.get_axis(axis_idx)
                if val < -AXIS_DEADZONE:
                    state |= 1 << neg_bit
                elif val > AXIS_DEADZONE:
                    state |= 1 << pos_bit
            except Exception:
                pass

        try:
            if self._joystick.get_numhats() > 0:
                hx, hy = self._joystick.get_hat(0)
                if hx < 0:
                    state |= 1 << 1  # Left
                elif hx > 0:
                    state |= 1 << 0  # Right
                if hy > 0:
                    state |= 1 << 2  # Up
                elif hy < 0:
                    state |= 1 << 3  # Down
        except Exception:
            pass

        state &= 0xFF
        if state != self._state:
            self._state = state
            self.state_changed.emit(state)

    def shutdown(self):
        if self._initialized:
            self._initialized = False
            if hasattr(self, "_timer"):
                self._timer.stop()
            try:
                pygame.joystick.quit()
                pygame.display.quit()
            except Exception:
                pass
