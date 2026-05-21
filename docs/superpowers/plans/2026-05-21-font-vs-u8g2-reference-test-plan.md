# Font vs u8g2 Reference Test Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Create `test_font_vs_u8g2` that compares eui font rendering pixel-by-pixel against u8g2 across 16 text scenarios, then iteratively fix rendering bugs until all cases pass.

**Architecture:** Single C executable links eui + u8g2 core. Scenario data table drives two comparison layers: glyph-level (per-codepoint bitmap compare) and canvas-level (full 128x64 buffer pixel compare). Font data shared via existing test headers. Failures produce BMP debug images.

**Tech Stack:** C99, CMake, eui library, u8g2 core sources (u8g2_setup.c, u8g2_font.c, u8g2_ll_hvline.c, u8g2_buffer.c, u8x8_setup.c), CTest

**Known pre-existing issues:** wqy12 CJK unicode glyph rendering has bugs in eui — `eui_font_u8g2_draw_glyph()` returns 0 for many Chinese characters. This test will expose these bugs; later tasks will fix them.

---

### Task 1: Add CMake build entry

**Files:**
- Modify: `test/CMakeLists.txt`

- [ ] **Step 1: Add u8g2 test target**

Append to `test/CMakeLists.txt`:

```cmake
# ---- u8g2 reference comparison test ----
set(U8G2_DIR "${CMAKE_SOURCE_DIR}/../u8g2" CACHE PATH "u8g2 source directory")

if(EXISTS ${U8G2_DIR})
    add_executable(test_font_vs_u8g2 test_font_vs_u8g2.c)

    target_sources(test_font_vs_u8g2 PRIVATE
        ${U8G2_DIR}/csrc/u8g2_setup.c
        ${U8G2_DIR}/csrc/u8g2_font.c
        ${U8G2_DIR}/csrc/u8g2_ll_hvline.c
        ${U8G2_DIR}/csrc/u8g2_buffer.c
        ${U8G2_DIR}/csrc/u8x8_setup.c
    )

    target_include_directories(test_font_vs_u8g2 PRIVATE
        ${CMAKE_BINARY_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${U8G2_DIR}/csrc
    )
    target_link_libraries(test_font_vs_u8g2 PRIVATE eui)
    add_test(NAME font_vs_u8g2 COMMAND test_font_vs_u8g2)
endif()
```

- [ ] **Step 2: Configure and verify CMake sees u8g2**

```bash
cmake -B build -DEUI_BUILD_TESTS=ON -DEUI_COLOR_DEPTH=1 -DEUI_BUILD_EXAMPLES=OFF
```

Expected: No errors, `test_font_vs_u8g2` in build targets.

- [ ] **Step 3: Commit**

```bash
git add test/CMakeLists.txt && git commit -m "build: add test_font_vs_u8g2 CMake target with u8g2 deps"
```

---

### Task 2: Create test skeleton with scenario table

**Files:**
- Create: `test/test_font_vs_u8g2.c`

- [ ] **Step 1: Write complete test file (skeleton with scenario table, empty test function stubs)**

