#include "eui/eui_font.h"
#include "eui/eui_config.h"
#include "eui/eui_types.h"
#include "eui/eui_allocator.h"
#include "eui/eui_font_wqy13.h"
#include "common/eui_test.h"
#include <stdio.h>

#if EUI_FONT_ENABLE_U8G2 && EUI_FONT_ENABLE_KERNING

#include "eui/eui_font_internal.h"
#include "eui/eui_font_u8g2_internal.h"

#if EUI_COLOR_DEPTH == 1
#include "u8g2.h"
#include "eui/eui_canvas.h"
#include <string.h>
#include <stdlib.h>
#endif

static void check_kerning_pair(const eui_font_t *font, char first, char second,
                                const char *desc)
{
    /* Look up first char normally (no prev) */
    int32_t idx1 = font->lookup_glyph(font, (uint16_t)(uint8_t)first, 0);
    if (idx1 < 0) { printf("  %c->%c: first char not found\n", first, second); return; }
    u8g2_glyph_t g1;
    decode_glyph_at(font, (uint32_t)idx1, &g1);

    /* Look up second char with prev=first (kerning pair lookup) */
    int32_t idx2 = font->lookup_glyph(font, (uint16_t)(uint8_t)second, (uint16_t)(uint8_t)first);
    if (idx2 < 0) { printf("  %c->%c: second char not found\n", first, second); return; }
    u8g2_glyph_t g2;
    decode_glyph_at(font, (uint32_t)idx2, &g2);

    /* Look up second char normally (no prev) for comparison */
    int32_t idx3 = font->lookup_glyph(font, (uint16_t)(uint8_t)second, 0);
    if (idx3 < 0) { printf("  %c->%c: second char normal not found\n", first, second); return; }
    u8g2_glyph_t g3;
    decode_glyph_at(font, (uint32_t)idx3, &g3);

    printf("  %-20s: 1st='%c' (adv=%u)  2nd='%c' prev='%c' (adv=%u xoff=%d)  "
           "2nd_no_prev='%c' (adv=%u xoff=%d)  %s\n",
           desc,
           first, g1.x_advance,
           second, first, g2.x_advance, g2.x_offset,
           second, g3.x_advance, g3.x_offset,
           (g2.x_advance != g3.x_advance || g2.x_offset != g3.x_offset) ? "KERN!" : "no kern");
}

static void test_wqy13_kerning_pairs(void)
{
    printf("wqy13 English kerning pair check:\n");

    /* Common Latin kerning pairs that proportional fonts often kern */
    check_kerning_pair(&eui_font_wqy13, 'T', 'o', "To");
    check_kerning_pair(&eui_font_wqy13, 'T', 'a', "Ta");
    check_kerning_pair(&eui_font_wqy13, 'A', 'V', "AV");
    check_kerning_pair(&eui_font_wqy13, 'W', 'A', "WA");
    check_kerning_pair(&eui_font_wqy13, 'V', 'A', "VA");
    check_kerning_pair(&eui_font_wqy13, 'P', 'o', "Po");
    check_kerning_pair(&eui_font_wqy13, 'F', 'i', "Fi");
    check_kerning_pair(&eui_font_wqy13, 'T', 'T', "TT");
    check_kerning_pair(&eui_font_wqy13, 'L', 'T', "LT");
    check_kerning_pair(&eui_font_wqy13, 'Y', 'o', "Yo");
}

static void test_wqy13_kerning_width_comparison(void)
{
    TEST("wqy13 'To' width same with and without kerning context");
    uint16_t w_kerning = 0, w_nokern = 0;
    const char *str = "To";
    
    /* Kerning-aware width */
    {
        uint16_t prev = 0;
        const char *s = str;
        while (*s) {
            uint16_t c = (uint8_t)*s;
            int32_t idx = eui_font_wqy13.lookup_glyph(&eui_font_wqy13, c, prev);
            if (idx >= 0) {
                u8g2_glyph_t g;
                if (decode_glyph_at(&eui_font_wqy13, (uint32_t)idx, &g))
                    w_kerning += g.x_advance;
            }
            prev = c;
            s++;
        }
    }
    
    /* No-kerning width */
    {
        const char *s = str;
        while (*s) {
            uint16_t c = (uint8_t)*s;
            int32_t idx = eui_font_wqy13.lookup_glyph(&eui_font_wqy13, c, 0);
            if (idx >= 0) {
                u8g2_glyph_t g;
                if (decode_glyph_at(&eui_font_wqy13, (uint32_t)idx, &g))
                    w_nokern += g.x_advance;
            }
            s++;
        }
    }
    
    if (w_kerning != w_nokern) FAIL("width differs with kerning context");
    if (w_kerning == 0) FAIL("width is 0");
    printf(" (width=%u) PASS\n", w_kerning);
    tests_passed++;
}

