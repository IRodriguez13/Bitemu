/**
 * Debug test para open bus
 */

#include <stdio.h>
#include "be2/memory.h"

// Simple test sin necesidad de console.h
struct test_impl {
    genesis_mem_t mem;
};

int main(void) {
    struct test_impl impl;
    genesis_mem_init(&impl.mem);
    
    printf("Testing open bus at 0x500000...\n");
    
    // Test byte read
    uint8_t b = genesis_mem_read8(&impl.mem, 0x500000);
    printf("Byte read: 0x%02X (b & 0xC0 = 0x%02X, expected 0x40)\n", 
           b, (b & 0xC0u));
    
    // Test word read
    uint16_t w = genesis_mem_read16(&impl.mem, 0x500002);
    printf("Word read: 0x%04X\n", w);
    printf("  High byte: 0x%02X (>>8 & 0xC0 = 0x%02X, expected 0x40)\n", 
           (w >> 8), ((w >> 8) & 0xC0u));
    printf("  Low byte: 0x%02X (& 0xC0 = 0x%02X, expected 0x40)\n", 
           (w & 0xFF), (w & 0xC0u));
    
    return 0;
}
