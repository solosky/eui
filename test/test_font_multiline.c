#include "eui/eui_canvas.h"
#include "eui/eui_font.h"
#include "eui/eui_font_builtin.h"
#include "eui/eui_types.h"
#include "eui/eui_allocator.h"
#include "eui/eui_config.h"
#include <stdio.h>
#include <string.h>

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

static eui_display_hal_t mock_display = {
    .caps = { .width = MOCK_W, .height = MOCK_H, .color_depth = EUI_COLOR_DEPTH, .buffer_mode = EUI_BUFFER_FULL, .has_gram = false },
    .init = NULL,
    .write_buffer = mock_write_buffer,
};

static void test_draw_str_clipped(void)
{
    TEST("draw_str_clipped clips text to rect");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    eui_canvas_set_font(c, &eui_font_builtin);
    eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);
    eui_canvas_set_color(c, EUI_COLOR_WHITE);

    /* Draw full string without clip to get pixel count */
    eui_canvas_clear(c);
    uint16_t full_w = eui_canvas_draw_str(c, 5, 10, "ABCDEFGH");
    eui_canvas_commit(c);
    int full_px = count_pixels();
    if (full_w != 64) FAIL("full string width should be 64");
    if (full_px == 0) FAIL("full string should produce pixels");

    /* Draw with clip rect smaller than text */
    eui_canvas_clear(c);
    memset(mock_buf, 0, sizeof(mock_buf));
    eui_rect_t clip = {10, 0, 30, 64};
    uint16_t clipped_w = eui_canvas_draw_str_clipped(c, &clip, 5, 10, "ABCDEFGH");
    eui_canvas_commit(c);
    int clipped_px = count_pixels();

    if (clipped_w != 64) FAIL("clipped string should still report full width 64");
    if (clipped_px >= full_px) FAIL("clipped draw should produce fewer pixels");
    if (clipped_px == 0) FAIL("clipped draw should produce some pixels");

    /* Verify clip was restored by drawing after */
    eui_canvas_clear(c);
    memset(mock_buf, 0, sizeof(mock_buf));
    eui_canvas_draw_str(c, 5, 10, "ABCDEFGH");
    eui_canvas_commit(c);
    int after_px = count_pixels();
    if (after_px != full_px) FAIL("clip should be restored after draw_str_clipped");

    eui_canvas_destroy(c);
    PASS();
}

static void test_draw_str_ellipsis_fits(void)
{
    TEST("draw_str_ellipsis when text fits (no ellipsis)");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    eui_canvas_set_font(c, &eui_font_builtin);
    eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);
    eui_canvas_set_color(c, EUI_COLOR_WHITE);

    /* "AB" width = 16, max_width=32, should draw "AB" unchanged */
    eui_canvas_clear(c);
    memset(mock_buf, 0, sizeof(mock_buf));
    uint16_t w1 = eui_canvas_draw_str_ellipsis(c, 0, 0, "AB", 32);
    eui_canvas_commit(c);
    int px1 = count_pixels();

    eui_canvas_clear(c);
    memset(mock_buf, 0, sizeof(mock_buf));
    uint16_t w2 = eui_canvas_draw_str(c, 0, 0, "AB");
    eui_canvas_commit(c);
    int px2 = count_pixels();

    if (w1 != 16) FAIL("width should be 16 for 'AB'");
    if (w1 != w2) FAIL("ellipsis width should match plain draw when fitting");
    if (px1 != px2) FAIL("pixel count should match plain draw when fitting");

    eui_canvas_destroy(c);
    PASS();
}

static void test_draw_str_ellipsis_truncates(void)
{
    TEST("draw_str_ellipsis truncates with '...'");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    eui_canvas_set_font(c, &eui_font_builtin);
    eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);
    eui_canvas_set_color(c, EUI_COLOR_WHITE);

    /* "ABCDE" width=40, max_width=32, ellipsis "..." = 24, avail=8 */
    /* Only "A" fits (8px), then "...". Total = 32. */
    eui_canvas_clear(c);
    memset(mock_buf, 0, sizeof(mock_buf));
    uint16_t w = eui_canvas_draw_str_ellipsis(c, 0, 0, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 32);
    eui_canvas_commit(c);
    int px = count_pixels();

    if (w != 32) FAIL("ellipsis truncated width should be 32 (A + '...')");
    if (px == 0) FAIL("ellipsis should produce pixels");

    /* Verify ellipsis output differs from full output */
    eui_canvas_clear(c);
    memset(mock_buf, 0, sizeof(mock_buf));
    eui_canvas_draw_str(c, 0, 0, "A...");
    eui_canvas_commit(c);
    int px_expected = count_pixels();

    if (px != px_expected) FAIL("ellipsis output should match drawing 'A...' directly");

    eui_canvas_destroy(c);
    PASS();
}

