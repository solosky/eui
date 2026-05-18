#include "eui/eui_allocator.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define POOL_SIZE 32768
static uint8_t mem_pool[POOL_SIZE];

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { tests_run++; printf("  %s... ", name); } while(0)
#define PASS() do { tests_passed++; printf("PASS\n"); return; } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); return; } while(0)

static void test_alloc_basic(void) {
    TEST("basic alloc/free");

    void *ptrs[100];
    for (int i = 0; i < 100; i++) {
        ptrs[i] = eui_malloc(32);
        if (!ptrs[i]) FAIL("allocation returned NULL");
        memset(ptrs[i], 0xAA, 32);
    }

    for (int i = 0; i < 100; i++) {
        memset(ptrs[i], (uint8_t)i, 32);
    }
    for (int i = 0; i < 100; i++) {
        uint8_t *p = (uint8_t*)ptrs[i];
        for (int j = 0; j < 32; j++) {
            if (p[j] != (uint8_t)i) FAIL("memory overlap detected");
        }
    }

    for (int i = 0; i < 100; i++) {
        eui_free(ptrs[i]);
    }

    PASS();
}

static void test_alloc_stats(void) {
    TEST("allocator stats");

    void *p1 = eui_malloc(64);
    void *p2 = eui_malloc(128);

    eui_allocator_stats_t stats;
    eui_allocator_get_stats(&stats);
    if (stats.alloc_count < 2) FAIL("alloc count not incremented");

    eui_free(p1);
    eui_free(p2);

    eui_allocator_get_stats(&stats);
    if (stats.free_count < 2) FAIL("free count not incremented");

    PASS();
}

static uint8_t custom_buf[1024];
static size_t custom_offset = 0;

static void* custom_alloc(size_t size, void *ctx) {
    (void)ctx;
    if (custom_offset + size > sizeof(custom_buf)) return NULL;
    void *p = custom_buf + custom_offset;
    custom_offset += size;
    return p;
}

static void custom_free(void *ptr, void *ctx) {
    (void)ptr; (void)ctx;
}

static void test_custom_allocator(void) {
    TEST("custom allocator");

    custom_offset = 0;

    eui_allocator_t custom = { custom_alloc, custom_free, NULL };
    eui_set_allocator(&custom);

    void *p = eui_malloc(64);
    if (!p) FAIL("custom alloc failed");
    if (p < (void*)custom_buf || p >= (void*)(custom_buf + sizeof(custom_buf)))
        FAIL("custom alloc returned pointer outside buffer");

    eui_free(p);

    eui_set_allocator(NULL);

    PASS();
}

int main(void) {
    printf("=== Allocator Tests ===\n");

    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);

    test_alloc_basic();
    test_alloc_stats();
    test_custom_allocator();

    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
