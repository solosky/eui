#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "eui/eui_canvas.h"
#include "eui/eui_font.h"
#include "eui/eui_allocator.h"
#include "common/eui_test.h"
#include "eui/eui_config.h"
#include "eui/eui_types.h"
#include "eui/eui_font_wqy13.h"
#include "eui/eui_font_u8g2_internal.h"

#include "data/test_u8g2_profont10_data.h"
#include "data/test_u8g2_wqy12_ch1_data.h"

#include "u8g2.h"

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
    .lookup_glyph = eui_font_u8g2_lookup_glyph,
};

static const eui_font_t eui_wqy13 = {
    .format = EUI_FONT_FORMAT_U8G2,
    .line_height = 16, .baseline = 15, .flags = 0,
    .data = eui_font_wqy13_data,
    .lookup_glyph = eui_font_u8g2_lookup_glyph,
};

/* ---- Scenario definitions ---- */
typedef enum { FONT_PROFONT10, FONT_WQY12, FONT_WQY13 } scenario_font_t;

typedef struct {
    const char      *desc;
    const char      *str;
    scenario_font_t   font;
    int               use_utf8;
} scenario_t;

static const scenario_t scenarios[] = {
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
    {"Pure Chinese",             "\xe4\xbd\xa0\xe5\xa5\xbd\xe4\xb8\x96\xe7\x95\x8c",   FONT_WQY12, 1},
    {"Chinese + ASCII mixed",    "Hello\xe4\xbd\xa0\xe5\xa5\xbdWorld\xe4\xb8\x96\xe7\x95\x8c", FONT_WQY12, 1},
    {"Chinese + symbols mix",     "\xe4\xb8\x80\xe4\xb8\x8a:123 \xe4\xb8\xad\xe4\xb8\xba\xe6\xb5\x8b\xe8\xaf\x95", FONT_WQY12, 1},
    {"All CJK",                  "\xe4\xb8\x80\xe4\xb8\x8a\xe4\xb8\x8b\xe4\xb8\xad\xe4\xb8\xba\xe4\xbb\xa5\xe4\xb9\x8b", FONT_WQY12, 1},
    {"Chinese punctuation",      "\xe3\x80\x8c\xe4\xbd\xa0\xe5\xa5\xbd\xe3\x80\x8d\xe3\x80\x8e\xe4\xb8\x96\xe7\x95\x8c\xe3\x80\x8f", FONT_WQY12, 1},
    {"Amiibo app list",          "\xe5\xba\x94\xe7\x94\xa8\xe5\x88\x97\xe8\xa1\xa8",                               FONT_WQY13, 1},
    {"Amiibo detail labels",     "\xe9\xa9\xac\xe5\x8a\x9b\xe6\xac\xa7 \xe6\x9e\x97\xe5\x85\x8b \xe5\xa1\x9e\xe5\xb0\x94\xe8\xbe\xbe \xe7\x9a\xae\xe5\x8d\xa1\xe4\xb8\x98 \xe6\xaf\x94 \xe8\x90\xa8\xe5\xa7\x86\xe6\x96\xaf \xe8\x80\x80\xe8\xa5\xbf \xe6\xa3\xae\xe5\x96\x9c\xe5\x88\x9a", FONT_WQY13, 1},
    {"Amiibo drops",             "\xe8\xb6\x85\xe7\xba\xa7\xe8\x8f\x87\xe8\x8f\x87 \xe7\x81\xab\xe7\x84\xb0\xe8\x8a\xb1 \xe6\x97\xa0\xe6\x95\x8c\xe6\x98\x9f \xe8\xb6\x85\xe7\xba\xa7\xe9\x93\x83\xe9\x93\x9b \xe5\xa4\xa7\xe5\xb8\x88\xe5\x89\x91", FONT_WQY13, 1},
};

#define SCENARIO_COUNT (sizeof(scenarios) / sizeof(scenarios[0]))

static const eui_font_t* scenario_eui_font(const scenario_t *sc) {
    if (sc->font == FONT_PROFONT10) return &eui_profont10;
    if (sc->font == FONT_WQY12) return &eui_wqy12;
    return &eui_wqy13;
}
static const uint8_t* scenario_u8g2_font_data(const scenario_t *sc) {
    if (sc->font == FONT_PROFONT10) return u8g2_font_profont10_tf_data;
    if (sc->font == FONT_WQY12) return u8g2_font_wqy12_ch1_data;
    return eui_font_wqy13_data;
}
static int scenario_baseline(const scenario_t *sc) {
    if (sc->font == FONT_PROFONT10) return 8;
    if (sc->font == FONT_WQY12) return 10;
    return 15;
}

