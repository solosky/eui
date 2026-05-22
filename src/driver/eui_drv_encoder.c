#include "eui/driver/eui_drv_encoder.h"
#include "eui/eui_allocator.h"
#include <stdbool.h>
#include <string.h>

typedef struct {
    eui_input_hal_t        base;
    eui_hal_gpio_t gpio;
    uint8_t                pin_a;
    uint8_t                pin_b;
    uint8_t                pin_sw;
    uint8_t                prev_state;
    bool                   btn_pressed;
} encoder_t;

static int encoder_init(void *ud) {
    encoder_t *e = (encoder_t*)ud;
    e->prev_state = 0;
    e->btn_pressed = false;
    return 0;
}

static int encoder_deinit(void *ud) { (void)ud; return 0; }

static int encoder_poll(eui_event_t *evt, void *ud) {
    encoder_t *e = (encoder_t*)ud;

    uint8_t a = e->gpio.read_pin(e->pin_a, e->gpio.user_data) ? 1 : 0;
    uint8_t b = e->gpio.read_pin(e->pin_b, e->gpio.user_data) ? 1 : 0;
    uint8_t curr = (a << 1) | b;

    static const int8_t enc_table[4][4] = {
        {  0,  1, -1,  0 },
        { -1,  0,  0,  1 },
        {  1,  0,  0, -1 },
        {  0, -1,  1,  0 },
    };

    int8_t dir = enc_table[e->prev_state & 0x03][curr];
    e->prev_state = (e->prev_state & 0xFC) | curr;

    if (dir == 1) {
        evt->type = EUI_EVT_ENCODER_CW;
        return 1;
    }
    if (dir == -1) {
        evt->type = EUI_EVT_ENCODER_CCW;
        return 1;
    }

    bool sw = e->gpio.read_pin(e->pin_sw, e->gpio.user_data);
    if (sw && !e->btn_pressed) {
        e->btn_pressed = true;
        evt->type = EUI_EVT_ENCODER_CLICK;
        return 1;
    }
    if (!sw && e->btn_pressed) {
        e->btn_pressed = false;
    }

    return 0;
}

static void encoder_set_callback(void (*cb)(const eui_event_t *evt), void *user_data) {
    (void)cb;
    (void)user_data;
}

eui_input_hal_t* eui_drv_encoder_create(const eui_drv_encoder_config_t *cfg) {
    encoder_t *e = eui_malloc(sizeof(encoder_t));
    if (!e) return NULL;
    memset(e, 0, sizeof(*e));
    e->gpio = cfg->gpio;
    e->pin_a = cfg->pin_a;
    e->pin_b = cfg->pin_b;
    e->pin_sw = cfg->pin_sw;
    e->base.init = encoder_init;
    e->base.deinit = encoder_deinit;
    e->base.poll = encoder_poll;
    e->base.set_callback = encoder_set_callback;
    e->base.user_data = e;
    return &e->base;
}

void eui_drv_encoder_destroy(eui_input_hal_t *hal) {
    if (hal) eui_free(hal->user_data);
}
