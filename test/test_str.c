#include "eui/eui_str.h"
#include "eui/eui_allocator.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define POOL_SIZE 65536
static uint8_t mem_pool[POOL_SIZE];

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { tests_run++; printf("  %s... ", name); } while(0)
#define PASS() do { tests_passed++; printf("PASS\n"); return; } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); return; } while(0)

/* --------- empty string --------- */

static void test_empty_init(void) {
    TEST("empty init");
    eui_str_t s;
    eui_str_init(&s);
    if (eui_str_len(&s) != 0) FAIL("len not 0");
    if (!eui_str_empty(&s))  FAIL("not empty");
    if (strcmp(eui_str_cstr(&s), "") != 0) FAIL("cstr not empty");
    if (s.data != NULL) FAIL("data not NULL");
    eui_str_clear(&s);
    PASS();
}

static void test_zero_init_works(void) {
    TEST("zero init {0} works");
    eui_str_t s = {0};
    if (eui_str_len(&s) != 0) FAIL("len not 0");
    if (strcmp(eui_str_cstr(&s), "") != 0) FAIL("cstr not empty");
    eui_str_set(&s, "hello");
    if (strcmp(eui_str_cstr(&s), "hello") != 0) FAIL("set after zero init");
    eui_str_clear(&s);
    PASS();
}

/* --------- set / copy --------- */

static void test_set(void) {
    TEST("set from C string");
    eui_str_t s;
    eui_str_init(&s);
    eui_str_set(&s, "hello");
    if (eui_str_len(&s) != 5) FAIL("len wrong");
    if (strcmp(eui_str_cstr(&s), "hello") != 0) FAIL("content wrong");
    if (s.cap < 6) FAIL("cap too small");
    eui_str_set(&s, "abc");
    if (strcmp(eui_str_cstr(&s), "abc") != 0) FAIL("overwrite failed");
    eui_str_clear(&s);
    PASS();
}

static void test_set_null(void) {
    TEST("set NULL -> empty");
    eui_str_t s;
    eui_str_init(&s);
    eui_str_set(&s, NULL);
    if (strcmp(eui_str_cstr(&s), "") != 0) FAIL("not empty");
    eui_str_clear(&s);
    PASS();
}

static void test_copy(void) {
    TEST("copy from another eui_str_t");
    eui_str_t a, b;
    eui_str_init(&a);
    eui_str_init(&b);
    eui_str_set(&a, "source");
    eui_str_copy(&b, &a);
    if (strcmp(eui_str_cstr(&b), "source") != 0) FAIL("copy failed");
    if (eui_str_len(&b) != 6) FAIL("len wrong");
    /* modify original, copy should be independent */
    eui_str_set(&a, "modified");
    if (strcmp(eui_str_cstr(&b), "source") != 0) FAIL("not independent copy");
    eui_str_clear(&a);
    eui_str_clear(&b);
    PASS();
}

static void test_copy_empty(void) {
    TEST("copy empty string");
    eui_str_t a, b;
    eui_str_init(&a);
    eui_str_init(&b);
    /* a is empty */
    eui_str_copy(&b, &a);
    if (strcmp(eui_str_cstr(&b), "") != 0) FAIL("not empty");
    eui_str_clear(&a);
    eui_str_clear(&b);
    PASS();
}

/* --------- append --------- */

static void test_append_cstr(void) {
    TEST("append C string");
    eui_str_t s;
    eui_str_init(&s);
    eui_str_set(&s, "hello");
    eui_str_append(&s, " world");
    if (strcmp(eui_str_cstr(&s), "hello world") != 0) FAIL("append failed");
    if (eui_str_len(&s) != 11) FAIL("len wrong");
    eui_str_clear(&s);
    PASS();
}

static void test_append_null(void) {
    TEST("append NULL does nothing");
    eui_str_t s;
    eui_str_init(&s);
    eui_str_set(&s, "hello");
    size_t old_len = eui_str_len(&s);
    eui_str_append(&s, NULL);
    if (eui_str_len(&s) != old_len) FAIL("len changed");
    if (strcmp(eui_str_cstr(&s), "hello") != 0) FAIL("content changed");
    eui_str_clear(&s);
    PASS();
}

