#include "eui/eui_font.h"
#include "eui/eui_config.h"
#include "eui/eui_types.h"
#include "eui/eui_allocator.h"
#include "test_font_kerning.h"
#include <stdio.h>

#if EUI_FONT_ENABLE_U8G2 && EUI_FONT_ENABLE_KERNING

#define POOL_SIZE 32768
static uint8_t mem_pool[POOL_SIZE];

static int tests_run = 0, tests_passed = 0;
#define TEST(n) do { printf("  %s... ", n); tests_run++; } while(0)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(m) do { printf("FAIL: %s\n", m); return; } while(0)

static const eui_font_t kern_font = {
    .format = EUI_FONT_FORMAT_U8G2,
    .line_height = 8,
    .baseline = 7,
    .flags = EUI_FONT_HAS_KERNING | EUI_FONT_HAS_UNICODE,
    .data = test_kern_font_data,
#if EUI_FONT_ENABLE_U8G2
    .lookup_glyph = NULL,
#endif
};

static void test_kerning_enabled_flag(void)
{
    TEST("kerning font has KERNING flag");
    if (!(kern_font.flags & EUI_FONT_HAS_KERNING)) FAIL("missing kerning flag");
    PASS();
}

static void test_kerning_unicode_flag(void)
{
    TEST("kerning font has UNICODE flag");
    if (!(kern_font.flags & EUI_FONT_HAS_UNICODE)) FAIL("missing unicode flag");
    PASS();
}

static void test_kerning_normal_lookup(void)
{
    TEST("kerning font returns glyph for 'T'");
    uint8_t w = eui_font_get_char_width(&kern_font, 'T');
    if (w != 8) FAIL("expected 8");
    PASS();
}

static void test_kerning_a_lookup(void)
{
    TEST("kerning font returns glyph for 'A'");
    uint8_t w = eui_font_get_char_width(&kern_font, 'A');
    if (w != 8) FAIL("expected 8");
    PASS();
}

static void test_kerning_out_of_range(void)
{
    TEST("kerning font out-of-range char returns 0");
    uint8_t w = eui_font_get_char_width(&kern_font, 'z');
    if (w != 0) FAIL("expected 0");
    PASS();
}

static void test_kerning_str_width(void)
{
    TEST("kerning font string width 'TA'");
    uint16_t w = eui_font_get_str_width(&kern_font, "TA");
    if (w != 16) FAIL("expected 16");
    PASS();
}

static void test_kerning_null_font(void)
{
    TEST("kerning null font returns 0");
    uint8_t w = eui_font_get_char_width(NULL, 'A');
    if (w != 0) FAIL("expected 0");
    PASS();
}

int main(void)
{
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);
    printf("=== Kerning Tests ===\n");
    test_kerning_enabled_flag();
    test_kerning_unicode_flag();
    test_kerning_normal_lookup();
    test_kerning_a_lookup();
    test_kerning_out_of_range();
    test_kerning_str_width();
    test_kerning_null_font();
    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
#else
int main(void)
{
    printf("=== Kerning Tests ===\n");
    printf("  SKIP: EUI_FONT_ENABLE_U8G2 or EUI_FONT_ENABLE_KERNING disabled\n");
    return 0;
}
#endif