```c
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "eui/eui_canvas.h"
#include "eui/eui_font.h"
#include "eui/eui_allocator.h"
#include "eui/eui_config.h"
#include "eui/eui_types.h"
#include "../src/eui_font_u8g2_internal.h"

#include "test_u8g2_profont10_data.h"
#include "test_u8g2_wqy12_ch1_data.h"

#include "u8g2.h"

#define POOL_SIZE 131072
static uint8_t mem_pool[POOL_SIZE];

static int tests_run = 0, tests_passed = 0;
#define TEST(n)  do { printf("  %-45s ... ", n); tests_run++; } while(0)
#define PASS()   do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(m)  do { printf("FAIL: %s\n", m); } while(0)

/* ---- Shared font definitions ---- */
static const eui_font_t eui_profont10 = {
    .format = EUI_FONT_FORMAT_U8G2,
    .line_height = 10, .baseline = 8, .flags = 0,
    .data = u8g2_font_profont10_tf_data,
    .lookup_glyph = NULL,
};

static const eui_font_t eui_wqy12 = {
    .format = EUI_FONT_FORMAT_U8G2,
    .line_height = 12, .baseline = 10, .flags = 0,
    .data = u8g2_font_wqy12_ch1_data,
    .lookup_glyph = NULL,
};

/* ---- Scenario definitions ---- */
typedef enum { FONT_PROFONT10, FONT_WQY12 } scenario_font_t;

typedef struct {
    const char    *desc;       /* human-readable description */
    const char    *str;        /* test string (UTF-8) */
    scenario_font_t font;      /* which font to use */
    int             use_utf8;  /* 1 = use u8g2_DrawUTF8, 0 = u8g2_DrawStr */
} scenario_t;

static const scenario_t scenarios[] = {
    /* ---- profont10 ASCII ---- */
    {"Pure English",             "Hello World",              FONT_PROFONT10, 0},
    {"Uppercase only",           "ABCDEFGHIJKLM",            FONT_PROFONT10, 0},
    {"Lowercase only",           "abcdefghijklm",            FONT_PROFONT10, 0},
    {"Mixed case",               "HeLlO wOrLd",              FONT_PROFONT10, 0},
    {"Numbers",                  "0123456789",               FONT_PROFONT10, 0},
    {"Punctuation",              "!@#$%^&*()-+=",           FONT_PROFONT10, 0},
    {"Single char",              "A",                        FONT_PROFONT10, 0},
    {"Space only",               "     ",                    FONT_PROFONT10, 0},
    {"Empty string",             "",                         FONT_PROFONT10, 0},
    {"Amiibo",                   "Amiibo",                   FONT_PROFONT10, 0},

    /* ---- wqy12 Chinese/Unicode ---- */
    {"Pure Chinese",             "\xe4\xbd\xa0\xe5\xa5\xbd\xe4\xb8\x96\xe7\x95\x8c",  FONT_WQY12, 1},
    {"Chinese + ASCII mixed",    "Hello\xe4\xbd\xa0\xe5\xa5\xbdWorld\xe4\xb8\x96\xe7\x95\x8c", FONT_WQY12, 1},
    {"Chinese + Unicode symbols","\xe6\xb8\xa9\xe5\xba\xa6:25\xe2\x84\x83 \xe2\x98\x85\xe8\xaf\x84\xe7\xba\xa7A", FONT_WQY12, 1},
    {"All CJK",                  "\xe4\xb8\x80\xe4\xb8\x8a\xe4\xb8\x8b\xe4\xb8\xad\xe4\xb8\xba\xe4\xbb\xa5\xe4\xb9\x8b", FONT_WQY12, 1},
    {"Chinese punctuation",      "\xe3\x80\x8c\xe4\xbd\xa0\xe5\xa5\xbd\xe3\x80\x8d\xe3\x80\x8e\xe4\xb8\x96\xe7\x95\x8c\xe3\x80\x8f", FONT_WQY12, 1},
};

#define SCENARIO_COUNT (sizeof(scenarios) / sizeof(scenarios[0]))

/* ---- Helpers: get font data pointer for both eui and u8g2 ---- */
static const eui_font_t* scenario_eui_font(const scenario_t *sc) {
    return (sc->font == FONT_PROFONT10) ? &eui_profont10 : &eui_wqy12;
}
static const uint8_t* scenario_u8g2_font_data(const scenario_t *sc) {
    return (sc->font == FONT_PROFONT10)
        ? u8g2_font_profont10_tf_data : u8g2_font_wqy12_ch1_data;
}
static uint16_t scenario_baseline(const scenario_t *sc) {
    return (sc->font == FONT_PROFONT10) ? 8u : 10u;
}

/* ---- Forward declarations (implemented in later tasks) ---- */
static int test_glyph_compare(const scenario_t *sc);
static int test_canvas_compare(const scenario_t *sc);
static void write_bmp(const char *fn, const uint8_t *buf,
                      uint16_t w, uint16_t h);

/* ---- Main ---- */
int main(void) {
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);
    printf("=== Font vs u8g2 Reference Test ===\n");
    printf("Scenarios: %zu\n\n", SCENARIO_COUNT);

    int total_fail = 0;
    for (size_t i = 0; i < SCENARIO_COUNT; i++) {
        const scenario_t *sc = &scenarios[i];
        printf("--- Scenario %zu: %s ---\n", i + 1, sc->desc);

        int gf = test_glyph_compare(sc);
        printf("  Layer 1 (glyph): %s (%d mismatches)\n", gf ? "FAIL" : "PASS", gf);

        int cf = test_canvas_compare(sc);
        printf("  Layer 2 (canvas): %s (%d mismatches)\n", cf ? "FAIL" : "PASS", cf);

        if (gf) total_fail++;
        if (cf) total_fail++;
    }

    printf("\n=== Results: %zu scenarios, %d failures ===\n",
           SCENARIO_COUNT, total_fail);
    return total_fail > 0 ? 1 : 0;
}

/* ---- Stubs ---- */
static int test_glyph_compare(const scenario_t *sc) { (void)sc; return 0; }
static int test_canvas_compare(const scenario_t *sc) { (void)sc; return 0; }
static void write_bmp(const char *fn, const uint8_t *buf,
                      uint16_t w, uint16_t h) { (void)fn;(void)buf;(void)w;(void)h; }
```

