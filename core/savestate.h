/**
 * Bitemu - Save State System (módulo universal)
 *
 * Formato .bst compartido por todas las consolas. Política:
 * - Header común: magic, version, console_id, rom_crc32
 * - Secciones con tamaño prefijado (forward/backward compat)
 * - Validación: magic, console_id, version, CRC32
 * - ABI Guard: cada consola tiene su test en tests/beN/test_abi_guard.c
 *
 * Cada consola implementa save_state/load_state vía console_ops_t
 * y usa estas funciones para I/O y validación.
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef BITEMU_SAVESTATE_H
#define BITEMU_SAVESTATE_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define BST_MAGIC       "BEMU"
#define BST_VERSION     5

enum {
    BST_CONSOLE_GB   = 1,
    BST_CONSOLE_GEN  = 2,
};

typedef struct {
    char     magic[4];
    uint32_t version;
    uint32_t console_id;
    uint32_t rom_crc32;
    uint32_t state_size;
} bst_header_t;

/* I/O helpers */
int bst_write_all(FILE *f, const void *buf, size_t n);
int bst_read_all(FILE *f, void *buf, size_t n);

/* Section: uint32 size + data. On load: zero-fill deficit, skip surplus. */
int bst_write_section(FILE *f, const void *data, uint32_t size);
int bst_read_section(FILE *f, void *dest, uint32_t dest_size);

/* Header */
int bst_write_header(FILE *f, uint32_t console_id, uint32_t rom_crc32);
int bst_read_header(FILE *f, bst_header_t *out);

/* Validación: 0=OK, -1=error. Loggea motivo. */
int bst_validate_header(const bst_header_t *hdr, uint32_t expected_console_id,
                        uint32_t current_rom_crc32);

#endif /* BITEMU_SAVESTATE_H */
