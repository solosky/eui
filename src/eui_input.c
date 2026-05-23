#include "eui/eui_input.h"
#include <string.h>

static void process_raw_event(eui_input_manager_t *mgr, const eui_event_t *raw, uint32_t now_ms)
{
    switch (raw->type) {
    case EUI_EVT_KEY_PRESS:
    case EUI_EVT_KEY_RELEASE: {
        int key_index = raw->data.key;
        if (key_index < 0 || key_index >= EUI_KEY_COUNT) return;

        if (raw->type == EUI_EVT_KEY_PRESS) {
            if ((now_ms - mgr->key_state[key_index].last_change_ms) < mgr->debounce_ms) {
                return; /* bounce */
            }
            mgr->key_state[key_index].pressed = true;
            mgr->key_state[key_index].last_change_ms = now_ms;
            eui_event_queue_push(&mgr->queue, raw);
            mgr->key_state[key_index].long_press_fired = false;
            mgr->key_state[key_index].next_repeat_ms = now_ms + mgr->long_press_ms;
        } else {
            if (mgr->key_state[key_index].pressed) {
                eui_event_queue_push(&mgr->queue, raw);
                mgr->key_state[key_index].pressed = false;
            }
        }
        break;
    }
    case EUI_EVT_ENCODER_CW:
    case EUI_EVT_ENCODER_CCW:
    case EUI_EVT_ENCODER_CLICK:
    case EUI_EVT_TOUCH_DOWN:
    case EUI_EVT_TOUCH_UP:
    case EUI_EVT_TOUCH_MOVE:
        eui_event_queue_push(&mgr->queue, raw);
        break;
    case EUI_EVT_KEY_REPEAT:
    default:
        break;
    }
}

void eui_input_init(eui_input_manager_t *mgr, eui_input_drv_t *hal)
{
    memset(mgr, 0, sizeof(*mgr));
    mgr->hal = hal;
    mgr->debounce_ms = 20;
    mgr->long_press_ms = 500;
    mgr->repeat_interval_ms = 50;
    eui_event_queue_init(&mgr->queue);

    /* Pre-bias last_change_ms so first press passes debounce check */
    for (int i = 0; i < EUI_KEY_COUNT; i++) {
        mgr->key_state[i].last_change_ms = 0xFFFFFFFFu - mgr->debounce_ms;
    }
}

void eui_input_set_debounce(eui_input_manager_t *mgr, uint32_t ms)
{
    mgr->debounce_ms = ms;
}

void eui_input_set_long_press(eui_input_manager_t *mgr, uint32_t ms)
{
    mgr->long_press_ms = ms;
}

void eui_input_set_repeat_interval(eui_input_manager_t *mgr, uint32_t ms)
{
    mgr->repeat_interval_ms = ms;
}

void eui_input_update(eui_input_manager_t *mgr, uint32_t now_ms)
{
    eui_event_t raw;

    while (mgr->hal->poll(&raw, mgr->hal->user_data) > 0) {
        process_raw_event(mgr, &raw, now_ms);
    }

    /* Check for long-press / repeat */
    for (int i = 0; i < EUI_KEY_COUNT; i++) {
        if (!mgr->key_state[i].pressed) continue;

        if (!mgr->key_state[i].long_press_fired && now_ms >= mgr->key_state[i].next_repeat_ms) {
            eui_event_t rep = { .type = EUI_EVT_KEY_REPEAT, .data = { .key = (eui_key_t)i }, .timestamp = now_ms };
            eui_event_queue_push(&mgr->queue, &rep);
            mgr->key_state[i].long_press_fired = true;
            mgr->key_state[i].next_repeat_ms = now_ms + mgr->repeat_interval_ms;
        } else if (mgr->key_state[i].long_press_fired && now_ms >= mgr->key_state[i].next_repeat_ms) {
            eui_event_t rep = { .type = EUI_EVT_KEY_REPEAT, .data = { .key = (eui_key_t)i }, .timestamp = now_ms };
            eui_event_queue_push(&mgr->queue, &rep);
            mgr->key_state[i].next_repeat_ms = now_ms + mgr->repeat_interval_ms;
        }
    }

    mgr->last_poll_ms = now_ms;
}

bool eui_input_get_event(eui_input_manager_t *mgr, eui_event_t *event)
{
    return eui_event_queue_pop(&mgr->queue, event);
}
