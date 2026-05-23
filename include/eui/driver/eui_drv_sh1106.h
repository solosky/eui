#ifndef EUI_DRV_SH1106_H
#define EUI_DRV_SH1106_H

#include "eui/eui_display_drv.h"
#include "eui/hal/eui_hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    eui_hal_i2c_t i2c;
    uint16_t      width;
    uint16_t      height;
    uint8_t       i2c_addr;
} eui_drv_sh1106_config_t;

eui_display_drv_t* eui_drv_sh1106_create(const eui_drv_sh1106_config_t *cfg);
void eui_drv_sh1106_destroy(eui_display_drv_t *hal);

#ifdef __cplusplus
}
#endif

#endif /* EUI_DRV_SH1106_H */
