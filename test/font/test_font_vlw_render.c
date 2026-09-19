#include "eui/eui_canvas.h"
#include "eui/eui_font.h"
#include "eui/eui_font_internal.h"
#include "eui/eui_types.h"
#include "eui/eui_allocator.h"
#include "common/eui_test.h"
#include "eui/eui_config.h"
#include "data/test_vlw_font.h"
#include <stdio.h>
#include <string.h>

#define MOCK_W 128
#define MOCK_H 64
#if EUI_COLOR_DEPTH == 1
#define MOCK_BUF_SIZE (MOCK_W * MOCK_H / 8)
#elif EUI_COLOR_DEPTH == 8
#define MOCK_BUF_SIZE (MOCK_W * MOCK_H)
#else
#define MOCK_BUF_SIZE (MOCK_W * MOCK_H * 2)
#endif

static uint8_t mock_buf[MOCK_BUF_SIZE];

static void mock_write_buffer(const uint8_t *b, const eui_rect_t *r, void *ud)
{
    (void)ud;
#if EUI_COLOR_DEPTH == 1
    int bytes_per_row = r->w / 8;
    for (int row = 0; row < (int)r->h; row++) {
        memcpy(mock_buf + ((r->y + row) * (MOCK_W / 8) + r->x / 8),
               b + row * bytes_per_row, bytes_per_row);
    }
#elif EUI_COLOR_DEPTH == 8
    int bytes_per_row = r->w;
    for (int row = 0; row < (int)r->h; row++) {
        memcpy(mock_buf + ((r->y + row) * MOCK_W + r->x),
               b + row * bytes_per_row, bytes_per_row);
    }
#else
    int bytes_per_row = r->w * 2;
    for (int row = 0; row < (int)r->h; row++) {
        memcpy(mock_buf + ((r->y + row) * MOCK_W * 2 + r->x * 2),
               b + row * bytes_per_row, bytes_per_row);
    }
#endif
}

static eui_display_drv_t mock_display = {
    .caps = { .width = MOCK_W, .height = MOCK_H, .color_depth = EUI_COLOR_DEPTH, .buffer_mode = EUI_BUFFER_FULL, .has_gram = false },
    .init = NULL,
    .write_buffer = mock_write_buffer,
};

static int pixel_is_ink(int x, int y)
{
#if EUI_COLOR_DEPTH == 1
    uint8_t b = mock_buf[y * (MOCK_W / 8) + x / 8];
    return (b >> (7 - (x % 8))) & 1;
#elif EUI_COLOR_DEPTH == 8
    return mock_buf[y * MOCK_W + x] != 0;
#else
    uint16_t v = ((uint16_t*)mock_buf)[y * MOCK_W + x];
    return v != 0;
#endif
}

/* Synthetic font glyph cell: ascent 10, descent 2, glyphs 8x8 with
 * dy=8 (top 2 rows below cell top), dx=0, xAdvance 8. */

static void test_vlw_init_metrics(void)
{
    TEST("eui_font_vlw_init derives metrics from data");
    eui_font_t f;
    memset(&f, 0, sizeof(f));
    eui_font_vlw_init(&f, test_vlw_font_data);
    if (f.format != EUI_FONT_FORMAT_VLW) FAIL("format should be VLW");
    if (f.data != test_vlw_font_data) FAIL("data pointer not set");
    if (f.baseline != 10) FAIL("baseline should be 10");
    if (f.line_height != 12) FAIL("line_height should be 12");
    PASS();
}

static void test_vlw_find_glyph(void)
{
    TEST("eui_font_vlw_find_glyph returns glyph fields");
    eui_font_t f;
    eui_font_vlw_init(&f, test_vlw_font_data);
    eui_vlw_glyph_t g;
    if (!eui_font_vlw_find_glyph(&f, 'A', &g)) FAIL("'A' should be found");
    if (g.height != 8 || g.width != 8) FAIL("'A' should be 8x8");
    if (g.x_advance != 8) FAIL("'A' advance should be 8");
    if (g.dy != 8) FAIL("'A' dy should be 8");
    if (g.dx != 0) FAIL("'A' dx should be 0");
    if (!g.bitmap) FAIL("bitmap pointer missing");
    if (eui_font_vlw_find_glyph(&f, 'z', &g)) FAIL("'z' should not be found");
    PASS();
}

