#include "eui/driver/eui_drv_buttons.h"
#include "eui/eui_allocator.h"
#include <string.h>

typedef struct {
    eui_input_hal_t         base;
    eui_hal_gpio_t                gpio;
    const eui_drv_buttons_map_t  *map;
    uint8_t                 count;
    uint8_t                 prev_state;
} buttons_t;

static int buttons_init(void *ud) {
    buttons_t *b = (buttons_t*)ud;
    b->prev_state = 0;
    return 0;
}

static int buttons_deinit(void *ud) { (void)ud; return 0; }

static int buttons_poll(eui_event_t *evt, void *ud) {
    buttons_t *b = (buttons_t*)ud;
    uint8_t curr = 0;

    for (int i = 0; i < b->count; i++) {
        if (b->gpio.read_pin(b->map[i].pin_id, b->gpio.user_data)) {
            curr |= (1u << i);
        }
    }

    for (int i = 0; i < b->count; i++) {
        uint8_t mask = (1u << i);
        bool was_pressed = (b->prev_state & mask) != 0;
        bool is_pressed  = (curr & mask) != 0;
        if (is_pressed && !was_pressed) {
            evt->type = EUI_EVT_KEY_PRESS;
            evt->data.key = b->map[i].key;
            b->prev_state = curr;
            return 1;
        }
        if (!is_pressed && was_pressed) {
            evt->type = EUI_EVT_KEY_RELEASE;
            evt->data.key = b->map[i].key;
            b->prev_state = curr;
            return 1;
        }
    }
    b->prev_state = curr;
    return 0;
}

static void buttons_set_callback(void (*cb)(const eui_event_t *evt), void *user_data) {
    (void)cb;
    (void)user_data;
}

eui_input_hal_t* eui_drv_buttons_create(const eui_drv_buttons_config_t *cfg) {
    buttons_t *b = eui_malloc(sizeof(buttons_t));
    if (!b) return NULL;
    memset(b, 0, sizeof(*b));
    b->gpio = cfg->gpio;
    b->map = cfg->map;
    b->count = cfg->count;
    b->base.init = buttons_init;
    b->base.deinit = buttons_deinit;
    b->base.poll = buttons_poll;
    b->base.set_callback = buttons_set_callback;
    b->base.user_data = b;
    return &b->base;
}

void eui_drv_buttons_destroy(eui_input_hal_t *hal) {
    if (hal) eui_free(hal->user_data);
}