- [ ] **Step 2: Build and run skeleton**

```bash
cmake --build build --target test_font_vs_u8g2 -j4
./build/test/test_font_vs_u8g2
```

Expected: Build succeeds. All 16 scenarios print with 0 failures (stubs return 0). Exit code 0.

- [ ] **Step 3: Commit**

```bash
git add test/test_font_vs_u8g2.c && git commit -m "test: add font_vs_u8g2 skeleton with 16 scenarios"
```

---

### Task 3: Implement Layer 1 — Glyph-level comparison

**Files:**
- Modify: `test/test_font_vs_u8g2.c` — replace `test_glyph_compare` stub + add u8g2 helpers

- [ ] **Step 1: Replace stubs with full glyph comparison implementation**

Replace the `test_glyph_compare` stub and add the helper functions above `main()`:

```c
/* ===== Layer 1: Glyph Comparison ===== */

#define GLYPH_BUF_W 16
#define GLYPH_BUF_H 32

static uint8_t  u8g2_tile_buf[GLYPH_BUF_W * GLYPH_BUF_H / 8];
static u8g2_t   u8g2_ctx;

/* Minimal display info for 16x32 pixel area */
static u8x8_display_info_t u8x8_glyph_di = {
    .tile_width = GLYPH_BUF_W / 8,
    .tile_height = GLYPH_BUF_H / 8,
    .pixel_width = GLYPH_BUF_W,
    .pixel_height = GLYPH_BUF_H,
    .default_x_offset = 0,
};

static void u8g2_init_for_glyph(const uint8_t *font_data)
{
    memset(&u8g2_ctx, 0, sizeof(u8g2_ctx));
    u8x8_SetupDefaults(u8g2_GetU8x8(&u8g2_ctx));
    u8g2_GetU8x8(&u8g2_ctx)->display_info = &u8x8_glyph_di;
    u8g2_SetupBuffer(&u8g2_ctx, u8g2_tile_buf, GLYPH_BUF_H / 8,
                     u8g2_ll_hvline_vertical_top_lsb, &u8g2_cb_r0);
    u8g2_SetFont(&u8g2_ctx, font_data);
}

/* Convert u8g2 vertical-LSB-top tile region to horizontal-MSB-left buffer */
static void extract_u8g2_glyph_to_horiz(const u8g2_t *ug,
    uint8_t *dst, uint8_t dst_stride,
    uint8_t x0, uint8_t y0, uint8_t gw, uint8_t gh)
{
    uint8_t tw = u8g2_GetBufferTileWidth(ug);
    const uint8_t *src = u8g2_GetBufferPtr(ug);
    memset(dst, 0, gh * dst_stride);
    for (uint8_t row = 0; row < gh; row++) {
        uint16_t y = (uint16_t)y0 + row;
        for (uint8_t col = 0; col < gw; col++) {
            uint16_t byte_off = (y / 8) * tw + x0 + col;
            uint8_t  bit_off  = y & 7;
            if (src[byte_off] & (1u << bit_off)) {
                dst[row * dst_stride + col / 8] |= (1u << (7 - (col % 8)));
            }
        }
    }
}

static int test_glyph_compare(const scenario_t *sc)
{
    const eui_font_t *eui_f = scenario_eui_font(sc);
    const uint8_t    *u8g2_data = scenario_u8g2_font_data(sc);
    int baseline = (int)scenario_baseline(sc);

    uint8_t max_w = u8g2_data[9];   /* HDR_MAX_CHAR_W */
    uint8_t max_h = u8g2_data[10];  /* HDR_MAX_CHAR_H */
    uint16_t buf_stride = (max_w + 15) / 16 * 2;
    uint8_t *eui_buf  = calloc((size_t)max_h * buf_stride + 32, 1);
    uint8_t *u8g2_buf = calloc((size_t)max_h * buf_stride, 1);
    if (!eui_buf || !u8g2_buf) { free(eui_buf); free(u8g2_buf); return -1; }

    u8g2_init_for_glyph(u8g2_data);

    int mismatches = 0, idx = 0;
    const char *s = sc->str;

    while (*s) {
        /* decode UTF-8 */
        uint32_t cp;
        const uint8_t *p = (const uint8_t *)s;
        if      ((p[0] & 0x80) == 0)                     { cp = p[0];               s += 1; }
        else if ((p[0] & 0xE0) == 0xC0 && (p[1]&0xC0)==0x80) { cp = ((p[0]&0x1F)<<6)|(p[1]&0x3F); s+=2; }
        else if ((p[0] & 0xF0) == 0xE0 && (p[1]&0xC0)==0x80 && (p[2]&0xC0)==0x80)
                                             { cp = ((p[0]&0x0F)<<12)|((p[1]&0x3F)<<6)|(p[2]&0x3F); s+=3; }
        else                                 { cp = p[0];               s += 1; }

        /* get glyph metrics via eui */
        int16_t g_off = eui_font_u8g2_lookup_glyph(eui_f, (uint16_t)cp, 0);
        if (g_off < 0) { idx++; continue; }

        u8g2_glyph_t geo = {0};
        if (!decode_glyph_at(eui_f, (uint16_t)g_off, &geo)) { idx++; continue; }

        /* render with u8g2 */
        memset(u8g2_tile_buf, 0, sizeof(u8g2_tile_buf));
        u8g2_DrawGlyph(&u8g2_ctx, 0, (u8g2_uint_t)baseline, (uint16_t)cp);

        /* u8g2 glyph top row = baseline + geo.y_offset (y_offset is negative for ascent) */
        int16_t u8g2_y0 = (int16_t)baseline + (int16_t)geo.y_offset;
        if (u8g2_y0 < 0) u8g2_y0 = 0;
        extract_u8g2_glyph_to_horiz(&u8g2_ctx, u8g2_buf, buf_stride,
                                      (uint8_t)geo.x_offset, (uint8_t)u8g2_y0,
                                      geo.width, geo.height);

        /* render with eui */
        memset(eui_buf, 0, max_h * buf_stride + 32);
        uint8_t eui_adv = eui_font_u8g2_draw_glyph(eui_f, (uint16_t)cp,
                                                     eui_buf, buf_stride, 1);

        /* compare glyph bitmaps pixel-by-pixel */
        for (uint8_t row = 0; row < geo.height; row++) {
            for (uint8_t b = 0; b < buf_stride; b++) {
                if (u8g2_buf[row * buf_stride + b] != eui_buf[row * buf_stride + b]) {
                    if (mismatches < 3)
                        printf("  [g%d U+%04X] row=%d byte=%d u8g2=0x%02X eui=0x%02X\n",
                               idx, (unsigned)cp, row, b,
                               u8g2_buf[row * buf_stride + b],
                               eui_buf[row * buf_stride + b]);
                    mismatches++;
                }
            }
        }

        /* compare advance */
        if (eui_adv != geo.x_advance) {
            if (mismatches < 3)
                printf("  [g%d U+%04X] advance: eui=%d u8g2=%d\n",
                       idx, (unsigned)cp, eui_adv, geo.x_advance);
            mismatches++;
        }

        idx++;
    }

    free(eui_buf);
    free(u8g2_buf);
    return mismatches;
}
```

