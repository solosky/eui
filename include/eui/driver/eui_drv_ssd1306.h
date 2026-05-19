#ifndef EUI_DRV_SSD1306_H
#define EUI_DRV_SSD1306_H

#include "eui/eui_display_hal.h"
#include "eui/hal/eui_hal_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    eui_hal_i2c_t i2c;
    uint16_t      width;
    uint16_t      height;
    uint8_t       i2c_addr;
} eui_drv_ssd1306_config_t;

eui_display_hal_t* eui_drv_ssd1306_create(const eui_drv_ssd1306_config_t *cfg);
void eui_drv_ssd1306_destroy(eui_display_hal_t *hal);

#ifdef __cplusplus
}
#endif

#endif /* EUI_DRV_SSD1306_H */