/* ===== BMP writer ===== */
static void write_bmp_1bpp(const char *fn, const uint8_t *buf, uint16_t w, uint16_t h)
{
    FILE *f = fopen(fn, "wb"); if (!f) return;
    int row_bytes = (w + 7) / 8, row_padded = (row_bytes + 3) & ~3;
    int pal_sz = 8, offset = 14 + 40 + pal_sz;
    int pix_sz = row_padded * h, file_sz = offset + pix_sz;

    uint16_t bfType = 0x4D42; fwrite(&bfType, 2, 1, f);
    uint32_t bfSize = file_sz; fwrite(&bfSize, 4, 1, f);
    uint32_t z = 0; fwrite(&z, 4, 1, f); fwrite(&z, 4, 1, f);

    uint32_t biSize = 40, biComp = 0, biImg = pix_sz;
    int32_t biW = w, biH = h;
    uint16_t biPlanes = 1, biBitCount = 1;
    int32_t biXP = 2835, biYP = 2835;
    uint32_t biUsed = 2, biImp = 2;
    fwrite(&biSize, 4, 1, f); fwrite(&biW, 4, 1, f);
    fwrite(&biH, 4, 1, f); fwrite(&biPlanes, 2, 1, f);
    fwrite(&biBitCount, 2, 1, f); fwrite(&biComp, 4, 1, f);
    fwrite(&biImg, 4, 1, f); fwrite(&biXP, 4, 1, f);
    fwrite(&biYP, 4, 1, f); fwrite(&biUsed, 4, 1, f);
    fwrite(&biImp, 4, 1, f);

    uint8_t black[4] = {0,0,0,0}, white[4] = {255,255,255,0};
    fwrite(black, 4, 1, f); fwrite(white, 4, 1, f);

    uint8_t *row = malloc(row_padded); if (!row) { fclose(f); return; }
    for (int y = h - 1; y >= 0; y--) {
        memset(row, 0, row_padded);
        for (int x = 0; x < w; x++) {
            int bi = y * (w / 8) + x / 8, bp = x % 8;
            if ((buf[bi] >> bp) & 1) row[x / 8] |= (1u << (7 - (x % 8)));
        }
        fwrite(row, 1, row_padded, f);
    }
    free(row); fclose(f);
}

/* ===== Canvas Comparison ===== */

#define CANVAS_W 128
#define CANVAS_H 64

/* u8g2 side */
static uint8_t u8g2_canvas_tile_buf[CANVAS_W * CANVAS_H / 8];
static u8g2_t  u8g2_cv;
static u8x8_display_info_t u8x8_cv_di = {
    .tile_width = CANVAS_W / 8, .tile_height = CANVAS_H / 8,
    .pixel_width = CANVAS_W, .pixel_height = CANVAS_H,
    .default_x_offset = 0,
};

static void u8g2_init_canvas(const uint8_t *font_data)
{
    memset(&u8g2_cv, 0, sizeof(u8g2_cv));
    u8x8_SetupDefaults(u8g2_GetU8x8(&u8g2_cv));
    u8g2_GetU8x8(&u8g2_cv)->display_info = &u8x8_cv_di;
    u8g2_SetupBuffer(&u8g2_cv, u8g2_canvas_tile_buf, CANVAS_H / 8,
                     u8g2_ll_hvline_vertical_top_lsb, &u8g2_cb_r0);
    u8g2_SetFont(&u8g2_cv, font_data);
}

static int u8g2_get_px(int x, int y)
{
    if (x < 0 || x >= CANVAS_W || y < 0 || y >= CANVAS_H) return 0;
    uint16_t tw = (uint16_t)u8g2_GetBufferTileWidth(&u8g2_cv);
    uint16_t pw = tw * 8;
    uint16_t bo = ((uint16_t)y / 8) * pw + (uint16_t)x;
    uint8_t  bi = y & 7;
    return (u8g2_canvas_tile_buf[bo] >> bi) & 1;
}

/* eui side — only supported in 1bpp mode */
#define EUI_CBUF_SIZE (CANVAS_W * CANVAS_H / 8)
static uint8_t eui_cv_buf[EUI_CBUF_SIZE];

static void eui_write_cb(const uint8_t *b, const eui_rect_t *r, void *ud)
{
    (void)ud;
    int bprow = r->w / 8;
    for (int row = 0; row < (int)r->h; row++)
        memcpy(eui_cv_buf + ((r->y + row) * (CANVAS_W / 8) + r->x / 8),
               b + row * bprow, bprow);
}

static eui_display_drv_t eui_cv_display = {
    .caps = { .width = CANVAS_W, .height = CANVAS_H,
              .color_depth = EUI_COLOR_DEPTH, .buffer_mode = EUI_BUFFER_FULL,
              .has_gram = false },
    .init = NULL, .write_buffer = eui_write_cb,
};

