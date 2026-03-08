/**
 * Bitemu - Minimal test harness (single header, zero dependencies).
 *
 * Usage:
 *   TEST(name) { ... ASSERT_EQ(a, b); ... }
 *   RUN(name);
 *   REPORT();
 */

#ifndef BITEMU_TEST_HARNESS_H
#define BITEMU_TEST_HARNESS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int _th_pass;
extern int _th_fail;
static int _th_cur_ok;
static const char *_th_cur_name;

#define TEST(name) static void test_##name(void)

#define RUN(name) do {                                  \
    _th_cur_ok = 1;                                     \
    _th_cur_name = #name;                               \
    test_##name();                                      \
    if (_th_cur_ok) {                                   \
        printf("  PASS  %s\n", #name);                  \
        _th_pass++;                                     \
    } else {                                            \
        _th_fail++;                                     \
    }                                                   \
} while (0)

#define ASSERT_TRUE(cond) do {                                              \
    if (!(cond)) {                                                          \
        printf("  FAIL  %s  (%s:%d) expected TRUE: %s\n",                   \
               _th_cur_name, __FILE__, __LINE__, #cond);                    \
        _th_cur_ok = 0;                                                     \
        return;                                                             \
    }                                                                       \
} while (0)

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

#define ASSERT_EQ(a, b) do {                                                \
    long long _a = (long long)(a), _b = (long long)(b);                     \
    if (_a != _b) {                                                         \
        printf("  FAIL  %s  (%s:%d) %s == %lld, expected %s == %lld\n",     \
               _th_cur_name, __FILE__, __LINE__, #a, _a, #b, _b);          \
        _th_cur_ok = 0;                                                     \
        return;                                                             \
    }                                                                       \
} while (0)

#define ASSERT_NEQ(a, b) do {                                               \
    long long _a = (long long)(a), _b = (long long)(b);                     \
    if (_a == _b) {                                                         \
        printf("  FAIL  %s  (%s:%d) %s should != %s (both %lld)\n",        \
               _th_cur_name, __FILE__, __LINE__, #a, #b, _a);              \
        _th_cur_ok = 0;                                                     \
        return;                                                             \
    }                                                                       \
} while (0)

#define ASSERT_RANGE(val, lo, hi) do {                                      \
    long long _v = (long long)(val);                                        \
    if (_v < (long long)(lo) || _v > (long long)(hi)) {                     \
        printf("  FAIL  %s  (%s:%d) %s == %lld, expected [%lld..%lld]\n",   \
               _th_cur_name, __FILE__, __LINE__, #val, _v,                  \
               (long long)(lo), (long long)(hi));                           \
        _th_cur_ok = 0;                                                     \
        return;                                                             \
    }                                                                       \
} while (0)

#define SUITE(name) printf("\n── %s ──\n", name)

#define REPORT() do {                                       \
    printf("\n════════════════════════════════\n");          \
    printf("  Total: %d  Pass: %d  Fail: %d\n",            \
           _th_pass + _th_fail, _th_pass, _th_fail);       \
    printf("════════════════════════════════\n");            \
    return _th_fail ? 1 : 0;                                \
} while (0)

#endif /* BITEMU_TEST_HARNESS_H */
