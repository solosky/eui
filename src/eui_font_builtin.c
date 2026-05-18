#include "eui/eui_font.h"
#include "eui/eui_font_builtin.h"

/*
 * Built-in 8x8 fixed-width font, BDF format.
 * Covers ASCII 'A' (65) through 'H' (72).
 *
 * u8g2 BDF binary layout:
 *   [0] first_char (65 = 'A')
 *   [1] last_char (72 = 'H')
 *   [2] line_height (8)
 *   [3..] glyph offset table: 2 bytes per character (uint16 offset from glyph data start)
 *   [glyph data area]:
 *     For each glyph: width, height, x_offset(signed), y_offset(signed), x_advance,
 *     then bitmap: ceil(width/8)*height bytes
 */

static const uint8_t builtin_glyph_data[] = {
    /* 'A' */
    8, 8, 0, 0, 8,
    0x18, 0x3C, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00,
    /* 'B' */
    8, 8, 0, 0, 8,
    0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00,
    /* 'C' */
    8, 8, 0, 0, 8,
    0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C, 0x00,
    /* 'D' */
    8, 8, 0, 0, 8,
    0x78, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0x78, 0x00,
    /* 'E' */
    8, 8, 0, 0, 8,
    0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x7E, 0x00,
    /* 'F' */
    8, 8, 0, 0, 8,
    0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x60, 0x00,
    /* 'G' */
    8, 8, 0, 0, 8,
    0x3C, 0x66, 0x60, 0x6E, 0x66, 0x66, 0x3C, 0x00,
    /* 'H' */
    8, 8, 0, 0, 8,
    0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00,
};

/* Each glyph entry is 5 (header) + 8 (bitmap) = 13 bytes */
#define GLYPH_ENTRY_SIZE  13
#define GLYPH_COUNT       8
#define OFFSET_TABLE_SIZE (GLYPH_COUNT * 2)
#define TOTAL_SIZE         (3 + OFFSET_TABLE_SIZE + GLYPH_COUNT * GLYPH_ENTRY_SIZE)

#define FIRST_CHAR 65
#define LAST_CHAR  72

const uint8_t eui_font_builtin_data[TOTAL_SIZE] = {
    FIRST_CHAR,      /* byte 0: first_char */
    LAST_CHAR,       /* byte 1: last_char */
    8,               /* byte 2: line_height */
    /* offset table: 2 bytes per char, little-endian */
    0, 0,            /* 'A' offset 0 */
    13, 0,           /* 'B' offset 13 */
    26, 0,           /* 'C' offset 26 */
    39, 0,           /* 'D' offset 39 */
    52, 0,           /* 'E' offset 52 */
    65, 0,           /* 'F' offset 65 */
    78, 0,           /* 'G' offset 78 */
    91, 0,           /* 'H' offset 91 */
    /* glyph data area follows */
    /* 'A' */
    8, 8, 0, 0, 8,
    0x18, 0x3C, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00,
    /* 'B' */
    8, 8, 0, 0, 8,
    0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00,
    /* 'C' */
    8, 8, 0, 0, 8,
    0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C, 0x00,
    /* 'D' */
    8, 8, 0, 0, 8,
    0x78, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0x78, 0x00,
    /* 'E' */
    8, 8, 0, 0, 8,
    0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x7E, 0x00,
    /* 'F' */
    8, 8, 0, 0, 8,
    0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x60, 0x00,
    /* 'G' */
    8, 8, 0, 0, 8,
    0x3C, 0x66, 0x60, 0x6E, 0x66, 0x66, 0x3C, 0x00,
    /* 'H' */
    8, 8, 0, 0, 8,
    0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00,
};

const eui_font_t eui_font_builtin = {
    .format = EUI_FONT_FORMAT_BDF,  /* 0 */
    .line_height = 8,
    .baseline = 7,
    .flags = EUI_FONT_FIXED_WIDTH,
    .data = eui_font_builtin_data,
};
