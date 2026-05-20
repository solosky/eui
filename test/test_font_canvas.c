#include "eui/eui_canvas.h"
#include "eui/eui_font.h"
#include "eui/eui_font_builtin.h"
#include "eui/eui_types.h"
#include "eui/eui_allocator.h"
#include "eui/eui_config.h"
#include <stdio.h>
#include <string.h>

#if EUI_FONT_ENABLE_U8G2
#include "test_u8g2_font.h"
#endif

#define POOL_SIZE 32768
static uint8_t mem_pool[POOL_SIZE];

static int tests_run = 0, tests_passed = 0;
#define TEST(n) do { printf("  %s... ", n); tests_run++; } while(0)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(m) do { printf("FAIL: %s\n", m); return; } while(0)

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

static eui_display_hal_t mock_display = {
    .caps = { .width = MOCK_W, .height = MOCK_H, .color_depth = EUI_COLOR_DEPTH, .buffer_mode = EUI_BUFFER_FULL, .has_gram = false },
    .init = NULL,
    .write_buffer = mock_write_buffer,
};

static int count_pixels(void)
{
#if EUI_COLOR_DEPTH == 1
    int count = 0;
    for (int i = 0; i < (int)sizeof(mock_buf); i++) {
        uint8_t b = mock_buf[i];
        while (b) { count += (b & 1); b >>= 1; }
    }
    return count;
#elif EUI_COLOR_DEPTH == 16
    int count = 0;
    uint16_t *p = (uint16_t*)mock_buf;
    int num = (int)sizeof(mock_buf) / 2;
    for (int i = 0; i < num; i++) {
        if (p[i] != 0) count++;
    }
    return count;
#else
    int count = 0;
    for (int i = 0; i < (int)sizeof(mock_buf); i++) {
        if (mock_buf[i] != 0) count++;
    }
    return count;
#endif
}

/* ---------------------------------------------------------------------------
 * Test 1: eui_canvas_set_font sets font correctly
 * ---------------------------------------------------------------------------*/
static void test_set_font(void)
{
    TEST("set_font sets font on canvas");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    eui_canvas_set_font(c, &eui_font_builtin);

    /* Verify font is set by checking str_width matches the font */
    uint16_t w = eui_canvas_str_width(c, "A");
    if (w != 8) FAIL("str_width should be 8 for built-in font 'A'");

    eui_canvas_destroy(c);
    PASS();
}

/* ---------------------------------------------------------------------------
 * Test 2: eui_canvas_draw_str renders text pixels (built-in font)
 * ---------------------------------------------------------------------------*/
static void test_draw_str_renders(void)
{
    TEST("draw_str renders text pixels");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    eui_canvas_set_font(c, &eui_font_builtin);
    eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);
    eui_canvas_set_color(c, EUI_COLOR_WHITE);

    eui_canvas_clear(c);
    memset(mock_buf, 0, sizeof(mock_buf));

    uint16_t w = eui_canvas_draw_str(c, 0, 0, "A");
    eui_canvas_commit(c);

    if (w != 8) FAIL("draw_str should return width 8 for 'A'");

    int px = count_pixels();
    if (px == 0) FAIL("draw_str should render some pixels for 'A'");

    /* Built-in 'A' glyph has exactly 18 bits set across 8 rows */
    if (px != 18) FAIL("pixel count for 'A' should be 18");

    eui_canvas_destroy(c);
    PASS();
}

/* ---------------------------------------------------------------------------
 * Test 3: NULL canvas returns 0
 * ---------------------------------------------------------------------------*/
static void test_draw_str_null_canvas(void)
{
    TEST("draw_str NULL canvas returns 0");
    if (eui_canvas_draw_str(NULL, 0, 0, "A") != 0) FAIL("expected 0");
    PASS();
}

/* ---------------------------------------------------------------------------
 * Test 4: NULL string returns 0
 * ---------------------------------------------------------------------------*/
static void test_draw_str_null_str(void)
{
    TEST("draw_str NULL string returns 0");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    eui_canvas_set_font(c, &eui_font_builtin);

    if (eui_canvas_draw_str(c, 0, 0, NULL) != 0) FAIL("expected 0");

    eui_canvas_destroy(c);
    PASS();
}

/* ---------------------------------------------------------------------------
 * Test 5: No font set returns 0
 * ---------------------------------------------------------------------------*/
static void test_draw_str_no_font(void)
{
    TEST("draw_str with no font returns 0");
    eui_canvas_t *c = eui_canvas_create(&mock_display);

    if (eui_canvas_draw_str(c, 0, 0, "A") != 0) FAIL("expected 0 with no font");

    eui_canvas_destroy(c);
    PASS();
}

