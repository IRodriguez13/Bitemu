/**
 * Bitemu - Game Boy PPU
 * Picture Processing Unit
 */

#ifndef BITEMU_GB_PPU_H
#define BITEMU_GB_PPU_H

#include <stdint.h>

#define GB_FRAMEBUFFER_SIZE (160 * 144)

struct gb_mem;

typedef struct gb_ppu 
{
    int mode;
    int dot_counter;
    uint8_t framebuffer[GB_FRAMEBUFFER_SIZE];
    int mode3_dots;
    int window_line;
    uint8_t stat_irq_line;
    uint8_t latched_scy;
    uint8_t latched_scx;
} gb_ppu_t;

void gb_ppu_init(gb_ppu_t *ppu);
void gb_ppu_step(gb_ppu_t *ppu, struct gb_mem *mem, int cycles);

#endif /* BITEMU_GB_PPU_H */
