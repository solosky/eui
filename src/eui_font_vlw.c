#include "eui/eui_font.h"
#include "eui/eui_font_internal.h"

#define VLW_HEADER_SIZE 24
#define VLW_GLYPH_HEADER_SIZE 28

/*
 * VLW (Processing/TFT_eSPI smooth font) binary layout, all fields big-endian:
 *
 * File header (24 bytes):
 *   +0  glyph count
 *   +4  encoder version (ignored)
 *   +8  nominal size / yAdvance
 *   +12 ignored
 *   +16 ascent
 *   +20 descent
 *
 * Glyph header (28 bytes each, sorted ascending by code point):
 *   +0  unicode code point
 *   +4  height
 *   +8  width
 *   +12 xAdvance
 *   +16 dY (baseline up to bitmap top)
 *   +20 dX (cursor to bitmap left edge, signed)
 *   +24 spare
 *
 * Bitmaps follow the glyph table, one byte per pixel, row-major with
 * stride == width. Pixel value is alpha: 0xFF = opaque ink, 0x00 = fully
 * transparent background (matches LovyanGFX VLWfont).
 */

static int32_t vlw_read_int32(const uint8_t *p)
{
    return ((int32_t)p[0] << 24) | ((int32_t)p[1] << 16) |
           ((int32_t)p[2] << 8)  |  (int32_t)p[3];
}

/*
 * Binary search glyph table for code point c.
 * Returns pointer to glyph header (28 bytes) or NULL if not found.
 */
static const uint8_t* find_glyph(const eui_font_t *font, uint16_t c)
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
        if ((int32_t)c == v) return gh;
        if ((int32_t)c < v) hi = mid - 1;
        else lo = mid + 1;
    }
    return NULL;
}

/* Find glyph index for code point c. Returns index (0..count-1) or -1. */
static int find_glyph_index(const eui_font_t *font, uint16_t c)
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
        if ((int32_t)c == v) return mid;
        if ((int32_t)c < v) hi = mid - 1;
        else lo = mid + 1;
    }
    return -1;
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
        int32_t h = vlw_read_int32(gh + 4);
        int32_t w = vlw_read_int32(gh + 8);
        offset += w * h;
    }
    return bitmap_base + offset;
}

int eui_font_vlw_find_glyph(const eui_font_t *font, uint16_t cp,
                              eui_vlw_glyph_t *out)
{
    if (!font || !font->data || !out) return 0;
    const uint8_t *gh = find_glyph(font, cp);
    if (!gh) return 0;

    int idx = find_glyph_index(font, cp);
    if (idx < 0) return 0;

    out->height    = vlw_read_int32(gh + 4);
    out->width     = vlw_read_int32(gh + 8);
    out->x_advance = vlw_read_int32(gh + 12);
    out->dy        = vlw_read_int32(gh + 16);
    out->dx        = vlw_read_int32(gh + 20);
    out->bitmap    = glyph_bitmap(font, idx);
    return 1;
}

uint8_t eui_font_vlw_get_char_width(const eui_font_t *font, char c)
{
    const uint8_t *g = find_glyph(font, (uint16_t)(uint8_t)c);
    if (!g) return 0;
    /* report xAdvance so callers can lay out text by cursor advance */
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
    int idx = find_glyph_index(font, (uint16_t)(uint8_t)c);
    if (idx < 0) return 0;

    const uint8_t *p = font->data;
    const uint8_t *glyphs = p + VLW_HEADER_SIZE;
    const uint8_t *gh = glyphs + idx * VLW_GLYPH_HEADER_SIZE;

    int32_t h = vlw_read_int32(gh + 4);
    int32_t w = vlw_read_int32(gh + 8);
    int32_t x_advance = vlw_read_int32(gh + 12);

    if (w <= 0 || h <= 0) return (uint8_t)x_advance;

    const uint8_t *bitmap = glyph_bitmap(font, idx);

    for (int32_t row = 0; row < h; row++) {
        for (int32_t col = 0; col < w; col++) {
            uint8_t alpha = bitmap[row * w + col];
            if (alpha == 0) continue;
            if (color_depth == 1) {
                if (alpha >= 128)
                    buf[row * buf_stride + col / 8] |= (1u << (7 - (col % 8)));
            } else if (color_depth == 2) {
                if (alpha >= 128) {
                    uint8_t shift = 6u - 2u * (uint8_t)(col % 4u);
                    buf[row * buf_stride + col / 4] |= (3u << shift);
                }
            } else if (color_depth == 4) {
                if (alpha >= 128) {
                    uint8_t shift = (uint8_t)(4u * (1u - (col & 1u)));
                    buf[row * buf_stride + col / 2] |= (0x0Fu << shift);
                }
            } else {
                /* 8bpp: store the alpha value for anti-aliasing */
                buf[row * buf_stride + col] = alpha;
            }
        }
    }

    return (uint8_t)x_advance;
}

/*
 * Initialize a font descriptor from raw VLW data and derive the line
 * metrics the same way LovyanGFX does: baseline = maxAscent and
 * line_height = maxAscent + maxDescent, refined by scanning every
 * printable glyph's dY (distance from baseline to bitmap top).
 */
void eui_font_vlw_init(eui_font_t *font, const uint8_t *data)
{
    if (!font || !data) return;

    int32_t count   = vlw_read_int32(data);
    int32_t size    = vlw_read_int32(data + 8);
    int32_t ascent  = vlw_read_int32(data + 16);
    int32_t descent = vlw_read_int32(data + 20);
    (void)size;

    int32_t max_ascent  = ascent;
    int32_t max_descent = descent;

    const uint8_t *glyphs = data + VLW_HEADER_SIZE;
    for (int32_t i = 0; i < count; i++) {
        const uint8_t *gh = glyphs + i * VLW_GLYPH_HEADER_SIZE;
        int32_t uni = vlw_read_int32(gh);
        if (!((uni > 0xFF) || ((uni > 0x20) && (uni < 0xA0) && (uni != 0x7F))))
            continue;
        if (uni == 0x3000) continue;

        int32_t h  = vlw_read_int32(gh + 4);
        int32_t dy = vlw_read_int32(gh + 16);
        if (max_ascent < dy) max_ascent = dy;
        if (max_descent < h - dy) max_descent = h - dy;
    }

    font->format = EUI_FONT_FORMAT_VLW;
    font->flags = 0;
    font->data = data;
    font->baseline = (uint8_t)max_ascent;
    font->line_height = (uint8_t)(max_ascent + max_descent);
}
