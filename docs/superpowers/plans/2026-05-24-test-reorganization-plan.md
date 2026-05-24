# Test Reorganization Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Reorganize 23 flat test files into modular subdirectories, extract common test utilities, merge 3 redundant test pairs, and modernize test CMake.

**Architecture:** `test/common/` provides shared macros and canvas helpers; `test/data/` isolates font fixture data; modules (allocator, event, input, str, canvas, font, view, drivers) each get a subdirectory. Two internal headers move from `src/` to `include/eui/` to eliminate `../src/` references. CMake `eui_add_test()` function replaces 120 lines of boilerplate.

**Tech Stack:** C99, CMake 3.22+, assert.h-based testing

---

## File Map

| File | Responsibility |
|---|---|
| `test/common/eui_test.h` | TEST/PASS/FAIL macros, TLSF init, summary helper |
| `test/common/eui_test_canvas.h` | Canvas create/destroy convenience wrappers |
| `test/data/` | 5 font fixture header files (moved from test/) |
| `test/allocator/test_allocator.c` | TLSF allocator unit tests |
| `test/event/test_event.c` | Event queue push/pop/overwrite tests |
| `test/input/test_input.c` | Input debounce/long-press/release tests |
| `test/str/test_str.c` | eui_str_t comprehensive tests |
| `test/canvas/test_canvas.c` | Canvas draw primitives (1bpp) |
| `test/canvas/test_canvas_16bpp.c` | Canvas draw + kerning (16bpp BMP output) |
| `test/canvas/test_canvas_render.c` | Canvas rendering integration (256x750) |
| `test/font/test_font_formats.c` | BDF + VLW + U8G2 format tests (merged) |
| `test/font/test_font_kerning.c` | U8G2 kerning data structure tests |
| `test/font/test_font_canvas.c` | Font-on-canvas draw_str tests |
| `test/font/test_font_multiline.c` | Multiline layout, ellipsis, clipping |
| `test/font/test_font_vs_u8g2.c` | Comparison tests vs real u8g2 library |
| `test/font/test_font_wqy13_render.c` | wqy13 font rendering test |
| `test/font/test_font_real_u8g2_render.c` | Profont10 + wqy12 real data render (merged) |
| `test/font/test_wqy13_kerning.c` | wqy13 kerning + u8g2 comparison (merged) |
| `test/view/test_view.c` | View lifecycle + dispatcher tests |
| `test/view/test_widget.c` | Widget focus chain + bridge tests |
| `test/view/test_view_transition.c` | Transition uncovered-pixel coverage |
| `test/view/test_view_transition_render.c` | Raylib integration render test |
| `test/drivers/test_drivers.c` | Display/input driver init tests |
| `test/CMakeLists.txt` | Rewritten with eui_add_test() function |
| `include/eui/eui_font_internal.h` | Moved from src/ — internal BDF/VLW/U8G2 helper decls |
| `include/eui/eui_font_u8g2_internal.h` | Moved from src/ — u8g2 glyph decode structs |

> **Task execution order is critical:** Tasks MUST be executed in sequential order. Tasks 6-9 update test files to use `#include "eui/eui_font_internal.h"` which depends on Task 5 (moving those headers from `src/` to `include/eui/`).

---

### Task 1: Create directory structure

**Files:**
- Create: `test/common/`, `test/data/`, `test/allocator/`, `test/event/`, `test/input/`, `test/str/`, `test/canvas/`, `test/font/`, `test/view/`, `test/drivers/`

- [ ] **Step 1: Create all subdirectories**

```bash
mkdir -p test/common test/data test/allocator test/event test/input test/str test/canvas test/font test/view test/drivers
```

- [ ] **Step 2: Commit**

```bash
git add test/common/ test/data/ test/allocator/ test/event/ test/input/ test/str/ test/canvas/ test/font/ test/view/ test/drivers/
git commit -m "chore: create test module subdirectories"
```

---

### Task 2: Create test/common/eui_test.h

**Files:**
- Create: `test/common/eui_test.h`

- [ ] **Step 1: Write eui_test.h**

```c
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
```

- [ ] **Step 2: Commit**

```bash
git add test/common/eui_test.h
git commit -m "feat(test): add shared test macros and TLSF init header"
```

---

### Task 3: Create test/common/eui_test_canvas.h

