#include "eui/eui_font.h"
#include "eui/eui_font_builtin.h"
#include "eui/eui_allocator.h"
#include "data/test_vlw_font.h"
#if EUI_FONT_ENABLE_U8G2
#include "data/test_u8g2_font.h"
#endif
#include <stdio.h>
#include <string.h>
#include "common/eui_test.h"

static void test_format(void)
{
    TEST("built-in font is BDF format");
    if (eui_font_builtin.format != EUI_FONT_FORMAT_BDF) FAIL("wrong format");
    PASS();
}

static void test_char_width(void)
{
    TEST("char width returns 8 for built-in font");
    uint8_t w = eui_font_get_char_width(&eui_font_builtin, 'A');
    if (w != 8) FAIL("expected width 8 for 'A'");
    PASS();
}

static void test_str_width(void)
{
    TEST("string width sums char widths");
    uint16_t w = eui_font_get_str_width(&eui_font_builtin, "AB");
    if (w != 16) FAIL("expected width 16 for 'AB'");
    PASS();
}

static void test_height(void)
{
    TEST("font height");
    uint8_t h = eui_font_get_height(&eui_font_builtin);
    if (h != 8) FAIL("expected height 8");
    PASS();
}

static void test_baseline(void)
{
    TEST("font baseline");
    uint8_t b = eui_font_get_baseline(&eui_font_builtin);
    if (b != 7) FAIL("expected baseline 7");
    PASS();
}

static void test_null_font(void)
{
    TEST("null font returns 0");
    uint8_t w = eui_font_get_char_width(NULL, 'A');
    if (w != 0) FAIL("null font should return 0");
    PASS();
}

static void test_out_of_range_char(void)
{
    TEST("out-of-range char returns 0");
    uint8_t w = eui_font_get_char_width(&eui_font_builtin, '\x01');
    if (w != 0) FAIL("char 0x01 should be out of range");
    PASS();
}

static void test_draw_char(void)
{
    TEST("draw_char writes glyph pixels");
    uint8_t buf[8] = {0};
    uint8_t adv = eui_font_draw_char(&eui_font_builtin, 'A', buf, 1, 1);
    if (adv != 8) FAIL("expected advance 8");

    uint8_t expected[8] = {0x38, 0x44, 0x44, 0x7C, 0x44, 0x44, 0x44, 0x00};
    if (memcmp(buf, expected, 8) != 0) {
        printf("FAIL: glyph mismatch, got: ");
        for (int i = 0; i < 8; i++) printf("%02X ", buf[i]);
        printf("\n");
        return;
    }
    PASS();
}

static void test_draw_char_b(void)
{
    TEST("draw_char 'B' produces correct glyph");
    uint8_t buf[8] = {0};
    uint8_t adv = eui_font_draw_char(&eui_font_builtin, 'B', buf, 1, 1);
    if (adv != 8) FAIL("expected advance 8");

    uint8_t expected[8] = {0x78, 0x44, 0x44, 0x78, 0x44, 0x44, 0x78, 0x00};
    if (memcmp(buf, expected, 8) != 0) FAIL("glyph mismatch for 'B'");
    PASS();
}

static void test_empty_str(void)
{
    TEST("empty string width is 0");
    uint16_t w = eui_font_get_str_width(&eui_font_builtin, "");
    if (w != 0) FAIL("expected 0 for empty string");
    PASS();
}

static void test_flags(void)
{
    TEST("built-in font is fixed width");
    if (!(eui_font_builtin.flags & EUI_FONT_FIXED_WIDTH)) FAIL("expected fixed width flag");
    PASS();
}

static const eui_font_t test_vlw_font = {
    .format = EUI_FONT_FORMAT_VLW,
    .line_height = TEST_VLW_ASCENT + TEST_VLW_DESCENT,
    .baseline = TEST_VLW_ASCENT,
    .flags = EUI_FONT_FIXED_WIDTH,
    .data = test_vlw_font_data,
};

static void test_vlw_char_width(void)
{
    TEST("VLW char width returns 8 for 'A'");
    uint8_t w = eui_font_get_char_width(&test_vlw_font, 'A');
    if (w != 8) FAIL("expected width 8 for 'A'");
    PASS();
}

static void test_vlw_str_width(void)
{
    TEST("VLW string width sums char widths");
    uint16_t w = eui_font_get_str_width(&test_vlw_font, "AB");
    if (w != 16) FAIL("expected width 16 for 'AB'");
    PASS();
}

static void test_vlw_height(void)
{
    TEST("VLW font height");
    uint8_t h = eui_font_get_height(&test_vlw_font);
    if (h != 12) FAIL("expected height 12");
    PASS();
}

