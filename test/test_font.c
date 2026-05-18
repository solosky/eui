#include "eui/eui_font.h"
#include "eui/eui_font_builtin.h"
#include "eui/eui_allocator.h"
#include <stdio.h>
#include <string.h>

#define POOL_SIZE 32768
static uint8_t mem_pool[POOL_SIZE];

static int tests_run = 0, tests_passed = 0;
#define TEST(n) do { printf("  %s... ", n); tests_run++; } while(0)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(m) do { printf("FAIL: %s\n", m); return; } while(0)

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
    uint8_t w = eui_font_get_char_width(&eui_font_builtin, 'z');
    if (w != 0) FAIL("'z' should be out of range");
    PASS();
}

static void test_draw_char(void)
{
    TEST("draw_char writes glyph pixels");
    uint8_t buf[8] = {0};
    uint8_t adv = eui_font_draw_char(&eui_font_builtin, 'A', buf, 1, 1);
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

static void test_draw_char_b(void)
{
    TEST("draw_char 'B' produces correct glyph");
    uint8_t buf[8] = {0};
    uint8_t adv = eui_font_draw_char(&eui_font_builtin, 'B', buf, 1, 1);
    if (adv != 8) FAIL("expected advance 8");

    uint8_t expected[8] = {0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00};
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

static void test_vlw_format_returns_zero(void)
{
    TEST("VLW format returns 0");
    eui_font_t vlw_font = {
        .format = EUI_FONT_FORMAT_VLW,
        .line_height = 10,
        .baseline = 9,
        .flags = 0,
        .data = NULL,
    };
    uint8_t w = eui_font_get_char_width(&vlw_font, 'A');
    if (w != 0) FAIL("VLW should return 0");
    PASS();
}

int main(void)
{
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);
    printf("=== Font Tests ===\n");
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
    test_vlw_format_returns_zero();
    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
