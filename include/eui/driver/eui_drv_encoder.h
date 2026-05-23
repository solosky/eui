#ifndef EUI_DRV_ENCODER_H
#define EUI_DRV_ENCODER_H

#include <stdbool.h>
#include "eui/eui_input_drv.h"
#include "eui/hal/eui_hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    eui_hal_gpio_t gpio;
    uint8_t        pin_a;
    uint8_t        pin_b;
    uint8_t        pin_sw;
    uint32_t       poll_interval_us;
} eui_drv_encoder_config_t;

eui_input_drv_t* eui_drv_encoder_create(const eui_drv_encoder_config_t *cfg);
void eui_drv_encoder_destroy(eui_input_drv_t *hal);

#ifdef __cplusplus
}
#endif

#endif /* EUI_DRV_ENCODER_H */