static void test_vlw_baseline(void)
{
    TEST("VLW font baseline");
    uint8_t b = eui_font_get_baseline(&test_vlw_font);
    if (b != 10) FAIL("expected baseline 10");
    PASS();
}

static void test_vlw_out_of_range(void)
{
    TEST("VLW out-of-range char returns 0");
    uint8_t w = eui_font_get_char_width(&test_vlw_font, 'z');
    if (w != 0) FAIL("'z' should be out of range");
    PASS();
}

static void test_vlw_empty_str(void)
{
    TEST("VLW empty string width is 0");
    uint16_t w = eui_font_get_str_width(&test_vlw_font, "");
    if (w != 0) FAIL("expected 0 for empty string");
    PASS();
}

static void test_vlw_draw_char_a(void)
{
    TEST("VLW draw_char 'A' produces correct glyph");
    uint8_t buf[8] = {0};
    uint8_t adv = eui_font_draw_char(&test_vlw_font, 'A', buf, 1, 1);
    if (adv != 8) FAIL("expected advance 8");

    uint8_t expected[8] = {0x18, 0x3C, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00};
    if (memcmp(buf, expected, 8) != 0) {
        printf("FAIL: glyph mismatch, got: ");
        for (int i = 0; i < 8; i++) printf("%02X ", buf[i]);
        printf("\n");
        return;
    }
    PASS();
}

static void test_vlw_draw_char_b(void)
{
    TEST("VLW draw_char 'B' produces correct glyph");
    uint8_t buf[8] = {0};
    uint8_t adv = eui_font_draw_char(&test_vlw_font, 'B', buf, 1, 1);
    if (adv != 8) FAIL("expected advance 8");

    uint8_t expected[8] = {0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00};
    if (memcmp(buf, expected, 8) != 0) FAIL("glyph mismatch for 'B'");
    PASS();
}

static void test_vlw_null_data(void)
{
    TEST("VLW null data returns 0");
    eui_font_t f = {
        .format = EUI_FONT_FORMAT_VLW,
        .line_height = 10,
        .baseline = 8,
        .flags = 0,
        .data = NULL,
    };
    uint8_t w = eui_font_get_char_width(&f, 'A');
    if (w != 0) FAIL("null data should return 0");
    PASS();
}

static void test_bdf_null_buffer(void)
{
    TEST("BDF draw_char with NULL buffer returns 0");
    uint8_t adv = eui_font_draw_char(&eui_font_builtin, 'A', NULL, 1, 1);
    if (adv != 0) FAIL("expected 0 with NULL buffer");
    PASS();
}

static void test_vlw_null_buffer(void)
{
    TEST("VLW draw_char with NULL buffer returns 0");
    uint8_t adv = eui_font_draw_char(&test_vlw_font, 'A', NULL, 1, 1);
    if (adv != 0) FAIL("expected 0 with NULL buffer");
    PASS();
}

static void test_bdf_null_str(void)
{
    TEST("BDF str_width with NULL string returns 0");
    uint16_t w = eui_font_get_str_width(&eui_font_builtin, NULL);
    if (w != 0) FAIL("expected 0 with NULL string");
    PASS();
}

static void test_vlw_null_str(void)
{
    TEST("VLW str_width with NULL string returns 0");
    uint16_t w = eui_font_get_str_width(&test_vlw_font, NULL);
    if (w != 0) FAIL("expected 0 with NULL string");
    PASS();
}

static void test_bdf_all_chars(void)
{
    TEST("BDF all chars in range have valid widths");
    for (int c = 32; c <= 126; c++) {
        uint8_t w = eui_font_get_char_width(&eui_font_builtin, (char)c);
        if (w == 0) {
            printf("FAIL: char %d (0x%02X) has width 0\n", c, c);
            return;
        }
    }
    PASS();
}

static void test_vlw_all_chars(void)
{
    TEST("VLW all test chars A-H return width 8");
    const char *chars = "ABCDEFGH";
    for (; *chars; chars++) {
        uint8_t w = eui_font_get_char_width(&test_vlw_font, *chars);
        if (w != 8) {
            printf("FAIL: char '%c' has width %d, expected 8\n", *chars, w);
            return;
        }
    }
    PASS();
}

static void test_bdf_draw_char_edge(void)
{
    TEST("BDF draw_char with small buffer doesn't overflow");
    uint8_t buf[16] = {0};
    uint8_t adv = eui_font_draw_char(&eui_font_builtin, 'A', buf, 1, 1);
    if (adv != 8) FAIL("expected advance 8");
    for (int i = 8; i < 16; i++) {
        if (buf[i] != 0) {
            printf("FAIL: buf[%d] = 0x%02X, expected 0\n", i, buf[i]);
            return;
        }
    }
    PASS();
}