**Files:**
- Create: `test/common/eui_test_canvas.h`

- [ ] **Step 1: Write eui_test_canvas.h**

```c
#ifndef EUI_TEST_CANVAS_H
#define EUI_TEST_CANVAS_H

#include "eui/eui_canvas.h"
#include "eui/eui_display_drv.h"
#include "eui/eui_font_builtin.h"

static inline eui_canvas_t *eui_test_canvas_new(eui_display_drv_t *drv) {
    return eui_canvas_create(drv);
}

static inline void eui_test_canvas_free(eui_canvas_t *c) {
    eui_canvas_destroy(c);
}

#endif
```

- [ ] **Step 2: Commit**

```bash
git add test/common/eui_test_canvas.h
git commit -m "feat(test): add canvas convenience wrapper header"
```

---

### Task 4: Move test data files to test/data/

**Files:**
- Move: `test/test_vlw_font.h` → `test/data/test_vlw_font.h`
- Move: `test/test_u8g2_font.h` → `test/data/test_u8g2_font.h`
- Move: `test/test_u8g2_profont10_data.h` → `test/data/test_u8g2_profont10_data.h`
- Move: `test/test_u8g2_wqy12_ch1_data.h` → `test/data/test_u8g2_wqy12_ch1_data.h`
- Move: `test/test_font_kerning.h` → `test/data/test_font_kerning.h`

- [ ] **Step 1: Move data files**

```bash
git mv test/test_vlw_font.h          test/data/test_vlw_font.h
git mv test/test_u8g2_font.h          test/data/test_u8g2_font.h
git mv test/test_u8g2_profont10_data.h test/data/test_u8g2_profont10_data.h
git mv test/test_u8g2_wqy12_ch1_data.h test/data/test_u8g2_wqy12_ch1_data.h
git mv test/test_font_kerning.h       test/data/test_font_kerning.h
```

- [ ] **Step 2: Commit**

```bash
git commit -m "refactor(test): move font data headers to test/data/"
```

---

### Task 5: Move internal headers to include/eui/

**Files:**
- Move: `src/eui_font_internal.h` → `include/eui/eui_font_internal.h`
- Move: `src/eui_font_u8g2_internal.h` → `include/eui/eui_font_u8g2_internal.h`
- Modify: `src/eui_font.c`, `src/eui_font_vlw.c`, `src/eui_font_u8g2.c`, `src/eui_font_wqy13.c`, `src/eui_canvas.c`

- [ ] **Step 1: Move headers**

```bash
git mv src/eui_font_internal.h       include/eui/eui_font_internal.h
git mv src/eui_font_u8g2_internal.h  include/eui/eui_font_u8g2_internal.h
```

- [ ] **Step 2: Update includes in src/ files**

In each source file, change the local include to the public path:

| File | Old include | New include |
|---|---|---|
| `src/eui_font.c:2` | `#include "eui_font_internal.h"` | `#include "eui/eui_font_internal.h"` |
| `src/eui_font_vlw.c:2` | `#include "eui_font_internal.h"` | `#include "eui/eui_font_internal.h"` |
| `src/eui_font_wqy13.c:3` | `#include "eui_font_internal.h"` | `#include "eui/eui_font_internal.h"` |
| `src/eui_font_u8g2.c:2` | `#include "eui_font_u8g2_internal.h"` | `#include "eui/eui_font_u8g2_internal.h"` |
| `src/eui_canvas.c:8` | `#include "eui_font_u8g2_internal.h"` | `#include "eui/eui_font_u8g2_internal.h"` |

- [ ] **Step 3: Commit**

```bash
git add include/eui/ src/
git commit -m "refactor: move internal font headers to include/eui/, update src includes"
```

---

### Task 6: Move simple test files (allocator, event, input, str, drivers)

**Files:**
- Move: `test/test_allocator.c` → `test/allocator/test_allocator.c`
- Move: `test/test_event.c` → `test/event/test_event.c`
- Move: `test/test_input.c` → `test/input/test_input.c`
- Move: `test/test_str.c` → `test/str/test_str.c`
- Move: `test/test_drivers.c` → `test/drivers/test_drivers.c`

- [ ] **Step 1: Move files with git mv**

