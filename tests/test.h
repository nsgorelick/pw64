/*
 * Minimal test harness for PicWorks (pw64).
 * No external test framework — run via: make -C tests check
 */
#ifndef PW64_TEST_H
#define PW64_TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int test_failures;
extern int test_runs;
extern const char *test_current;

#define TEST(name) static void name(void); static void name##_wrap(void) { \
    test_current = #name; test_runs++; name(); } static void name(void)

#define RUN_TEST(name) do { test_current = #name; name##_wrap(); } while (0)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", test_current, __LINE__, #cond); \
        test_failures++; \
        return; \
    } \
} while (0)

#define ASSERT_EQ_INT(a, b) do { \
    int _a = (a); int _b = (b); \
    if (_a != _b) { \
        fprintf(stderr, "FAIL %s:%d: %s (%d != %d)\n", \
            test_current, __LINE__, #a " == " #b, _a, _b); \
        test_failures++; \
        return; \
    } \
} while (0)

#define ASSERT_NEAR(a, b, eps) do { \
    double _a = (double)(a); double _b = (double)(b); double _e = (double)(eps); \
    if (_a < _b - _e || _a > _b + _e) { \
        fprintf(stderr, "FAIL %s:%d: %s (%g != %g +/- %g)\n", \
            test_current, __LINE__, #a " ~= " #b, _a, _b, _e); \
        test_failures++; \
        return; \
    } \
} while (0)

#define ASSERT_MEM_EQ(a, b, n) do { \
    if (memcmp((a), (b), (size_t)(n)) != 0) { \
        fprintf(stderr, "FAIL %s:%d: memcmp %s\n", \
            test_current, __LINE__, #a " vs " #b); \
        test_failures++; \
        return; \
    } \
} while (0)

void test_report(void);

#endif                          /* PW64_TEST_H */