/* ---------------------------------------------------------------------------
 * Test 6: Centered alignment places text correctly
 * ---------------------------------------------------------------------------*/
static void test_draw_str_aligned_center(void)
{
    TEST("draw_str_aligned centered places text correctly");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    eui_canvas_set_font(c, &eui_font_builtin);
    eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);
    eui_canvas_set_color(c, EUI_COLOR_WHITE);

    eui_canvas_clear(c);
    memset(mock_buf, 0, sizeof(mock_buf));

    /* Canvas 128x64, center at (64,32). "AB" width=16, height=8.
     * With ALIGN_CENTER|ALIGN_MIDDLE:
     *   dx = 64 - 16/2 = 56, dy = 32 - 8/2 = 28
     * So text renders at (56,28)..(71,35). */
    uint16_t w = eui_canvas_draw_str_aligned(c, 64, 32,
                                              EUI_ALIGN_CENTER, EUI_ALIGN_MIDDLE,
                                              "AB");
    eui_canvas_commit(c);

    if (w != 16) FAIL("centered draw_str 'AB' should return width 16");

    int px = count_pixels();
    if (px == 0) FAIL("centered text should produce pixels");

#if EUI_COLOR_DEPTH == 1
    /* Verify no pixels in the top-left corner (0,0) */
    {
        int bi = 0 * (MOCK_W / 8) + 0 / 8;
        if (mock_buf[bi] & (1u << 0))
            FAIL("centered text should not be in top-left corner");
    }
#endif

    eui_canvas_destroy(c);
    PASS();
}

/* ---------------------------------------------------------------------------
 * Test 7: u8g2 font renders via canvas (conditional)
 * ---------------------------------------------------------------------------*/
static void test_draw_str_u8g2_font(void)
{
#if EUI_FONT_ENABLE_U8G2
    TEST("draw_str renders u8g2 font via canvas");
    eui_canvas_t *c = eui_canvas_create(&mock_display);

    static const eui_font_t u8g2_test_font = {
        .format = EUI_FONT_FORMAT_U8G2,
        .line_height = TEST_U8G2_FONT_HEIGHT,
        .baseline = TEST_U8G2_FONT_BASELINE,
        .flags = EUI_FONT_FIXED_WIDTH,
        .data = test_u8g2_font_data,
        .lookup_glyph = NULL,
    };

    eui_canvas_set_font(c, &u8g2_test_font);
    eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);
    eui_canvas_set_color(c, EUI_COLOR_WHITE);

    eui_canvas_clear(c);
    memset(mock_buf, 0, sizeof(mock_buf));

    uint16_t w = eui_canvas_draw_str(c, 0, 0, "A");
    eui_canvas_commit(c);

    if (w != 8) FAIL("u8g2 draw_str should return width 8 for 'A'");

    int px = count_pixels();
    if (px == 0) FAIL("u8g2 draw_str should render pixels");

    /* u8g2 test font 'A' glyph has 28 bits set */
    if (px != 28) FAIL("u8g2 'A' pixel count should be 28");

    eui_canvas_destroy(c);
    PASS();
#else
    TEST("draw_str u8g2 font (skipped)");
    PASS();
#endif
}

/* ---------------------------------------------------------------------------
 * Test 8: eui_canvas_str_width matches font
 * ---------------------------------------------------------------------------*/
static void test_str_width(void)
{
    TEST("canvas_str_width matches font width");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    eui_canvas_set_font(c, &eui_font_builtin);

    uint16_t w = eui_canvas_str_width(c, "A");
    if (w != 8) FAIL("str_width should be 8 for 'A'");

    /* Also test empty string */
    if (eui_canvas_str_width(c, "") != 0) FAIL("str_width should be 0 for empty string");

    eui_canvas_destroy(c);
    PASS();
}

/* ---------------------------------------------------------------------------
 * Test 9: eui_canvas_font_height matches font
 * ---------------------------------------------------------------------------*/
static void test_font_height(void)
{
    TEST("canvas_font_height matches font height");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    eui_canvas_set_font(c, &eui_font_builtin);

    uint16_t h = eui_canvas_font_height(c);
    if (h != 8) FAIL("font_height should be 8 for built-in font");

    eui_canvas_destroy(c);
    PASS();
}

/* ---------------------------------------------------------------------------
 * Test 10: Clipped drawing doesn't render outside rect
 * ---------------------------------------------------------------------------*/
