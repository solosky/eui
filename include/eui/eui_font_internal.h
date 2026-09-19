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

/* Decoded VLW glyph, used by the canvas text renderer. */
typedef struct {
    int32_t height;     /**< Bitmap height in pixels. */
    int32_t width;      /**< Bitmap width in pixels. */
    int32_t x_advance;  /**< Cursor advance in pixels. */
    int32_t dy;         /**< Distance from baseline up to bitmap top. */
    int32_t dx;         /**< Offset from cursor to bitmap left edge. */
    const uint8_t *bitmap; /**< Bitmap pixels (alpha, 0xFF = opaque ink). */
} eui_vlw_glyph_t;

/**
 * Look up a glyph by code point (supports any BMP code point, unlike
 * the char-based public API).
 * @return 1 if found, 0 if the glyph is absent.
 */
int eui_font_vlw_find_glyph(const eui_font_t *font, uint16_t cp,
                              eui_vlw_glyph_t *out);

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
