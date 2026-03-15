"""
Estilos retro para Bitemu: gradientes oscuros, bordes tipo plástico, tipografía retro.
Paleta inspirada en Sega Genesis / arcade.
"""
from .profile import ConsoleProfile


def retro_gradient(bg: tuple[int, int, int], darker: int = 15) -> str:
    """Gradiente vertical oscuro para fondos."""
    r, g, b = bg
    r2 = max(0, r - darker)
    g2 = max(0, g - darker)
    b2 = max(0, b - darker)
    return f"qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 rgb({r},{g},{b}),stop:1 rgb({r2},{g2},{b2}))"


def plastic_border(accent: tuple[int, int, int], width: int = 2) -> str:
    """Borde tipo plástico con highlight sutil."""
    r, g, b = accent
    r_hi = min(255, r + 40)
    g_hi = min(255, g + 40)
    b_hi = min(255, b + 40)
    return (
        f"border: {width}px solid rgb({r},{g},{b});"
        f" border-radius: 8px;"
    )


def header_style(profile: ConsoleProfile) -> str:
    """AppHeader con gradiente y borde retro."""
    bg = profile.splash_bg
    r, g, b = bg
    r2, g2, b2 = max(0, r - 12), max(0, g - 12), max(0, b - 12)
    acc = profile.splash_accent
    return (
        f"AppHeader {{"
        f" background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
        f" stop: 0 rgb({r},{g},{b}), stop: 1 rgb({r2},{g2},{b2}));"
        f" border: 1px solid rgb({acc[0]},{acc[1]},{acc[2]});"
        f" border-radius: 8px;"
        f" }}"
    )


def footer_style() -> str:
    """AppFooter con gradiente oscuro y borde plástico."""
    return (
        "AppFooter { "
        "background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, "
        "stop: 0 rgb(28,28,36), stop: 1 rgb(18,18,24));"
        "border: 1px solid rgb(60,60,75);"
        "border-radius: 8px;"
        "}"
    )


def list_style(profile: ConsoleProfile) -> str:
    """ListWithPreviewPanel: lista con gradiente y borde retro."""
    bg = profile.splash_bg
    acc = profile.splash_accent
    fg = profile.splash_fg
    return (
        f"QListWidget {{"
        f" background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, "
        f"stop: 0 rgb({bg[0]},{bg[1]},{bg[2]}), "
        f"stop: 1 rgb({max(0,bg[0]-20)},{max(0,bg[1]-20)},{max(0,bg[2]-20)}));"
        f" color: rgb({fg[0]},{fg[1]},{fg[2]});"
        f" border: 1px solid rgb({acc[0]},{acc[1]},{acc[2]});"
        f" border-radius: 8px; padding: 8px;"
        f" font-family: 'JetBrains Mono', 'Fira Code', 'Consolas', monospace;"
        f" font-size: 13px; }}"
        " QListWidget::item { padding: 8px 12px; }"
        f" QListWidget::item:selected {{ background: rgb({min(255,acc[0]+80)},{min(255,acc[1]+80)},{min(255,acc[2]+80)});"
        f" border-left: 4px solid rgb({acc[0]},{acc[1]},{acc[2]}); }}"
        " QListWidget::item:hover { background: rgb(70, 70, 80); }"
    )


def key_hint_style(profile: ConsoleProfile) -> str:
    """Caja de tecla en footer: estilo plástico."""
    bg = profile.splash_bg
    acc = profile.splash_accent
    return (
        f"background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, "
        f"stop: 0 rgb({min(255,bg[0]+35)},{min(255,bg[1]+35)},{min(255,bg[2]+35)}), "
        f"stop: 1 rgb({bg[0]},{bg[1]},{bg[2]}));"
        f"color: rgb({acc[0]},{acc[1]},{acc[2]});"
        "border: 1px solid rgb(80, 80, 90);"
        "border-radius: 4px; padding: 4px 8px;"
        "font-family: 'JetBrains Mono', 'Fira Code', monospace;"
        "font-weight: bold; font-size: 12px;"
    )
