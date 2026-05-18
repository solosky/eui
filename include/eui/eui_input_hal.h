#ifndef EUI_INPUT_HAL_H
#define EUI_INPUT_HAL_H

#include <stdint.h>

typedef enum {
    EUI_EVT_KEY_PRESS,
    EUI_EVT_KEY_RELEASE,
    EUI_EVT_KEY_REPEAT,
    EUI_EVT_ENCODER_CW,
    EUI_EVT_ENCODER_CCW,
    EUI_EVT_ENCODER_CLICK,
    EUI_EVT_TOUCH_DOWN,
    EUI_EVT_TOUCH_UP,
    EUI_EVT_TOUCH_MOVE,
} eui_event_type_t;

typedef enum {
    EUI_KEY_UP = 0,
    EUI_KEY_DOWN,
    EUI_KEY_LEFT,
    EUI_KEY_RIGHT,
    EUI_KEY_OK,
    EUI_KEY_BACK,
    EUI_KEY_COUNT
} eui_key_t;

typedef struct {
    eui_event_type_t type;
    union {
        eui_key_t key;
        int16_t   enc_delta;
        struct { int16_t x, y; } touch;
    } data;
    uint32_t timestamp;
} eui_event_t;

typedef struct eui_input_hal_t {
    int  (*init)(void *user_data);
    int  (*deinit)(void *user_data);
    int  (*poll)(eui_event_t *event, void *user_data);
    void (*set_callback)(void (*cb)(const eui_event_t *evt), void *user_data);
    void *user_data;
} eui_input_hal_t;

#endif /* EUI_INPUT_HAL_H */
