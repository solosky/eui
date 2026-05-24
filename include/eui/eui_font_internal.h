#ifndef EUI_FONT_INTERNAL_H
#define EUI_FONT_INTERNAL_H

#include "eui/eui_font.h"

uint8_t eui_font_bdf_get_char_width(const eui_font_t *font, char c);
uint16_t eui_font_bdf_get_str_width(const eui_font_t *font, const char *str);
uint8_t eui_font_bdf_draw_char(const eui_font_t *font, char c,
                                uint8_t *buf, uint16_t buf_stride,
                                uint8_t color_depth);

uint8_t eui_font_vlw_get_char_width(const eui_font_t *font, char c);
uint16_t eui_font_vlw_get_str_width(const eui_font_t *font, const char *str);
uint8_t eui_font_vlw_draw_char(const eui_font_t *font, char c,
                                uint8_t *buf, uint16_t buf_stride,
                                uint8_t color_depth);

#if EUI_FONT_ENABLE_U8G2
uint8_t  eui_font_u8g2_get_char_width(const eui_font_t *font, char c);
uint16_t eui_font_u8g2_get_str_width(const eui_font_t *font, const char *str);
uint8_t  eui_font_u8g2_draw_char(const eui_font_t *font, char c,
                                  uint8_t *buf, uint16_t buf_stride,
                                  uint8_t color_depth);
uint8_t  eui_font_u8g2_draw_glyph(const eui_font_t *font, uint16_t encoding,
                                   uint8_t *buf, uint16_t buf_stride,
                                   uint8_t color_depth);
int32_t  eui_font_u8g2_lookup_glyph(const eui_font_t *font, uint16_t encoding, uint16_t prev);
#endif

#endif
