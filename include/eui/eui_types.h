#ifndef EUI_TYPES_H
#define EUI_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "eui/eui_config.h"

/* ---- Rectangle ---- */
typedef struct {
    int16_t x, y;
    uint16_t w, h;
} eui_rect_t;

/* ---- Color abstraction ---- */
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

typedef enum {
    EUI_COLOR_BLACK = 0,
#if EUI_COLOR_DEPTH == 16
    EUI_COLOR_WHITE = 0xFFFF,
#else
    EUI_COLOR_WHITE = 1,
#endif
} eui_color_id_t;

eui_color_t eui_color_from_rgb(uint8_t r, uint8_t g, uint8_t b);
eui_color_t eui_color_from_gray(uint8_t gray);

/* ---- Alignment ---- */
typedef enum {
    EUI_ALIGN_LEFT   = 0x01,
    EUI_ALIGN_CENTER = 0x02,
    EUI_ALIGN_RIGHT  = 0x04,
    EUI_ALIGN_TOP    = 0x10,
    EUI_ALIGN_MIDDLE = 0x20,
    EUI_ALIGN_BOTTOM = 0x40,
} eui_align_t;

/* ---- Bitmap (merged icon + bitmap) ---- */
typedef struct {
    uint16_t width;
    uint16_t height;
    uint8_t  color_depth;
    const uint8_t *data;
} eui_bitmap_t;

/* ---- Font ---- */

struct eui_font;

typedef int16_t (*eui_font_lookup_fn)(const struct eui_font *font,
                                       uint16_t encoding, uint16_t prev);

typedef struct eui_font {
    uint8_t  format;
    uint8_t  line_height;
    uint8_t  baseline;
    uint8_t  flags;
    const uint8_t *data;
#if EUI_FONT_ENABLE_U8G2
    eui_font_lookup_fn lookup_glyph;
#endif
} eui_font_t;

#define EUI_FONT_FIXED_WIDTH  (1u << 0)
#define EUI_FONT_HAS_KERNING  (1u << 1)
#define EUI_FONT_HAS_UNICODE  (1u << 2)

/* ---- Buffer mode ---- */
typedef enum {
    EUI_BUFFER_FULL   = (1u << 0),
    EUI_BUFFER_PAGE   = (1u << 1),
    EUI_BUFFER_DIRECT = (1u << 2),
} eui_buffer_mode_t;

/* ---- Animation types (placeholder for now) ---- */
typedef enum {
    EUI_ANIM_NONE = 0,
    EUI_ANIM_SLIDE_LEFT,
    EUI_ANIM_SLIDE_RIGHT,
    EUI_ANIM_FADE,
    EUI_ANIM_SCALE,
    EUI_ANIM_SLIDE_UP,
} eui_anim_type_t;

/* ---- Utility functions ---- */
bool eui_rect_intersect(const eui_rect_t *a, const eui_rect_t *b, eui_rect_t *out);
bool eui_rect_contains(const eui_rect_t *rect, int16_t x, int16_t y);
void eui_rect_union(const eui_rect_t *a, const eui_rect_t *b, eui_rect_t *out);

#endif /* EUI_TYPES_H */
