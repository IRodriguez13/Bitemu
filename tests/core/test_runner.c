/**
 * Bitemu test runner - invokes all core test suites.
 * Build: see Makefile target 'test-core'.
 */

#include "test_harness.h"

int _th_pass;
int _th_fail;

extern void run_memory_tests(void);
extern void run_apu_tests(void);
extern void run_timer_tests(void);
extern void run_api_tests(void);

int main(void)
{
    printf("Bitemu Core Tests\n");

    run_memory_tests();
    run_apu_tests();
    run_timer_tests();
    run_api_tests();

    REPORT();
}