static void test_append_empty(void) {
    TEST("append empty string does nothing");
    eui_str_t s;
    eui_str_init(&s);
    eui_str_set(&s, "hello");
    eui_str_append(&s, "");
    if (eui_str_len(&s) != 5) FAIL("len changed");
    eui_str_clear(&s);
    PASS();
}

static void test_append_str(void) {
    TEST("append another eui_str_t");
    eui_str_t a, b;
    eui_str_init(&a);
    eui_str_init(&b);
    eui_str_set(&a, "hello ");
    eui_str_set(&b, "world");
    eui_str_append_str(&a, &b);
    if (strcmp(eui_str_cstr(&a), "hello world") != 0) FAIL("append_str failed");
    eui_str_clear(&a);
    eui_str_clear(&b);
    PASS();
}

static void test_append_char(void) {
    TEST("append single char");
    eui_str_t s;
    eui_str_init(&s);
    eui_str_append_char(&s, 'a');
    eui_str_append_char(&s, 'b');
    eui_str_append_char(&s, 'c');
    if (strcmp(eui_str_cstr(&s), "abc") != 0) FAIL("append_char failed");
    if (eui_str_len(&s) != 3) FAIL("len wrong");
    eui_str_clear(&s);
    PASS();
}

static void test_append_to_empty(void) {
    TEST("append to empty string");
    eui_str_t s;
    eui_str_init(&s);
    eui_str_append(&s, "hello");
    if (strcmp(eui_str_cstr(&s), "hello") != 0) FAIL("append to empty");
    eui_str_clear(&s);
    PASS();
}

/* --------- printf --------- */

static void test_printf(void) {
    TEST("printf");
    eui_str_t s;
    eui_str_init(&s);
    eui_str_printf(&s, "value=%d", 42);
    if (strcmp(eui_str_cstr(&s), "value=42") != 0) FAIL("printf failed");
    eui_str_clear(&s);
    PASS();
}

static void test_printf_overwrite(void) {
    TEST("printf overwrites");
    eui_str_t s;
    eui_str_init(&s);
    eui_str_set(&s, "old content");
    eui_str_printf(&s, "new %s", "value");
    if (strcmp(eui_str_cstr(&s), "new value") != 0) FAIL("overwrite failed");
    eui_str_clear(&s);
    PASS();
}

static void test_append_printf(void) {
    TEST("append_printf");
    eui_str_t s;
    eui_str_init(&s);
    eui_str_set(&s, "hello ");
    eui_str_append_printf(&s, "%s %d", "world", 2026);
    if (strcmp(eui_str_cstr(&s), "hello world 2026") != 0) FAIL("append_printf failed");
    eui_str_clear(&s);
    PASS();
}

/* --------- growth --------- */

static void test_growth_geometric(void) {
    TEST("geometric growth");
    eui_str_t s;
    eui_str_init(&s);

    /* build a long string char by char, check cap grows geometrically */
    size_t last_cap = s.cap;
    for (int i = 0; i < 200; i++) {
        eui_str_append_char(&s, 'a' + (char)(i % 26));
        if (s.cap > last_cap) {
            last_cap = s.cap;
        } else if (s.cap < last_cap) {
            FAIL("cap shrank");
        }
    }
    if (eui_str_len(&s) != 200) FAIL("len wrong after growth");
    if (s.cap < 201) FAIL("cap too small for content");

    eui_str_clear(&s);
    PASS();
}

/* --------- compare --------- */

static void test_equals_cstr(void) {
    TEST("equals C string");
    eui_str_t s;
    eui_str_init(&s);
    eui_str_set(&s, "hello");
    if (!eui_str_equals(&s, "hello")) FAIL("should be equal");
    if (eui_str_equals(&s, "world"))   FAIL("should not be equal");
    if (eui_str_equals(&s, "helloo"))  FAIL("should not be equal (longer)");
    if (eui_str_equals(&s, "hell"))    FAIL("should not be equal (shorter)");
    if (eui_str_equals(&s, ""))       FAIL("should not equal empty (s is 'hello')");
    eui_str_clear(&s);
    PASS();
}

static void test_equals_empty(void) {
    TEST("equals with empty strings");
    eui_str_t s;
    eui_str_init(&s);
    if (!eui_str_equals(&s, ""))    FAIL("empty should equal empty str");
    if (!eui_str_equals(&s, NULL))  FAIL("empty should equal NULL");
    eui_str_clear(&s);
    PASS();
}

