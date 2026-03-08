# Branding System

How visual identity works across Bitemu editions.

---

## Overview

Every Bitemu edition (Game Boy, Genesis, GBA) shares the same frontend
codebase.  Visual differentiation comes entirely from the `ConsoleProfile`
dataclass defined in `frontend/bitemu_gui/profile.py`.  No rendering code
needs to change when adding a new console — only a new profile with the
right colors.

---

## ConsoleProfile color fields

| Field | Type | Purpose |
|-------|------|---------|
| `splash_bg` | `(R, G, B)` | Splash screen background (darkest color) |
| `splash_fg` | `(R, G, B)` | Logo pixel-art text (lightest color) |
| `splash_accent` | `(R, G, B)` | Divider line, secondary decorations |
| `splash_sub` | `(R, G, B)` | Version subtitle text |
| `ansi_color` | `str` | ANSI SGR code for the terminal banner (e.g. `"32"` = green) |

These fields have defaults matching the Game Boy DMG palette, so existing
profiles that don't specify them continue to work.

---

## Where colors are used

### 1. Splash screen (`game_widget.py → _draw_splash`)

Rendered when no ROM is loaded.  The pixel-art "BITEMU" logo, divider line,
and version string all read their colors from `self._profile.splash_*`.

### 2. Terminal banner (`main.py → _colored_banner`)

The ASCII art printed to stderr on startup uses `profile.ansi_color` to
wrap the text in ANSI escape sequences.  Terminals that don't support color
simply ignore the escape codes.

### 3. Future: window icon, status bar accent, etc.

The same profile fields can be reused for any UI element that should match
the edition's branding.

---

## Palette reference

### Game Boy (Bitemu)

```
splash_bg     = (0x0F, 0x38, 0x0F)   # DMG darkest green
splash_fg     = (0x9B, 0xBC, 0x0F)   # DMG lightest green
splash_accent = (0x30, 0x62, 0x30)   # DMG mid-dark green
splash_sub    = (0x8B, 0xAC, 0x0F)   # DMG mid-light green
ansi_color    = "32"                  # green
```

### Game Boy Advance (Bitemu+) — planned

```
splash_bg     = (0x1B, 0x05, 0x33)   # deep indigo
splash_fg     = (0x5B, 0x3E, 0x96)   # GBA purple
splash_accent = (0x3A, 0x20, 0x6A)   # dark purple
splash_sub    = (0xC8, 0xC8, 0xD4)   # light grey
ansi_color    = "35"                  # magenta
```

### Sega Genesis (Bitemu 2) — planned

```
splash_bg     = (0x0C, 0x1A, 0x3C)   # Sega dark blue
splash_fg     = (0x21, 0x76, 0xFF)   # Sega blue
splash_accent = (0xD4, 0xA0, 0x17)   # Sega gold
splash_sub    = (0x7E, 0xB8, 0xFF)   # light blue
ansi_color    = "34"                  # blue
```

---

## How to add a new edition

1. **Create the profile** in `profile.py`:

```python
PROFILE_MYCONS = ConsoleProfile(
    name="My Console",
    fb_width=256,
    fb_height=240,
    rom_extensions=["rom"],
    window_title="Bitemu X — My Console",
    splash_bg=(0x10, 0x10, 0x10),
    splash_fg=(0xFF, 0xCC, 0x00),
    splash_accent=(0x80, 0x60, 0x00),
    splash_sub=(0xFF, 0xEE, 0x88),
    ansi_color="33",
)
```

2. **Set it as default** (or pass it to `MainWindow`):

```python
DEFAULT_PROFILE = PROFILE_MYCONS
```

3. **Done.** The splash, banner, and any future branded UI elements
   automatically use the new palette.  No other files need editing.

---

## Design principles

- **Data-driven**: all branding is in the profile, not scattered across code.
- **Backwards compatible**: color fields have defaults, so old profiles work.
- **Portable**: ANSI codes work on Linux, macOS, and Windows Terminal.
  The splash uses Qt's QPainter, which works everywhere PySide6 runs.
- **Consistent**: the same 4-color scheme (bg, fg, accent, sub) is reused
  across all branded surfaces, keeping each edition visually cohesive.