- [ ] **Step 2: Build and test**

```bash
cmake --build build --target test_font_vs_u8g2 -j4 2>&1
./build/test/test_font_vs_u8g2 2>&1
```

Expected: Build succeeds. profont10 scenarios pass (0 mismatches). wqy12 scenarios may fail — record the failures.

- [ ] **Step 3: Commit**

```bash
git add test/test_font_vs_u8g2.c && git commit -m "test: implement Layer 1 glyph comparison"
```

---

### Task 4: Implement Layer 2 — Canvas-level comparison

**Files:**
- Modify: `test/test_font_vs_u8g2.c` — replace `test_canvas_compare` stub

- [ ] **Step 1: Add canvas comparison implementation**

Replace the `test_canvas_compare` stub with:

```c
/* ===== Layer 2: Canvas Comparison ===== */

#define CANVAS_W 128
#define CANVAS_H 64

/* ---- u8g2 canvas-size buffer ---- */
static uint8_t  u8g2_canvas_tile_buf[CANVAS_W * CANVAS_H / 8];
static u8g2_t   u8g2_canvas_ctx;
static u8x8_display_info_t u8x8_canvas_di = {
    .tile_width  = CANVAS_W / 8,
    .tile_height = CANVAS_H / 8,
    .pixel_width = CANVAS_W,
    .pixel_height = CANVAS_H,
    .default_x_offset = 0,
};

static void u8g2_init_for_canvas(const uint8_t *font_data)
{
    memset(&u8g2_canvas_ctx, 0, sizeof(u8g2_canvas_ctx));
    u8x8_SetupDefaults(u8g2_GetU8x8(&u8g2_canvas_ctx));
    u8g2_GetU8x8(&u8g2_canvas_ctx)->display_info = &u8x8_canvas_di;
    u8g2_SetupBuffer(&u8g2_canvas_ctx, u8g2_canvas_tile_buf, CANVAS_H / 8,
                     u8g2_ll_hvline_vertical_top_lsb, &u8g2_cb_r0);
    u8g2_SetFont(&u8g2_canvas_ctx, font_data);
}

/* ---- eui canvas mock ---- */
#if EUI_COLOR_DEPTH == 1
#define EUI_CANVAS_BUF_SIZE (CANVAS_W * CANVAS_H / 8)
#elif EUI_COLOR_DEPTH == 8
#define EUI_CANVAS_BUF_SIZE (CANVAS_W * CANVAS_H)
#else
#define EUI_CANVAS_BUF_SIZE (CANVAS_W * CANVAS_H * 2)
#endif

static uint8_t eui_canvas_mock_buf[EUI_CANVAS_BUF_SIZE];

static void eui_write_buffer_cb(const uint8_t *b, const eui_rect_t *r, void *ud)
{
    (void)ud;
#if EUI_COLOR_DEPTH == 1
    int bprow = r->w / 8;
    for (int row = 0; row < (int)r->h; row++)
        memcpy(eui_canvas_mock_buf + ((r->y + row) * (CANVAS_W / 8) + r->x / 8),
               b + row * bprow, bprow);
#else
    /* For higher depths, similar memcpy with appropriate stride */
#endif
}

static eui_display_hal_t eui_canvas_display = {
    .caps = { .width = CANVAS_W, .height = CANVAS_H,
              .color_depth = EUI_COLOR_DEPTH, .buffer_mode = EUI_BUFFER_FULL,
              .has_gram = false },
    .init = NULL,
    .write_buffer = eui_write_buffer_cb,
};

/* Read pixel from eui canvas buffer (horizontal LSB-left) */
static int eui_get_pixel(int x, int y)
{
    if (x < 0 || x >= CANVAS_W || y < 0 || y >= CANVAS_H) return 0;
#if EUI_COLOR_DEPTH == 1
    int bi = y * (CANVAS_W / 8) + x / 8;
    int bp = x % 8;
    return (eui_canvas_mock_buf[bi] >> bp) & 1;
#else
    return 0; /* 1bpp only */
#endif
}

/* Read pixel from u8g2 tile buffer (vertical LSB-top) */
static int u8g2_get_pixel(int x, int y)
{
    if (x < 0 || x >= CANVAS_W || y < 0 || y >= CANVAS_H) return 0;
    uint8_t tw = u8g2_GetBufferTileWidth(&u8g2_canvas_ctx);
    const uint8_t *src = u8g2_GetBufferPtr(&u8g2_canvas_ctx);
    uint16_t bo = ((uint16_t)y / 8) * tw + x;
    uint8_t  bi = y & 7;
    return (src[bo] >> bi) & 1;
}

static int test_canvas_compare(const scenario_t *sc)
{
    const uint8_t *u8g2_data = scenario_u8g2_font_data(sc);
    const eui_font_t *eui_f = scenario_eui_font(sc);
    int baseline = (int)scenario_baseline(sc);

    /* ---- u8g2: render string to tile buffer ---- */
    u8g2_init_for_canvas(u8g2_data);
    memset(u8g2_canvas_tile_buf, 0, sizeof(u8g2_canvas_tile_buf));
    if (sc->use_utf8)
        u8g2_DrawUTF8(&u8g2_canvas_ctx, 0, (u8g2_uint_t)baseline, sc->str);
    else
        u8g2_DrawStr(&u8g2_canvas_ctx, 0, (u8g2_uint_t)baseline, sc->str);

    /* ---- eui: render string to canvas mock buffer ---- */
    memset(eui_canvas_mock_buf, 0, sizeof(eui_canvas_mock_buf));
    eui_canvas_t *cv = eui_canvas_create(&eui_canvas_display);
    if (!cv) return -1;
    eui_canvas_set_font(cv, eui_f);
    eui_canvas_set_color(cv, EUI_COLOR_WHITE);
    eui_canvas_set_bg_color(cv, EUI_COLOR_BLACK);
    eui_canvas_clear(cv);
    eui_canvas_draw_str(cv, 0, (int16_t)baseline, sc->str);
    eui_canvas_commit(cv);
    eui_canvas_destroy(cv);

    /* ---- compare pixel-by-pixel ---- */
    int mismatches = 0;
    for (int y = 0; y < CANVAS_H; y++) {
        for (int x = 0; x < CANVAS_W; x++) {
            int u8g2_px = u8g2_get_pixel(x, y);
            int eui_px  = eui_get_pixel(x, y);
            if (u8g2_px != eui_px) {
                if (mismatches == 0)
                    printf("  First mismatch at (%d,%d): u8g2=%d eui=%d\n",
                           x, y, u8g2_px, eui_px);
                mismatches++;
            }
        }
    }

    /* ---- compare string widths ---- */
    uint16_t eui_w = eui_font_get_str_width(eui_f, sc->str);
    if (sc->use_utf8) {
        u8g2_uint_t u8g2_w = u8g2_GetUTF8Width(&u8g2_canvas_ctx, sc->str);
        if (eui_w != (uint16_t)u8g2_w) {
            printf("  Width mismatch: eui=%d u8g2=%d\n", eui_w, (int)u8g2_w);
            mismatches++;
        }
    } else {
        u8g2_uint_t u8g2_w = u8g2_GetStrWidth(&u8g2_canvas_ctx, sc->str);
        if (eui_w != (uint16_t)u8g2_w) {
            printf("  Width mismatch: eui=%d u8g2=%d\n", eui_w, (int)u8g2_w);
            mismatches++;
        }
    }

    return mismatches;
}
```

