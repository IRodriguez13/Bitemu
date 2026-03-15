/**
 * Bitemu - Save State System Subsystem 
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "savestate.h"
#include "utils/log.h"
#include <string.h>

int bst_write_all(FILE *f, const void *buf, size_t n)
{
    return fwrite(buf, 1, n, f) == n ? 0 : -1;
}

int bst_read_all(FILE *f, void *buf, size_t n)
{
    return fread(buf, 1, n, f) == n ? 0 : -1;
}

int bst_write_section(FILE *f, const void *data, uint32_t size)
{
    int err = bst_write_all(f, &size, sizeof(size));
    err |= bst_write_all(f, data, size);
    return err;
}

int bst_read_section(FILE *f, void *dest, uint32_t dest_size)
{
    uint32_t stored;
    if (bst_read_all(f, &stored, sizeof(stored)) != 0)
        return -1;
    memset(dest, 0, dest_size);
    uint32_t to_read = stored < dest_size ? stored : dest_size;
    if (to_read > 0 && bst_read_all(f, dest, to_read) != 0)
        return -1;
    if (stored > dest_size && fseek(f, (long)(stored - dest_size), SEEK_CUR) != 0)
        return -1;
    return 0;
}

int bst_write_header(FILE *f, uint32_t console_id, uint32_t rom_crc32)
{
    bst_header_t hdr;
    memcpy(hdr.magic, BST_MAGIC, 4);
    hdr.version = BST_VERSION;
    hdr.console_id = console_id;
    hdr.rom_crc32 = rom_crc32;
    hdr.state_size = 0;
    return bst_write_all(f, &hdr, sizeof(hdr));
}

int bst_read_header(FILE *f, bst_header_t *out)
{
    return bst_read_all(f, out, sizeof(*out));
}

int bst_validate_header(const bst_header_t *hdr, uint32_t expected_console_id,
                        uint32_t current_rom_crc32)
{
    if (memcmp(hdr->magic, BST_MAGIC, 4) != 0)
    {
        log_error("Invalid save state: bad magic");
        return -1;
    }
    if (hdr->console_id != expected_console_id)
    {
        log_error("Invalid save state: wrong console (expected %u, got %u)",
                  (unsigned)expected_console_id, (unsigned)hdr->console_id);
        return -1;
    }
    if (hdr->version < BST_VERSION)
    {
        log_error("Save state v%u is obsolete (need v%u). Please load the ROM and re-save.",
                  (unsigned)hdr->version, BST_VERSION);
        return -1;
    }
    if (hdr->version > BST_VERSION)
    {
        log_error("Save state v%u was created by a newer Bitemu. Please update Bitemu.",
                  (unsigned)hdr->version);
        return -1;
    }
    if (hdr->rom_crc32 != current_rom_crc32)
    {
        log_error("ROM CRC mismatch in state file (expected %08X, got %08X)",
                  (unsigned)current_rom_crc32, (unsigned)hdr->rom_crc32);
        return -1;
    }
    return 0;
}








