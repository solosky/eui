#ifndef EUI_FONT_BUILTIN_H
#define EUI_FONT_BUILTIN_H

#include <stdint.h>
#include "eui/eui_types.h"

/**
 * @brief Raw data of the built-in font (WQY 13px Chinese bitmap font).
 */
extern const uint8_t eui_font_builtin_data[];

/**
 * @brief Pre-defined built-in font descriptor (WQY 13px).
 *
 * This font includes both ASCII and CJK glyph coverage.
 * It is available for use without any external font file.
 */
extern const eui_font_t eui_font_builtin;

#endif
