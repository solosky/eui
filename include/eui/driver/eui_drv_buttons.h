#ifndef EUI_DRV_BUTTONS_H
#define EUI_DRV_BUTTONS_H

#include <stdbool.h>
#include "eui/eui_input_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool (*read_pin)(uint8_t pin_id, void *user_data);
    void (*delay_us)(uint32_t us, void *user_data);
    void *user_data;
} eui_drv_buttons_gpio_t;

typedef struct {
    uint8_t   pin_id;
    eui_key_t key;
} eui_drv_button_map_t;

typedef struct {
    eui_drv_buttons_gpio_t     gpio;
    const eui_drv_button_map_t *map;
    uint8_t                     count;
} eui_drv_buttons_config_t;

eui_input_hal_t* eui_drv_buttons_create(const eui_drv_buttons_config_t *cfg);
void eui_drv_buttons_destroy(eui_input_hal_t *hal);

#ifdef __cplusplus
}
#endif

#endif /* EUI_DRV_BUTTONS_H */
