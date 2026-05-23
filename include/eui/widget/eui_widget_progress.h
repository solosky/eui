#ifndef EUI_WIDGET_PROGRESS_H
#define EUI_WIDGET_PROGRESS_H

#include "eui/widget/eui_widget.h"

/**
 * @brief Progress bar widget.
 *
 * Displays a horizontal progress bar indicating completion percentage.
 * Supports both determinate (0-100%) and indeterminate (animated) modes.
 */
typedef struct {
    eui_widget_t widget;
    uint8_t value;          /**< Current progress value (0-100). */
    bool indeterminate;     /**< true for indeterminate (animated) mode. */
} eui_progress_t;

/**
 * @brief Create a progress bar widget.
 *
 * @param x  Left edge.
 * @param y  Top edge.
 * @param w  Width.
 * @param h  Height.
 * @return Pointer to the underlying eui_widget_t, or NULL on failure.
 */
eui_widget_t* eui_progress_create(int16_t x, int16_t y, uint16_t w, uint16_t h);

/**
 * @brief Set the progress value (0-100 percent).
 *
 * Has no effect when the bar is in indeterminate mode.
 *
 * @param prog    Pointer to the progress bar widget.
 * @param percent Percentage value (clamped to 0-100).
 */
void eui_progress_set_value(eui_widget_t *prog, uint8_t percent);

/**
 * @brief Enable or disable indeterminate (animated) mode.
 *
 * In indeterminate mode the bar shows a sweeping animation instead of
 * a fixed fill level, suitable for operations with unknown duration.
 *
 * @param prog  Pointer to the progress bar widget.
 * @param indet true to enable indeterminate mode, false for determinate.
 */
void eui_progress_set_indeterminate(eui_widget_t *prog, bool indet);

#endif /* EUI_WIDGET_PROGRESS_H */