static void test_clipped_no_draw_outside(void)
{
    TEST("draw_str_clipped does not render outside clip rect");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    eui_canvas_set_font(c, &eui_font_builtin);
    eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);
    eui_canvas_set_color(c, EUI_COLOR_WHITE);

    eui_canvas_clear(c);
    memset(mock_buf, 0, sizeof(mock_buf));

    /* "ABCDEFGH" is 64px wide. Clip rect is only 30px wide.
     * Only characters that fit within clip (x<30) should render. */
    eui_rect_t clip = {10, 0, 30, 64};
    uint16_t clipped_w = eui_canvas_draw_str_clipped(c, &clip, 0, 10, "ABCDEFGH");
    eui_canvas_commit(c);

    if (clipped_w != 64) FAIL("clipped draw_str should return full width 64");

    int clip_px = count_pixels();
    if (clip_px == 0) FAIL("clipped text should produce some pixels");

#if EUI_COLOR_DEPTH == 1
    /* Verify NO pixels exist outside the clip rect (x >= 10+30 = 40) */
    for (int y = 0; y < (int)MOCK_H; y++) {
        for (int x = 40; x < (int)MOCK_W; x++) {
            int bi = y * (MOCK_W / 8) + x / 8;
            if (mock_buf[bi] & (1u << (x % 8)))
                FAIL("pixel should not exist outside clip rect");
        }
    }
#endif

    eui_canvas_destroy(c);
    PASS();
}

/* ---------------------------------------------------------------------------
 * Test 11: Ellipsis doesn't truncate when text fits
 * ---------------------------------------------------------------------------*/
static void test_ellipsis_no_truncation(void)
{
    TEST("draw_str_ellipsis does not truncate when text fits");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    eui_canvas_set_font(c, &eui_font_builtin);
    eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);
    eui_canvas_set_color(c, EUI_COLOR_WHITE);

    /* "AB" width=16, max_width=32 >> 16, so no truncation needed */
    eui_canvas_clear(c);
    memset(mock_buf, 0, sizeof(mock_buf));
    uint16_t w1 = eui_canvas_draw_str_ellipsis(c, 0, 0, "AB", 32);
    eui_canvas_commit(c);
    int px1 = count_pixels();

    /* Compare with plain draw_str */
    eui_canvas_clear(c);
    memset(mock_buf, 0, sizeof(mock_buf));
    uint16_t w2 = eui_canvas_draw_str(c, 0, 0, "AB");
    eui_canvas_commit(c);
    int px2 = count_pixels();

    if (w1 != 16) FAIL("ellipsis should return width 16 for 'AB'");
    if (w1 != w2) FAIL("ellipsis width should match plain draw when fitting");
    if (px1 != px2) FAIL("ellipsis pixel count should match plain draw when fitting");

    eui_canvas_destroy(c);
    PASS();
}

/* ---------------------------------------------------------------------------
 * Test 12: Vertical center alignment
 * ---------------------------------------------------------------------------*/
static void test_str_aligned_v_center(void)
{
    TEST("draw_str_aligned vertical center alignment");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    eui_canvas_set_font(c, &eui_font_builtin);
    eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);
    eui_canvas_set_color(c, EUI_COLOR_WHITE);

    eui_canvas_clear(c);
    memset(mock_buf, 0, sizeof(mock_buf));

    /* "A" width=8, height=8. Draw at (0, 36) with ALIGN_MIDDLE vert.
     * dy = 36 - 8/2 = 32. Text renders at y=32..39. */
    uint16_t w = eui_canvas_draw_str_aligned(c, 0, 36,
                                              EUI_ALIGN_LEFT, EUI_ALIGN_MIDDLE,
                                              "A");
    eui_canvas_commit(c);

    if (w != 8) FAIL("vertical center 'A' should return width 8");

    int px = count_pixels();
    if (px != 18) FAIL("pixel count for 'A' should be 18");

#if EUI_COLOR_DEPTH == 1
    /* Verify NO pixels in row 0 (text is at y=32..39) */
    for (int x = 0; x < 8; x++) {
        int bi = 0 * (MOCK_W / 8) + x / 8;
        if (mock_buf[bi] & (1u << (x % 8)))
            FAIL("vertical center text should not be at row 0");
    }
#endif

    eui_canvas_destroy(c);
    PASS();
}

int main(void)
{
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);
    printf("=== Canvas Font Tests ===\n");
    test_set_font();
    test_draw_str_renders();
    test_draw_str_null_canvas();
    test_draw_str_null_str();
    test_draw_str_no_font();
    test_draw_str_aligned_center();
    test_draw_str_u8g2_font();
    test_str_width();
    test_font_height();
    test_clipped_no_draw_outside();
    test_ellipsis_no_truncation();
    test_str_aligned_v_center();
    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