static void test_multiple_draw_calls(void)
{
    TEST("multiple draw_char calls accumulate");
    uint8_t buf[8] = {0};
    eui_font_draw_char(&eui_font_builtin, 'A', buf, 1, 1);
    eui_font_draw_char(&eui_font_builtin, 'B', buf, 1, 1);
    int nonzero = 0;
    for (int i = 0; i < 8; i++) {
        if (buf[i] != 0) nonzero = 1;
    }
    if (!nonzero) FAIL("expected non-zero buffer after multiple draws");
    PASS();
}

#if EUI_FONT_ENABLE_U8G2

static const eui_font_t test_u8g2_font = {
    .format = EUI_FONT_FORMAT_U8G2,
    .line_height = TEST_U8G2_FONT_HEIGHT,
    .baseline = TEST_U8G2_FONT_BASELINE,
    .flags = EUI_FONT_FIXED_WIDTH,
    .data = test_u8g2_font_data,
    .lookup_glyph = NULL,
};

static void test_u8g2_format(void)
{
    TEST("u8g2 font format check");
    if (test_u8g2_font.format != EUI_FONT_FORMAT_U8G2) FAIL("wrong format");
    PASS();
}

static void test_u8g2_char_width_a(void)
{
    TEST("u8g2 char width 'A' = 8");
    uint8_t w = eui_font_get_char_width(&test_u8g2_font, 'A');
    if (w != 8) FAIL("expected 8");
    PASS();
}

static void test_u8g2_char_width_h(void)
{
    TEST("u8g2 char width 'H' = 8");
    uint8_t w = eui_font_get_char_width(&test_u8g2_font, 'H');
    if (w != 8) FAIL("expected 8");
    PASS();
}

static void test_u8g2_out_of_range(void)
{
    TEST("u8g2 out-of-range char returns 0");
    uint8_t w = eui_font_get_char_width(&test_u8g2_font, 'z');
    if (w != 0) FAIL("expected 0");
    PASS();
}

static void test_u8g2_str_width(void)
{
    TEST("u8g2 string width 'AB' = 16");
    uint16_t w = eui_font_get_str_width(&test_u8g2_font, "AB");
    if (w != 16) FAIL("expected 16");
    PASS();
}

static void test_u8g2_height(void)
{
    TEST("u8g2 font height");
    uint8_t h = eui_font_get_height(&test_u8g2_font);
    if (h != 8) FAIL("expected 8");
    PASS();
}

static void test_u8g2_baseline(void)
{
    TEST("u8g2 font baseline");
    uint8_t b = eui_font_get_baseline(&test_u8g2_font);
    if (b != 7) FAIL("expected 7");
    PASS();
}

static void test_u8g2_empty_str(void)
{
    TEST("u8g2 empty string width = 0");
    uint16_t w = eui_font_get_str_width(&test_u8g2_font, "");
    if (w != 0) FAIL("expected 0");
    PASS();
}

static void test_u8g2_draw_a(void)
{
    TEST("u8g2 draw_char 'A' glyph");
    uint8_t buf[16] = {0};
    uint8_t adv = eui_font_draw_char(&test_u8g2_font, 'A', buf, 1, 1);
    if (adv != 8) FAIL("expected advance 8");

    uint8_t expected[8] = {0x18, 0x3C, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00};
    if (memcmp(buf, expected, 8) != 0) {
        printf("FAIL: got ");
        for (int i = 0; i < 8; i++) printf("%02X ", buf[i]);
        printf("\n");
        return;
    }
    PASS();
}

static void test_u8g2_draw_b(void)
{
    TEST("u8g2 draw_char 'B' glyph");
    uint8_t buf[16] = {0};
    uint8_t adv = eui_font_draw_char(&test_u8g2_font, 'B', buf, 1, 1);
    if (adv != 8) FAIL("expected advance 8");

    uint8_t expected[8] = {0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00};
    if (memcmp(buf, expected, 8) != 0) FAIL("glyph mismatch");
    PASS();
}

static void test_u8g2_draw_c(void)
{
    TEST("u8g2 draw_char 'C' glyph");
    uint8_t buf[16] = {0};
    uint8_t adv = eui_font_draw_char(&test_u8g2_font, 'C', buf, 1, 1);
    if (adv != 8) FAIL("expected advance 8");

    uint8_t expected[8] = {0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C, 0x00};
    if (memcmp(buf, expected, 8) != 0) FAIL("glyph mismatch");
    PASS();
}

