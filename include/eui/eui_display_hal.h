#ifndef EUI_DISPLAY_HAL_H
#define EUI_DISPLAY_HAL_H

#include "eui_types.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t width;
    uint16_t height;
    uint8_t  color_depth;
    uint8_t  buffer_mode;
    bool     has_gram;
    bool     hw_scroll;
} eui_display_caps_t;

typedef struct eui_display_hal_t {
    eui_display_caps_t caps;

    int  (*init)(void *user_data);
    int  (*deinit)(void *user_data);
    void (*draw_pixel)(int16_t x, int16_t y, eui_color_t color, void *user_data);
    void (*write_buffer)(const uint8_t *buffer, const eui_rect_t *rect, void *user_data);
    void (*set_contrast)(uint8_t level, void *user_data);
    void (*set_power)(bool on, void *user_data);
    void (*set_invert)(bool invert, void *user_data);
    void (*fill_rect)(int16_t x, int16_t y, uint16_t w, uint16_t h,
                      eui_color_t color, void *user_data);

    void *user_data;
} eui_display_hal_t;

#endif /* EUI_DISPLAY_HAL_H */
