/**
 * Bitemu - CRC32 (IEEE 802.3, polynomial 0xEDB88320)
 * Table-driven implementation; no external dependencies.
 */

#include "crc32.h"

static uint32_t crc_table[256];
static int table_ready;

static void build_table(void)
{
    for (uint32_t i = 0; i < 256; i++)
    {
        uint32_t c = i;
        for (int j = 0; j < 8; j++)
            c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
        crc_table[i] = c;
    }
    table_ready = 1;
}

uint32_t bitemu_crc32(const uint8_t *data, size_t len)
{
    if (!table_ready)
        build_table();

    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++)
        crc = crc_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFF;
}