#if EUI_FONT_ENABLE_MULTILINE
static void test_draw_str_in_rect_center(void)
{
    TEST("draw_str_in_rect centers text");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    eui_canvas_set_font(c, &eui_font_builtin);
    eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);
    eui_canvas_set_color(c, EUI_COLOR_WHITE);

    /* "A" width=8 in rect {10,10,40,24}, centered both axes */
    /* dx = 10 + (40-8)/2 = 26, dy = 10 + (24-8)/2 = 18 */
    eui_canvas_clear(c);
    memset(mock_buf, 0, sizeof(mock_buf));
    eui_rect_t r = {10, 10, 40, 24};
    uint16_t w = eui_canvas_draw_str_in_rect(c, &r, "A", EUI_ALIGN_CENTER, EUI_ALIGN_MIDDLE);
    eui_canvas_commit(c);

    if (w != 8) FAIL("width should be 8 for single char 'A'");

    int px = count_pixels();
    if (px == 0) FAIL("centered text should produce pixels");

    /* Verify pixels are within the rect area (not at x<10 or x>=50) */
#if EUI_COLOR_DEPTH == 1
    int px_left = 0, px_right = 0;
    for (int y = 0; y < (int)MOCK_H; y++) {
        for (int x = 0; x < 10; x++) {
            int byte_idx = y * (MOCK_W / 8) + x / 8;
            int bit_pos = x % 8;
            if (mock_buf[byte_idx] & (1u << bit_pos)) px_left++;
        }
        for (int x = 50; x < (int)MOCK_W; x++) {
            int byte_idx = y * (MOCK_W / 8) + x / 8;
            int bit_pos = x % 8;
            if (mock_buf[byte_idx] & (1u << bit_pos)) px_right++;
        }
    }
    if (px_left > 0) FAIL("pixels found left of rect");
    if (px_right > 0) FAIL("pixels found right of rect");
#endif

    eui_canvas_destroy(c);
    PASS();
}

static void test_draw_str_in_rect_right(void)
{
    TEST("draw_str_in_rect right-aligns text");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    eui_canvas_set_font(c, &eui_font_builtin);
    eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);
    eui_canvas_set_color(c, EUI_COLOR_WHITE);

    /* "AB" width=16 in rect {0,0,64,8}, right-aligned */
    /* dx = 0 + (64-16) = 48 */
    eui_canvas_clear(c);
    memset(mock_buf, 0, sizeof(mock_buf));
    eui_rect_t r = {0, 0, 64, 8};
    uint16_t w = eui_canvas_draw_str_in_rect(c, &r, "AB", EUI_ALIGN_RIGHT, EUI_ALIGN_TOP);
    eui_canvas_commit(c);

    if (w != 16) FAIL("width should be 16 for 'AB'");

    int px = count_pixels();
    if (px == 0) FAIL("right-aligned text should produce pixels");

#if EUI_COLOR_DEPTH == 1
    /* Verify text starts at x >= 48 (right edge - width) */
    int px_before_48 = 0;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 48; x++) {
            int byte_idx = y * (MOCK_W / 8) + x / 8;
            int bit_pos = x % 8;
            if (mock_buf[byte_idx] & (1u << bit_pos)) px_before_48++;
        }
    }
    if (px_before_48 > 0) FAIL("right-aligned text should not appear before x=48");
#endif

    eui_canvas_destroy(c);
    PASS();
}

#endif /* EUI_FONT_ENABLE_MULTILINE */

#if EUI_FONT_ENABLE_MULTILINE

static void test_multiline_simple(void)
{
    TEST("multiline draws newline-separated lines");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    eui_canvas_set_font(c, &eui_font_builtin);
    eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);
    eui_canvas_set_color(c, EUI_COLOR_WHITE);

    /* "AA\nBB\nCC" with max_width=128 should produce 3 lines */
    eui_canvas_clear(c);
    memset(mock_buf, 0, sizeof(mock_buf));
    eui_rect_t r = {0, 0, 128, 64};
    uint16_t h = eui_canvas_draw_str_multiline(c, &r, "AA\nBB\nCC", 8, EUI_ALIGN_LEFT);
    eui_canvas_commit(c);

    if (h != 24) { printf("expected 24 got %u\n", (unsigned)h); FAIL("multiline height wrong"); }

    int px = count_pixels();
    if (px == 0) FAIL("multiline should produce pixels");

    /* "AA" = 18*2=36 pixels, "BB" = 20*2=40, "CC" = ? Let's verify consistent drawing */
    /* Just verify pixels exist on different rows */
