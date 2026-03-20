"""
Componentes de layout reutilizables — estilo Genesis Plus GX.
AppHeader, AppFooter, ListWithPreviewPanel para un frontend genérico.
"""
from typing import Callable, TypeVar, Generic

from PySide6.QtCore import Qt, Signal
from PySide6.QtGui import QPixmap
from PySide6.QtWidgets import (
    QWidget,
    QVBoxLayout,
    QHBoxLayout,
    QLabel,
    QFrame,
    QScrollArea,
    QListWidget,
    QListWidgetItem,
    QSizePolicy,
    QSplitter,
)

from .profile import ConsoleProfile
from .keys import key_name
from .styles import header_style, footer_style, list_style, key_hint_style

T = TypeVar("T")


def _key_for_action(joypad_bit: int, keyboard_map: dict) -> str | None:
    """Devuelve la tecla mapeada a un bit del joypad (ej. 4=A, 5=B)."""
    for qt_key, bit in keyboard_map.items():
        if bit == joypad_bit:
            return key_name(qt_key)
    return None


class AppHeader(QFrame):
    """Barra superior: título + logo opcional. Estilo oscuro con bordes redondeados."""

    def __init__(
        self,
        title: str,
        profile: ConsoleProfile | None = None,
        logo_path: str | None = None,
        parent=None,
    ):
        super().__init__(parent)
        self._profile = profile
        self._title = title
        layout = QHBoxLayout(self)
        layout.setContentsMargins(16, 10, 16, 10)
        layout.setSpacing(12)

        self._title_label = QLabel(title)
        self._title_label.setStyleSheet(
            "font-size: 18px; font-family: monospace; font-weight: bold;"
        )
        layout.addWidget(self._title_label)

        layout.addStretch()

        if logo_path:
            pix = QPixmap(logo_path)
            if not pix.isNull():
                scaled = pix.scaled(
                    120, 40,
                    Qt.AspectRatioMode.KeepAspectRatio,
                    Qt.TransformationMode.SmoothTransformation,
                )
                logo_lbl = QLabel()
                logo_lbl.setPixmap(scaled)
                layout.addWidget(logo_lbl)

        self._apply_style()

    def _apply_style(self):
        profile = self._profile or ConsoleProfile(
            name="", fb_width=160, fb_height=144, rom_extensions=[],
            window_title="", splash_bg=(40, 40, 50), splash_fg=(200, 200, 210),
            splash_accent=(100, 100, 120),
        )
        self.setStyleSheet(header_style(profile))
        fg = profile.splash_fg
        self._title_label.setStyleSheet(
            f"color: rgb({fg[0]},{fg[1]},{fg[2]});"
            " font-size: 18px; font-family: 'JetBrains Mono', 'Fira Code', monospace;"
            " font-weight: bold; background: transparent;"
        )

    def set_profile(self, profile: ConsoleProfile):
        self._profile = profile
        self._apply_style()

    def set_title(self, title: str):
        self._title = title
        self._title_label.setText(title)


class AppFooter(QFrame):
    """Barra inferior con prompts de acción: [Tecla] Acción. Opcional InputConfig."""

    def __init__(
        self,
        actions: list[tuple[str, str]] | None = None,
        input_config=None,
        joypad_labels: dict[int, str] | None = None,
        profile: ConsoleProfile | None = None,
        parent=None,
    ):
        """
        actions: [(key_hint, label), ...] — ej. [("B", "Volver"), ("A", "Cargar ROM")]
        input_config: si se pasa, key_hint puede ser joypad_bit (int) y se resuelve la tecla.
        joypad_labels: {4: "A", 5: "B", ...} para mostrar el nombre del botón.
        """
        super().__init__(parent)
        self._input_config = input_config
        self._profile = profile
        self._joypad_labels = joypad_labels or {4: "A", 5: "B", 6: "Select", 7: "Start"}
        self._actions: list[tuple[str, str]] = []
        self._labels: list[QLabel] = []

        layout = QHBoxLayout(self)
        layout.setContentsMargins(16, 8, 16, 8)
        layout.setSpacing(24)

        self._container = QWidget()
        self._container_layout = QHBoxLayout(self._container)
        self._container_layout.setContentsMargins(0, 0, 0, 0)
        self._container_layout.setSpacing(24)
        layout.addWidget(self._container)

        self.setStyleSheet(footer_style())
        if actions:
            self.set_actions(actions)

    def set_actions(self, actions: list[tuple[str | int, str]]):
        """Actualiza las acciones. key puede ser str ("B") o int (joypad bit 5)."""
        resolved: list[tuple[str, str]] = []
        for k, label in actions:
            if isinstance(k, int) and self._input_config and self._profile:
                bit = k
                km = self._input_config.get_keyboard_map(self._profile)
                key_str = _key_for_action(bit, km)
                btn_name = self._joypad_labels.get(bit, f"Btn{bit}")
                resolved.append((key_str or btn_name, label))
            else:
                resolved.append((str(k), label))
        self._actions = resolved
        self._build_prompts()

    def _build_prompts(self):
        for lbl in self._labels:
            lbl.deleteLater()
        self._labels.clear()

        while self._container_layout.count():
            item = self._container_layout.takeAt(0)
            w = item.widget()
            if w:
                w.deleteLater()

        profile = self._profile or getattr(self.parent(), "_profile", None) or ConsoleProfile(
            name="", fb_width=160, fb_height=144, rom_extensions=[],
            window_title="", splash_bg=(30, 30, 38), splash_fg=(200, 200, 210),
            splash_accent=(100, 100, 120),
        )
        for key_hint, label in self._actions:
            key_box = QLabel(key_hint)
            key_box.setStyleSheet(key_hint_style(profile))
            self._container_layout.addWidget(key_box)

            text_lbl = QLabel(label)
            text_lbl.setStyleSheet(
                "color: rgb(180, 180, 190); font-family: monospace;"
                " font-size: 12px; background: transparent;"
            )
            self._container_layout.addWidget(text_lbl)
            self._labels.extend([key_box, text_lbl])

        self._container_layout.addStretch()

    def set_profile(self, profile: ConsoleProfile):
        """Actualiza el perfil y reaplica estilos a los hints."""
        self._profile = profile
        self._build_prompts()