```bash
git mv test/test_allocator.c test/allocator/test_allocator.c
git mv test/test_event.c     test/event/test_event.c
git mv test/test_input.c     test/input/test_input.c
git mv test/test_str.c       test/str/test_str.c
git mv test/test_drivers.c   test/drivers/test_drivers.c
```

- [ ] **Step 2: Update test_allocator.c — replace macros and TLSF init with common header**

Read `test/allocator/test_allocator.c`.

Remove lines 6-14 (POOL_SIZE, mem_pool, tests_run, tests_passed, TEST/PASS/FAIL macros).

Replace with `#include "common/eui_test.h"` after the eui header include.

Replace `main()` body: remove `eui_allocator_init_tlsf(mem_pool, POOL_SIZE);`, add `eui_test_init();`.

Change printf summary to `return eui_test_summary();`.

Final file:

```c
#include "eui/eui_allocator.h"
#include "common/eui_test.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

static void test_alloc_basic(void) {
    TEST("basic alloc/free");
    void *ptrs[100];
    for (int i = 0; i < 100; i++) {
        ptrs[i] = eui_malloc(32);
        if (!ptrs[i]) FAIL("allocation returned NULL");
        memset(ptrs[i], 0xAA, 32);
    }
    for (int i = 0; i < 100; i++)
        memset(ptrs[i], (uint8_t)i, 32);
    for (int i = 0; i < 100; i++) {
        uint8_t *p = (uint8_t*)ptrs[i];
        for (int j = 0; j < 32; j++)
            if (p[j] != (uint8_t)i) FAIL("memory overlap detected");
    }
    for (int i = 0; i < 100; i++)
        eui_free(ptrs[i]);
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
static void custom_free(void *ptr, void *ctx) { (void)ptr; (void)ctx; }

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
    eui_test_init();
    printf("=== Allocator Tests ===\n");
    test_alloc_basic();
    test_alloc_stats();
    test_custom_allocator();
    return eui_test_summary();
}
```

- [ ] **Step 3: Update test_event.c — replace macros with common header**

Remove lines 5-10 (TEST/PASS/FAIL macros, tests_run, tests_passed variables).

Replace with `#include "common/eui_test.h"` after eui header.

Change `main()`: replace `printf("\n%d/%d tests passed\n"...); return tests_passed == tests_run ? 0 : 1;` with `return eui_test_summary();`.

Keep all test functions unchanged.

- [ ] **Step 4: Update test_input.c — replace macros with common header**

Remove lines 5-9 (tests_run, tests_passed, TEST/PASS/FAIL macros).

Replace with `#include "common/eui_test.h"` after eui header.

Change `main()`: replace summary print + return with `return eui_test_summary();`.

Keep all test functions unchanged.

- [ ] **Step 5: Update test_str.c — replace macros and TLSF init with common header**

Remove lines 7-15 (POOL_SIZE, mem_pool, tests_run, tests_passed, TEST/PASS/FAIL macros).

Replace with `#include "common/eui_test.h"` after eui headers.

In `main()`: replace `eui_allocator_init_tlsf(mem_pool, POOL_SIZE);` with `eui_test_init();`.

Replace printf summary + return with `return eui_test_summary();`.

Remove `all_tests()` wrapper — call test functions directly from `main()`.

Keep all test functions unchanged.

- [ ] **Step 6: Update test_drivers.c — replace macros with common header**

Remove lines 16-20 (tests_run, tests_passed, TEST/PASS/FAIL macros).

Replace with `#include "common/eui_test.h"` after eui headers.

Change `main()`: replace summary print + return with `return eui_test_summary();`.

Keep all test functions unchanged.

- [ ] **Step 7: Commit**

```bash
git add test/allocator/test_allocator.c test/event/test_event.c test/input/test_input.c test/str/test_str.c test/drivers/test_drivers.c
git commit -m "refactor(test): move simple tests to module dirs, use common header"
```

---

### Task 7: Move and update canvas test files

**Files:**
- Move: `test/test_canvas.c` → `test/canvas/test_canvas.c`
- Move: `test/test_canvas_16bpp.c` → `test/canvas/test_canvas_16bpp.c`
- Move: `test/test_canvas_render.c` → `test/canvas/test_canvas_render.c`

- [ ] **Step 1: Move files with git mv**

```bash
git mv test/test_canvas.c        test/canvas/test_canvas.c
git mv test/test_canvas_16bpp.c  test/canvas/test_canvas_16bpp.c
git mv test/test_canvas_render.c test/canvas/test_canvas_render.c
```