#if EUI_COLOR_DEPTH == 1
    /* Check that pixels exist in row 0 (first line) and row 8 (second line) */
    int row0_px = 0, row8_px = 0;
    for (int x = 0; x < (int)MOCK_W; x++) {
        int bi = 0 * (MOCK_W / 8) + x / 8;
        if (mock_buf[bi] & (1u << (x % 8))) row0_px++;
        bi = 8 * (MOCK_W / 8) + x / 8;
        if (mock_buf[bi] & (1u << (x % 8))) row8_px++;
    }
    if (row0_px == 0) FAIL("no pixels on first line row");
    if (row8_px == 0) FAIL("no pixels on second line row");
#endif

    eui_canvas_destroy(c);
    PASS();
}

static void test_multiline_word_wrap(void)
{
    TEST("multiline word-wrap at word boundaries");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    eui_canvas_set_font(c, &eui_font_builtin);
    eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);
    eui_canvas_set_color(c, EUI_COLOR_WHITE);

    /* With max_width=16, "AA BB CC" (each char 8px) should wrap at spaces */
    /* Line1: "AA " (3 chars=24px? no, only AA fits, space makes line_w exceed) */
    /* Actually: AA=16 fits, then space pushes to 24 > 16, wraps at space. */
    /* Line1: "AA" (2 chars), Line2: "BB" (2 chars) + " "? ... */
    eui_canvas_clear(c);
    memset(mock_buf, 0, sizeof(mock_buf));
    eui_rect_t r = {0, 0, 16, 64};
    uint16_t h = eui_canvas_draw_str_multiline(c, &r, "AA BB CC", 8, EUI_ALIGN_LEFT);
    eui_canvas_commit(c);

    /* Should produce multiple lines */
    if (h < 16) { printf("expected >=16 got %u\n", (unsigned)h); FAIL("word wrap too few lines"); }
    if (h > 48) { printf("expected <=48 got %u\n", (unsigned)h); FAIL("word wrap too many lines"); }

    int px = count_pixels();
    if (px == 0) FAIL("word wrap should produce pixels");

    eui_canvas_destroy(c);
    PASS();
}

static void test_multiline_long_word(void)
{
    TEST("multiline long word hard-breaks");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    eui_canvas_set_font(c, &eui_font_builtin);
    eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);
    eui_canvas_set_color(c, EUI_COLOR_WHITE);

    /* "ABCD" is 32px wide, max_width=16 forces char-level break */
    /* No spaces, so chars break at 2-char boundary: "AB" on line1, "CD" on line2 */
    eui_canvas_clear(c);
    memset(mock_buf, 0, sizeof(mock_buf));
    eui_rect_t r = {0, 0, 16, 64};
    uint16_t h = eui_canvas_draw_str_multiline(c, &r, "ABCD", 8, EUI_ALIGN_LEFT);
    eui_canvas_commit(c);

    if (h != 16) { printf("expected 16 got %u\n", (unsigned)h); FAIL("long word break height wrong"); }

    int px = count_pixels();
    if (px == 0) FAIL("long word break should produce pixels");

    eui_canvas_destroy(c);
    PASS();
}

static void test_multiline_empty(void)
{
    TEST("multiline empty string returns 0");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    eui_canvas_set_font(c, &eui_font_builtin);
    eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);

    eui_rect_t r = {0, 0, 128, 64};
    uint16_t h = eui_canvas_draw_str_multiline(c, &r, "", 8, EUI_ALIGN_LEFT);
    if (h != 0) { printf("expected 0 got %u\n", (unsigned)h); FAIL("empty string should return 0"); }

    eui_canvas_destroy(c);
    PASS();
}

