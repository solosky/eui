#ifndef EUI_DISPLAY_DRV_H
#define EUI_DISPLAY_DRV_H

#include "eui_types.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Capabilities descriptor for a display device.
 */
typedef struct {
    uint16_t width;        /**< Display width in pixels. */
    uint16_t height;       /**< Display height in pixels. */
    uint8_t  color_depth;  /**< Bits per pixel (1, 4, 8, or 16). */
    uint8_t  buffer_mode;  /**< Preferred buffer strategy (eui_buffer_mode_t). */
    bool     has_gram;     /**< True if the display has its own GRAM (frame buffer). */
    bool     hw_scroll;    /**< True if hardware scrolling is supported. */
} eui_display_caps_t;

/**
 * @brief Display HAL interface (abstracted driver).
 *
 * The application implements these callbacks to drive the physical
 * display hardware.  All coordinates are in the display's native
 * orientation.
 */
typedef struct eui_display_drv_t {
    eui_display_caps_t caps; /**< Display capabilities (read-only after init). */

    /**
     * @brief Initialize the display hardware.
     * @param user_data  The eui_display_drv_t::user_data pointer.
     * @return 0 on success, negative on error.
     */
    int  (*init)(void *user_data);

    /**
     * @brief Deinitialize the display hardware.
     * @param user_data  The eui_display_drv_t::user_data pointer.
     * @return 0 on success, negative on error.
     */
    int  (*deinit)(void *user_data);

    /**
     * @brief Set a single pixel to the given color.
     * @param x         X-coordinate.
     * @param y         Y-coordinate.
     * @param color     Pixel color in native format.
     * @param user_data The eui_display_drv_t::user_data pointer.
     */
    void (*draw_pixel)(int16_t x, int16_t y, eui_color_t color, void *user_data);

    /**
     * @brief Write a full rectangular region from a pixel buffer.
     *
     * The buffer layout matches the display's color depth and is
     * row-major, top-to-bottom.
     *
     * @param buffer    Source pixel data.
     * @param rect      Target rectangle on the display.
     * @param user_data The eui_display_drv_t::user_data pointer.
     */
    void (*write_buffer)(const uint8_t *buffer, const eui_rect_t *rect, void *user_data);

    /**
     * @brief Set the display contrast.
     * @param level     Contrast level (0-255).
     * @param user_data The eui_display_drv_t::user_data pointer.
     */
    void (*set_contrast)(uint8_t level, void *user_data);

    /**
     * @brief Turn the display power on or off.
     * @param on        true to enable, false to disable.
     * @param user_data The eui_display_drv_t::user_data pointer.
     */
    void (*set_power)(bool on, void *user_data);

    /**
     * @brief Invert all pixels on the display.
     * @param invert    true to invert, false for normal.
     * @param user_data The eui_display_drv_t::user_data pointer.
     */
    void (*set_invert)(bool invert, void *user_data);

    /**
     * @brief Fill a rectangle with a solid color (hardware-accelerated).
     * @param x         Left edge.
     * @param y         Top edge.
     * @param w         Width.
     * @param h         Height.
     * @param color     Fill color.
     * @param user_data The eui_display_drv_t::user_data pointer.
     */
    void (*fill_rect)(int16_t x, int16_t y, uint16_t w, uint16_t h,
                      eui_color_t color, void *user_data);

    void *user_data; /**< Application-defined pointer passed to all callbacks. */
} eui_display_drv_t;

#endif /* EUI_DISPLAY_DRV_H */
