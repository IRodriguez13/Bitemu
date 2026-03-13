/**
 * Bitemu - Helpers
 * Utilidades de soporte
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "helpers.h"

uint8_t read_u8(const uint8_t *mem, uint32_t addr) 
{
    return mem[addr];
}

uint16_t read_u16(const uint8_t *mem, uint32_t addr) 
{
    return (uint16_t)mem[addr] | ((uint16_t)mem[addr + 1] << 8);
}

uint32_t read_u32(const uint8_t *mem, uint32_t addr) 
{
    return (uint32_t)mem[addr] | ((uint32_t)mem[addr + 1] << 8) |
           ((uint32_t)mem[addr + 2] << 16) | ((uint32_t)mem[addr + 3] << 24);
}