static void test_equals_str(void) {
    TEST("equals_str between two eui_str_t");
    eui_str_t a, b, c;
    eui_str_init(&a); eui_str_init(&b); eui_str_init(&c);
    eui_str_set(&a, "same");
    eui_str_set(&b, "same");
    eui_str_set(&c, "different");
    if (!eui_str_equals_str(&a, &b)) FAIL("should be equal");
    if (eui_str_equals_str(&a, &c))  FAIL("should not be equal");
    eui_str_clear(&a); eui_str_clear(&b); eui_str_clear(&c);
    PASS();
}

static void test_cmp(void) {
    TEST("strcmp ordering");
    eui_str_t a, b;
    eui_str_init(&a); eui_str_init(&b);
    eui_str_set(&a, "apple");
    eui_str_set(&b, "banana");
    int r = eui_str_cmp(&a, &b);
    if (r >= 0) FAIL("apple should be < banana");
    if (eui_str_cmp(&a, &a) != 0) FAIL("same should be 0");
    eui_str_clear(&a); eui_str_clear(&b);
    PASS();
}

static void test_cmp_empty(void) {
    TEST("cmp with empty strings");
    eui_str_t a, b;
    eui_str_init(&a); eui_str_init(&b);
    /* both empty */
    if (eui_str_cmp(&a, &b) != 0) FAIL("empty vs empty should be 0");
    eui_str_set(&a, "a");
    if (eui_str_cmp(&a, &b) <= 0) FAIL("non-empty > empty");
    eui_str_clear(&a); eui_str_clear(&b);
    PASS();
}

/* --------- trim --------- */

static void test_trim_left(void) {
    TEST("trim left");
    eui_str_t s;
    eui_str_init(&s);
    eui_str_set(&s, "  \t hello");
    eui_str_trim_left(&s);
    if (strcmp(eui_str_cstr(&s), "hello") != 0) FAIL("trim_left failed");
    eui_str_clear(&s);
    PASS();
}

static void test_trim_right(void) {
    TEST("trim right");
    eui_str_t s;
    eui_str_init(&s);
    eui_str_set(&s, "hello  \t ");
    eui_str_trim_right(&s);
    if (strcmp(eui_str_cstr(&s), "hello") != 0) FAIL("trim_right failed");
    eui_str_clear(&s);
    PASS();
}

static void test_trim(void) {
    TEST("trim both sides");
    eui_str_t s;
    eui_str_init(&s);
    eui_str_set(&s, "  hello  ");
    eui_str_trim(&s);
    if (strcmp(eui_str_cstr(&s), "hello") != 0) FAIL("trim failed");
    eui_str_clear(&s);
    PASS();
}

static void test_trim_all_whitespace(void) {
    TEST("trim all whitespace -> empty");
    eui_str_t s;
    eui_str_init(&s);
    eui_str_set(&s, "   ");
    eui_str_trim(&s);
    if (eui_str_len(&s) != 0) FAIL("should be empty");
    if (strcmp(eui_str_cstr(&s), "") != 0) FAIL("cstr should be empty");
    eui_str_clear(&s);
    PASS();
}

static void test_trim_empty_string(void) {
    TEST("trim empty string");
    eui_str_t s;
    eui_str_init(&s);
    eui_str_trim(&s);
    if (eui_str_len(&s) != 0) FAIL("len should be 0");
    eui_str_clear(&s);
    PASS();
}

/* --------- substr --------- */

static void test_substr(void) {
    TEST("substr normal");
    eui_str_t src, dst;
    eui_str_init(&src); eui_str_init(&dst);
    eui_str_set(&src, "hello world");
    eui_str_substr(&dst, &src, 6, 5);
    if (strcmp(eui_str_cstr(&dst), "world") != 0) FAIL("substr failed");
    eui_str_clear(&src); eui_str_clear(&dst);
    PASS();
}

