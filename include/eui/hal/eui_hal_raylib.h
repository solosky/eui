#ifndef EUI_HAL_RAYLIB_H
#define EUI_HAL_RAYLIB_H

#include "eui/eui_display_hal.h"
#include "eui/eui_input_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

eui_display_hal_t* eui_hal_raylib_create_display(uint16_t width, uint16_t height,
                                                  uint8_t color_depth);
void eui_hal_raylib_destroy_display(eui_display_hal_t *hal);
void eui_hal_raylib_refresh(void);

eui_input_hal_t* eui_hal_raylib_create_input(void);
void eui_hal_raylib_destroy_input(eui_input_hal_t *hal);

int eui_hal_raylib_window_should_close(void);

void eui_hal_raylib_save_screenshot(const char *filename);

#ifdef __cplusplus
}
#endif

#endif /* EUI_HAL_RAYLIB_H */