static void test_str_multiline_height(void)
{
    TEST("str_multiline_height matches expected");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    eui_canvas_set_font(c, &eui_font_builtin);

    /* 3 newline-separated lines with line_height=10 */
    uint16_t h = eui_canvas_str_multiline_height(c, "AA\nBB\nCC", 128, 10);
    if (h != 30) { printf("expected 30 got %u\n", (unsigned)h); FAIL("str_multiline_height wrong"); }

    /* 4 word-wrap lines with line_height=8 */
    h = eui_canvas_str_multiline_height(c, "AA BB CC DD", 16, 0);
    if (h < 24) { printf("expected >=24 got %u\n", (unsigned)h); FAIL("word-wrap height too small"); }

    eui_canvas_destroy(c);
    PASS();
}

static void test_line_height_override(void)
{
    TEST("custom line_height produces correct spacing");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    eui_canvas_set_font(c, &eui_font_builtin);
    eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);
    eui_canvas_set_color(c, EUI_COLOR_WHITE);

    /* Two lines with line_height=16 */
    eui_canvas_clear(c);
    memset(mock_buf, 0, sizeof(mock_buf));
    eui_rect_t r = {0, 0, 128, 64};
    uint16_t h = eui_canvas_draw_str_multiline(c, &r, "AA\nBB", 16, EUI_ALIGN_LEFT);
    eui_canvas_commit(c);

    if (h != 32) { printf("expected 32 got %u\n", (unsigned)h); FAIL("custom line_height wrong"); }

    int px = count_pixels();
    if (px == 0) FAIL("custom line_height should produce pixels");

#if EUI_COLOR_DEPTH == 1
    /* Verify second line starts at row 16, not row 8 */
    /* Row 8-15 should be empty (gap between lines) */
    int gap_px = 0;
    for (int y = 9; y < 16; y++) {
        for (int x = 0; x < (int)MOCK_W; x++) {
            int bi = y * (MOCK_W / 8) + x / 8;
            if (mock_buf[bi] & (1u << (x % 8))) gap_px++;
        }
    }
    if (gap_px > 0) { printf("found %d gap pixels\n", gap_px); FAIL("custom line_height should leave gap"); }
#endif

    eui_canvas_destroy(c);
    PASS();
}

#endif /* EUI_FONT_ENABLE_MULTILINE */