class ListWithPreviewPanel(QWidget, Generic[T]):
    """
    Panel genérico: lista a la izquierda + preview a la derecha.
    Emite item_selected(item) al elegir. Configurable por callbacks.
    """

    item_selected = Signal(object)
    item_activated = Signal(object)

    def __init__(
        self,
        profile: ConsoleProfile,
        list_width: int = 280,
        item_to_display: Callable[[T], str] = str,
        preview_widget_factory: Callable[[QWidget], QWidget] | None = None,
        parent=None,
    ):
        """
        item_to_display: función para mostrar cada item en la lista.
        preview_widget_factory: recibe el contenedor, devuelve el widget de preview.
        """
        super().__init__(parent)
        self._profile = profile
        self._list_width = list_width
        self._item_to_display = item_to_display
        self._items: list[T] = []
        self._preview_widget: QWidget | None = None

        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)

        splitter = QSplitter(Qt.Orientation.Horizontal)

        self._list = QListWidget()
        self._list.setMinimumWidth(list_width)
        self._list.setMaximumWidth(list_width)
        self._list.setFrameShape(QFrame.Shape.NoFrame)
        self._list.setStyleSheet(
            f"QListWidget {{ background: rgb({profile.splash_bg[0]},{profile.splash_bg[1]},{profile.splash_bg[2]});"
            f" color: rgb({profile.splash_fg[0]},{profile.splash_fg[1]},{profile.splash_fg[2]});"
            " border: none; border-radius: 8px; padding: 8px;"
            " font-family: monospace; font-size: 13px; }"
            " QListWidget::item { padding: 8px 12px; }"
            " QListWidget::item:selected { background: rgb(60, 60, 70);"
            f" border-left: 3px solid rgb({profile.splash_accent[0]},{profile.splash_accent[1]},{profile.splash_accent[2]}); }}"
            " QListWidget::item:hover { background: rgb(50, 50, 60); }"
        )
        self._list.currentRowChanged.connect(self._on_row_changed)
        self._list.itemActivated.connect(self._on_item_activated)
        splitter.addWidget(self._list)

        preview_container = QFrame()
        preview_container.setMinimumWidth(280)
        bg = profile.splash_bg
        preview_container.setStyleSheet(
            f"QFrame {{ background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, "
            f"stop: 0 rgb({bg[0]},{bg[1]},{bg[2]}), "
            f"stop: 1 rgb({max(0,bg[0]-15)},{max(0,bg[1]-15)},{max(0,bg[2]-15)})); "
            f"border: 1px solid rgb({profile.splash_accent[0]},{profile.splash_accent[1]},{profile.splash_accent[2]}); "
            "border-radius: 8px; }"
        )
        preview_layout = QVBoxLayout(preview_container)
        preview_layout.setContentsMargins(16, 16, 16, 16)
        preview_layout.setAlignment(Qt.AlignmentFlag.AlignTop | Qt.AlignmentFlag.AlignHCenter)

        if preview_widget_factory:
            self._preview_widget = preview_widget_factory(preview_container)
            preview_layout.addWidget(self._preview_widget)
        else:
            self._placeholder = QLabel("Seleccioná un juego")
            self._placeholder.setStyleSheet(
                f"color: rgb({profile.splash_accent[0]},{profile.splash_accent[1]},{profile.splash_accent[2]});"
                " font-family: monospace; font-size: 14px;"
            )
            preview_layout.addWidget(self._placeholder)

        splitter.addWidget(preview_container)
        splitter.setSizes([list_width, 400])
        splitter.setStretchFactor(0, 0)
        splitter.setStretchFactor(1, 1)

        layout.addWidget(splitter)

    def set_items(self, items: list[T]):
        self._items = items
        self._list.clear()
        for item in items:
            self._list.addItem(QListWidgetItem(self._item_to_display(item)))
        if items:
            self._list.setCurrentRow(0)

    def set_preview_updater(self, updater: Callable[[T | None], None]):
        """Callback para actualizar el preview cuando cambia la selección."""
        self._preview_updater = updater

    def get_preview_widget(self) -> QWidget | None:
        """Widget de preview para que el padre lo configure."""
        return self._preview_widget

    def _on_row_changed(self, row: int):
        item = self._items[row] if 0 <= row < len(self._items) else None
        if hasattr(self, "_preview_updater") and self._preview_updater:
            self._preview_updater(item)
        self.item_selected.emit(item)

    def _on_item_activated(self, _list_item):
        row = self._list.currentRow()
        item = self._items[row] if 0 <= row < len(self._items) else None
        if item is not None:
            self.item_activated.emit(item)

    def current_item(self) -> T | None:
        row = self._list.currentRow()
        if 0 <= row < len(self._items):
            return self._items[row]
        return None

    def set_profile(self, profile: ConsoleProfile):
        self._profile = profile
        self._list.setStyleSheet(list_style(profile))
