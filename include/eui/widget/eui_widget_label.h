#ifndef EUI_WIDGET_LABEL_H
#define EUI_WIDGET_LABEL_H

#include "eui/widget/eui_widget.h"
#include "eui/eui_str.h"

/**
 * @brief Label widget: displays a read-only text string.
 *
 * Supports custom fonts and horizontal/vertical alignment.
 * The label does not accept input focus by default.
 */
typedef struct {
    eui_widget_t       widget;
    eui_str_t          text;      /**< The displayed text content. */
    const eui_font_t  *font;      /**< Font to use (NULL = use canvas default). */
    eui_align_t        h_align;   /**< Horizontal alignment (LEFT, CENTER, RIGHT). */
    eui_align_t        v_align;   /**< Vertical alignment (TOP, MIDDLE, BOTTOM). */
} eui_label_t;

/**
 * @brief Create a label widget.
 *
 * @param text  Initial text content (may be NULL).
 * @param x     Left edge.
 * @param y     Top edge.
 * @return Pointer to the underlying eui_widget_t, or NULL on failure.
 */
eui_widget_t* eui_label_create(const char *text, int16_t x, int16_t y);

/**
 * @brief Set the label's displayed text.
 *
 * @param label  Pointer to the label widget.
 * @param text   Null-terminated string to display.
 */
void eui_label_set_text(eui_widget_t *label, const char *text);

/**
 * @brief Set the font used to render the label.
 *
 * @param label  Pointer to the label widget.
 * @param font   Pointer to the font, or NULL to use the canvas default.
 */
void eui_label_set_font(eui_widget_t *label, const eui_font_t *font);

/**
 * @brief Set the text alignment within the label's bounding box.
 *
 * @param label  Pointer to the label widget.
 * @param h      Horizontal alignment.
 * @param v      Vertical alignment.
 */
void eui_label_set_align(eui_widget_t *label, eui_align_t h, eui_align_t v);

#endif
