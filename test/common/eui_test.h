#ifndef EUI_TEST_H
#define EUI_TEST_H

#include <stdio.h>
#include "eui/eui_allocator.h"

#define EUI_TEST_POOL_SIZE 65536

static int tests_run = 0;
static int tests_passed = 0;

static inline void eui_test_init(void) {
    static uint8_t pool[EUI_TEST_POOL_SIZE];
    static int inited = 0;
    if (!inited) {
        eui_allocator_init_tlsf(pool, EUI_TEST_POOL_SIZE);
        inited = 1;
    }
}

#define TEST(name) do { printf("  %s... ", name); tests_run++; } while(0)
#define PASS()     do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(msg)  do { printf("FAIL: %s\n", msg); return; } while(0)

static inline int eui_test_summary(void) {
    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}

#endif
