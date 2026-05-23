#ifndef EUI_TYPES_H
#define EUI_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "eui/eui_config.h"

/**
 * @brief Axis-aligned rectangle.
 */
typedef struct {
    int16_t  x; /**< Left edge (column), may be negative. */
    int16_t  y; /**< Top edge (row), may be negative. */
    uint16_t w; /**< Width in pixels. */
    uint16_t h; /**< Height in pixels. */
} eui_rect_t;

/**
 * @brief Native color type.
 *
 * Width depends on EUI_COLOR_DEPTH:
 * - 1, 4, 8 → uint8_t
 * - 16       → uint16_t
 */
#if EUI_COLOR_DEPTH == 1
typedef uint8_t  eui_color_t;
#elif EUI_COLOR_DEPTH == 4
typedef uint8_t  eui_color_t;
#elif EUI_COLOR_DEPTH == 8
typedef uint8_t  eui_color_t;
#elif EUI_COLOR_DEPTH == 16
typedef uint16_t eui_color_t;
#else
#error "EUI_COLOR_DEPTH must be 1, 4, 8, or 16"
#endif

/**
 * @brief Predefined named colors.
 */
typedef enum {
    EUI_COLOR_BLACK = 0,
#if EUI_COLOR_DEPTH == 16
    EUI_COLOR_WHITE = 0xFFFF,
#else
    EUI_COLOR_WHITE = 1,
#endif
} eui_color_id_t;

/**
 * @brief Convert 8-bit RGB components to the native color format.
 *
 * @param r  Red component (0-255).
 * @param g  Green component (0-255).
 * @param b  Blue component (0-255).
 * @return The color encoded in the native pixel format.
 */
eui_color_t eui_color_from_rgb(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Convert an 8-bit grayscale value to the native color format.
 *
 * @param gray  Grayscale intensity (0-255).
 * @return The color encoded in the native pixel format.
 */
eui_color_t eui_color_from_gray(uint8_t gray);

/**
 * @brief Horizontal and vertical alignment flags.
 *
 * Multiple flags can be OR-ed together (one horizontal + one vertical).
 */
typedef enum {
    EUI_ALIGN_LEFT   = 0x01, /**< Align to the left edge. */
    EUI_ALIGN_CENTER = 0x02, /**< Center horizontally. */
    EUI_ALIGN_RIGHT  = 0x04, /**< Align to the right edge. */
    EUI_ALIGN_TOP    = 0x10, /**< Align to the top edge. */
    EUI_ALIGN_MIDDLE = 0x20, /**< Center vertically. */
    EUI_ALIGN_BOTTOM = 0x40, /**< Align to the bottom edge. */
} eui_align_t;

/**
 * @brief Bitmap image descriptor.
 *
 * Supports multiple color depths (1, 4, 8, 16 bits per pixel).
 * The data is stored in a platform-native byte order.
 */
typedef struct {
    uint16_t width;        /**< Image width in pixels. */
    uint16_t height;       /**< Image height in pixels. */
    uint8_t  color_depth;  /**< Bits per pixel (1, 4, 8, or 16). */
    const uint8_t *data;   /**< Raw pixel data buffer. */
} eui_bitmap_t;

/* ---- Font ---- */

struct eui_font;

/**
 * @brief Glyph lookup callback for U8G2 format fonts.
 *
 * @param font      The font being queried.
 * @param encoding  Unicode code point of the requested glyph.
 * @param prev      Previous glyph encoding (for kerning, 0 if none).
 * @return Glyph bitmap index or negative on failure.
 */
typedef int32_t (*eui_font_lookup_fn)(const struct eui_font *font,
                                       uint16_t encoding, uint16_t prev);

/**
 * @brief Font descriptor.
 *
 * Supports three internal formats: BDF, VLW, and U8G2.
 */
typedef struct eui_font {
    uint8_t  format;      /**< One of EUI_FONT_FORMAT_BDF/VLW/U8G2. */
    uint8_t  line_height; /**< Full line height in pixels. */
    uint8_t  baseline;    /**< Baseline offset from the top of the glyph cell. */
    uint8_t  flags;       /**< Combination of EUI_FONT_* flags. */
    const uint8_t *data;  /**< Font file data in the respective format. */
#if EUI_FONT_ENABLE_U8G2
    eui_font_lookup_fn lookup_glyph; /**< U8G2 glyph lookup callback. */
#endif
} eui_font_t;

/** @brief Font glyphs have a fixed width (monospace). */
#define EUI_FONT_FIXED_WIDTH  (1u << 0)
/** @brief Font contains kerning pair information. */
#define EUI_FONT_HAS_KERNING  (1u << 1)
/** @brief Font supports Unicode (multi-byte) characters. */
#define EUI_FONT_HAS_UNICODE  (1u << 2)

/**
 * @brief Display buffer strategy.
 */
typedef enum {
    EUI_BUFFER_FULL   = (1u << 0), /**< Single full-frame buffer. */
    EUI_BUFFER_PAGE   = (1u << 1), /**< Page-at-a-time rendering. */
    EUI_BUFFER_DIRECT = (1u << 2), /**< Direct (no buffer, write per pixel). */
} eui_buffer_mode_t;

/**
 * @brief Scene/view transition animation types.
 */
typedef enum {
    EUI_ANIM_NONE        = 0, /**< No animation, instant switch. */
    EUI_ANIM_SLIDE_LEFT,      /**< Slide incoming view from the right. */
    EUI_ANIM_SLIDE_RIGHT,     /**< Slide incoming view from the left. */
    EUI_ANIM_FADE,            /**< Cross-fade between views. */
    EUI_ANIM_SCALE,           /**< Scale incoming view from center. */
    EUI_ANIM_SLIDE_UP,        /**< Slide incoming view from the bottom. */
} eui_anim_type_t;

/* ---- Utility functions ---- */

/**
 * @brief Compute the intersection of two rectangles.
 *
 * @param a   First rectangle.
 * @param b   Second rectangle.
 * @param out [out] Receives the intersection rectangle (unchanged if disjoint).
 * @return true if the rectangles overlap, false if they are disjoint.
 */
bool eui_rect_intersect(const eui_rect_t *a, const eui_rect_t *b, eui_rect_t *out);

/**
 * @brief Test whether a point lies inside a rectangle.
 *
 * @param rect  The rectangle to test against.
 * @param x     X-coordinate of the point.
 * @param y     Y-coordinate of the point.
 * @return true if the point is inside the rectangle (edges inclusive).
 */
bool eui_rect_contains(const eui_rect_t *rect, int16_t x, int16_t y);

/**
 * @brief Compute the union (bounding) rectangle of two rectangles.
 *
 * The result is the smallest rectangle that contains both @p a and @p b.
 *
 * @param a   First rectangle.
 * @param b   Second rectangle.
 * @param out [out] Receives the union rectangle.
 */
void eui_rect_union(const eui_rect_t *a, const eui_rect_t *b, eui_rect_t *out);

#endif /* EUI_TYPES_H */
