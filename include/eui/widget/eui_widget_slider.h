#ifndef EUI_WIDGET_SLIDER_H
#define EUI_WIDGET_SLIDER_H

#include "eui/widget/eui_widget.h"

/**
 * @brief Slider widget: a draggable control for selecting a numeric value.
 *
 * Displays a horizontal track with a thumb that the user can slide
 * to select a value within a configurable min/max range.
 */
typedef struct {
    eui_widget_t widget;
    int16_t value, min, max; /**< Current value and range bounds. */
} eui_slider_t;

/**
 * @brief Create a slider widget.
 *
 * @param x  Left edge.
 * @param y  Top edge.
 * @param w  Width.
 * @param h  Height.
 * @return Pointer to the underlying eui_widget_t, or NULL on failure.
 */
eui_widget_t* eui_slider_create(int16_t x, int16_t y, uint16_t w, uint16_t h);

/**
 * @brief Set the slider's minimum and maximum value range.
 *
 * @param slider  Pointer to the slider widget.
 * @param min     Minimum value.
 * @param max     Maximum value.
 */
void eui_slider_set_range(eui_widget_t *slider, int16_t min, int16_t max);

/**
 * @brief Set the slider's current value.
 *
 * The value is clamped to the configured range.
 *
 * @param slider  Pointer to the slider widget.
 * @param value   New value.
 */
void eui_slider_set_value(eui_widget_t *slider, int16_t value);

/**
 * @brief Get the slider's current value.
 *
 * @param slider  Pointer to the slider widget.
 * @return The current slider value.
 */
int16_t eui_slider_get_value(const eui_widget_t *slider);

#endif /* EUI_WIDGET_SLIDER_H */
