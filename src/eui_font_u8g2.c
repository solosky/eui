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
#define HDR_X_OFFSET        11
#define HDR_Y_OFFSET        12
#define HDR_ASCENT_A        13
#define HDR_DESCENT_G       14
#define HDR_ASCENT_PARA     15
#define HDR_DESCENT_PARA    16
#define HDR_START_POS_UPPER_A 17
#define HDR_START_POS_LOWER_A 19
#define U8G2_HEADER_SIZE    21

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

static int16_t default_lookup(const eui_font_t *font, uint16_t encoding, uint16_t prev)
{
    (void)font; (void)prev;
    if (encoding >= 'A' && encoding <= 'H') return (int16_t)(encoding - 'A');
    return -1;
}

typedef struct {
    uint16_t encoding;
    uint8_t  glyph_index;
} u8g2_enc_entry_t;

/* Scan past the index table to find the encoding table after the terminator */
static uint16_t read_encoding_table(const eui_font_t *font,
                                     u8g2_enc_entry_t *entries,
                                     uint16_t max_entries)
{
    if (!(font->flags & EUI_FONT_HAS_UNICODE)) return 0;

    const uint8_t *data = font->data;
    const uint8_t *p = data + U8G2_HEADER_SIZE;
    while (p[1] != 0) {
        p += p[1];
    }
    const uint8_t *enc = p + 2;

    uint16_t cnt = enc[1] | ((uint16_t)enc[2] << 8);
    if (cnt > max_entries) cnt = max_entries;

    enc += 3;
    for (uint16_t i = 0; i < cnt; i++) {
        entries[i].encoding    = enc[0] | ((uint16_t)enc[1] << 8);
        entries[i].glyph_index = enc[2];
        enc += 3;
    }
    return cnt;
}

static int16_t encoding_lookup(const eui_font_t *font, uint16_t encoding, uint16_t prev)
{
    u8g2_enc_entry_t entries[16];
    uint16_t cnt = read_encoding_table(font, entries, 16);
    (void)prev;

    for (uint16_t i = 0; i < cnt; i++) {
        if (entries[i].encoding == encoding) return (int16_t)entries[i].glyph_index;
    }
    return -1;
}

int16_t eui_font_u8g2_lookup_glyph(const eui_font_t *font, uint16_t encoding, uint16_t prev)
{
    if (!font || !font->data) return -1;

    if (font->lookup_glyph) {
        return font->lookup_glyph(font, encoding, prev);
    }

    if (font->flags & EUI_FONT_HAS_UNICODE) {
        return encoding_lookup(font, encoding, prev);
    }

    return default_lookup(font, encoding, prev);
}

static void rle_decode(bit_reader_t *br, uint8_t width, uint8_t height,
                        uint8_t bp0, uint8_t bp1,
                        uint8_t *buf, uint16_t buf_stride,
                        uint8_t color_depth)
{
    uint8_t max0 = ((1u << bp0) - 1);
    uint8_t max1 = ((1u << bp1) - 1);
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

uint8_t decode_glyph_at(const eui_font_t *font, uint8_t idx, u8g2_glyph_t *glyph)
{
    const uint8_t *p = font->data;
    uint8_t  bpcw = p[HDR_BITS_PER_CHAR_W], bpch = p[HDR_BITS_PER_CHAR_H];
    uint8_t  bpcx = p[HDR_BITS_PER_CHAR_X], bpcy = p[HDR_BITS_PER_CHAR_Y];
    uint8_t  bpdx = p[HDR_BITS_PER_DELTA_X];
    uint8_t  mcw = p[HDR_MAX_CHAR_W];
    uint8_t  mch = p[HDR_MAX_CHAR_H];

    /* Walk the index table to find the idx'th glyph entry */
    const uint8_t *entry = font->data + U8G2_HEADER_SIZE;
    for (uint8_t i = 0; i < idx; i++) {
        if (entry[1] == 0) return 0;
        entry += entry[1];
    }
    if (entry[1] == 0) return 0;

    /* Glyph data starts at entry + 2 (skip encoding and total_size) */
    bit_reader_t br;
    br.data = font->data;
    br.byte_pos = (uint16_t)(entry - font->data) + 2;
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

uint8_t get_bitmap_pixel(const uint8_t *data, uint16_t byte_off,
                          uint8_t bit_off, uint16_t pixel_idx)
{
    uint8_t bp0 = data[HDR_BITS_PER_0];
    uint8_t bp1 = data[HDR_BITS_PER_1];
    uint8_t max0 = ((1u << bp0) - 1);
    uint8_t max1 = ((1u << bp1) - 1);
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

    if (color_depth == 1) {
        const uint8_t *p = font->data;
        uint8_t bp0 = p[HDR_BITS_PER_0];
        uint8_t bp1 = p[HDR_BITS_PER_1];

        bit_reader_t br;
        br.data = font->data;
        br.byte_pos = g.bitmap_byte;
        br.bit_pos = g.bitmap_bit;

        rle_decode(&br, g.width, g.height, bp0, bp1, buf, buf_stride, color_depth);
    }
    return g.x_advance;
}
