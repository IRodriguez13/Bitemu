# PPU (Picture Processing Unit)

## English

### 7.1 Overview

The PPU renders the 160×144 pixel display. It uses a tile-based system with a background layer and up to 40 sprites (8×8 or 8×16).

### 7.2 Timing

- **CPU clock**: 4.194304 MHz (1 T-cycle = 1 dot for most timing).
- **Scanline**: 456 dots.
- **Visible scanlines**: 144 (lines 0–143).
- **VBlank**: 10 scanlines (lines 144–153).
- **Total**: 154 scanlines × 456 dots = 70224 dots per frame.

### 7.3 Modes

| Mode | Dots | Description |
|------|------|-------------|
| OAM (2) | 80 | OAM search |
| Transfer (3) | 172–289 | Pixel transfer to LCD |
| HBlank (0) | 456 − mode2 − mode3 | Horizontal blanking |
| VBlank (1) | 10 × 456 | Vertical blanking |

### 7.4 Tile Data

- **Tiles**: 8×8 pixels, 16 bytes each (2 bits per pixel → 4 shades).
- **Tile map**: 32×32 tiles. Two maps: 0x9800, 0x9C00 (LCDC bit 3).
- **Tile data**: 0x8000 (unsigned, tiles 0–255) or 0x9000 (signed, tiles −128–127).
- **LCDC bit 4**: 0 = signed (0x9000), 1 = unsigned (0x8000).

### 7.5 Pixel Format

Each pixel is a 2-bit color index (0–3). Palettes map indices to shades:

- **BGP** (0xFF47): Background palette. Bits 0–1 = color 0, 2–3 = color 1, etc.
- **OBP0, OBP1** (0xFF48, 0xFF49): Sprite palettes.

DMG shades: 0=white, 1=light gray, 2=dark gray, 3=black (0xFF, 0xAA, 0x55, 0x00).

### 7.6 Sprites (OAM)

Each sprite: 4 bytes (Y, X, tile, flags).

- **Flags**: Priority, Y-flip, X-flip, palette (OBP0/OBP1).
- **Size**: 8×8 or 8×16 (LCDC bit 2).
- **Rendering**: Back-to-front (higher OAM index = higher priority when overlapping).

### 7.7 Framebuffer

- **Size**: 160×144.
- **Format**: 1 byte per pixel (grayscale 0–255).
- **Output**: `ppu->framebuffer`; frontend converts to RGB for display.

### 7.8 VBlank Interrupt

When entering VBlank (LY=144), the PPU sets `IF bit 0` (VBlank interrupt).

### 7.9 Implementation Notes

- `get_pixel_from_tile()`: Handles signed/unsigned tile addressing. Bounds check for VRAM.
- `render_scanline()`: Draws BG (if enabled), then sprites (if enabled).
- LCD off (LCDC bit 7 = 0): LY=0, STAT mode = HBlank.

---

## Español

### 7.1 Resumen

El PPU renderiza la pantalla de 160×144 píxeles. Sistema basado en tiles con capa de fondo y hasta 40 sprites (8×8 o 8×16).

### 7.2 Timing

- **Reloj CPU**: 4.194304 MHz.
- **Scanline**: 456 dots.
- **Scanlines visibles**: 144 (líneas 0–143).
- **VBlank**: 10 scanlines (144–153).
- **Total**: 154 × 456 = 70224 dots por frame.

### 7.3 Modos

| Modo | Dots | Descripción |
|------|------|-------------|
| OAM (2) | 80 | Búsqueda OAM |
| Transfer (3) | 172–289 | Transferencia de píxeles |
| HBlank (0) | 456 − mode2 − mode3 | Blanking horizontal |
| VBlank (1) | 10 × 456 | Blanking vertical |

### 7.4 Datos de tiles

- **Tiles**: 8×8 píxeles, 16 bytes cada uno (2 bits/píxel).
- **Mapa de tiles**: 32×32. Dos mapas: 0x9800, 0x9C00.
- **Datos**: 0x8000 (unsigned) o 0x9000 (signed).

### 7.5 Paletas

- **BGP**: Paleta de fondo.
- **OBP0, OBP1**: Paletas de sprites.
- Tonos DMG: blanco, gris claro, gris oscuro, negro.

### 7.6 Sprites (OAM)

4 bytes por sprite: Y, X, tile, flags. Tamaño 8×8 o 8×16 según LCDC.

### 7.7 Framebuffer

160×144, 1 byte/píxel. El frontend convierte a RGB.

### 7.8 Interrupción VBlank

Al entrar en VBlank: `IF |= 0x01`.