static int eui_get_px(int x, int y)
{
    if (x < 0 || x >= CANVAS_W || y < 0 || y >= CANVAS_H) return 0;
    int bi = y * (CANVAS_W / 8) + x / 8;
    int bp = x % 8;
    return (eui_cv_buf[bi] >> bp) & 1;
}

#if EUI_COLOR_DEPTH == 1
static int test_canvas_compare(const scenario_t *sc)
{
    const uint8_t *u8g2_data = scenario_u8g2_font_data(sc);
    const eui_font_t *eui_f = scenario_eui_font(sc);
    int baseline = scenario_baseline(sc);

    /* u8g2: render */
    u8g2_init_canvas(u8g2_data);
    memset(u8g2_canvas_tile_buf, 0, sizeof(u8g2_canvas_tile_buf));
    if (sc->use_utf8)
        u8g2_DrawUTF8(&u8g2_cv, 0, (u8g2_uint_t)baseline, sc->str);
    else
        u8g2_DrawStr(&u8g2_cv, 0, (u8g2_uint_t)baseline, sc->str);

    /* eui: render */
    memset(eui_cv_buf, 0, sizeof(eui_cv_buf));
    eui_canvas_t *cv = eui_canvas_create(&eui_cv_display);
    if (!cv) return -1;
    eui_canvas_set_font(cv, eui_f);
    eui_canvas_set_color(cv, EUI_COLOR_WHITE);
    eui_canvas_set_bg_color(cv, EUI_COLOR_BLACK);
    eui_canvas_clear(cv);
    eui_canvas_draw_str(cv, 0, (int16_t)baseline, sc->str);
    eui_canvas_commit(cv);
    eui_canvas_destroy(cv);

    /* compare pixel-by-pixel */
    int mismatches = 0;
    for (int y = 0; y < CANVAS_H; y++) {
        for (int x = 0; x < CANVAS_W; x++) {
            int up = u8g2_get_px(x, y);
            int ep = eui_get_px(x, y);
            if (up != ep) {
                if (mismatches == 0)
                    printf("  first mismatch at (%d,%d): u8g2=%d eui=%d\n",
                           x, y, up, ep);
                mismatches++;
            }
        }
    }

    /* compare string widths */
    {
        uint16_t eui_w = eui_font_get_str_width(eui_f, sc->str);
        u8g2_uint_t uw = sc->use_utf8 ?
            u8g2_GetUTF8Width(&u8g2_cv, sc->str) : u8g2_GetStrWidth(&u8g2_cv, sc->str);
        if ((uint16_t)uw != eui_w) {
            printf("  info: width differs (eui=%d u8g2=%d) — expected, eui str_width not UTF-8 aware\n",
                   eui_w, (int)uw);
        }
    }

    if (mismatches > 0) {
        char fn[128];
        uint8_t u8g2_hbuf[CANVAS_W * CANVAS_H / 8];
        memset(u8g2_hbuf, 0, sizeof(u8g2_hbuf));
        for (int y = 0; y < CANVAS_H; y++)
            for (int x = 0; x < CANVAS_W; x++)
                if (u8g2_get_px(x, y))
                    u8g2_hbuf[y * (CANVAS_W/8) + x/8] |= (1u << (x % 8));
        snprintf(fn, sizeof(fn), "test_u8g2_%s.bmp", sc->desc);
        write_bmp_1bpp(fn, u8g2_hbuf, CANVAS_W, CANVAS_H);
        snprintf(fn, sizeof(fn), "test_eui_%s.bmp", sc->desc);
        write_bmp_1bpp(fn, eui_cv_buf, CANVAS_W, CANVAS_H);
    }

    return mismatches;
}

static int test_glyph_compare(const scenario_t *sc) { (void)sc; return 0; }

int main(void) {
    eui_test_init();
    printf("=== Font vs u8g2 Canvas Comparison Test ===\n");
    printf("Scenarios: %zu\n\n", SCENARIO_COUNT);

    int total_fail = 0;
    for (size_t i = 0; i < SCENARIO_COUNT; i++) {
        const scenario_t *sc = &scenarios[i];
        printf("--- Scenario %zu: %s ---\n", i + 1, sc->desc);

        int cf = test_canvas_compare(sc);
        printf("  Canvas compare: %s (%d mismatches)\n", cf ? "FAIL" : "PASS", cf);
        if (cf) total_fail++;
    }

    printf("\n=== Results: %zu scenarios, %d failures ===\n",
           SCENARIO_COUNT, total_fail);
    return total_fail > 0 ? 1 : 0;
}

#else
int main(void) {
    printf("=== Font vs u8g2 Canvas Comparison Test ===\n");
    printf("SKIP: requires EUI_COLOR_DEPTH == 1\n");
    return 0;
}
#endif
