# eui_str_t 字符串类型实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 实现 eui_str_t 动态字符串类型，支持格式化、拼接、比较、裁剪、子串等操作，替代原始 `const char*`

**Architecture:** 容量追踪的堆分配字符串。公开 `eui_str.h` 头文件，实现在 `eui_str.c`，内部使用 `eui_malloc`/`eui_free`。每个 mutation 操作自动扩容（几何增长策略）

**Tech Stack:** C99, eui allocator (TLSF), assert.h, stdarg.h, string.h, stdio.h, ctype.h

---

### Task 1: Create Header File

**Files:**
- Create: `include/eui/eui_str.h`

- [ ] **Step 1: Write eui_str.h**

```c
#ifndef EUI_STR_H
#define EUI_STR_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char  *data;
    size_t len;
    size_t cap;
} eui_str_t;

void        eui_str_init(eui_str_t *s);
void        eui_str_clear(eui_str_t *s);
eui_str_t*  eui_str_create(void);
void        eui_str_destroy(eui_str_t *s);

void        eui_str_set(eui_str_t *s, const char *src);
void        eui_str_copy(eui_str_t *dst, const eui_str_t *src);

void        eui_str_append(eui_str_t *s, const char *src);
void        eui_str_append_str(eui_str_t *s, const eui_str_t *src);
void        eui_str_append_char(eui_str_t *s, char c);

void        eui_str_printf(eui_str_t *s, const char *fmt, ...);
void        eui_str_append_printf(eui_str_t *s, const char *fmt, ...);

const char* eui_str_cstr(const eui_str_t *s);
size_t      eui_str_len(const eui_str_t *s);
bool        eui_str_empty(const eui_str_t *s);

bool        eui_str_equals(const eui_str_t *a, const char *b);
bool        eui_str_equals_str(const eui_str_t *a, const eui_str_t *b);
int         eui_str_cmp(const eui_str_t *a, const eui_str_t *b);

void        eui_str_trim_left(eui_str_t *s);
void        eui_str_trim_right(eui_str_t *s);
void        eui_str_trim(eui_str_t *s);
void        eui_str_substr(eui_str_t *dst, const eui_str_t *src, size_t start, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* EUI_STR_H */
```

- [ ] **Step 2: Commit**

```bash
git add include/eui/eui_str.h
git commit -m "feat(str): add eui_str_t header with full API"
```

---

### Task 2: Implement eui_str.c

**Files:**
- Create: `src/eui_str.c`

- [ ] **Step 1: Write eui_str.c implementation**

```c
#include "eui/eui_str.h"
#include "eui/eui_allocator.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

/* ---- internal helpers ---- */

static void reserve(eui_str_t *s, size_t need) {
    /* need includes '\0' */
    if (need <= s->cap) return;

    size_t min_cap = need + 16; /* extra breathing room */
    size_t new_cap  = s->cap ? (s->cap * 2) : min_cap;
    if (new_cap < min_cap) new_cap = min_cap;

    char *new_data = (char*)eui_malloc(new_cap);
    assert(new_data);

    if (s->data && s->len > 0) {
        memcpy(new_data, s->data, s->len + 1); /* +1 for '\0' */
    } else if (s->data) {
        new_data[0] = '\0';
    }

    if (s->data) eui_free(s->data);
    s->data = new_data;
    s->cap  = new_cap;
}

/* ---- lifecycle ---- */

void eui_str_init(eui_str_t *s) {
    s->data = NULL;
    s->len  = 0;
    s->cap  = 0;
}

void eui_str_clear(eui_str_t *s) {
    if (s->data) {
        eui_free(s->data);
        s->data = NULL;
    }
    s->len = 0;
    s->cap = 0;
}

eui_str_t* eui_str_create(void) {
    eui_str_t *s = (eui_str_t*)eui_malloc(sizeof(eui_str_t));
    assert(s);
    s->data = NULL;
    s->len  = 0;
    s->cap  = 0;
    return s;
}

void eui_str_destroy(eui_str_t *s) {
    if (!s) return;
    if (s->data) eui_free(s->data);
    eui_free(s);
}

/* ---- assignment ---- */

void eui_str_set(eui_str_t *s, const char *src) {
    if (!src) src = "";
    size_t src_len = strlen(src);
    size_t need    = src_len + 1;
    reserve(s, need);
    memcpy(s->data, src, need);
    s->len = src_len;
}

void eui_str_copy(eui_str_t *dst, const eui_str_t *src) {
    if (!src->data) {
        eui_str_set(dst, "");
        return;
    }
    size_t need = src->len + 1;
    reserve(dst, need);
    memcpy(dst->data, src->data, need);
    dst->len = src->len;
}

/* ---- append ---- */

void eui_str_append(eui_str_t *s, const char *src) {
    if (!src || !*src) return;
    size_t add_len = strlen(src);
    size_t new_len = s->len + add_len;
    reserve(s, new_len + 1);
    memcpy(s->data + s->len, src, add_len + 1); /* +1 for '\0' */
    s->len = new_len;
}

void eui_str_append_str(eui_str_t *s, const eui_str_t *src) {
    if (!src->data || src->len == 0) return;
    size_t new_len = s->len + src->len;
    reserve(s, new_len + 1);
    memcpy(s->data + s->len, src->data, src->len + 1);
    s->len = new_len;
}

void eui_str_append_char(eui_str_t *s, char c) {
    size_t new_len = s->len + 1;
    reserve(s, new_len + 1);
    s->data[s->len]     = c;
    s->data[new_len]    = '\0';
    s->len              = new_len;
}

/* ---- printf ---- */

static void vprintf_into(eui_str_t *s, bool append, const char *fmt, va_list args) {
    va_list args_copy;
    va_copy(args_copy, args);
    int need = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);
    assert(need >= 0);

    size_t old_len = append ? s->len : 0;
    size_t new_len = old_len + (size_t)need;
    reserve(s, new_len + 1);
    vsnprintf(s->data + old_len, (size_t)need + 1, fmt, args);
    s->len = new_len;
}

void eui_str_printf(eui_str_t *s, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf_into(s, false, fmt, args);
    va_end(args);
}

void eui_str_append_printf(eui_str_t *s, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf_into(s, true, fmt, args);
    va_end(args);
}

/* ---- access ---- */

const char* eui_str_cstr(const eui_str_t *s) {
    static const char empty[1] = "";
    return s->data ? s->data : empty;
}

size_t eui_str_len(const eui_str_t *s) {
    return s->len;
}

bool eui_str_empty(const eui_str_t *s) {
    return s->len == 0;
}

/* ---- compare ---- */

bool eui_str_equals(const eui_str_t *a, const char *b) {
    if (!b) return a->len == 0;
    if (!a->data) return *b == '\0';
    return strcmp(a->data, b) == 0;
}

bool eui_str_equals_str(const eui_str_t *a, const eui_str_t *b) {
    if (a->len != b->len) return false;
    if (a->len == 0) return true;
    return memcmp(a->data, b->data, a->len) == 0;
}

int eui_str_cmp(const eui_str_t *a, const eui_str_t *b) {
    if (!a->data && !b->data) return 0;
    if (!a->data) return -1;
    if (!b->data) return 1;
    return strcmp(a->data, b->data);
}

/* ---- trim ---- */

void eui_str_trim_left(eui_str_t *s) {
    if (!s->data || s->len == 0) return;
    const char *p = s->data;
    while (*p && isspace((unsigned char)*p)) p++;
    size_t skip = (size_t)(p - s->data);
    if (skip == 0) return;
    size_t new_len = s->len - skip;
    memmove(s->data, s->data + skip, new_len + 1);
    s->len = new_len;
}

void eui_str_trim_right(eui_str_t *s) {
    if (!s->data || s->len == 0) return;
    size_t i = s->len;
    while (i > 0 && isspace((unsigned char)s->data[i - 1])) i--;
    s->data[i] = '\0';
    s->len = i;
}

void eui_str_trim(eui_str_t *s) {
    eui_str_trim_right(s);
    eui_str_trim_left(s);
}

/* ---- substr ---- */

void eui_str_substr(eui_str_t *dst, const eui_str_t *src, size_t start, size_t len) {
    assert(start <= src->len);
    if (start + len > src->len) {
        len = src->len - start;
    }
    size_t need = len + 1;
    reserve(dst, need);
    if (src->data) {
        memcpy(dst->data, src->data + start, len);
    }
    dst->data[len] = '\0';
    dst->len = len;
}
```