- [ ] **Step 2: Build and test**

```bash
cmake --build build --target test_font_vs_u8g2 -j4 2>&1
./build/test/test_font_vs_u8g2 2>&1
```

Expected: Build succeeds. profont10 canvas scenarios should pass. wqy12 may fail — record.

- [ ] **Step 3: Commit**

```bash
git add test/test_font_vs_u8g2.c && git commit -m "test: implement Layer 2 canvas comparison"
```

---

### Task 5: Add BMP debug output for failures

**Files:**
- Modify: `test/test_font_vs_u8g2.c` — replace `write_bmp` stub

- [ ] **Step 1: Replace `write_bmp` stub with 1bpp BMP writer**

Replace the `write_bmp` stub:

```c
/* Write a 1bpp BMP from a buffer (horizontal LSB-left format) */
static void write_bmp_1bpp(const char *fn, const uint8_t *buf, uint16_t w, uint16_t h)
{
    FILE *f = fopen(fn, "wb");
    if (!f) return;

    int row_bytes = (w + 7) / 8;
    int row_padded = (row_bytes + 3) & ~3;
    int pixel_data_size = row_padded * h;
    int palette_size = 8;
    int offset = 14 + 40 + palette_size;
    int file_size = offset + pixel_data_size;

    /* BMP file header */
    uint16_t bfType = 0x4D42;
    fwrite(&bfType, 2, 1, f);
    uint32_t bfSize = file_size;
    fwrite(&bfSize, 4, 1, f);
    uint32_t zero32 = 0;
    fwrite(&zero32, 4, 1, f);
    uint32_t bfOffBits = offset;
    fwrite(&bfOffBits, 4, 1, f);

    /* DIB header */
    uint32_t biSize = 40;
    int32_t  biWidth = w, biHeight = h;
    uint16_t biPlanes = 1, biBitCount = 1;
    uint32_t biCompression = 0, biSizeImage = pixel_data_size;
    int32_t  biXPels = 2835, biYPels = 2835;
    uint32_t biClrUsed = 2, biClrImportant = 2;
    fwrite(&biSize, 4, 1, f);
    fwrite(&biWidth, 4, 1, f);
    fwrite(&biHeight, 4, 1, f);
    fwrite(&biPlanes, 2, 1, f);
    fwrite(&biBitCount, 2, 1, f);
    fwrite(&biCompression, 4, 1, f);
    fwrite(&biSizeImage, 4, 1, f);
    fwrite(&biXPels, 4, 1, f);
    fwrite(&biYPels, 4, 1, f);
    fwrite(&biClrUsed, 4, 1, f);
    fwrite(&biClrImportant, 4, 1, f);

    /* Palette: black (0), white (1) */
    uint8_t black[4] = {0,0,0,0}, white[4] = {255,255,255,0};
    fwrite(black, 4, 1, f);
    fwrite(white, 4, 1, f);

    /* Pixels: bottom-up, BMP expects MSB-left (bit 7 = leftmost) */
    /* Our buffer is LSB-left. Convert per row. */
    uint8_t *row = malloc(row_padded);
    if (!row) { fclose(f); return; }
    for (int y = h - 1; y >= 0; y--) {
        memset(row, 0, row_padded);
        for (int x = 0; x < w; x++) {
            int bi = y * (w / 8) + x / 8;
            int bp = x % 8;
            if ((buf[bi] >> bp) & 1)
                row[x / 8] |= (1u << (7 - (x % 8)));
        }
        fwrite(row, 1, row_padded, f);
    }
    free(row);
    fclose(f);
}

/* Write a 1bpp BMP from u8g2 vertical buffer */
static void write_bmp_from_u8g2(const char *fn, const u8g2_t *ug,
                                 uint16_t w, uint16_t h)
{
    FILE *f = fopen(fn, "wb");
    if (!f) return;

    int row_bytes = (w + 7) / 8;
    int row_padded = (row_bytes + 3) & ~3;
    int pixel_data_size = row_padded * h;
    int offset = 14 + 40 + 8;
    int file_size = offset + pixel_data_size;

    uint16_t bfType = 0x4D42;
    fwrite(&bfType, 2, 1, f);
    uint32_t bfSize = file_size; fwrite(&bfSize, 4, 1, f);
    uint32_t z = 0; fwrite(&z, 4, 1, f);
    uint32_t bfOffBits = offset; fwrite(&bfOffBits, 4, 1, f);

    uint32_t biSize = 40, biCompression = 0, biSizeImage = pixel_data_size;
    int32_t  biW = w, biH = h;
    uint16_t biPlanes = 1, biBitCount = 1;
    int32_t biXPels = 2835, biYPels = 2835;
    uint32_t biClrUsed = 2, biClrImportant = 2;
    fwrite(&biSize, 4, 1, f);
    fwrite(&biW, 4, 1, f);
    fwrite(&biH, 4, 1, f);
    fwrite(&biPlanes, 2, 1, f);
    fwrite(&biBitCount, 2, 1, f);
    fwrite(&biCompression, 4, 1, f);
    fwrite(&biSizeImage, 4, 1, f);
    fwrite(&biXPels, 4, 1, f);
    fwrite(&biYPels, 4, 1, f);
    fwrite(&biClrUsed, 4, 1, f);
    fwrite(&biClrImportant, 4, 1, f);

    uint8_t black[4] = {0,0,0,0}, white[4] = {255,255,255,0};
    fwrite(black, 4, 1, f);
    fwrite(white, 4, 1, f);

    uint8_t tw = u8g2_GetBufferTileWidth(ug);
    const uint8_t *src = u8g2_GetBufferPtr(ug);
    uint8_t *row = malloc(row_padded);
    if (!row) { fclose(f); return; }
    for (int y = h - 1; y >= 0; y--) {
        memset(row, 0, row_padded);
        for (int x = 0; x < w; x++) {
            uint16_t bo = ((uint16_t)y / 8) * tw + x;
            uint8_t  bi = y & 7;
            if (src[bo] & (1u << bi))
                row[x / 8] |= (1u << (7 - (x % 8)));
        }
        fwrite(row, 1, row_padded, f);
    }
    free(row);
    fclose(f);
}
```

