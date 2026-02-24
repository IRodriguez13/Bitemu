/**
 * Bitemu - Game Boy PPU
 * Picture Processing Unit
 */

#ifndef BITEMU_GB_PPU_H
#define BITEMU_GB_PPU_H

#include <stdint.h>

typedef struct gb_ppu {
    uint8_t lcdc, stat, scy, scx, ly, lyc;
} gb_ppu_t;

void gb_ppu_init(gb_ppu_t *ppu);
void gb_ppu_step(gb_ppu_t *ppu, int cycles);

#endif /* BITEMU_GB_PPU_H */