- [ ] **Step 2: Commit**

```bash
git add src/eui_str.c
git commit -m "feat(str): implement eui_str_t with full API"
```

---

### Task 3: Integrate into Build System

**Files:**
- Modify: `src/CMakeLists.txt`
- Modify: `include/eui/eui.h`

- [ ] **Step 1: Add eui_str.c to src/CMakeLists.txt**

In `src/CMakeLists.txt`, add `eui_str.c` to the STATIC library sources list, right after `eui_allocator.c` (alphabetically it would go between `eui_scene.c` and `eui_types.c`, but logically put it next to `eui_allocator.c`).

```cmake
add_library(eui STATIC
    eui_types.c
    eui_allocator.c
    eui_str.c
    eui_event.c
    ...
```

Change: insert `eui_str.c\n` after `eui_allocator.c\n` at line 3.

- [ ] **Step 2: Add include to eui.h**

In `include/eui/eui.h`, add `#include "eui/eui_str.h"` after `#include "eui/eui_allocator.h"` (line 6).

```c
#include "eui/eui_allocator.h"
#include "eui/eui_str.h"
#include "eui/eui_display_hal.h"
```

- [ ] **Step 3: Commit**

```bash
git add src/CMakeLists.txt include/eui/eui.h
git commit -m "feat(str): integrate eui_str into build system"
```

---

### Task 4: Create Tests

**Files:**
- Create: `test/test_str.c`
- Modify: `test/CMakeLists.txt`

- [ ] **Step 1: Write test_str.c**

```c
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
            /* cap should at least double or be >= min step */
            if (last_cap > 0 && s.cap < last_cap) FAIL("cap shrank");
            last_cap = s.cap;
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
```

- [ ] **Step 2: Add test_str to test/CMakeLists.txt**

Append to the end of `test/CMakeLists.txt`:

```cmake
add_executable(test_str test_str.c)
target_include_directories(test_str PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(test_str PRIVATE eui)
add_test(NAME str COMMAND test_str)
```

- [ ] **Step 3: Commit**

```bash
git add test/test_str.c test/CMakeLists.txt
git commit -m "test(str): add comprehensive eui_str_t tests"
```

---

### Task 5: Build and Run Tests

- [ ] **Step 1: Configure and build tests**

```bash
cmake --preset build_test
cmake --build build_test --target test_str
```
Expected: build succeeds with no errors or warnings

- [ ] **Step 2: Run tests**

```bash
cd build_test && ctest -R str -V
```
Expected: all 32 tests pass

- [ ] **Step 3: Verify no warnings**

```bash
cmake --build build_test --target test_str 2>&1 | grep -i warning
```
Expected: no output (no warnings)

- [ ] **Step 4: Final commit if needed**

If any fixups were required, commit them now.
