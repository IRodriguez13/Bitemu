/**
 * Bitemu - Helpers
 * Utilidades de soporte
 */

#ifndef BITEMU_HELPERS_H
#define BITEMU_HELPERS_H

#include <stdint.h>

uint8_t read_u8(const uint8_t *mem, uint32_t addr);
uint16_t read_u16(const uint8_t *mem, uint32_t addr);
uint32_t read_u32(const uint8_t *mem, uint32_t addr);

#endif /* BITEMU_HELPERS_H */
