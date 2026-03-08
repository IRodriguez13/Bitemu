/**
 * Bitemu - CRC32 (IEEE 802.3)
 * Used for ROM validation in save states.
 */

#ifndef BITEMU_CRC32_H
#define BITEMU_CRC32_H

#include <stddef.h>
#include <stdint.h>

uint32_t bitemu_crc32(const uint8_t *data, size_t len);

#endif /* BITEMU_CRC32_H */