- [ ] **Step 2: Add BMP dump call in canvas comparison on failure**

In `test_canvas_compare`, after the pixel comparison loop, add:

```c
    if (mismatches > 0) {
        char fn_expected[128], fn_actual[128];
        snprintf(fn_expected, sizeof(fn_expected),
                 "test_vs_u8g2_expected_%s.bmp", sc->desc);
        snprintf(fn_actual, sizeof(fn_actual),
                 "test_vs_u8g2_actual_%s.bmp", sc->desc);
        write_bmp_from_u8g2(fn_expected, &u8g2_canvas_ctx, CANVAS_W, CANVAS_H);
        write_bmp_1bpp(fn_actual, eui_canvas_mock_buf, CANVAS_W, CANVAS_H);
        printf("  Wrote: %s, %s\n", fn_expected, fn_actual);
    }
```

- [ ] **Step 3: Build and test**

```bash
cmake --build build --target test_font_vs_u8g2 -j4
./build/test/test_font_vs_u8g2 2>&1
```

Expected: profont10 scenarios pass, wqy12 fail with BMP files written.

- [ ] **Step 4: Commit**

```bash
git add test/test_font_vs_u8g2.c && git commit -m "test: add BMP debug output for failures"
```

---

### Task 6: Debug and fix ASCII rendering (if any failures)

