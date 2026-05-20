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
#define HDR_START_POS_UNICODE 21
#define U8G2_HEADER_SIZE    23

typedef struct {
    const uint8_t *data;
    uint16_t byte_pos;
    uint8_t  bit_pos;
} bit_reader_t;

static uint8_t br_read(bit_reader_t *br, uint8_t num_bits)
{
    uint8_t val;
    uint8_t bit_pos = br->bit_pos;
    uint8_t bit_pos_plus_cnt;

    val = br->data[br->byte_pos];
    val >>= bit_pos;
    bit_pos_plus_cnt = bit_pos;
    bit_pos_plus_cnt += num_bits;
    if (bit_pos_plus_cnt >= 8)
    {
        uint8_t s = 8;
        s -= bit_pos;
        br->byte_pos++;
        val |= br->data[br->byte_pos] << s;
        bit_pos_plus_cnt -= 8;
    }
    val &= (1u << num_bits) - 1;
    br->bit_pos = bit_pos_plus_cnt;
    return val;
}

static int8_t br_read_signed(bit_reader_t *br, uint8_t num_bits)
{
    int8_t v, d;
    v = (int8_t)br_read(br, num_bits);
    d = 1;
    num_bits--;
    d <<= num_bits;
    v -= d;
    return v;
}

static uint16_t get_be16(const uint8_t *p)
{
    return ((uint16_t)p[0] << 8) | p[1];
}

/* Search the unicode encoding table for a glyph.
 * Returns pointer to packed glyph data, or NULL if not found. */
static const uint8_t* find_glyph_data_unicode(const eui_font_t *font, uint16_t encoding)
{
    const uint8_t *p = font->data;
    uint16_t unicode_off = get_be16(p + HDR_START_POS_UNICODE);
    if (unicode_off == 0) return NULL;

    const uint8_t *jump_table = p + U8G2_HEADER_SIZE + unicode_off;
    const uint8_t *lt = jump_table;

    for (;;) {
        uint16_t block_off = get_be16(lt);
        uint16_t last_unicode = get_be16(lt + 2);

        if (last_unicode == 0xFFFF) return NULL;

        const uint8_t *block = jump_table + block_off;
        lt += 4;

        /* Compute block end boundary from next jump entry */
        uint16_t next_off = get_be16(lt);
        const uint8_t *block_end = jump_table + next_off;

        if (last_unicode >= encoding) {
            /* Linear search within this block */
            const uint8_t *entry = block;
            while (entry + 3 <= block_end) {
                uint16_t code = get_be16(entry);
                uint8_t jump = entry[2];
                if (jump < 3 || jump > 60) return NULL;
                if (code == encoding) return entry + 3;
                entry += jump;
            }
            return NULL;
        }
    }
}

/* Walk the index table to find glyph data for the given encoding.
 * Returns pointer to packed glyph data, or NULL if not found. */