static void test_null_safety(void)
{
    TEST("null safety for all text functions");

    /* draw_str */
    if (eui_canvas_draw_str(NULL, 0, 0, "A") != 0) FAIL("draw_str NULL canvas");
    {
        eui_canvas_t *c = eui_canvas_create(&mock_display);
        if (eui_canvas_draw_str(c, 0, 0, NULL) != 0) FAIL("draw_str NULL str");
        c->font = NULL;
        if (eui_canvas_draw_str(c, 0, 0, "A") != 0) FAIL("draw_str NULL font");
        eui_canvas_destroy(c);
    }

    /* draw_str_aligned */
    if (eui_canvas_draw_str_aligned(NULL, 0, 0, EUI_ALIGN_LEFT, EUI_ALIGN_TOP, "A") != 0)
        FAIL("draw_str_aligned NULL canvas");
    {
        eui_canvas_t *c = eui_canvas_create(&mock_display);
        if (eui_canvas_draw_str_aligned(c, 0, 0, EUI_ALIGN_LEFT, EUI_ALIGN_TOP, NULL) != 0)
            FAIL("draw_str_aligned NULL str");
        c->font = NULL;
        if (eui_canvas_draw_str_aligned(c, 0, 0, EUI_ALIGN_LEFT, EUI_ALIGN_TOP, "A") != 0)
            FAIL("draw_str_aligned NULL font");
        eui_canvas_destroy(c);
    }

    /* draw_str_clipped */
    {
        eui_rect_t clip = {0, 0, 10, 10};
        if (eui_canvas_draw_str_clipped(NULL, &clip, 0, 0, "A") != 0)
            FAIL("draw_str_clipped NULL canvas");
        {
            eui_canvas_t *c = eui_canvas_create(&mock_display);
            if (eui_canvas_draw_str_clipped(c, NULL, 0, 0, "A") != 0)
                FAIL("draw_str_clipped NULL clip_rect");
            c->font = NULL;
            if (eui_canvas_draw_str_clipped(c, &clip, 0, 0, "A") != 0)
                FAIL("draw_str_clipped NULL font");
            eui_canvas_destroy(c);
        }
        {
            eui_canvas_t *c = eui_canvas_create(&mock_display);
            eui_canvas_set_font(c, &eui_font_builtin);
            if (eui_canvas_draw_str_clipped(c, &clip, 0, 0, NULL) != 0)
                FAIL("draw_str_clipped NULL str");
            eui_canvas_destroy(c);
        }
    }

    /* draw_str_ellipsis */
    if (eui_canvas_draw_str_ellipsis(NULL, 0, 0, "A", 100) != 0)
        FAIL("draw_str_ellipsis NULL canvas");
    {
        eui_canvas_t *c = eui_canvas_create(&mock_display);
        if (eui_canvas_draw_str_ellipsis(c, 0, 0, NULL, 100) != 0)
            FAIL("draw_str_ellipsis NULL str");
        c->font = NULL;
        if (eui_canvas_draw_str_ellipsis(c, 0, 0, "A", 100) != 0)
            FAIL("draw_str_ellipsis NULL font");
        eui_canvas_destroy(c);
    }

    /* str_width */
    if (eui_canvas_str_width(NULL, "A") != 0) FAIL("str_width NULL canvas");
    {
        eui_canvas_t *c = eui_canvas_create(&mock_display);
        if (eui_canvas_str_width(c, NULL) != 0) FAIL("str_width NULL str");
        eui_canvas_destroy(c);
    }

    /* font_height */
    if (eui_canvas_font_height(NULL) != 0) FAIL("font_height NULL canvas");

#if EUI_FONT_ENABLE_MULTILINE
    /* draw_str_in_rect */
    {
        eui_rect_t r = {0, 0, 10, 10};
        if (eui_canvas_draw_str_in_rect(NULL, &r, "A", EUI_ALIGN_LEFT, EUI_ALIGN_TOP) != 0)
            FAIL("draw_str_in_rect NULL canvas");
        {
            eui_canvas_t *c = eui_canvas_create(&mock_display);
            if (eui_canvas_draw_str_in_rect(c, NULL, "A", EUI_ALIGN_LEFT, EUI_ALIGN_TOP) != 0)
                FAIL("draw_str_in_rect NULL rect");
            if (eui_canvas_draw_str_in_rect(c, &r, NULL, EUI_ALIGN_LEFT, EUI_ALIGN_TOP) != 0)
                FAIL("draw_str_in_rect NULL str");
            c->font = NULL;
            if (eui_canvas_draw_str_in_rect(c, &r, "A", EUI_ALIGN_LEFT, EUI_ALIGN_TOP) != 0)
                FAIL("draw_str_in_rect NULL font");
            eui_canvas_destroy(c);
        }
    }

    /* draw_str_multiline */
    {
        eui_rect_t r = {0, 0, 10, 10};
        if (eui_canvas_draw_str_multiline(NULL, &r, "A", 8, EUI_ALIGN_LEFT) != 0)
            FAIL("draw_str_multiline NULL canvas");
        {
            eui_canvas_t *c = eui_canvas_create(&mock_display);
            if (eui_canvas_draw_str_multiline(c, NULL, "A", 8, EUI_ALIGN_LEFT) != 0)
                FAIL("draw_str_multiline NULL rect");
            if (eui_canvas_draw_str_multiline(c, &r, NULL, 8, EUI_ALIGN_LEFT) != 0)
                FAIL("draw_str_multiline NULL str");
            c->font = NULL;
            if (eui_canvas_draw_str_multiline(c, &r, "A", 8, EUI_ALIGN_LEFT) != 0)
                FAIL("draw_str_multiline NULL font");
            eui_canvas_destroy(c);
        }
    }

    /* str_multiline_height */
    if (eui_canvas_str_multiline_height(NULL, "A\nB", 100, 8) != 0)
        FAIL("str_multiline_height NULL canvas");
    {
        eui_canvas_t *c = eui_canvas_create(&mock_display);
        if (eui_canvas_str_multiline_height(c, NULL, 100, 8) != 0)
            FAIL("str_multiline_height NULL str");
        eui_canvas_set_font(c, &eui_font_builtin);
        if (eui_canvas_str_multiline_height(c, "A\nB", 100, 8) == 0)
            FAIL("str_multiline_height should work with valid params");
        eui_canvas_destroy(c);
    }
#endif

    PASS();
}

int main(void)
{
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);
    printf("=== Multiline Text Tests ===\n");
    test_draw_str_clipped();
    test_draw_str_ellipsis_fits();
    test_draw_str_ellipsis_truncates();
#if EUI_FONT_ENABLE_MULTILINE
    test_draw_str_in_rect_center();
    test_draw_str_in_rect_right();
    test_multiline_simple();
    test_multiline_word_wrap();
    test_multiline_long_word();
    test_multiline_empty();
    test_str_multiline_height();
    test_line_height_override();
#endif
    test_null_safety();
    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