static void test_wqy13_ascii_is_proportional(void)
{
    TEST("wqy13 ASCII chars have varying advance (proportional, not monospaced)");
    uint8_t min_adv = 255, max_adv = 0;
    int count = 0;
    for (char c = 0x21; c <= 0x7E; c++) {
        int32_t idx = eui_font_wqy13.lookup_glyph(&eui_font_wqy13, (uint8_t)c, 0);
        if (idx < 0) continue;
        u8g2_glyph_t g;
        if (!decode_glyph_at(&eui_font_wqy13, (uint32_t)idx, &g)) continue;
        count++;
        if (g.x_advance < min_adv) min_adv = g.x_advance;
        if (g.x_advance > max_adv) max_adv = g.x_advance;
    }
    if (count == 0) FAIL("no ASCII glyphs found");
    if (min_adv == max_adv) FAIL("unexpectedly monospaced");
    printf(" (adv range: %u-%u, %d chars) ", min_adv, max_adv, count);
    PASS();
}

#if EUI_COLOR_DEPTH == 1

/* ================= BMP 1bpp writer ================= */
static void write_bmp_1bpp(const char *fn, const uint8_t *buf, uint16_t w, uint16_t h)
{
    FILE *f = fopen(fn, "wb"); if (!f) { printf("  cannot open %s\n", fn); return; }
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

    uint8_t *row = malloc(row_padded);
    for (int y = h - 1; y >= 0; y--) {
        memset(row, 0, row_padded);
        for (int x = 0; x < w; x++) {
            int bi = y * (w / 8) + x / 8, bp = x % 8;
            if ((buf[bi] >> bp) & 1) row[x / 8] |= (1u << (7 - (x % 8)));
        }
        fwrite(row, 1, row_padded, f);
    }
    free(row); fclose(f);
    printf("  saved: %s (%ux%u)\n", fn, w, h);
}

/* ================= u8g2 canvas ================= */
#define CANVAS_W 128
#define CANVAS_H 48

static uint8_t u8g2_tile_buf[CANVAS_W * CANVAS_H / 8];
static u8g2_t  u8g2_cv;
static u8x8_display_info_t u8x8_di = {
    .tile_width = CANVAS_W / 8, .tile_height = CANVAS_H / 8,
    .pixel_width = CANVAS_W, .pixel_height = CANVAS_H,
    .default_x_offset = 0,
};

static void u8g2_canvas_init(const uint8_t *font_data)
{
    memset(&u8g2_cv, 0, sizeof(u8g2_cv));
    u8x8_SetupDefaults(u8g2_GetU8x8(&u8g2_cv));
    u8g2_GetU8x8(&u8g2_cv)->display_info = &u8x8_di;
    u8g2_SetupBuffer(&u8g2_cv, u8g2_tile_buf, CANVAS_H / 8,
                     u8g2_ll_hvline_vertical_top_lsb, &u8g2_cb_r0);
    u8g2_SetFont(&u8g2_cv, font_data);
    u8g2_SetFontMode(&u8g2_cv, 1);
    u8g2_SetDrawColor(&u8g2_cv, 1);
}

/* ================= EUI canvas ================= */
static uint8_t eui_buf[CANVAS_W * CANVAS_H / 8];

static void eui_write_cb(const uint8_t *b, const eui_rect_t *r, void *ud)
{
    (void)ud;
    int bprow = r->w / 8;
    for (int row = 0; row < (int)r->h; row++)
        memcpy(eui_buf + ((r->y + row) * (CANVAS_W / 8) + r->x / 8),
               b + row * bprow, bprow);
}

