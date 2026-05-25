#include "eui/eui_font.h"
#include "eui/eui_font_internal.h"

#define VLW_HEADER_SIZE 24
#define VLW_GLYPH_HEADER_SIZE 28

static int32_t vlw_read_int32(const uint8_t *p)
{
    return ((int32_t)p[0] << 24) | ((int32_t)p[1] << 16) |
           ((int32_t)p[2] << 8)  |  (int32_t)p[3];
}

/*
 * Binary search glyph table for Unicode code point c.
 * Returns pointer to glyph header (28 bytes) or NULL if not found.
 * Glyph table is sorted by code point in ascending order.
 */
static const uint8_t* find_glyph(const eui_font_t *font, char c)
{
    const uint8_t *p = font->data;
    if (!p) return NULL;

    int32_t count = vlw_read_int32(p);
    if (count <= 0) return NULL;

    const uint8_t *glyphs = p + VLW_HEADER_SIZE;
    int lo = 0, hi = count - 1;

    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        const uint8_t *gh = glyphs + mid * VLW_GLYPH_HEADER_SIZE;
        int32_t v = vlw_read_int32(gh);
        if ((int32_t)(uint8_t)c == v) return gh;
        if ((int32_t)(uint8_t)c < v) hi = mid - 1;
        else lo = mid + 1;
    }
    return NULL;
}

/*
 * Find start of bitmap data for glyph at target_idx.
 * Glyph bitmaps are concatenated sequentially after glyph headers.
 */
static const uint8_t* glyph_bitmap(const eui_font_t *font, int target_idx)
{
    const uint8_t *p = font->data;
    int32_t count = vlw_read_int32(p);
    const uint8_t *glyphs = p + VLW_HEADER_SIZE;
    const uint8_t *bitmap_base = glyphs + count * VLW_GLYPH_HEADER_SIZE;
    int32_t offset = 0;
    for (int i = 0; i < target_idx; i++) {
        const uint8_t *gh = glyphs + i * VLW_GLYPH_HEADER_SIZE;
        int32_t w = vlw_read_int32(gh + 4);
        int32_t h = vlw_read_int32(gh + 8);
        offset += w * h;
    }
    return bitmap_base + offset;
}

/*
 * Find glyph index for Unicode code point c.
 * Returns index (0..count-1) or -1 if not found.
 */
static int find_glyph_index(const eui_font_t *font, char c)
{
    const uint8_t *p = font->data;
    if (!p) return -1;

    int32_t count = vlw_read_int32(p);
    if (count <= 0) return -1;

    const uint8_t *glyphs = p + VLW_HEADER_SIZE;
    int lo = 0, hi = count - 1;

    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        const uint8_t *gh = glyphs + mid * VLW_GLYPH_HEADER_SIZE;
        int32_t v = vlw_read_int32(gh);
        if ((int32_t)(uint8_t)c == v) return mid;
        if ((int32_t)(uint8_t)c < v) hi = mid - 1;
        else lo = mid + 1;
    }
    return -1;
}

uint8_t eui_font_vlw_get_char_width(const eui_font_t *font, char c)
{
    const uint8_t *g = find_glyph(font, c);
    if (!g) return 0;
    return (uint8_t)vlw_read_int32(g + 12);
}

uint16_t eui_font_vlw_get_str_width(const eui_font_t *font, const char *str)
{
    uint16_t w = 0;
    while (*str) {
        w += eui_font_vlw_get_char_width(font, *str);
        str++;
    }
    return w;
}

uint8_t eui_font_vlw_draw_char(const eui_font_t *font, char c,
                                uint8_t *buf, uint16_t buf_stride,
                                uint8_t color_depth)
{
    int idx = find_glyph_index(font, c);
    if (idx < 0) return 0;

    const uint8_t *p = font->data;
    const uint8_t *glyphs = p + VLW_HEADER_SIZE;
    const uint8_t *gh = glyphs + idx * VLW_GLYPH_HEADER_SIZE;

    int32_t w = vlw_read_int32(gh + 4);
    int32_t h = vlw_read_int32(gh + 8);
    int32_t x_advance = vlw_read_int32(gh + 12);

    if (w <= 0 || h <= 0) return (uint8_t)x_advance;

    const uint8_t *bitmap = glyph_bitmap(font, idx);

    for (int32_t row = 0; row < h; row++) {
        for (int32_t col = 0; col < w; col++) {
            uint8_t pixel = bitmap[row * w + col];
            /* pixel 0=ink(opaque), 255=bg(transparent), threshold at 128 */
            if (pixel < 128) {
                if (color_depth == 1) {
                    buf[row * buf_stride + col / 8] |= (1u << (7 - (col % 8)));
                } else if (color_depth == 2) {
                    uint8_t shift = 6u - 2u * (uint8_t)(col % 4u);
                    buf[row * buf_stride + col / 4] |= (3u << shift);
                } else {
                    buf[row * buf_stride + col] = 0xFF;
                }
            }
        }
    }

    return (uint8_t)x_advance;
}
