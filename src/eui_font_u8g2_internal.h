#ifndef EUI_FONT_U8G2_INTERNAL_H
#define EUI_FONT_U8G2_INTERNAL_H

#include "eui/eui_font.h"
#include <stdint.h>

typedef struct {
    int8_t  x_offset;
    int8_t  y_offset;
    uint8_t width;
    uint8_t height;
    uint8_t x_advance;
    uint16_t bitmap_byte;
    uint8_t  bitmap_bit;
} u8g2_glyph_t;

uint8_t decode_glyph_at(const eui_font_t *font, uint16_t data_off, u8g2_glyph_t *glyph);
uint8_t get_bitmap_pixel(const uint8_t *data, uint16_t byte_off,
                          uint8_t bit_off, uint16_t pixel_idx);

#endif