- [ ] **Step 2: Update test_canvas.c**

Remove lines 8-14 (POOL_SIZE, mem_pool, tests_run, tests_passed, TEST/PASS/FAIL).

Add `#include "common/eui_test.h"` after eui includes.

Remove mock write_buffer + count_pixels + mock_display definitions (these stay — they are module-specific).

In `main()`: replace `eui_allocator_init_tlsf(mem_pool, POOL_SIZE);` with `eui_test_init();`.

Replace printf summary + return with `return eui_test_summary();`.

- [ ] **Step 3: Update test_canvas_16bpp.c**

Remove lines 37-38 (POOL_SIZE, mem_pool).

Add `#include "common/eui_test.h"` after eui includes (file has no tests_run/tests_passed — it's a BMP output test).

Change `#include "eui_font_internal.h"` to `#include "eui/eui_font_internal.h"` (it already exists in src/; will move in Task 11).

Change `#include "test_font_kerning.h"` to `#include "data/test_font_kerning.h"`.

Remove `eui_allocator_init_tlsf(mem_pool, POOL_SIZE);` in main, add `eui_test_init();`.

Note: this file has custom `printf`-style pass/fail, not TEST/PASS/FAIL macros. Leave its custom reporting as-is but add `eui_test_init()`.

- [ ] **Step 4: Update test_canvas_render.c**

Remove lines 12-16 (POOL_SIZE, mem_pool conditional).

Add `#include "common/eui_test.h"` — the file already has TEST/PASS/FAIL macros; remove the local definitions (lines 62-64: TESTS_RUN, TEST, PASS, FAIL definitions... actually let me check if this file has its own macros).

Wait — this file defines its own `tests_run`, `tests_passed`, `TEST`, `PASS`, `FAIL` at lines 61-64. Remove those and use `#include "common/eui_test.h"`.

In `main()`: replace TLSF init with `eui_test_init();`.

Replace printf summary + return with `return eui_test_summary();`.

- [ ] **Step 5: Commit**

```bash
git add test/canvas/
git commit -m "refactor(test): move canvas tests to module dir, use common header"
```

---

### Task 8: Move and update view test files

**Files:**
- Move: `test/test_view.c` → `test/view/test_view.c`
- Move: `test/test_widget.c` → `test/view/test_widget.c`
- Move: `test/test_view_transition.c` → `test/view/test_view_transition.c`
- Move: `test/test_view_transition_render.c` → `test/view/test_view_transition_render.c`

- [ ] **Step 1: Move files with git mv**

```bash
git mv test/test_view.c                    test/view/test_view.c
git mv test/test_widget.c                  test/view/test_widget.c
git mv test/test_view_transition.c         test/view/test_view_transition.c
git mv test/test_view_transition_render.c  test/view/test_view_transition_render.c
```

- [ ] **Step 2: Update test_view.c**

Remove lines 6-7 (POOL_SIZE, mem_pool).

Add `#include "common/eui_test.h"` after eui includes.

Remove lines 27-30 (tests_run, tests_passed, TEST/PASS/FAIL macros).

In `main()`: replace `eui_allocator_init_tlsf(mem_pool, POOL_SIZE);` with `eui_test_init();`.

Replace printf summary + return with `return eui_test_summary();`.

- [ ] **Step 3: Update test_widget.c**

Remove lines 5-8 (tests_run, tests_passed, TEST/PASS/FAIL macros). Replace with `#include "common/eui_test.h"`.

Replace printf summary + return with `return eui_test_summary();`.

- [ ] **Step 4: Update test_view_transition.c**

Remove lines 10-11 (POOL_SIZE, mem_pool).

Add `#include "common/eui_test.h"`.

Remove lines 55-58 (tests_run, tests_passed, TEST/PASS/FAIL macros).

In `main()`: replace `eui_allocator_init_tlsf(mem_pool, POOL_SIZE);` with `eui_test_init();`.

Replace printf summary + return with `return eui_test_summary();`.

- [ ] **Step 5: Update test_view_transition_render.c**

Remove lines 11-12 (POOL_SIZE, mem_pool).

Add `#include "common/eui_test.h"`.

Remove lines 21-24 (tests_run, tests_passed, TEST/PASS/FAIL macros).

In `main()`: replace `eui_allocator_init_tlsf(mem_pool, POOL_SIZE);` with `eui_test_init();`.

Replace printf summary + return with `return eui_test_summary();`.

- [ ] **Step 6: Commit**

```bash
git add test/view/
git commit -m "refactor(test): move view tests to module dir, use common header"
```

---

### Task 9: Move non-merge font test files

**Files:**
- Move: `test/test_font_kerning.c` → `test/font/test_font_kerning.c`
- Move: `test/test_font_canvas.c` → `test/font/test_font_canvas.c`
- Move: `test/test_font_multiline.c` → `test/font/test_font_multiline.c`
- Move: `test/test_font_vs_u8g2.c` → `test/font/test_font_vs_u8g2.c`
- Move: `test/test_font_wqy13_render.c` → `test/font/test_font_wqy13_render.c`
- Move: `test/test_font_real_u8g2_render.c` → `test/font/test_font_real_u8g2_render.c`

- [ ] **Step 1: Move files with git mv**

```bash
git mv test/test_font_kerning.c            test/font/test_font_kerning.c
git mv test/test_font_canvas.c             test/font/test_font_canvas.c
git mv test/test_font_multiline.c          test/font/test_font_multiline.c
git mv test/test_font_vs_u8g2.c            test/font/test_font_vs_u8g2.c
git mv test/test_font_wqy13_render.c       test/font/test_font_wqy13_render.c
git mv test/test_font_real_u8g2_render.c   test/font/test_font_real_u8g2_render.c
```

- [ ] **Step 2: Update test_font_kerning.c**

Remove lines 13-14 (POOL_SIZE, mem_pool).

Add `#include "common/eui_test.h"` after eui includes.

Remove lines 16-19 (tests_run, tests_passed, TEST/PASS/FAIL macros).

Change `#include "../src/eui_font_internal.h"` to `#include "eui/eui_font_internal.h"`.

Change `#include "../src/eui_font_u8g2_internal.h"` to `#include "eui/eui_font_u8g2_internal.h"`.

Change `#include "test_font_kerning.h"` to `#include "data/test_font_kerning.h"`.

In `main()`: replace `eui_allocator_init_tlsf(mem_pool, POOL_SIZE);` with `eui_test_init();`.

Replace printf summary + return with `return eui_test_summary();`.

- [ ] **Step 3: Update test_font_canvas.c**

Remove lines 14-15 (POOL_SIZE, mem_pool).

Add `#include "common/eui_test.h"` after eui includes.

Remove lines 17-20 (tests_run, tests_passed, TEST/PASS/FAIL macros).

Change `#include "test_u8g2_font.h"` to `#include "data/test_u8g2_font.h"`.

The `#if EUI_FONT_ENABLE_U8G2` guard around the data include stays.

In `main()`: replace `eui_allocator_init_tlsf(mem_pool, POOL_SIZE);` with `eui_test_init();`.

Replace printf summary + return with `return eui_test_summary();`.

- [ ] **Step 4: Update test_font_multiline.c**

Remove lines 10-11 (POOL_SIZE, mem_pool).

Add `#include "common/eui_test.h"` after eui includes.

Remove lines 13-16 (tests_run, tests_passed, TEST/PASS/FAIL macros).

In `main()`: replace `eui_allocator_init_tlsf(mem_pool, POOL_SIZE);` with `eui_test_init();`.

Replace printf summary + return with `return eui_test_summary();`.

- [ ] **Step 5: Update test_font_vs_u8g2.c**

Remove lines 22-23 (POOL_SIZE, mem_pool).

Add `#include "common/eui_test.h"` after eui includes.

Remove lines 25-28 (tests_run, tests_passed, TEST/PASS/FAIL macros).

Change `#include "test_u8g2_profont10_data.h"` to `#include "data/test_u8g2_profont10_data.h"`.

Change `#include "test_u8g2_wqy12_ch1_data.h"` to `#include "data/test_u8g2_wqy12_ch1_data.h"`.

Change `#include "../src/eui_font_u8g2_internal.h"` to `#include "eui/eui_font_u8g2_internal.h"`.

Remove the `extern int32_t eui_font_u8g2_lookup_glyph(...)` declaration (line 20) — it's already in the header.

In `main()`: replace `eui_allocator_init_tlsf(mem_pool, POOL_SIZE);` with `eui_test_init();`.

Replace printf summary + return with `return eui_test_summary();`.

- [ ] **Step 6: Update test_font_wqy13_render.c**

Remove lines 6-7 (POOL_SIZE, mem_pool).

Add `#include "common/eui_test.h"` after eui includes.

Remove lines 8-11 (tests_run, tests_passed, TEST/PASS/FAIL macros).

In `main()`: replace `eui_allocator_init_tlsf(mem_pool, POOL_SIZE);` with `eui_test_init();`.

Replace printf summary + return with `return eui_test_summary();`.

- [ ] **Step 7: Update test_font_real_u8g2_render.c** (pre-merge, add common header)

Remove lines 17-18 (POOL_SIZE, mem_pool). Add `#include "common/eui_test.h"`. Remove local TEST/PASS/FAIL macros if present (check file — lines may vary).

Change `#include "test_u8g2_profont10_data.h"` to `#include "data/test_u8g2_profont10_data.h"`.

Change `#include "test_u8g2_wqy12_ch1_data.h"` to `#include "data/test_u8g2_wqy12_ch1_data.h"`.

Change `#include "../src/eui_font_u8g2_internal.h"` to `#include "eui/eui_font_u8g2_internal.h"`.

Change `#include "../src/eui_font_internal.h"` to `#include "eui/eui_font_internal.h"`.

Replace TLSF init + summary code in main().

- [ ] **Step 8: Commit**

```bash
git add test/font/
git commit -m "refactor(test): move font tests to module dir, fix include paths"
```

---

### Task 10: Merge test_font.c + test_font_u8g2.c → test_font_formats.c

**Files:**
- Move(base): `test/test_font.c` → `test/font/test_font_formats.c`
- Delete: `test/test_font_u8g2.c` (merged into above)
- Remove(during merge): `test/test_font_u8g2.c` from working tree

- [ ] **Step 1: Move base file**

```bash
git mv test/test_font.c test/font/test_font_formats.c
```

- [ ] **Step 2: Edit test_font_formats.c — add common header and fix data includes**

Remove lines 8-14: POOL_SIZE, mem_pool, tests_run, tests_passed, TEST/PASS/FAIL macros.

Add after eui includes:
```c
#include "common/eui_test.h"
```

Change `#include "test_vlw_font.h"` to `#include "data/test_vlw_font.h"`.

Since we need U8G2 tests too, add:
```c
#if EUI_FONT_ENABLE_U8G2
#include "data/test_u8g2_font.h"
#endif
```

Replace TLSF init + summary in main().

- [ ] **Step 3: Merge U8G2 test functions from test_font_u8g2.c**

Copy all U8G2 test functions from `test_font_u8g2.c` into `test_font_formats.c`, after the VLW test functions.

Wrap u8g2-specific code with `#if EUI_FONT_ENABLE_U8G2` / `#endif`. The u8g2 font definition (`test_font` as U8G2 format) and all `test_u8g2_*` functions go inside this guard.

The `test_font_formats.c` will have this structure:
```c
#include "eui/eui_font.h"
#include "eui/eui_font_builtin.h"
#include "eui/eui_allocator.h"
#include "data/test_vlw_font.h"
#include "common/eui_test.h"
#include <stdio.h>
#include <string.h>

#if EUI_FONT_ENABLE_U8G2
#include "data/test_u8g2_font.h"
#endif

/* ---- BDF tests ---- */
static void test_format(void) { ... }
static void test_char_width(void) { ... }
/* ... all BDF + VLW test functions ... */

#if EUI_FONT_ENABLE_U8G2
/* ---- U8G2 tests ---- */
static void test_u8g2_format(void) { ... }
static void test_u8g2_char_width_a(void) { ... }
/* ... all U8G2 test functions ... */
#endif

int main(void) {
    eui_test_init();
    printf("=== Font Format Tests ===\n");
    /* BDF tests */
    test_format();
    test_char_width();
    /* ... */
    /* VLW tests */
    test_vlw_char_width();
    /* ... */
#if EUI_FONT_ENABLE_U8G2
    test_u8g2_format();
    test_u8g2_char_width_a();
    /* ... */
#endif
    return eui_test_summary();
}
```

- [ ] **Step 4: Remove original test_font_u8g2.c**

```bash
git rm test/test_font_u8g2.c
```

- [ ] **Step 5: Commit**

```bash
git add test/font/test_font_formats.c
git commit -m "refactor(test): merge BDF+VLW+U8G2 format tests into test_font_formats.c"
```

---

### Task 11: Merge test_font_wqy12_chinese.c → test_font_real_u8g2_render.c

**Files:**
- Modify: `test/font/test_font_real_u8g2_render.c`
- Delete: `test/test_font_wqy12_chinese.c`

- [ ] **Step 1: Read test_font_wqy12_chinese.c to identify functions to merge**

From `test/test_font_wqy12_chinese.c`, extract:
- `font_get_be16()` helper (line 39-42)
- `render_glyph()` function (line 46-72)
- `render_grid()` function  
- `save_bmp_1bpp()` function
- `main()` content — grid rendering of all wqy12 chinese characters

- [ ] **Step 2: Merge into test_font_real_u8g2_render.c**

Add the wqy12 grid rendering functions into `test/font/test_font_real_u8g2_render.c`.

Add a new test function `test_wqy12_chinese_grid()` that renders all wqy12 characters in a grid and saves to BMP.

Call `test_wqy12_chinese_grid()` from `main()` after existing tests.

- [ ] **Step 3: Remove original test_font_wqy12_chinese.c**

```bash
git rm test/test_font_wqy12_chinese.c
```

- [ ] **Step 4: Commit**

```bash
git add test/font/test_font_real_u8g2_render.c
git commit -m "refactor(test): merge wqy12 chinese grid test into real_u8g2_render"
```

---

### Task 12: Merge test_wqy13_kerning.c + test_wqy13_u8g2_kerning.c → test_wqy13_kerning.c

**Files:**
- Move(base): `test/test_wqy13_kerning.c` → `test/font/test_wqy13_kerning.c`
- Delete: `test/test_wqy13_u8g2_kerning.c` (merged into above)

- [ ] **Step 1: Move base file**

```bash
git mv test/test_wqy13_kerning.c test/font/test_wqy13_kerning.c
```

- [ ] **Step 2: Update includes in test_wqy13_kerning.c**

Add `#include "common/eui_test.h"`.

Remove lines 13-19: POOL_SIZE, mem_pool, tests_run/tests_passed, TEST/PASS/FAIL/INFO macros.

Change `#include "../src/eui_font_internal.h"` to `#include "eui/eui_font_internal.h"`.

Change `#include "../src/eui_font_u8g2_internal.h"` to `#include "eui/eui_font_u8g2_internal.h"`.

Replace TLSF init + summary code in main().

- [ ] **Step 3: Merge U8G2 kerning comparison from test_wqy13_u8g2_kerning.c**

From `test_wqy13_u8g2_kerning.c`, extract:
- BMP writer function
- u8g2 canvas setup
- Kerning comparison rendering functions
- Test functions that compare EUI kerning output vs u8g2 native output

Wrap all u8g2-dependent code with `#if EUI_FONT_ENABLE_U8G2 && EUI_FONT_ENABLE_KERNING`.

Structure:
```c
#if EUI_FONT_ENABLE_U8G2 && EUI_FONT_ENABLE_KERNING
/* BMP writer + u8g2 comparison functions */
#endif
```

Add calls in `main()` inside the feature guard.

- [ ] **Step 4: Remove original test_wqy13_u8g2_kerning.c**

```bash
git rm test/test_wqy13_u8g2_kerning.c
```

- [ ] **Step 5: Commit**

```bash
git add test/font/test_wqy13_kerning.c
git commit -m "refactor(test): merge wqy13 kerning tests into single file"
```

---

### Task 13: Rewrite test/CMakeLists.txt

**Files:**
- Modify: `test/CMakeLists.txt`

- [ ] **Step 1: Write new CMakeLists.txt**

```cmake
function(eui_add_test name source)
    cmake_parse_arguments(TEST "" "" "INCLUDES;SOURCES;LIBS" ${ARGN})
    add_executable(${name} ${source} ${TEST_SOURCES})
    target_include_directories(${name} PRIVATE
        ${CMAKE_BINARY_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${TEST_INCLUDES}
    )
    target_link_libraries(${name} PRIVATE eui ${TEST_LIBS})
    add_test(NAME ${name} COMMAND ${name})
endfunction()

# ---- Common / Foundation ----
eui_add_test(test_allocator     allocator/test_allocator.c)
eui_add_test(test_event         event/test_event.c)
eui_add_test(test_input         input/test_input.c)
eui_add_test(test_str           str/test_str.c)

# ---- Canvas ----
eui_add_test(test_canvas        canvas/test_canvas.c)
eui_add_test(test_canvas_16bpp  canvas/test_canvas_16bpp.c)
eui_add_test(test_canvas_render canvas/test_canvas_render.c)

# ---- Font ----
eui_add_test(test_font_formats          font/test_font_formats.c)
eui_add_test(test_font_kerning          font/test_font_kerning.c)
eui_add_test(test_font_canvas           font/test_font_canvas.c)
eui_add_test(test_font_multiline        font/test_font_multiline.c)
eui_add_test(test_font_wqy13_render     font/test_font_wqy13_render.c)
eui_add_test(test_font_real_u8g2_render font/test_font_real_u8g2_render.c)

# ---- View ----
eui_add_test(test_view                    view/test_view.c)
eui_add_test(test_widget                  view/test_widget.c)
eui_add_test(test_view_transition         view/test_view_transition.c)
eui_add_test(test_view_transition_render  view/test_view_transition_render.c
    LIBS eui_drv_raylib)

# ---- Drivers ----
eui_add_test(test_drivers drivers/test_drivers.c)

# ---- u8g2 reference comparison tests ----
set(U8G2_DIR "${CMAKE_SOURCE_DIR}/../u8g2" CACHE PATH "u8g2 source directory")

if(EXISTS ${U8G2_DIR})
    set(U8G2_SOURCES
        ${U8G2_DIR}/csrc/u8g2_setup.c
        ${U8G2_DIR}/csrc/u8g2_font.c
        ${U8G2_DIR}/csrc/u8g2_ll_hvline.c
        ${U8G2_DIR}/csrc/u8g2_buffer.c
        ${U8G2_DIR}/csrc/u8g2_kerning.c
        ${U8G2_DIR}/csrc/u8g2_intersection.c
        ${U8G2_DIR}/csrc/u8g2_hvline.c
        ${U8G2_DIR}/csrc/u8x8_setup.c
        ${U8G2_DIR}/csrc/u8x8_display.c
        ${U8G2_DIR}/csrc/u8x8_capture.c
        ${U8G2_DIR}/csrc/u8x8_gpio.c
        ${U8G2_DIR}/csrc/u8x8_u16toa.c
        ${U8G2_DIR}/csrc/u8x8_byte.c
        ${U8G2_DIR}/csrc/u8x8_8x8.c
        ${U8G2_DIR}/csrc/u8x8_cad.c
    )

    eui_add_test(test_font_vs_u8g2 font/test_font_vs_u8g2.c
        SOURCES  ${U8G2_SOURCES}
        INCLUDES ${U8G2_DIR}/csrc
    )
    eui_add_test(test_wqy13_kerning font/test_wqy13_kerning.c
        SOURCES  ${U8G2_SOURCES}
        INCLUDES ${U8G2_DIR}/csrc
    )
endif()
```

- [ ] **Step 2: Commit**

```bash
git add test/CMakeLists.txt
git commit -m "refactor(test): rewrite CMakeLists.txt with eui_add_test() function"
```

---

### Task 14: Build and verify

- [ ] **Step 1: Configure and build**

```bash
cmake -B build -DEUI_BUILD_TESTS=ON -DEUI_BUILD_EXAMPLES=OFF
cmake --build build -j$(nproc)
```

Expected: Build succeeds with no errors.

- [ ] **Step 2: Run all tests**

```bash
cd build && ctest --output-on-failure
```

Expected: All tests pass (same count as before refactor: `test/*.c` count).

- [ ] **Step 3: Verify no ../src/ references remain in test files**

```bash
grep -r '\.\./src/' test/ || echo "No ../src/ references found"
```

Expected: "No ../src/ references found".

- [ ] **Step 4: Verify no leftover files in test/ root**

```bash
ls test/*.c test/*.h 2>/dev/null
```

Expected: Only `test/CMakeLists.txt` and config files remain in flat directory; all `.c` and `.h` files should be in subdirectories.

- [ ] **Step 5: Commit any final fixes if needed**
