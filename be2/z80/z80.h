/**
 * BE2 - Z80 (Genesis sound CPU)
 *
 * Driver de sonido. Ejecuta cuando tiene el bus (bus_req=0).
 * Mapa: 0x0000-0x1FFF RAM, 0x4000 YM2612, 0x6000 bank, 0x7F11 PSG, 0x8000-0xFFFF ROM bank.
 * Instrucciones típicas de drivers + AND/OR/XOR/SUB/CP (HL), CB, parcial IX/IY/ED (LDIR/LDDR, CPI/CPD, INI/INIR, OUTI/OTIR, OUTD/OTDR).
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef BITEMU_GENESIS_Z80_H
#define BITEMU_GENESIS_Z80_H

#include <stdint.h>

typedef struct gen_z80 gen_z80_t;

struct gen_z80 {
    uint16_t pc, sp, af, bc, de, hl, ix, iy;
    uint8_t i, r;
    uint8_t iff1, iff2;
    uint8_t halted;
    uint8_t bank;       /* Z80 bank para 0x8000-0xFFFF */
};

void gen_z80_init(gen_z80_t *z80);
void gen_z80_reset(gen_z80_t *z80);

/* Ejecuta hasta 'cycles' ciclos Z80. ctx = genesis_impl_t* para callbacks. */
int gen_z80_step(gen_z80_t *z80, void *ctx, int cycles);

#endif /* BITEMU_GENESIS_Z80_H */
