#include "eui/eui_font.h"
#include "eui/eui_config.h"
#include "eui/eui_allocator.h"
#include "test_u8g2_font.h"
#include <stdio.h>
#include <string.h>

#if EUI_FONT_ENABLE_U8G2

#define POOL_SIZE 32768
static uint8_t mem_pool[POOL_SIZE];

static int tests_run = 0, tests_passed = 0;
#define TEST(n) do { printf("  %s... ", n); tests_run++; } while(0)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(m) do { printf("FAIL: %s\n", m); return; } while(0)

static const eui_font_t test_font = {
    .format = EUI_FONT_FORMAT_U8G2,
    .line_height = TEST_U8G2_FONT_HEIGHT,
    .baseline = TEST_U8G2_FONT_BASELINE,
    .flags = EUI_FONT_FIXED_WIDTH,
    .data = test_u8g2_font_data,
#if EUI_FONT_ENABLE_U8G2
    .lookup_glyph = NULL,
#endif
};

static void test_u8g2_format(void)
{
    TEST("u8g2 font format check");
    if (test_font.format != EUI_FONT_FORMAT_U8G2) FAIL("wrong format");
    PASS();
}

static void test_u8g2_char_width_a(void)
{
    TEST("u8g2 char width 'A' = 8");
    uint8_t w = eui_font_get_char_width(&test_font, 'A');
    if (w != 8) FAIL("expected 8");
    PASS();
}

static void test_u8g2_char_width_h(void)
{
    TEST("u8g2 char width 'H' = 8");
    uint8_t w = eui_font_get_char_width(&test_font, 'H');
    if (w != 8) FAIL("expected 8");
    PASS();
}

static void test_u8g2_out_of_range(void)
{
    TEST("u8g2 out-of-range char returns 0");
    uint8_t w = eui_font_get_char_width(&test_font, 'z');
    if (w != 0) FAIL("expected 0");
    PASS();
}

static void test_u8g2_str_width(void)
{
    TEST("u8g2 string width 'AB' = 16");
    uint16_t w = eui_font_get_str_width(&test_font, "AB");
    if (w != 16) FAIL("expected 16");
    PASS();
}

static void test_u8g2_height(void)
{
    TEST("u8g2 font height");
    uint8_t h = eui_font_get_height(&test_font);
    if (h != 8) FAIL("expected 8");
    PASS();
}

static void test_u8g2_baseline(void)
{
    TEST("u8g2 font baseline");
    uint8_t b = eui_font_get_baseline(&test_font);
    if (b != 7) FAIL("expected 7");
    PASS();
}

static void test_u8g2_empty_str(void)
{
    TEST("u8g2 empty string width = 0");
    uint16_t w = eui_font_get_str_width(&test_font, "");
    if (w != 0) FAIL("expected 0");
    PASS();
}

static void test_u8g2_draw_a(void)
{
    TEST("u8g2 draw_char 'A' glyph");
    uint8_t buf[16] = {0};
    uint8_t adv = eui_font_draw_char(&test_font, 'A', buf, 1, 1);
    if (adv != 8) FAIL("expected advance 8");

    uint8_t expected[8] = {0x18, 0x3C, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00};
    if (memcmp(buf + test_font.baseline, expected, 8) != 0) {
        printf("FAIL: got ");
        for (int i = 0; i < 8; i++) printf("%02X ", buf[test_font.baseline + i]);
        printf("\n");
        return;
    }
    PASS();
}

static void test_u8g2_draw_b(void)
{
    TEST("u8g2 draw_char 'B' glyph");
    uint8_t buf[16] = {0};
    uint8_t adv = eui_font_draw_char(&test_font, 'B', buf, 1, 1);
    if (adv != 8) FAIL("expected advance 8");

    uint8_t expected[8] = {0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00};
    if (memcmp(buf + test_font.baseline, expected, 8) != 0) FAIL("glyph mismatch");
    PASS();
}

static void test_u8g2_draw_c(void)
{
    TEST("u8g2 draw_char 'C' glyph");
    uint8_t buf[16] = {0};
    uint8_t adv = eui_font_draw_char(&test_font, 'C', buf, 1, 1);
    if (adv != 8) FAIL("expected advance 8");

    uint8_t expected[8] = {0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C, 0x00};
    if (memcmp(buf + test_font.baseline, expected, 8) != 0) FAIL("glyph mismatch");
    PASS();
}

static void test_u8g2_draw_d(void)
{
    TEST("u8g2 draw_char 'D' glyph");
    uint8_t buf[16] = {0};
    uint8_t adv = eui_font_draw_char(&test_font, 'D', buf, 1, 1);
    if (adv != 8) FAIL("expected advance 8");

    uint8_t expected[8] = {0x78, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0x78, 0x00};
    if (memcmp(buf + test_font.baseline, expected, 8) != 0) FAIL("glyph mismatch");
    PASS();
}

static void test_u8g2_draw_e(void)
{
    TEST("u8g2 draw_char 'E' glyph");
    uint8_t buf[16] = {0};
    uint8_t adv = eui_font_draw_char(&test_font, 'E', buf, 1, 1);
    if (adv != 8) FAIL("expected advance 8");

    uint8_t expected[8] = {0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x7E, 0x00};
    if (memcmp(buf + test_font.baseline, expected, 8) != 0) FAIL("glyph mismatch");
    PASS();
}

static void test_u8g2_draw_f(void)
{
    TEST("u8g2 draw_char 'F' glyph");
    uint8_t buf[16] = {0};
    uint8_t adv = eui_font_draw_char(&test_font, 'F', buf, 1, 1);
    if (adv != 8) FAIL("expected advance 8");

    uint8_t expected[8] = {0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x60, 0x00};
    if (memcmp(buf + test_font.baseline, expected, 8) != 0) FAIL("glyph mismatch");
    PASS();
}

static void test_u8g2_draw_g(void)
{
    TEST("u8g2 draw_char 'G' glyph");
    uint8_t buf[16] = {0};
    uint8_t adv = eui_font_draw_char(&test_font, 'G', buf, 1, 1);
    if (adv != 8) FAIL("expected advance 8");

    uint8_t expected[8] = {0x3C, 0x66, 0x60, 0x6E, 0x66, 0x66, 0x3C, 0x00};
    if (memcmp(buf + test_font.baseline, expected, 8) != 0) FAIL("glyph mismatch");
    PASS();
}

static void test_u8g2_draw_h(void)
{
    TEST("u8g2 draw_char 'H' glyph");
    uint8_t buf[16] = {0};
    uint8_t adv = eui_font_draw_char(&test_font, 'H', buf, 1, 1);
    if (adv != 8) FAIL("expected advance 8");

    uint8_t expected[8] = {0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00};
    if (memcmp(buf + test_font.baseline, expected, 8) != 0) FAIL("glyph mismatch");
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

int main(void)
{
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);
    printf("=== U8G2 Font Tests ===\n");
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
    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
#else
int main(void)
{
    printf("=== U8G2 Font Tests ===\n");
    printf("  SKIP: EUI_FONT_ENABLE_U8G2 disabled\n");
    return 0;
}
#endif