**Files:**
- Modify: `test/test_font_vs_u8g2.c` (test fixes)
- Modify: `src/eui_font_u8g2.c` (rendering fixes)

- [ ] **Step 1: Analyze test output for profont10 scenarios**

```bash
./build/test/test_font_vs_u8g2 2>&1 | grep -A2 "profont10\|Pure\|Upper\|Lower\|Mixed\|Number\|Punct\|Single\|Space\|Empty\|Amiibo"
```

For each failed profont10 scenario, inspect the mismatch details. Common issues:
- **Glyph positioning**: `eui_font_u8g2_draw_glyph()` places glyph rows differently than u8g2 (different baseline/y_offset interpretation)
- **Bitmap decoding**: `get_bitmap_pixel()` RLE decode differs from u8g2's decode
- **Advance width**: x_advance calculation differs

- [ ] **Step 2: Fix positioning bug (most likely issue)**

If glyphs are shifted by 1 row, the formula in `eui_font_u8g2_draw_glyph()` needs adjustment.

Current eui formula:
```c
int16_t buf_row = (int16_t)baseline - (int16_t)g.height - (int16_t)g.y_offset + (int16_t)py;
```

u8g2 equivalent: glyph row `py` is at pixel row `baseline + y_offset + py`.

If they differ by `delta = (baseline - height - y_offset) - (baseline + y_offset) = -height - 2*y_offset`, update accordingly.

