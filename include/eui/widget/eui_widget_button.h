#ifndef EUI_WIDGET_BUTTON_H
#define EUI_WIDGET_BUTTON_H

#include "eui/widget/eui_widget.h"
#include "eui/eui_types.h"
#include "eui/eui_str.h"

/**
 * @brief Button click callback.
 *
 * Invoked when the button is activated (pressed and released).
 *
 * @param ctx  User context pointer registered with the callback.
 */
typedef void (*eui_button_callback_t)(void *ctx);

/**
 * @brief Button widget: a clickable control with a label and optional icon.
 *
 * Supports press-state tracking, a text label, and an optional
 * bitmap icon displayed alongside the text.
 */
typedef struct {
    eui_widget_t widget;
    eui_str_t   label;          /**< Button text label. */
    const eui_bitmap_t *icon;   /**< Optional icon bitmap (may be NULL). */
    eui_button_callback_t callback; /**< Click callback (may be NULL). */
    void *callback_ctx;         /**< Context passed to the callback. */
} eui_button_t;

/**
 * @brief Create a button widget.
 *
 * @param label  Button text (may be NULL).
 * @param x      Left edge.
 * @param y      Top edge.
 * @param w      Width.
 * @param h      Height.
 * @return Pointer to the underlying eui_widget_t, or NULL on failure.
 */
eui_widget_t* eui_button_create(const char *label, int16_t x, int16_t y,
                                 uint16_t w, uint16_t h);

/**
 * @brief Assign a click callback to the button.
 *
 * @param btn  Pointer to the button widget.
 * @param cb   Callback function to invoke on click.
 * @param ctx  User context pointer passed to the callback.
 */
void eui_button_set_callback(eui_widget_t *btn, eui_button_callback_t cb, void *ctx);

/**
 * @brief Assign a bitmap icon to the button.
 *
 * The icon is drawn to the left of the label text.
 *
 * @param btn  Pointer to the button widget.
 * @param bmp  Pointer to the bitmap descriptor (may be NULL to remove).
 */
void eui_button_set_bitmap(eui_widget_t *btn, const eui_bitmap_t *bmp);

#endif