static void test_substr_len_clamp(void) {
    TEST("substr len auto clamp");
    eui_str_t src, dst;
    eui_str_init(&src); eui_str_init(&dst);
    eui_str_set(&src, "hello");
    eui_str_substr(&dst, &src, 3, 100);
    if (strcmp(eui_str_cstr(&dst), "lo") != 0) FAIL("len not clamped");
    if (eui_str_len(&dst) != 2) FAIL("clamped len wrong");
    eui_str_clear(&src); eui_str_clear(&dst);
    PASS();
}

static void test_substr_from_start(void) {
    TEST("substr from start");
    eui_str_t src, dst;
    eui_str_init(&src); eui_str_init(&dst);
    eui_str_set(&src, "hello");
    eui_str_substr(&dst, &src, 0, 2);
    if (strcmp(eui_str_cstr(&dst), "he") != 0) FAIL("substr from 0 failed");
    eui_str_clear(&src); eui_str_clear(&dst);
    PASS();
}

static void test_substr_empty_result(void) {
    TEST("substr len=0 -> empty");
    eui_str_t src, dst;
    eui_str_init(&src); eui_str_init(&dst);
    eui_str_set(&src, "hello");
    eui_str_substr(&dst, &src, 0, 0);
    if (strcmp(eui_str_cstr(&dst), "") != 0) FAIL("should be empty");
    eui_str_clear(&src); eui_str_clear(&dst);
    PASS();
}

/* --------- clear / reuse --------- */

static void test_clear_reuse(void) {
    TEST("clear and reuse");
    eui_str_t s;
    eui_str_init(&s);
    eui_str_set(&s, "hello world long string");
    eui_str_clear(&s);
    if (s.data != NULL) FAIL("data not freed");
    if (s.len != 0) FAIL("len not 0");
    if (s.cap != 0) FAIL("cap not 0");
    /* reuse */
    eui_str_set(&s, "reused");
    if (strcmp(eui_str_cstr(&s), "reused") != 0) FAIL("reuse failed");
    eui_str_clear(&s);
    PASS();
}

/* --------- create / destroy --------- */

static void test_create_destroy(void) {
    TEST("create and destroy");
    eui_str_t *s = eui_str_create();
    if (!s) FAIL("create returned NULL");
    eui_str_set(s, "hello");
    if (strcmp(eui_str_cstr(s), "hello") != 0) FAIL("create+set failed");
    eui_str_destroy(s);
    PASS();
}

/* --------- empty --------- */

static void test_empty_check(void) {
    TEST("empty check");
    eui_str_t s;
    eui_str_init(&s);
    if (!eui_str_empty(&s)) FAIL("new should be empty");
    eui_str_set(&s, "x");
    if (eui_str_empty(&s)) FAIL("should not be empty");
    eui_str_clear(&s);
    if (!eui_str_empty(&s)) FAIL("cleared should be empty");
    PASS();
}

/* --------- multiple appends ---- */

static void test_many_appends(void) {
    TEST("many appends");
    eui_str_t s;
    eui_str_init(&s);
    for (int i = 0; i < 100; i++) {
        eui_str_append_printf(&s, "%d,", i);
    }
    if (eui_str_len(&s) < 100) FAIL("too short after many appends");
    /* verify first few chars */
    if (strncmp(eui_str_cstr(&s), "0,1,2,3,", 8) != 0) FAIL("content wrong");
    eui_str_clear(&s);
    PASS();
}

/* --------- helper to dump results ---- */

static int all_tests(void) {
    test_empty_init();
    test_zero_init_works();
    test_set();
    test_set_null();
    test_copy();
    test_copy_empty();
    test_append_cstr();
    test_append_null();
    test_append_empty();
    test_append_str();
    test_append_char();
    test_append_to_empty();
    test_printf();
    test_printf_overwrite();
    test_append_printf();
    test_growth_geometric();
    test_equals_cstr();
    test_equals_empty();
    test_equals_str();
    test_cmp();
    test_cmp_empty();
    test_trim_left();
    test_trim_right();
    test_trim();
    test_trim_all_whitespace();
    test_trim_empty_string();
    test_substr();
    test_substr_len_clamp();
    test_substr_from_start();
    test_substr_empty_result();
    test_clear_reuse();
    test_create_destroy();
    test_empty_check();
    test_many_appends();
    return (tests_passed == tests_run) ? 0 : 1;
}

int main(void) {
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);
    printf("=== eui_str Tests ===\n");
    int result = all_tests();
    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return result;
}
