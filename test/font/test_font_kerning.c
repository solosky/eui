#include "eui/eui_font.h"
#include "eui/eui_config.h"
#include "eui/eui_types.h"
#include "eui/eui_allocator.h"
#include "common/eui_test.h"
#include "data/test_font_kerning.h"
#include <stdio.h>

#if EUI_FONT_ENABLE_U8G2 && EUI_FONT_ENABLE_KERNING

#include "eui/eui_font_internal.h"
#include "eui/eui_font_u8g2_internal.h"

static const eui_font_t kern_font = {
    .format = EUI_FONT_FORMAT_U8G2,
    .line_height = 8,
    .baseline = 7,
    .flags = EUI_FONT_HAS_KERNING | EUI_FONT_HAS_UNICODE,
    .data = test_kern_font_data,
#if EUI_FONT_ENABLE_U8G2
    .lookup_glyph = eui_font_u8g2_lookup_glyph,
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

/* lookup_glyph for 'T' with no prev -> normal T, advance=8 */
static void test_normal_t_lookup(void)
{
    TEST("lookup_glyph('T', prev=0) returns normal T (adv=8)");
    int32_t idx = kern_font.lookup_glyph(&kern_font, 'T', 0);
    if (idx < 0) FAIL("glyph not found");
    u8g2_glyph_t g;
    if (!decode_glyph_at(&kern_font, (uint32_t)idx, &g)) FAIL("decode failed");
    if (g.x_advance != 8) FAIL("expected advance=8");
    if (g.x_offset != 0) FAIL("expected x_offset=0");
    PASS();
}

/* lookup_glyph for 'A' with no prev -> normal A, advance=8 */
static void test_normal_a_lookup(void)
{
    TEST("lookup_glyph('A', prev=0) returns normal A (adv=8)");
    int32_t idx = kern_font.lookup_glyph(&kern_font, 'A', 0);
    if (idx < 0) FAIL("glyph not found");
    u8g2_glyph_t g;
    if (!decode_glyph_at(&kern_font, (uint32_t)idx, &g)) FAIL("decode failed");
    if (g.x_advance != 8) FAIL("expected advance=8");
    if (g.x_offset != 0) FAIL("expected x_offset=0");
    PASS();
}

/* lookup_glyph for 'A' with prev='T' -> kerned A, advance=6, x_offset=-2 */
static void test_kerning_pair_lookup(void)
{
    TEST("lookup_glyph('A', prev='T') returns kerned A (adv=6, x_off=-2)");
    int32_t idx = kern_font.lookup_glyph(&kern_font, 'A', 'T');
    if (idx < 0) FAIL("kerning glyph not found");
    u8g2_glyph_t g;
    if (!decode_glyph_at(&kern_font, (uint32_t)idx, &g)) FAIL("decode failed");
    if (g.x_advance != 6) FAIL("expected advance=6");
    if (g.x_offset != -2) FAIL("expected x_offset=-2");
    PASS();
}

/* lookup_glyph for 'T' with prev='A' -> no kerning pair, returns normal T */
static void test_no_kerning_reverse(void)
{
    TEST("lookup_glyph('T', prev='A') returns normal T (no kerning pair)");
    int32_t idx = kern_font.lookup_glyph(&kern_font, 'T', 'A');
    if (idx < 0) FAIL("glyph not found");
    u8g2_glyph_t g;
    if (!decode_glyph_at(&kern_font, (uint32_t)idx, &g)) FAIL("decode failed");
    if (g.x_advance != 8) FAIL("expected advance=8 (no kerning for A+T)");
    if (g.x_offset != 0) FAIL("expected x_offset=0");
    PASS();
}

/* kerning-aware string width: "TA" should be 8 + 6 = 14 */
static void test_kerning_str_width_ta(void)
{
    TEST("kerning-aware string width 'TA' = 14 (8+6)");
    const char *str = "TA";
    uint16_t w = 0;
    uint16_t prev = 0;
    while (*str) {
        uint16_t c = (uint8_t)*str;
        int32_t idx = kern_font.lookup_glyph(&kern_font, c, prev);
        if (idx >= 0) {
            u8g2_glyph_t g;
            if (decode_glyph_at(&kern_font, (uint32_t)idx, &g)) {
                w += g.x_advance;
            }
        }
        prev = c;
        str++;
    }
    if (w != 14) FAIL("expected 14 (8+6 with kerning)");
    PASS();
}

/* no-kerning string width: "TA" with prev always 0 = 16 */
static void test_nokerning_str_width_ta(void)
{
    TEST("no-kerning string width 'TA' = 16 (8+8)");
    const char *str = "TA";
    uint16_t w = 0;
    while (*str) {
        uint16_t c = (uint8_t)*str;
        int32_t idx = kern_font.lookup_glyph(&kern_font, c, 0);
        if (idx >= 0) {
            u8g2_glyph_t g;
            if (decode_glyph_at(&kern_font, (uint32_t)idx, &g)) {
                w += g.x_advance;
            }
        }
        str++;
    }
    if (w != 16) FAIL("expected 16 (8+8 without kerning)");
    PASS();
}

/* kerning: "AT" should be 16 (8+8, no kerning pair for A+T) */
static void test_kerning_str_width_at(void)
{
    TEST("kerning-aware string width 'AT' = 16 (8+8, no A+T pair)");
    const char *str = "AT";
    uint16_t w = 0;
    uint16_t prev = 0;
    while (*str) {
        uint16_t c = (uint8_t)*str;
        int32_t idx = kern_font.lookup_glyph(&kern_font, c, prev);
        if (idx >= 0) {
            u8g2_glyph_t g;
            if (decode_glyph_at(&kern_font, (uint32_t)idx, &g)) {
                w += g.x_advance;
            }
        }
        prev = c;
        str++;
    }
    if (w != 16) FAIL("expected 16 (8+8, no kerning pair for A+T)");
    PASS();
}

static void test_kerning_out_of_range(void)
{
    TEST("kerning font out-of-range char returns -1");
    int32_t idx = kern_font.lookup_glyph(&kern_font, 'z', 0);
    if (idx != -1) FAIL("expected -1 for missing char");
    PASS();
}

static void test_kerning_null_font(void)
{
    TEST("kerning null font returns -1");
    int32_t idx = eui_font_u8g2_lookup_glyph(NULL, 'A', 0);
    if (idx != -1) FAIL("expected -1");
    PASS();
}

int main(void)
{
    eui_test_init();
    printf("=== Kerning Tests ===\n");
    test_kerning_enabled_flag();
    test_kerning_unicode_flag();
    test_normal_t_lookup();
    test_normal_a_lookup();
    test_kerning_pair_lookup();
    test_no_kerning_reverse();
    test_kerning_str_width_ta();
    test_nokerning_str_width_ta();
    test_kerning_str_width_at();
    test_kerning_out_of_range();
    test_kerning_null_font();
    return eui_test_summary();
}
#else
int main(void)
{
    printf("=== Kerning Tests ===\n");
    printf("  SKIP: EUI_FONT_ENABLE_U8G2 or EUI_FONT_ENABLE_KERNING disabled\n");
    return 0;
}
#endif
