#ifndef EUI_DRV_WEB_H
#define EUI_DRV_WEB_H

#include "eui/eui_display_drv.h"
#include "eui/eui_input_drv.h"

#ifdef __cplusplus
extern "C" {
#endif

eui_display_drv_t *eui_drv_web_create_display(uint16_t width, uint16_t height,
                                               uint8_t color_depth);
void eui_drv_web_destroy_display(eui_display_drv_t *drv);

eui_input_drv_t *eui_drv_web_create_input(void);
void eui_drv_web_destroy_input(eui_input_drv_t *drv);

#ifdef __cplusplus
}
#endif

#endif
