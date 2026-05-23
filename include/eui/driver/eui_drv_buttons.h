#ifndef EUI_DRV_BUTTONS_H
#define EUI_DRV_BUTTONS_H

#include <stdbool.h>
#include "eui/eui_input_drv.h"
#include "eui/hal/eui_hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t   pin_id;
    eui_key_t key;
} eui_drv_buttons_map_t;

typedef struct {
    eui_hal_gpio_t                gpio;
    const eui_drv_buttons_map_t  *map;
    uint8_t                        count;
} eui_drv_buttons_config_t;

eui_input_drv_t* eui_drv_buttons_create(const eui_drv_buttons_config_t *cfg);
void eui_drv_buttons_destroy(eui_input_drv_t *hal);

#ifdef __cplusplus
}
#endif

#endif /* EUI_DRV_BUTTONS_H */
