#include "eui/eui_font.h"
#include <stddef.h>

#define BDF_HEADER_SIZE 3

static const uint8_t* glyph_data_start(const eui_font_t *font)
{
    const uint8_t *p = font->data;
    uint8_t first = p[0];
    uint8_t last = p[1];
    uint8_t count = last - first + 1;
    return p + BDF_HEADER_SIZE + count * 2;
}

static const uint8_t* get_glyph(const eui_font_t *font, char c)
{
    if (font->format != EUI_FONT_FORMAT_BDF) return NULL;
    const uint8_t *p = font->data;
    uint8_t first = p[0];
    uint8_t last = p[1];
    if ((uint8_t)c < first || (uint8_t)c > last) return NULL;
    uint8_t idx = (uint8_t)c - first;
    const uint8_t *data_start = glyph_data_start(font);
    uint16_t offset = p[BDF_HEADER_SIZE + idx * 2] | ((uint16_t)p[BDF_HEADER_SIZE + idx * 2 + 1] << 8);
    return data_start + offset;
}

uint8_t eui_font_bdf_get_char_width(const eui_font_t *font, char c)
{
    const uint8_t *g = get_glyph(font, c);
    if (!g) return 0;
    return g[4];
}

uint16_t eui_font_bdf_get_str_width(const eui_font_t *font, const char *str)
{
    uint16_t w = 0;
    while (*str) {
        w += eui_font_bdf_get_char_width(font, *str);
        str++;
    }
    return w;
}

uint8_t eui_font_bdf_draw_char(const eui_font_t *font, char c,
                                uint8_t *buf, uint16_t buf_stride,
                                uint8_t color_depth)
{
    const uint8_t *g = get_glyph(font, c);
    if (!g) return 0;
    uint8_t w = g[0];
    uint8_t h = g[1];
    uint8_t x_advance = g[4];
    uint8_t bytes_per_row = (w + 7) / 8;
    const uint8_t *bitmap = g + 5;

    for (uint8_t row = 0; row < h; row++) {
        for (uint8_t col = 0; col < w; col++) {
            uint8_t byte = bitmap[row * bytes_per_row + col / 8];
            uint8_t bit = (byte >> (7 - (col % 8))) & 1;
            if (bit) {
                if (color_depth == 1) {
                    buf[row * buf_stride + col / 8] |= (1u << (7 - (col % 8)));
                } else if (color_depth == 2) {
                    uint8_t shift = 6u - 2u * (col % 4u);
                    buf[row * buf_stride + col / 4] |= (3u << shift);
                } else if (color_depth == 4) {
                    uint8_t shift = (uint8_t)(4u * (1u - (col & 1u)));
                    buf[row * buf_stride + col / 2] |= (0x0Fu << shift);
                }
            }
        }
    }
    return x_advance;
}
