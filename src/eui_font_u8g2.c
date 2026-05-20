#include "eui/eui_font.h"
#include "eui_font_u8g2_internal.h"
#include <stddef.h>

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
#define HDR_MAX_CHAR_H      10
#define HDR_START_POS_UPPER_A 17
#define HDR_START_POS_LOWER_A 19
#define U8G2_HEADER_SIZE    23

typedef struct {
    const uint8_t *data;
    uint16_t byte_pos;
    uint8_t  bit_pos;
} bit_reader_t;

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

static uint16_t get_be16(const uint8_t *p)
{
    return ((uint16_t)p[0] << 8) | p[1];
}

/* Walk the index table to find glyph data for the given encoding.
 * Returns pointer to packed glyph data, or NULL if not found. */
static const uint8_t* find_glyph_data(const eui_font_t *font, uint16_t encoding)
{
    if (!font || !font->data) return NULL;
    const uint8_t *p = font->data;
    const uint8_t *entry = p + U8G2_HEADER_SIZE;

    if (encoding >= 'a' && encoding <= 'z') {
        entry += get_be16(p + HDR_START_POS_LOWER_A);
    } else if (encoding >= 'A' && encoding <= 'Z') {
        entry += get_be16(p + HDR_START_POS_UPPER_A);
    }

    while (entry[1] != 0) {
        if (entry[0] == (uint8_t)encoding) return entry + 2;
        entry += entry[1];
    }
    return NULL;
}

int16_t eui_font_u8g2_lookup_glyph(const eui_font_t *font, uint16_t encoding, uint16_t prev)
{
    if (!font || !font->data) return -1;

    if (font->lookup_glyph) {
        return font->lookup_glyph(font, encoding, prev);
    }

    const uint8_t *gd = find_glyph_data(font, encoding);
    if (!gd) return -1;
    return (int16_t)(gd - font->data);
}

static void rle_decode(bit_reader_t *br, uint8_t width, uint8_t height,
                        uint8_t bp0, uint8_t bp1,
                        uint8_t *buf, uint16_t buf_stride,
                        uint8_t color_depth)
{
    uint8_t max0 = (uint8_t)((1u << bp0) - 1);
    uint8_t max1 = (uint8_t)((1u << bp1) - 1);
    uint8_t rt = 0;
    uint16_t remaining = (uint16_t)width * height;
    uint16_t px = 0;

    while (remaining > 0) {
        uint8_t bits = rt ? bp1 : bp0;
        uint8_t max_run = rt ? max1 : max0;
        uint16_t run = 0;
        uint8_t v;
        do {
            v = br_read(br, bits);
            run += v;
        } while (v == max_run);
        if (run > remaining) run = remaining;

        if (rt && color_depth == 1) {
            for (uint16_t k = 0; k < run; k++) {
                uint16_t row = px / width;
                uint16_t col = px % width;
                buf[row * buf_stride + col / 8] |= (1u << (7 - (col % 8)));
                px++;
            }
        } else {
            px += run;
        }
        remaining -= run;
        rt = !rt;
    }
}

uint8_t decode_glyph_at(const eui_font_t *font, uint16_t data_off, u8g2_glyph_t *glyph)
{
    /* data_off is the byte offset of the packed glyph data from font->data */
    const uint8_t *p = font->data;
    uint8_t bpcw = p[HDR_BITS_PER_CHAR_W], bpch = p[HDR_BITS_PER_CHAR_H];
    uint8_t bpcx = p[HDR_BITS_PER_CHAR_X], bpcy = p[HDR_BITS_PER_CHAR_Y];
    uint8_t bpdx = p[HDR_BITS_PER_DELTA_X];
    uint8_t mcw = p[HDR_MAX_CHAR_W];
    uint8_t mch = p[HDR_MAX_CHAR_H];

    bit_reader_t br;
    br.data = p;
    br.byte_pos = data_off;
    br.bit_pos = 0;

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
    glyph->bitmap_byte = br.byte_pos;
    glyph->bitmap_bit  = br.bit_pos;
    return 1;
}

/* Decode glyph metrics from glyph data pointer */
static uint8_t decode_glyph_metrics(const eui_font_t *font,
                                     const uint8_t *glyph_data,
                                     u8g2_glyph_t *glyph)
{
    const uint8_t *p = font->data;
    uint8_t bpcw = p[HDR_BITS_PER_CHAR_W], bpch = p[HDR_BITS_PER_CHAR_H];
    uint8_t bpcx = p[HDR_BITS_PER_CHAR_X], bpcy = p[HDR_BITS_PER_CHAR_Y];
    uint8_t bpdx = p[HDR_BITS_PER_DELTA_X];
    uint8_t mcw = p[HDR_MAX_CHAR_W];
    uint8_t mch = p[HDR_MAX_CHAR_H];

    bit_reader_t br;
    br.data = p;
    br.byte_pos = (uint16_t)(glyph_data - p);
    br.bit_pos = 0;

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
    glyph->bitmap_byte = br.byte_pos;
    glyph->bitmap_bit  = br.bit_pos;
    return 1;
}

static void find_glyph_from_encoding(const eui_font_t *font, uint16_t encoding,
                                      u8g2_glyph_t *glyph)
{
    const uint8_t *gd = find_glyph_data(font, encoding);
    if (gd) decode_glyph_metrics(font, gd, glyph);
}

uint8_t eui_font_u8g2_get_char_width(const eui_font_t *font, char c)
{
    if (!font || !font->data) return 0;
    u8g2_glyph_t g = {0};
    find_glyph_from_encoding(font, (uint8_t)c, &g);
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
    if (!font || !font->data || !buf) return 0;

    u8g2_glyph_t g = {0};
    const uint8_t *gd = find_glyph_data(font, (uint8_t)c);
    if (!gd) return 0;
    if (!decode_glyph_metrics(font, gd, &g)) return 0;

    for (uint16_t py = 0; py < g.height; py++) {
        for (uint16_t px = 0; px < g.width; px++) {
            uint16_t pix_idx = (uint16_t)py * g.width + px;
            if (get_bitmap_pixel(font->data, g.bitmap_byte, g.bitmap_bit, pix_idx)) {
                if (color_depth == 1) {
                    buf[py * buf_stride + px / 8] |= (1u << (7 - (px % 8)));
                }
            }
        }
    }
    return g.x_advance;
}

/* Alternative draw using bitmap pixel approach */
uint8_t get_bitmap_pixel(const uint8_t *data, uint16_t byte_off,
                          uint8_t bit_off, uint16_t pixel_idx)
{
    uint8_t bp0 = data[HDR_BITS_PER_0];
    uint8_t bp1 = data[HDR_BITS_PER_1];
    uint8_t max0 = (uint8_t)((1u << bp0) - 1);
    uint8_t max1 = (uint8_t)((1u << bp1) - 1);
    uint8_t rt = 0;
    uint16_t pos = 0;

    bit_reader_t br;
    br.data = data;
    br.byte_pos = byte_off;
    br.bit_pos = bit_off;

    while (pos <= pixel_idx) {
        uint8_t bits = rt ? bp1 : bp0;
        uint8_t max_run = rt ? max1 : max0;
        uint16_t run = 0;
        uint8_t v;
        do {
            v = br_read(&br, bits);
            run += v;
        } while (v == max_run);
        if (pos + run > pixel_idx) return rt;
        pos += run;
        rt = !rt;
    }
    return 0;
}
