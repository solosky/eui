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
    const char      *desc;
    const char      *str;
    scenario_font_t   font;
    int               use_utf8;
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
    {"Pure Chinese",             "\xe4\xbd\xa0\xe5\xa5\xbd\xe4\xb8\x96\xe7\x95\x8c",   FONT_WQY12, 1},
    {"Chinese + ASCII mixed",    "Hello\xe4\xbd\xa0\xe5\xa5\xbdWorld\xe4\xb8\x96\xe7\x95\x8c", FONT_WQY12, 1},
    {"Chinese + Unicode symbols","\xe6\xb8\xa9\xe5\xba\xa6:25\xe2\x84\x83 \xe2\x98\x85\xe8\xaf\x84\xe7\xba\xa7A", FONT_WQY12, 1},
    {"All CJK",                  "\xe4\xb8\x80\xe4\xb8\x8a\xe4\xb8\x8b\xe4\xb8\xad\xe4\xb8\xba\xe4\xbb\xa5\xe4\xb9\x8b", FONT_WQY12, 1},
    {"Chinese punctuation",      "\xe3\x80\x8c\xe4\xbd\xa0\xe5\xa5\xbd\xe3\x80\x8d\xe3\x80\x8e\xe4\xb8\x96\xe7\x95\x8c\xe3\x80\x8f", FONT_WQY12, 1},
};

#define SCENARIO_COUNT (sizeof(scenarios) / sizeof(scenarios[0]))

static const eui_font_t* scenario_eui_font(const scenario_t *sc) {
    return (sc->font == FONT_PROFONT10) ? &eui_profont10 : &eui_wqy12;
}
static const uint8_t* scenario_u8g2_font_data(const scenario_t *sc) {
    return (sc->font == FONT_PROFONT10)
        ? u8g2_font_profont10_tf_data : u8g2_font_wqy12_ch1_data;
}
static int scenario_baseline(const scenario_t *sc) {
    return (sc->font == FONT_PROFONT10) ? 8 : 10;
}

/* ---- Forward declarations ---- */
static int test_glyph_compare(const scenario_t *sc);
static int test_canvas_compare(const scenario_t *sc);
static void write_bmp_1bpp(const char *fn, const uint8_t *buf, uint16_t w, uint16_t h);

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
static void write_bmp_1bpp(const char *fn, const uint8_t *buf,
                           uint16_t w, uint16_t h) { (void)fn;(void)buf;(void)w;(void)h; }
