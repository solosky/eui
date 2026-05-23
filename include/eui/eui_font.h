#ifndef EUI_FONT_H
#define EUI_FONT_H

#include "eui/eui_types.h"
#include <stdint.h>

/**
 * @brief Font format identifiers.
 *
 * Used in eui_font_t::format to indicate the internal encoding.
 */
#define EUI_FONT_FORMAT_BDF  0 /**< BDF (Bitmap Distribution Format). */
#define EUI_FONT_FORMAT_VLW  1 /**< VLW (Adafruit GFX format). */
#define EUI_FONT_FORMAT_U8G2 2 /**< U8G2 font format (requires lookup callback). */

/**
 * @brief Get the width of a single ASCII character in the font.
 *
 * @param font  Pointer to an initialized eui_font_t.
 * @param c     The character to measure.
 * @return The character's width in pixels.
 */
uint8_t  eui_font_get_char_width(const eui_font_t *font, char c);

/**
 * @brief Get the total pixel width of a string when rendered with this font.
 *
 * @param font  Pointer to an initialized eui_font_t.
 * @param str   Null-terminated string to measure.
 * @return The string width in pixels.
 */
uint16_t eui_font_get_str_width(const eui_font_t *font, const char *str);

/**
 * @brief Get the line height of the font.
 *
 * @param font  Pointer to an initialized eui_font_t.
 * @return The font's line height in pixels.
 */
uint8_t  eui_font_get_height(const eui_font_t *font);

/**
 * @brief Get the baseline offset of the font.
 *
 * The baseline is the vertical distance from the top of the glyph cell
 * to the typographic baseline.
 *
 * @param font  Pointer to an initialized eui_font_t.
 * @return Baseline offset in pixels.
 */
uint8_t  eui_font_get_baseline(const eui_font_t *font);

/**
 * @brief Render a single character glyph into a pixel buffer.
 *
 * Draws the glyph for character @p c into the buffer at the current
 * cursor position implied by the buffer layout.
 *
 * @param font         Pointer to an initialized eui_font_t.
 * @param c            The character to render.
 * @param buf          Destination pixel buffer.
 * @param buf_stride   Number of bytes per row of @p buf.
 * @param color_depth  Bits per pixel of the destination buffer.
 * @return The character's advance width in pixels (for cursor advancement).
 */
uint8_t eui_font_draw_char(const eui_font_t *font, char c,
                            uint8_t *buf, uint16_t buf_stride,
                            uint8_t color_depth);

#endif /* EUI_FONT_H */
