/**
 * Bitemu - Game Boy Memory
 * Mapa de memoria y bancos
 */

#include <stdint.h>

#define GB_MEM_SIZE 0x10000

uint8_t gb_memory_read(uint8_t *mem, uint16_t addr) {
    (void)mem;
    (void)addr;
    return 0;
}

void gb_memory_write(uint8_t *mem, uint16_t addr, uint8_t val) {
    (void)mem;
    (void)addr;
    (void)val;
}
