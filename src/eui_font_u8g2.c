#include "eui/eui_font.h"
#include <stddef.h>

/* Header offsets */
#define HDR_GLYPH_CNT       0
#define HDR_BBX_MODE        1
#define HDR_BITS_PER_0      2
#define HDR_BITS_PER_1      3
#define HDR_BITS_PER_CHAR_W 4
#define HDR_BITS_PER_CHAR_H 5
#define HDR_BITS_PER_CHAR_X 6
#define HDR_BITS_PER_CHAR_Y 7
#define HDR_BITS_PER_DELTA_X 8
#define HDR_MAX_CHAR_W      9
#define HDR_MAX_CHAR_H      11
#define HDR_START_POS_UNICODE 19
#define U8G2_HEADER_SIZE    21

typedef struct {
    const uint8_t *data;
    uint16_t byte_pos;
    uint8_t  bit_pos;
} bit_reader_t;

typedef struct {
    int8_t  x_offset;
    int8_t  y_offset;
    uint8_t width;
    uint8_t height;
    uint8_t x_advance;
    uint16_t bitmap_byte;
    uint8_t  bitmap_bit;
} u8g2_glyph_t;

static void br_init(bit_reader_t *br, const uint8_t *data)
{
    br->data = data;
    br->byte_pos = U8G2_HEADER_SIZE;
    br->bit_pos = 0;
}

static uint8_t br_read(bit_reader_t *br, uint8_t num_bits)
{
    uint8_t val = 0;
    for (uint8_t i = 0; i < num_bits; i++) {
        uint8_t byte = br->data[br->byte_pos];
        val = (val << 1) | ((byte >> (7 - br->bit_pos)) & 1);
        br->bit_pos++;
        if (br->bit_pos >= 8) { br->bit_pos = 0; br->byte_pos++; }
    }
    return val;
}

static void br_skip(bit_reader_t *br, uint16_t num_bits)
{
    uint16_t total = (uint16_t)br->bit_pos + num_bits;
    br->byte_pos += total / 8;
    br->bit_pos = (uint8_t)(total % 8);
}

static int16_t default_lookup(const eui_font_t *font, uint16_t encoding, uint16_t prev)
{
    (void)font; (void)prev;
    if (encoding >= 'A' && encoding <= 'H') return (int16_t)(encoding - 'A');
    return -1;
}

int16_t eui_font_u8g2_lookup_glyph(const eui_font_t *font, uint16_t encoding, uint16_t prev)
{
    if (!font || !font->data) return -1;
    return default_lookup(font, encoding, prev);
}

/* Decode glyph at index. Returns 1 on success, placing data in *glyph. */
static uint8_t decode_glyph_at(const eui_font_t *font, uint8_t idx, u8g2_glyph_t *glyph)
{
    const uint8_t *p = font->data;
    uint8_t  glyph_cnt  = p[HDR_GLYPH_CNT];
    uint8_t  bp0 = p[HDR_BITS_PER_0], bp1 = p[HDR_BITS_PER_1];
    uint8_t  bpcw = p[HDR_BITS_PER_CHAR_W], bpch = p[HDR_BITS_PER_CHAR_H];
    uint8_t  bpcx = p[HDR_BITS_PER_CHAR_X], bpcy = p[HDR_BITS_PER_CHAR_Y];
    uint8_t  bpdx = p[HDR_BITS_PER_DELTA_X];
    uint8_t  mcw = p[HDR_MAX_CHAR_W] | ((uint16_t)p[HDR_MAX_CHAR_W+1] << 8);
    uint8_t  mch = p[HDR_MAX_CHAR_H] | ((uint16_t)p[HDR_MAX_CHAR_H+1] << 8);
    uint8_t  all0 = (1u << bp0) - 1, all1 = (1u << bp1) - 1;

    if (idx >= glyph_cnt) return 0;

    bit_reader_t br;
    br_init(&br, font->data);

    for (uint8_t i = 0; i <= idx; i++) {
        /* Skip intermediate runs until glyph boundary */
        uint8_t marker;
        while (1) {
            marker = br_read(&br, bp0);
            if (marker == all0) break;  /* glyph boundary */
            /* normal run: skip ones count and bitmap bits */
            uint8_t ones = br_read(&br, bp1);
            if (ones == all1) return 0;  /* end of data */
            br_skip(&br, (uint16_t)marker + ones);
        }
        /* Read glyph metrics */
        uint8_t cw = br_read(&br, bpcw);
        uint8_t ch = br_read(&br, bpch);
        uint8_t cx = br_read(&br, bpcx);
        uint8_t cy = br_read(&br, bpcy);
        uint8_t dx = br_read(&br, bpdx);

        glyph->width     = cw ? cw : mcw;
        glyph->height    = ch ? ch : mch;
        glyph->x_offset  = (int8_t)cx;
        glyph->y_offset  = (int8_t)cy;
        glyph->x_advance = dx ? dx : mcw;

        /* Record bitmap position */
        glyph->bitmap_byte = br.byte_pos;
        glyph->bitmap_bit  = br.bit_pos;

        /* Skip bitmap for all glyphs */
        uint16_t bm_bits = (uint16_t)glyph->width * glyph->height;
        br_skip(&br, bm_bits);
    }
    return 1;
}

/* Read one bitmap pixel from a specific bit position */
static uint8_t get_bitmap_pixel(const uint8_t *data, uint16_t byte_off,
                                 uint8_t bit_off, uint16_t pixel_idx)
{
    uint16_t total_bit = (uint16_t)byte_off * 8 + bit_off + pixel_idx;
    uint16_t byte_idx = total_bit / 8;
    uint8_t  bit_idx  = total_bit % 8;
    return (data[byte_idx] >> (7 - bit_idx)) & 1;
}

uint8_t eui_font_u8g2_get_char_width(const eui_font_t *font, char c)
{
    int16_t idx = eui_font_u8g2_lookup_glyph(font, (uint8_t)c, 0);
    if (idx < 0) return 0;
    u8g2_glyph_t g;
    if (!decode_glyph_at(font, (uint8_t)idx, &g)) return 0;
    return g.x_advance;
}

uint16_t eui_font_u8g2_get_str_width(const eui_font_t *font, const char *str)
{
    uint16_t w = 0;
    if (!font || !str) return 0;
    while (*str) { w += eui_font_u8g2_get_char_width(font, *str); str++; }
    return w;
}

uint8_t eui_font_u8g2_draw_char(const eui_font_t *font, char c,
                                 uint8_t *buf, uint16_t buf_stride,
                                 uint8_t color_depth)
{
    int16_t idx = eui_font_u8g2_lookup_glyph(font, (uint8_t)c, 0);
    if (idx < 0 || !buf) return 0;

    u8g2_glyph_t g;
    if (!decode_glyph_at(font, (uint8_t)idx, &g)) return 0;

    for (uint8_t row = 0; row < g.height; row++) {
        for (uint8_t col = 0; col < g.width; col++) {
            uint16_t px = (uint16_t)row * g.width + col;
            if (get_bitmap_pixel(font->data, g.bitmap_byte, g.bitmap_bit, px)) {
                if (color_depth == 1) {
                    buf[row * buf_stride + col / 8] |= (1u << (7 - (col % 8)));
                }
            }
        }
    }
    return g.x_advance;
}