static void test_vlw_canvas_render_position(void)
{
    TEST("canvas draw_str renders VLW glyph at baseline position");
    eui_font_t f;
    eui_font_vlw_init(&f, test_vlw_font_data);

    eui_canvas_t *c = eui_canvas_create(&mock_display);
    eui_canvas_set_font(c, &f);
    eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);
    eui_canvas_set_color(c, EUI_COLOR_WHITE);
    eui_canvas_clear(c);
    memset(mock_buf, 0, sizeof(mock_buf));

    uint16_t w = eui_canvas_draw_str(c, 0, 0, "A");
    eui_canvas_commit(c);
    eui_canvas_destroy(c);

    if (w != 8) FAIL("advance for 'A' should be 8");

    /* glyph top sits at (baseline - dy) = 2 rows below cell top */
    if (pixel_is_ink(0, 0) || pixel_is_ink(4, 0)) FAIL("row 0 should be empty");
    if (pixel_is_ink(0, 1) || pixel_is_ink(4, 1)) FAIL("row 1 should be empty");

    /* glyph row 0 = 0x18 = cols 3,4 ink; row 1 = 0x3C = cols 2..5 ink */
    if (!pixel_is_ink(3, 2) || !pixel_is_ink(4, 2)) FAIL("glyph row 0 missing at canvas row 2");
    if (pixel_is_ink(0, 2)) FAIL("glyph row 0 col 0 should be empty");
    if (!pixel_is_ink(2, 3) || !pixel_is_ink(5, 3)) FAIL("glyph row 1 missing at canvas row 3");

    /* total ink pixels: 2+4+4+6+4+4+4+0 = 28 */
    int count = 0;
    for (int y = 0; y < MOCK_H; y++)
        for (int x = 0; x < MOCK_W; x++)
            if (pixel_is_ink(x, y)) count++;
    if (count != 28) {
        printf("FAIL: expected 28 ink pixels, got %d\n", count);
        for (int y = 0; y < 12; y++) {
            printf("%2d |", y);
            for (int x = 0; x < 10; x++)
                printf("%c", pixel_is_ink(x, y) ? '#' : '.');
            printf("|\n");
        }
        return;
    }
    PASS();
}

static void test_vlw_canvas_missing_glyph(void)
{
    TEST("canvas draw_str skips missing VLW glyph");
    eui_font_t f;
    eui_font_vlw_init(&f, test_vlw_font_data);

    eui_canvas_t *c = eui_canvas_create(&mock_display);
    eui_canvas_set_font(c, &f);
    eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);
    eui_canvas_set_color(c, EUI_COLOR_WHITE);
    eui_canvas_clear(c);
    memset(mock_buf, 0, sizeof(mock_buf));

    uint16_t w = eui_canvas_draw_str(c, 0, 0, "zZ");
    eui_canvas_commit(c);
    eui_canvas_destroy(c);

    if (w != 0) FAIL("missing glyphs should advance 0");
    int count = 0;
    for (int i = 0; i < (int)sizeof(mock_buf); i++)
        if (mock_buf[i]) count++;
    if (count != 0) FAIL("missing glyphs should not render pixels");
    PASS();
}

static void test_vlw_canvas_two_chars(void)
{
    TEST("canvas draw_str advances between VLW glyphs");
    eui_font_t f;
    eui_font_vlw_init(&f, test_vlw_font_data);

    eui_canvas_t *c = eui_canvas_create(&mock_display);
    eui_canvas_set_font(c, &f);
    eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);
    eui_canvas_set_color(c, EUI_COLOR_WHITE);
    eui_canvas_clear(c);
    memset(mock_buf, 0, sizeof(mock_buf));

    uint16_t w = eui_canvas_draw_str(c, 0, 0, "AB");
    eui_canvas_commit(c);
    eui_canvas_destroy(c);

    if (w != 16) FAIL("two glyphs should advance 16");

    /* second glyph starts at x=8: 'B' row 0 = 0x7C = cols 2..5 -> canvas x 10..13 */
    if (!pixel_is_ink(10, 2) || !pixel_is_ink(13, 2)) FAIL("glyph 'B' missing at x+8");
    if (pixel_is_ink(8, 2)) FAIL("gap between glyphs should be empty");
    PASS();
}

int main(void)
{
    eui_test_init();
    printf("=== VLW Render Tests ===\n");
    test_vlw_init_metrics();
    test_vlw_find_glyph();
    test_vlw_canvas_render_position();
    test_vlw_canvas_missing_glyph();
    test_vlw_canvas_two_chars();
    return eui_test_summary();
}