- [ ] **Step 3: Fix RLE decode (if bitmap content differs)**

Compare `get_bitmap_pixel()` decoding vs u8g2's `u8g2_font_decode_glyph()` logic. The RLE format (paired runs with continue bit) must match exactly.

- [ ] **Step 4: Rebuild and re-test until profont10 scenarios all pass**

```bash
cmake --build build --target test_font_vs_u8g2 -j4 && ./build/test/test_font_vs_u8g2
```

- [ ] **Step 5: Commit fixes**

```bash
git add src/eui_font_u8g2.c test/test_font_vs_u8g2.c && git commit -m "fix: correct ASCII glyph rendering to match u8g2"
```

---

### Task 7: Debug and fix CJK/unicode rendering

**Files:**
- Modify: `src/eui_font_u8g2.c` — fix unicode glyph rendering

- [ ] **Step 1: Analyze wqy12 scenario failures**

```bash
./build/test/test_font_vs_u8g2 2>&1 | grep -A5 "Chinese\|CJK"
```

Likely issues:
1. `eui_font_u8g2_draw_glyph()` returns 0 for many CJK code points (glyph not found or decode fails)
2. Unicode lookup `find_glyph_data_unicode()` has cumulative offset errors in block iteration
3. `buf_row` calculation for Chinese chars (large height, different y_offset) goes negative

- [ ] **Step 2: Fix find_glyph_data_unicode boundary bug**

The jump table iteration may have an off-by-one error when transitioning between blocks. Check:
- `glyph_ptr += block_off;` then later `glyph_ptr = block_end;`
- The `next_off` value: ensure `block_end = glyph_ptr + next_off` is correct

- [ ] **Step 3: Fix buf_row going negative for tall Chinese glyphs**

For CJK glyphs with y_offset ~= -height (entire glyph above baseline):
```c
buf_row = baseline - height - y_offset + py
        = baseline - height - (-height) + py
        = baseline + py
```

This is fine — first row starts at `baseline` (typically 10), not negative.

If y_offset = -12 (taller glyph), height = 13, baseline = 10:
```
buf_row = 10 - 13 - (-12) + py = 9 + py
```
First row at 9, last at 21. Buffer may need to be taller.

- [ ] **Step 4: Iterate: fix → rebuild → test → repeat**

```bash
cmake --build build --target test_font_vs_u8g2 -j4 && ./build/test/test_font_vs_u8g2
```

Repeat until all 16 scenarios pass with 0 mismatches in both Layer 1 and Layer 2.

- [ ] **Step 5: Final verification**

```bash
cmake --build build -j4 && ctest -R font_vs_u8g2 --output-on-failure
```

Expected: `font_vs_u8g2` test PASSES.

- [ ] **Step 6: Commit final fixes**

```bash
git add src/eui_font_u8g2.c && git commit -m "fix: correct CJK unicode glyph rendering to match u8g2"
```

---

### Task 8: Final polish and full test run

**Files:**
- Modify: `test/test_font_vs_u8g2.c` — cleanup debug output, use TEST/PASS/FAIL macros

- [ ] **Step 1: Integrate TEST/PASS/FAIL macros into scenario reporting**

Wrap each scenario's Layer 1 and Layer 2 calls with TEST/PASS/FAIL:
```c
char buf[128];
snprintf(buf, sizeof(buf), "S%zu %s Layer1", i+1, sc->desc);
TEST(buf);
int gf = test_glyph_compare(sc);
if (gf) FAIL("mismatch"); else PASS();

snprintf(buf, sizeof(buf), "S%zu %s Layer2", i+1, sc->desc);
TEST(buf);
int cf = test_canvas_compare(sc);
if (cf) FAIL("mismatch"); else PASS();
```

- [ ] **Step 2: Run full test suite**

```bash
cmake --build build -j4 && ctest --output-on-failure -j4
```

Expected: All tests pass, including `font_vs_u8g2`.

- [ ] **Step 3: Final commit**

```bash
git add test/test_font_vs_u8g2.c && git commit -m "test: finalize font_vs_u8g2 with TEST/PASS/FAIL macros"
```
