# Save States — Binary Compatibility System

## Overview

Bitemu uses a versioned binary format (`.bst`) to snapshot the full emulator
state: CPU registers, PPU mode + framebuffer, APU channels, and memory
(WRAM, VRAM, OAM, I/O, ext RAM, timer, RTC).

The format is designed for **forward and backward compatibility** across
Bitemu versions without requiring manual migration code.

---

## File Format (BST v2)

```
┌───────────────────────────────────────────┐
│  Header (20 bytes, fixed for all versions)│
│  ┌─ magic:      "BEMU"  (4 bytes)        │
│  ├─ version:    uint32   (currently 2)    │
│  ├─ console_id: uint32   (1 = Game Boy)   │
│  ├─ rom_crc32:  uint32                    │
│  └─ state_size: uint32   (reserved)       │
├───────────────────────────────────────────┤
│  CPU Section                              │
│  ┌─ section_size: uint32                  │
│  └─ data: section_size bytes              │
├───────────────────────────────────────────┤
│  PPU Section                              │
│  ┌─ section_size: uint32                  │
│  └─ data: section_size bytes              │
├───────────────────────────────────────────┤
│  APU Section                              │
│  ┌─ section_size: uint32                  │
│  └─ data: section_size bytes              │
├───────────────────────────────────────────┤
│  Memory Section                           │
│  ┌─ section_size: uint32                  │
│  └─ fields: field-by-field (see below)    │
└───────────────────────────────────────────┘
```

Each section carries its own byte count. The loader reads
`min(stored, current)` bytes, zero-fills any deficit, and skips any surplus.

---

## The Golden Rule

> **New fields MUST be appended at the END of structs. Never reorder,
> rename, resize, or remove existing fields.**

This is enforced by the **ABI Guard test** (`tests/core/test_abi_guard.c`),
which freezes every field's byte offset. If a field moves, CI fails and the
release is blocked.

### How to add a new field

1. Add the field at the **end** of the struct (e.g. `gb_apu_t`).
2. Open `tests/core/test_abi_guard.c`:
   - Update `ASSERT_MIN_SIZE` to the new `sizeof`.
   - Add an `ASSERT_OFFSET` line for the new field.
3. Run `make test-core` — all tests must pass.
4. Commit. The CI pipeline will verify on all platforms.

Old save states will load correctly: the new field starts at zero (the
`read_section` function zero-fills the struct before reading).

### What if a critical bug requires changing a field's type?

Do NOT modify the existing field. Instead:

```c
uint16_t lfsr;             // offset 16, frozen forever (old)
// ... existing fields ...
uint32_t lfsr_extended;    // NEW, at the end, replaces lfsr internally
```

In your init/load code, migrate the old value if `lfsr_extended == 0`:

```c
if (apu->lfsr_extended == 0 && apu->lfsr != 0)
    apu->lfsr_extended = apu->lfsr;
```

---

## Compatibility Matrix

| Scenario | Result |
|----------|--------|
| v2 state on current Bitemu | Loads perfectly |
| v2 state on newer Bitemu (more fields) | Loads: new fields zero-init |
| Newer state on older Bitemu (fewer fields) | Loads: extra bytes skipped |
| v1 state (pre-sections) | Rejected with clear message |
| Wrong ROM | Rejected (CRC32 mismatch) |
| Wrong console | Rejected (console_id mismatch) |

---

## Battery Saves vs Save States

| | Battery Save (.sav) | Save State (.bst) |
|---|---|---|
| **What** | Cartridge SRAM (game's own save) | Full machine snapshot |
| **Format** | Raw SRAM dump (hardware-defined) | Bitemu-specific binary |
| **Compat** | Always compatible (hardware format) | Version-tolerant (section system) |
| **Scope** | Game progress only | Exact instant (CPU, PPU, APU, RAM) |
| **Fallback** | N/A | If .bst fails, .sav still works |

---

## For Future Console Editions

Each console (Genesis, GBA) implements its own `save_state`/`load_state`
via the `console_ops_t` interface. The header format is shared. Each
console's structs get their own ABI Guard test in their respective
test suite.

```
tests/core/test_abi_guard.c       ← Game Boy structs (gb_cpu_t, gb_ppu_t, gb_apu_t)
tests/be2/test_abi_guard.c        ← Genesis structs (gen_cpu_t, gen_vdp_t)
tests/be3/test_abi_guard.c        ← GBA structs (future)
```
