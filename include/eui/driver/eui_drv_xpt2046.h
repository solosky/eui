#ifndef EUI_DRV_XPT2046_H
#define EUI_DRV_XPT2046_H

#include "eui/eui_input_hal.h"
#include "eui/hal/eui_hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool (*read_irq)(void *user_data);
    void *user_data;
} eui_drv_xpt2046_irq_t;

typedef struct {
    eui_hal_spi_t         spi;
    eui_drv_xpt2046_irq_t irq;
    uint16_t              width;
    uint16_t              height;
} eui_drv_xpt2046_config_t;

eui_input_hal_t* eui_drv_xpt2046_create(const eui_drv_xpt2046_config_t *cfg);
void eui_drv_xpt2046_destroy(eui_input_hal_t *hal);

#ifdef __cplusplus
}
#endif

#endif /* EUI_DRV_XPT2046_H */
