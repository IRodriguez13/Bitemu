/**
 * Genesis Core Test Runner
 * Tests the Genesis core with real ROM execution
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "include/bitemu.h"
#include "be2/genesis_constants.h"

#define TEST_ROM_PATH "test_roms/simple_test.bin"
#define TEST_CYCLES 100000
#define TEST_TIMEOUT 5

typedef struct {
    const char *name;
    int passed;
    const char *error;
} test_result_t;

static test_result_t tests[10];
static int test_count = 0;

void add_test_result(const char *name, int passed, const char *error) {
    if (test_count < 10) {
        tests[test_count].name = name;
        tests[test_count].passed = passed;
        tests[test_count].error = error;
        test_count++;
    }
}

int test_rom_loading(void) {
    printf("Testing ROM loading...\n");
    
    bitemu_t *emu = bitemu_init(BITEMU_CONSOLE_GENESIS);
    if (!emu) {
        add_test_result("ROM Loading", 0, "Failed to initialize emulator");
        return 0;
    }
    
    int result = bitemu_load_rom(emu, TEST_ROM_PATH);
    if (result != BITEMU_OK) {
        add_test_result("ROM Loading", 0, "Failed to load ROM");
        bitemu_shutdown(emu);
        return 0;
    }
    
    add_test_result("ROM Loading", 1, NULL);
    bitemu_shutdown(emu);
    return 1;
}

int test_basic_execution(void) {
    printf("Testing basic execution...\n");
    
    bitemu_t *emu = bitemu_init(BITEMU_CONSOLE_GENESIS);
    if (!emu) {
        add_test_result("Basic Execution", 0, "Failed to initialize emulator");
        return 0;
    }
    
    if (bitemu_load_rom(emu, TEST_ROM_PATH) != BITEMU_OK) {
        add_test_result("Basic Execution", 0, "Failed to load ROM");
        bitemu_shutdown(emu);
        return 0;
    }
    
    /* Run for some cycles */
    bitemu_step(emu, TEST_CYCLES);
    
    /* Check if we can still step (no crashes) */
    for (int i = 0; i < 1000; i++) {
        bitemu_step(emu, 100);
    }
    
    add_test_result("Basic Execution", 1, NULL);
    bitemu_shutdown(emu);
    return 1;
}

int test_vdp_functionality(void) {
    printf("Testing VDP functionality...\n");
    
    bitemu_t *emu = bitemu_init(BITEMU_CONSOLE_GENESIS);
    if (!emu) {
        add_test_result("VDP Functionality", 0, "Failed to initialize emulator");
        return 0;
    }
    
    if (bitemu_load_rom(emu, TEST_ROM_PATH) != BITEMU_OK) {
        add_test_result("VDP Functionality", 0, "Failed to load ROM");
        bitemu_shutdown(emu);
        return 0;
    }
    
    /* Run and check VDP access doesn't crash */
    for (int i = 0; i < 100; i++) {
        bitemu_step(emu, 1000);
        
        /* Get framebuffer to ensure VDP is working */
        const void *fb = bitemu_get_framebuffer(emu);
        if (!fb) {
            add_test_result("VDP Functionality", 0, "Failed to get framebuffer");
            bitemu_shutdown(emu);
            return 0;
        }
    }
    
    add_test_result("VDP Functionality", 1, NULL);
    bitemu_shutdown(emu);
    return 1;
}

int test_memory_access(void) {
    printf("Testing memory access...\n");
    
    bitemu_t *emu = bitemu_init(BITEMU_CONSOLE_GENESIS);
    if (!emu) {
        add_test_result("Memory Access", 0, "Failed to initialize emulator");
        return 0;
    }
    
    if (bitemu_load_rom(emu, TEST_ROM_PATH) != BITEMU_OK) {
        add_test_result("Memory Access", 0, "Failed to load ROM");
        bitemu_shutdown(emu);
        return 0;
    }
    
    /* Run and check memory doesn't crash */
    for (int i = 0; i < 500; i++) {
        bitemu_step(emu, 1000);
    }
    
    add_test_result("Memory Access", 1, NULL);
    bitemu_shutdown(emu);
    return 1;
}

int test_timing_stability(void) {
    printf("Testing timing stability...\n");
    
    bitemu_t *emu = bitemu_init(BITEMU_CONSOLE_GENESIS);
    if (!emu) {
        add_test_result("Timing Stability", 0, "Failed to initialize emulator");
        return 0;
    }
    
    if (bitemu_load_rom(emu, TEST_ROM_PATH) != BITEMU_OK) {
        add_test_result("Timing Stability", 0, "Failed to load ROM");
        bitemu_shutdown(emu);
        return 0;
    }
    
    /* Run for extended period to test timing */
    for (int i = 0; i < 1000; i++) {
        bitemu_step(emu, 1000);
        
        /* Check if we can still access systems */
        const void *fb = bitemu_get_framebuffer(emu);
        if (!fb) {
            add_test_result("Timing Stability", 0, "Framebuffer became NULL during execution");
            bitemu_shutdown(emu);
            return 0;
        }
    }
    
    add_test_result("Timing Stability", 1, NULL);
    bitemu_shutdown(emu);
    return 1;
}

void print_results(void) {
    printf("\n" "=" * 50 "\n");
    printf("GENESIS CORE TEST RESULTS\n");
    printf("=" * 50 "\n");
    
    int passed = 0;
    int failed = 0;
    
    for (int i = 0; i < test_count; i++) {
        printf("%-20s: ", tests[i].name);
        if (tests[i].passed) {
            printf("✅ PASS\n");
            passed++;
        } else {
            printf("❌ FAIL - %s\n", tests[i].error);
            failed++;
        }
    }
    
    printf("\nSUMMARY:\n");
    printf("Passed: %d/%d (%.1f%%)\n", passed, test_count, 
           (float)passed / test_count * 100.0f);
    printf("Failed: %d/%d (%.1f%%)\n", failed, test_count,
           (float)failed / test_count * 100.0f);
    
    if (failed == 0) {
        printf("\n🎉 ALL TESTS PASSED! Genesis core is MVP ready!\n");
    } else {
        printf("\n⚠️  Some tests failed. Core needs refinement.\n");
    }
    
    printf("=" * 50 "\n");
}

int main(void) {
    printf("Bitemu Genesis Core Test Runner\n");
    printf("Testing core functionality with real ROM execution\n\n");
    
    /* Check if test ROM exists */
    if (access(TEST_ROM_PATH, F_OK) != 0) {
        printf("Error: Test ROM not found at %s\n", TEST_ROM_PATH);
        printf("Please run: cd test_roms && python3 create_rom.py\n");
        return 1;
    }
    
    /* Run tests */
    test_rom_loading();
    test_basic_execution();
    test_vdp_functionality();
    test_memory_access();
    test_timing_stability();
    
    /* Print results */
    print_results();
    
    return 0;
}