static const uint8_t* find_glyph_data(const eui_font_t *font, uint16_t encoding)
{
    if (!font || !font->data) return NULL;
    const uint8_t *p = font->data;
    const uint8_t *entry = p + U8G2_HEADER_SIZE;

    if (encoding >= 'a') {
        entry += get_be16(p + HDR_START_POS_LOWER_A);
    } else if (encoding >= 'A') {
        entry += get_be16(p + HDR_START_POS_UPPER_A);
    }

    while (entry[1] != 0) {
        if (entry[0] == (uint8_t)encoding) return entry + 2;
        entry += entry[1];
    }

    /* Fall back to unicode encoding table for code points > 255 */
    if (encoding > 255 || get_be16(p + HDR_START_POS_UNICODE) != 0) {
        return find_glyph_data_unicode(font, encoding);
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
    uint8_t a, b;
    uint8_t x = 0, y = 0;

    for (;;) {
        a = br_read(br, bp0);
        b = br_read(br, bp1);

        do {
            /* draw a zeros (background) - skip over them */
            {
                uint16_t cnt = a;
                for (;;) {
                    uint8_t rem = width - x;
                    if (cnt < rem) { x += (uint8_t)cnt; break; }
                    cnt -= rem;
                    x = 0; y++;
                }
            }

            /* draw b ones (foreground) */
            {
                uint16_t cnt = b;
                uint8_t lx = x, ly = y;
                for (;;) {
                    uint8_t rem = width - lx;
                    uint8_t cur = rem;
                    if (cnt < rem) cur = (uint8_t)cnt;
                    if (color_depth == 1) {
                        for (uint8_t i = 0; i < cur; i++) {
                            buf[ly * buf_stride + (lx + i) / 8]
                                |= (1u << (7 - ((lx + i) % 8)));
                        }
                    }
                    if (cnt < rem) { x = lx + cur; break; }
                    cnt -= rem;
                    lx = 0; ly++;
                }
            }
        } while(br_read(br, 1) != 0);

        if (y >= height) break;
    }
}

uint8_t decode_glyph_at(const eui_font_t *font, uint16_t data_off, u8g2_glyph_t *glyph)
{
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
    int8_t cx = br_read_signed(&br, bpcx);
    int8_t cy = br_read_signed(&br, bpcy);
    int8_t dx = br_read_signed(&br, bpdx);

    glyph->width     = cw ? cw : mcw;
    glyph->height    = ch ? ch : mch;
    glyph->x_offset  = cx;
    glyph->y_offset  = cy;
    glyph->x_advance = (uint8_t)(dx > 0 ? dx : (int8_t)mcw);
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
    int8_t cx = br_read_signed(&br, bpcx);
    int8_t cy = br_read_signed(&br, bpcy);
    int8_t dx = br_read_signed(&br, bpdx);

    glyph->width     = cw ? cw : mcw;
    glyph->height    = ch ? ch : mch;
    glyph->x_offset  = cx;
    glyph->y_offset  = cy;
    glyph->x_advance = (uint8_t)(dx > 0 ? dx : (int8_t)mcw);
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

uint8_t eui_font_u8g2_draw_glyph(const eui_font_t *font, uint16_t encoding,
                                  uint8_t *buf, uint16_t buf_stride,
                                  uint8_t color_depth)
{
    if (!font || !font->data || !buf) return 0;

    u8g2_glyph_t g = {0};
    const uint8_t *gd = find_glyph_data(font, encoding);
    if (!gd) return 0;
    if (!decode_glyph_metrics(font, gd, &g)) return 0;

    int8_t baseline = font->baseline;

    for (uint16_t py = 0; py < g.height; py++) {
        for (uint16_t px = 0; px < g.width; px++) {
            uint16_t pix_idx = (uint16_t)py * g.width + px;
            if (get_bitmap_pixel(font->data, g.bitmap_byte, g.bitmap_bit, pix_idx,
                                   g.width, g.height)) {
                int16_t buf_row = (int16_t)baseline - (int16_t)g.height - (int16_t)g.y_offset + (int16_t)py;
                if (buf_row < 0) continue;
                if (color_depth == 1) {
                    buf[(uint16_t)buf_row * buf_stride + px / 8] |= (1u << (7 - (px % 8)));
                }
            }
        }
    }
    return g.x_advance;
}

uint8_t eui_font_u8g2_draw_char(const eui_font_t *font, char c,
                                 uint8_t *buf, uint16_t buf_stride,
                                 uint8_t color_depth)
{
    return eui_font_u8g2_draw_glyph(font, (uint8_t)c, buf, buf_stride, color_depth);
}

/* Alternative draw using bitmap pixel approach */
uint8_t get_bitmap_pixel(const uint8_t *data, uint16_t byte_off,
                           uint8_t bit_off, uint16_t pixel_idx,
                           uint8_t glyph_width, uint8_t glyph_height)
{
    uint8_t bp0 = data[HDR_BITS_PER_0];
    uint8_t bp1 = data[HDR_BITS_PER_1];
    uint8_t x = 0, y = 0;
    uint16_t pos = 0;

    bit_reader_t br;
    br.data = data;
    br.byte_pos = byte_off;
    br.bit_pos = bit_off;

    for (;;) {
        uint8_t a = br_read(&br, bp0);
        uint8_t b = br_read(&br, bp1);

        do {
            /* skip a zeros */
            {
                uint16_t cnt = a;
                for (;;) {
                    uint8_t rem = glyph_width - x;
                    if (cnt < rem) { x += (uint8_t)cnt; pos += (uint8_t)cnt; break; }
                    cnt -= rem;
                    pos += rem;
                    x = 0; y++;
                }
            }

            /* check if target is within b ones */
            {
                uint16_t cnt = b;
                uint8_t lx = x, ly = y;
                for (;;) {
                    uint8_t rem = glyph_width - lx;
                    uint8_t cur = rem;
                    if (cnt < rem) cur = (uint8_t)cnt;
                    for (uint8_t i = 0; i < cur; i++) {
                        if (pos == pixel_idx) return 1;
                        pos++;
                    }
                    if (cnt < rem) { x = lx + cur; break; }
                    cnt -= rem;
                    lx = 0; ly++;
                }
            }

            /* early exit if we passed the target */
            if (pos > pixel_idx) return 0;
        } while(br_read(&br, 1) != 0);

        if (y >= glyph_height) break;
    }
    return 0;
}
