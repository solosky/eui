#ifndef EUI_DRV_ST7735_H
#define EUI_DRV_ST7735_H

#include "eui/eui_display_drv.h"
#include "eui/hal/eui_hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    eui_hal_spi_t spi;
    uint16_t      width;
    uint16_t      height;
    uint8_t       variant;
} eui_drv_st7735_config_t;

eui_display_drv_t* eui_drv_st7735_create(const eui_drv_st7735_config_t *cfg);
void eui_drv_st7735_destroy(eui_display_drv_t *hal);

#ifdef __cplusplus
}
#endif

#endif /* EUI_DRV_ST7735_H */
