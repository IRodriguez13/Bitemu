/**
 * Bitemu - CRC32 (IEEE 802.3)
 * Used for ROM validation in save states.
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef BITEMU_CRC32_H
#define BITEMU_CRC32_H

#include <stddef.h>
#include <stdint.h>

uint32_t bitemu_crc32(const uint8_t *data, size_t len);

#endif /* BITEMU_CRC32_H */