static eui_display_drv_t eui_display = {
    .caps = { .width = CANVAS_W, .height = CANVAS_H,
              .color_depth = EUI_COLOR_DEPTH, .buffer_mode = EUI_BUFFER_FULL,
              .has_gram = false },
    .init = NULL, .write_buffer = eui_write_cb,
};

/* ================= Render & save ================= */

static void dump_kerning_visual_check(void)
{
    const uint8_t *font = eui_font_wqy13_data;

    /* --- Row 1: u8g2 native DrawUTF8 (no kerning, standard path) --- */
    printf("\n--- ROW 1: u8g2 DrawUTF8 (native, no kerning table) ---\n");
    memset(u8g2_tile_buf, 0, sizeof(u8g2_tile_buf));
    u8g2_canvas_init(font);
    u8g2_SetFontPosBottom(&u8g2_cv);

    int16_t x = 0, y = 14;
    x += (int16_t)u8g2_DrawUTF8(&u8g2_cv, x, y, "To");
    x += 12;
    x += (int16_t)u8g2_DrawUTF8(&u8g2_cv, x, y, "AV");
    x += 12;
    x += (int16_t)u8g2_DrawUTF8(&u8g2_cv, x, y, "WA");
    x += 12;
    x += (int16_t)u8g2_DrawUTF8(&u8g2_cv, x, y, "TT");
    x = 0; y = 30;
    x += (int16_t)u8g2_DrawUTF8(&u8g2_cv, x, y, "Hello");
    x += 12;
    x += (int16_t)u8g2_DrawUTF8(&u8g2_cv, x, y, "World");

    write_bmp_1bpp("test_kerning_u8g2_native.bmp", u8g2_tile_buf, CANVAS_W, CANVAS_H);

    /* --- Row 2: u8g2 DrawExtUTF8 with NULL kerning table (same as native) --- */
    printf("\n--- ROW 2: u8g2 DrawExtUTF8 (NULL kerning table) ---\n");
    memset(u8g2_tile_buf, 0, sizeof(u8g2_tile_buf));
    u8g2_canvas_init(font);
    u8g2_SetFontPosBottom(&u8g2_cv);

    x = 0; y = 14;
    x += (int16_t)u8g2_DrawExtUTF8(&u8g2_cv, x, y, 0, NULL, "To");
    x += 12;
    x += (int16_t)u8g2_DrawExtUTF8(&u8g2_cv, x, y, 0, NULL, "AV");
    x += 12;
    x += (int16_t)u8g2_DrawExtUTF8(&u8g2_cv, x, y, 0, NULL, "WA");
    x += 12;
    x += (int16_t)u8g2_DrawExtUTF8(&u8g2_cv, x, y, 0, NULL, "TT");
    x = 0; y = 30;
    x += (int16_t)u8g2_DrawExtUTF8(&u8g2_cv, x, y, 0, NULL, "Hello");
    x += 12;
    x += (int16_t)u8g2_DrawExtUTF8(&u8g2_cv, x, y, 0, NULL, "World");

    write_bmp_1bpp("test_kerning_u8g2_ext_null.bmp", u8g2_tile_buf, CANVAS_W, CANVAS_H);

    /* --- Row 3: EUI canvas draw_str (kerning-aware, prev is tracked) --- */
    printf("\n--- ROW 3: EUI draw_str (kerning-aware, prev tracked) ---\n");
    memset(eui_buf, 0, sizeof(eui_buf));
    eui_canvas_t *c = eui_canvas_create(&eui_display);
    eui_canvas_set_font(c, &eui_font_wqy13);

    x = 0; y = 14;
    x += (int16_t)eui_canvas_draw_str(c, x, y, "To");
    x += 12;
    x += (int16_t)eui_canvas_draw_str(c, x, y, "AV");
    x += 12;
    x += (int16_t)eui_canvas_draw_str(c, x, y, "WA");
    x += 12;
    x += (int16_t)eui_canvas_draw_str(c, x, y, "TT");
    x = 0; y = 30;
    x += (int16_t)eui_canvas_draw_str(c, x, y, "Hello");
    x += 12;
    x += (int16_t)eui_canvas_draw_str(c, x, y, "World");

    eui_canvas_commit(c);
    eui_canvas_destroy(c);

    write_bmp_1bpp("test_kerning_eui_wqy13.bmp", eui_buf, CANVAS_W, CANVAS_H);

    /* Save kerning-aware buffer for later diff */
    uint8_t eui_kerning_buf[sizeof(eui_buf)];
    memcpy(eui_kerning_buf, eui_buf, sizeof(eui_buf));

    /* --- Row 4: EUI char-by-char (prev=0, NO kerning path) --- */
    printf("\n--- ROW 4: EUI char-by-char (prev=0, no kerning lookup) ---\n");
    memset(eui_buf, 0, sizeof(eui_buf));

    const char *strings[] = {"To", "AV", "WA", "TT", "Hello", "World", NULL};
    int16_t cx = 0, cy = 14;
    for (int i = 0; i < 6; i++) {
        if (i == 4) { cx = 0; cy = 30; }
        int16_t lx = cx;
        for (const char *s = strings[i]; *s; s++) {
            int32_t idx = eui_font_wqy13.lookup_glyph(&eui_font_wqy13, (uint8_t)*s, 0);
            if (idx >= 0) {
                u8g2_glyph_t g;
                if (decode_glyph_at(&eui_font_wqy13, (uint32_t)idx, &g)) {
                    for (uint8_t row = 0; row < g.height; row++) {
                        for (uint8_t col = 0; col < g.width; col++) {
                            uint16_t px = (uint16_t)row * g.width + col;
                            if (get_bitmap_pixel(eui_font_wqy13.data, g.bitmap_byte,
                                                    g.bitmap_bit, px, g.width, g.height)) {
                                int16_t gx = lx + col + g.x_offset;
                                int16_t gy = (int16_t)(cy - g.height - g.y_offset + row);
                                if (gx >= 0 && gx < CANVAS_W && gy >= 0 && gy < CANVAS_H) {
                                    int bi = gy * (CANVAS_W / 8) + gx / 8;
                                    int bp = gx % 8;
                                    eui_buf[bi] |= (1u << bp);
                                }
                            }
                        }
                    }
                    lx += g.x_advance;
                }
            }
        }
        cx += lx - cx + 12;
    }
    write_bmp_1bpp("test_kerning_eui_nokern.bmp", eui_buf, CANVAS_W, CANVAS_H);

    /* --- Pixel diff: EUI kerning-aware vs EUI no-kerning --- */
    printf("\n--- Pixel diff: EUI kerning vs EUI no-kerning ---\n");
    int diff = 0;
    size_t buf_sz = sizeof(eui_buf);
    for (size_t i = 0; i < buf_sz; i++) {
        uint8_t d = eui_kerning_buf[i] ^ eui_buf[i];
        while (d) { diff++; d &= d - 1; }
    }
    if (diff == 0)
        printf("  IDENTICAL: kerning has ZERO visual impact on wqy13\n");
    else
        printf("  %d pixels differ — kerning HAS effect\n", diff);
}

#endif /* EUI_COLOR_DEPTH == 1 */

int main(void)
{
    eui_test_init();
    printf("=== wqy13 Kerning Tests ===\n");
    printf("font flags: 0x%02X (HAS_KERNING=%s, HAS_UNICODE=%s)\n",
           eui_font_wqy13.flags,
           (eui_font_wqy13.flags & EUI_FONT_HAS_KERNING) ? "yes" : "no",
           (eui_font_wqy13.flags & EUI_FONT_HAS_UNICODE) ? "yes" : "no");
    printf("font: wqy13, format=U8G2, height=%u, baseline=%u\n\n",
           eui_font_wqy13.line_height, eui_font_wqy13.baseline);

    test_wqy13_kerning_pairs();
    printf("\n");
    test_wqy13_kerning_width_comparison();
    test_wqy13_ascii_is_proportional();

#if EUI_COLOR_DEPTH == 1
    dump_kerning_visual_check();
#endif

    return eui_test_summary();
}

#else

int main(void)
{
    printf("=== wqy13 Kerning Tests ===\n");
    printf("  SKIP: EUI_FONT_ENABLE_U8G2 or EUI_FONT_ENABLE_KERNING disabled\n");
    return 0;
}

#endif
