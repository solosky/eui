#ifndef EUI_INPUT_H
#define EUI_INPUT_H

#include "eui_input_hal.h"
#include "eui_event.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    eui_input_hal_t *hal;
    eui_event_queue_t queue;
    uint32_t last_poll_ms;
    uint32_t debounce_ms;
    uint32_t long_press_ms;
    uint32_t repeat_interval_ms;
    /* Debounce tracking for each key */
    struct {
        uint32_t last_change_ms;
        bool     pressed;
        bool     long_press_fired;
        uint32_t next_repeat_ms;
    } key_state[EUI_KEY_COUNT];
} eui_input_manager_t;

void eui_input_init(eui_input_manager_t *mgr, eui_input_hal_t *hal);
void eui_input_set_debounce(eui_input_manager_t *mgr, uint32_t ms);
void eui_input_set_long_press(eui_input_manager_t *mgr, uint32_t ms);
void eui_input_set_repeat_interval(eui_input_manager_t *mgr, uint32_t ms);
void eui_input_update(eui_input_manager_t *mgr, uint32_t now_ms);
bool eui_input_get_event(eui_input_manager_t *mgr, eui_event_t *event);

#endif /* EUI_INPUT_H */