static void test_u8g2_draw_d(void)
{
    TEST("u8g2 draw_char 'D' glyph");
    uint8_t buf[16] = {0};
    uint8_t adv = eui_font_draw_char(&test_u8g2_font, 'D', buf, 1, 1);
    if (adv != 8) FAIL("expected advance 8");

    uint8_t expected[8] = {0x78, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0x78, 0x00};
    if (memcmp(buf, expected, 8) != 0) FAIL("glyph mismatch");
    PASS();
}

static void test_u8g2_draw_e(void)
{
    TEST("u8g2 draw_char 'E' glyph");
    uint8_t buf[16] = {0};
    uint8_t adv = eui_font_draw_char(&test_u8g2_font, 'E', buf, 1, 1);
    if (adv != 8) FAIL("expected advance 8");

    uint8_t expected[8] = {0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x7E, 0x00};
    if (memcmp(buf, expected, 8) != 0) FAIL("glyph mismatch");
    PASS();
}

static void test_u8g2_draw_f(void)
{
    TEST("u8g2 draw_char 'F' glyph");
    uint8_t buf[16] = {0};
    uint8_t adv = eui_font_draw_char(&test_u8g2_font, 'F', buf, 1, 1);
    if (adv != 8) FAIL("expected advance 8");

    uint8_t expected[8] = {0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x60, 0x00};
    if (memcmp(buf, expected, 8) != 0) FAIL("glyph mismatch");
    PASS();
}

static void test_u8g2_draw_g(void)
{
    TEST("u8g2 draw_char 'G' glyph");
    uint8_t buf[16] = {0};
    uint8_t adv = eui_font_draw_char(&test_u8g2_font, 'G', buf, 1, 1);
    if (adv != 8) FAIL("expected advance 8");

    uint8_t expected[8] = {0x3C, 0x66, 0x60, 0x6E, 0x66, 0x66, 0x3C, 0x00};
    if (memcmp(buf, expected, 8) != 0) FAIL("glyph mismatch");
    PASS();
}

static void test_u8g2_draw_h(void)
{
    TEST("u8g2 draw_char 'H' glyph");
    uint8_t buf[16] = {0};
    uint8_t adv = eui_font_draw_char(&test_u8g2_font, 'H', buf, 1, 1);
    if (adv != 8) FAIL("expected advance 8");

    uint8_t expected[8] = {0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00};
    if (memcmp(buf, expected, 8) != 0) FAIL("glyph mismatch");
    PASS();
}

static void test_u8g2_null_font(void)
{
    TEST("u8g2 null font returns 0");
    uint8_t w = eui_font_get_char_width(NULL, 'A');
    if (w != 0) FAIL("expected 0");
    PASS();
}

static void test_u8g2_null_data(void)
{
    TEST("u8g2 null data returns 0");
    eui_font_t f = {
        .format = EUI_FONT_FORMAT_U8G2,
        .line_height = 8,
        .baseline = 7,
        .flags = 0,
        .data = NULL,
    };
    uint8_t w = eui_font_get_char_width(&f, 'A');
    if (w != 0) FAIL("expected 0");
    PASS();
}

#endif

int main(void)
{
    eui_test_init();
    printf("=== Font Format Tests ===\n");
    test_format();
    test_char_width();
    test_str_width();
    test_height();
    test_baseline();
    test_null_font();
    test_out_of_range_char();
    test_draw_char();
    test_draw_char_b();
    test_empty_str();
    test_flags();
    test_vlw_char_width();
    test_vlw_str_width();
    test_vlw_height();
    test_vlw_baseline();
    test_vlw_out_of_range();
    test_vlw_empty_str();
    test_vlw_draw_char_a();
    test_vlw_draw_char_b();
    test_vlw_null_data();
    test_bdf_null_buffer();
    test_vlw_null_buffer();
    test_bdf_null_str();
    test_vlw_null_str();
    test_bdf_all_chars();
    test_vlw_all_chars();
    test_bdf_draw_char_edge();
    test_multiple_draw_calls();
#if EUI_FONT_ENABLE_U8G2
    test_u8g2_format();
    test_u8g2_char_width_a();
    test_u8g2_char_width_h();
    test_u8g2_out_of_range();
    test_u8g2_str_width();
    test_u8g2_height();
    test_u8g2_baseline();
    test_u8g2_empty_str();
    test_u8g2_draw_a();
    test_u8g2_draw_b();
    test_u8g2_draw_c();
    test_u8g2_draw_d();
    test_u8g2_draw_e();
    test_u8g2_draw_f();
    test_u8g2_draw_g();
    test_u8g2_draw_h();
    test_u8g2_null_font();
    test_u8g2_null_data();
#endif
    return eui_test_summary();
}
