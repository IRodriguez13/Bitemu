# Memory

## English

### 6.1 Memory Map

| Address | Size | Description |
|---------|------|-------------|
| 0x0000–0x3FFF | 16 KB | ROM Bank 0 (fixed) |
| 0x4000–0x7FFF | 16 KB | ROM Bank 1–N (switchable via MBC) |
| 0x8000–0x9FFF | 8 KB | VRAM (tiles, tile maps) |
| 0xA000–0xBFFF | 8 KB | External RAM (cartridge, switchable) |
| 0xC000–0xCFFF | 4 KB | WRAM Bank 0 |
| 0xD000–0xDFFF | 4 KB | WRAM Bank 1 (DMG: same as bank 0) |
| 0xE000–0xFDFF | — | Echo RAM (mirror of 0xC000–0xDDFF) |
| 0xFE00–0xFE9F | 160 B | OAM (Sprite Attribute Table) |
| 0xFEA0–0xFEFF | — | Prohibited |
| 0xFF00–0xFF7F | 128 B | I/O Registers |
| 0xFF80–0xFFFE | 127 B | HRAM (High RAM) |
| 0xFFFF | 1 B | IE (Interrupt Enable) |

### 6.2 MBC1 (Memory Bank Controller)

Cartridges without battery use MBC1 for ROM/RAM banking:

- **0x0000–0x1FFF**: Write 0x0A to enable external RAM.
- **0x2000–0x3FFF**: Write 5-bit value to select ROM bank (1–31; 0 is treated as 1).
- **0x4000–0x5FFF**: Write 2-bit value for RAM bank (0–3) or upper ROM bits.

### 6.3 I/O Registers (selected)

| Addr | Name | Purpose |
|------|------|---------|
| 0xFF00 | JOYP | Joypad (read: bits 0–3 D-pad, 4–7 buttons; write: select) |
| 0xFF01 | SB | Serial transfer data |
| 0xFF02 | SC | Serial control (0x81 = transfer) |
| 0xFF04 | DIV | Divider (upper 8 bits of 16-bit counter) |
| 0xFF05 | TIMA | Timer counter |
| 0xFF06 | TMA | Timer modulo |
| 0xFF07 | TAC | Timer control (bit 2 = enable, bits 0–1 = clock select) |
| 0xFF0F | IF | Interrupt flags (bits 0–4) |
| 0xFF40 | LCDC | LCD control |
| 0xFF41 | STAT | LCD status |
| 0xFF42–0xFF45 | SCY, SCX, LY, LYC | Scroll, scanline |
| 0xFF46 | DMA | OAM DMA trigger (write = start transfer) |
| 0xFF47–0xFF49 | BGP, OBP0, OBP1 | Palettes |
| 0xFFFF | IE | Interrupt enable |

### 6.4 DMA (0xFF46)

Writing a byte `X` to 0xFF46 starts an OAM DMA transfer:
- Source: `(X << 8)` to `(X << 8) + 160`
- Destination: OAM (0xFE00–0xFE9F)
- 160 bytes copied. Implementation does instant copy; real hardware takes ~160 cycles.

### 6.5 IF Register

- **Read**: Returns `(value & 0x1F) | 0xE0` (bits 5–7 always 1).
- **Write**: Replaces lower 5 bits. Software can set or clear interrupt flags.
- Used for both hardware-triggered and software-triggered interrupts (e.g. tests).

### 6.6 Joypad (0xFF00)

- **Read**: Bits 4–5 select which group to read (0=D-pad, 1=buttons). Selected bits are 0 when pressed.
- **Write**: Only bits 4–5 are writable; they select the read group.
- `joypad_state` in `gb_mem_t`: 1 = pressed. Inverted when reading JOYP.

---

## Español

### 6.1 Mapa de memoria

| Dirección | Tamaño | Descripción |
|-----------|--------|-------------|
| 0x0000–0x3FFF | 16 KB | ROM Bank 0 (fija) |
| 0x4000–0x7FFF | 16 KB | ROM Bank 1–N (cambiable vía MBC) |
| 0x8000–0x9FFF | 8 KB | VRAM (tiles, mapas) |
| 0xA000–0xBFFF | 8 KB | RAM externa (cartucho) |
| 0xC000–0xCFFF | 4 KB | WRAM Bank 0 |
| 0xD000–0xDFFF | 4 KB | WRAM Bank 1 |
| 0xE000–0xFDFF | — | Echo RAM (espejo de 0xC000–0xDDFF) |
| 0xFE00–0xFE9F | 160 B | OAM (atributos de sprites) |
| 0xFEA0–0xFEFF | — | Prohibido |
| 0xFF00–0xFF7F | 128 B | Registros I/O |
| 0xFF80–0xFFFE | 127 B | HRAM |
| 0xFFFF | 1 B | IE (Interrupt Enable) |

### 6.2 MBC1

- **0x0000–0x1FFF**: Escribir 0x0A para habilitar RAM externa.
- **0x2000–0x3FFF**: Seleccionar banco ROM (1–31).
- **0x4000–0x5FFF**: Banco RAM (0–3).

### 6.3 DMA (0xFF46)

Escribir `X` en 0xFF46 inicia la transferencia OAM:
- Origen: `(X << 8)` a `(X << 8) + 160`
- Destino: OAM (0xFE00–0xFE9F)
- 160 bytes. Implementación: copia instantánea.

### 6.4 Registro IF

- **Lectura**: `(valor & 0x1F) | 0xE0`.
- **Escritura**: Reemplaza los 5 bits bajos. Software puede setear o limpiar flags.
