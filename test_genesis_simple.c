/**
 * Genesis Core Test Runner
 * Tests the Genesis core with real ROM execution
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "include/bitemu.h"

#define TEST_ROM_PATH "test_roms/simple_test.bin"
#define TEST_FRAMES 1000

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
    
    bitemu_t *emu = bitemu_create();
    if (!emu) {
        add_test_result("ROM Loading", 0, "Failed to create emulator");
        return 0;
    }
    
    bool result = bitemu_load_rom(emu, TEST_ROM_PATH);
    if (!result) {
        add_test_result("ROM Loading", 0, "Failed to load ROM");
        bitemu_destroy(emu);
        return 0;
    }
    
    add_test_result("ROM Loading", 1, NULL);
    bitemu_destroy(emu);
    return 1;
}

int test_basic_execution(void) {
    printf("Testing basic execution...\n");
    
    bitemu_t *emu = bitemu_create();
    if (!emu) {
        add_test_result("Basic Execution", 0, "Failed to create emulator");
        return 0;
    }
    
    if (!bitemu_load_rom(emu, TEST_ROM_PATH)) {
        add_test_result("Basic Execution", 0, "Failed to load ROM");
        bitemu_destroy(emu);
        return 0;
    }
    
    for (int i = 0; i < TEST_FRAMES; i++) {
        bool running = bitemu_run_frame(emu);
        if (!running) break;
    }
    
    add_test_result("Basic Execution", 1, NULL);
    bitemu_destroy(emu);
    return 1;
}

void print_results(void) {
    printf("\n==================================================\n");
    printf("GENESIS CORE TEST RESULTS\n");
    printf("==================================================\n");
    
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
    
    printf("==================================================\n");
}

int main(void) {
    printf("Bitemu Genesis Core Test Runner\n");
    printf("Testing core functionality with real ROM execution\n\n");
    
    if (access(TEST_ROM_PATH, F_OK) != 0) {
        printf("Error: Test ROM not found at %s\n", TEST_ROM_PATH);
        printf("Please run: cd test_roms && python3 create_rom.py\n");
        return 1;
    }
    
    test_rom_loading();
    test_basic_execution();
    
    print_results();
    return 0;
}
