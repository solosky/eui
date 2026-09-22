#include "eui/eui_input_edge.h"
#include <string.h>

void eui_input_edge_init(eui_input_edge_t *in)
{
    memset(in, 0, sizeof(*in));
}

static eui_input_edge_key_t *key_slot(eui_input_edge_t *in, uint8_t id)
{
    return (id < EUI_KEY_ID_MAX) ? &in->key[id] : NULL;
}

void eui_input_edge_on_event(eui_input_edge_t *in, const eui_event_t *raw)
{
    switch (raw->type) {
    case EUI_EVT_KEY_PRESS:
    case EUI_EVT_KEY_RELEASE: {
        eui_input_edge_key_t *k = key_slot(in, raw->data.key_id);
        if (k == NULL) return;
        bool emit_click = false;
        if (raw->type == EUI_EVT_KEY_PRESS) {
            if (!k->pressed) {
                k->pressed = true;
                k->armed = true;
                k->hold_fired = false;
                k->press_ms = raw->timestamp;
                k->press_edge = true;
            }
        } else if (k->pressed) {
            k->pressed = false;
            k->release_edge = true;
            if (k->armed) {   /* release-before-hold → click */
                k->armed = false;
                k->click_edge = true;
                emit_click = true;
            }
        }
        eui_event_queue_push(&in->queue, raw);   /* PRESS/RELEASE 透传 */
        if (emit_click) {
            /* CLICK 在 RELEASE 之后入队：pop 顺序保持时序 PRESS→RELEASE→CLICK */
            eui_event_t out = *raw;
            out.type = EUI_EVT_KEY_CLICK;
            eui_event_queue_push(&in->queue, &out);
        }
        break;
    }
    case EUI_EVT_ENCODER_CW:
    case EUI_EVT_ENCODER_CCW: {
        /* 契约：CW 带正 delta；CCW 带负 delta（>=0 的 CCW 非法，忽略） */
        if (raw->type == EUI_EVT_ENCODER_CCW && raw->data.enc_delta >= 0) return;
        eui_event_t out = *raw;
        out.type = EUI_EVT_ENC_STEP;
        eui_event_queue_push(&in->queue, &out);
        break;
    }
    default:
        /* ENC_CLICK / TOUCH_* / KEY_REPEAT：透传（现状无 view 消费 REPEAT） */
        eui_event_queue_push(&in->queue, raw);
        break;
    }
}

void eui_input_edge_tick(eui_input_edge_t *in, uint32_t now_ms)
{
    for (int i = 0; i < EUI_KEY_ID_MAX; i++) {
        eui_input_edge_key_t *k = &in->key[i];
        if (k->pressed && !k->hold_fired &&
            now_ms - k->press_ms >= EUI_INPUT_EDGE_DEFAULT_HOLD_MS) {
            k->hold_fired = true;
            k->armed = false;          /* hold 吞掉本次 click */
            k->press_edge = false;     /* hold 吞掉未消费的 press（Button_Class） */
            k->hold_edge = true;
            eui_event_t out = { .type = EUI_EVT_KEY_HOLD,
                                .data = { .key_id = (uint8_t)i },
                                .timestamp = now_ms };
            eui_event_queue_push(&in->queue, &out);
        }
    }
}

bool eui_input_edge_pop_event(eui_input_edge_t *in, eui_event_t *out)
{
    return eui_event_queue_pop(&in->queue, out);
}

void eui_input_edge_flush(eui_input_edge_t *in)
{
    eui_event_t sink;
    while (eui_event_queue_pop(&in->queue, &sink)) {}
    for (int i = 0; i < EUI_KEY_ID_MAX; i++) {
        eui_input_edge_key_t *k = &in->key[i];
        k->press_edge = k->release_edge = k->click_edge = k->hold_edge = false;
        if (!k->pressed) {   /* 按住键的电平与 hold 计时保留（Button_Class） */
            k->armed = true;
            k->hold_fired = false;
            k->press_ms = 0;
        }
    }
}

bool eui_input_edge_was_pressed(eui_input_edge_t *in, uint8_t id)
{
    if (id >= EUI_KEY_ID_MAX) return false;
    bool e = in->key[id].press_edge; in->key[id].press_edge = false; return e;
}

bool eui_input_edge_was_released(eui_input_edge_t *in, uint8_t id)
{
    if (id >= EUI_KEY_ID_MAX) return false;
    bool e = in->key[id].release_edge; in->key[id].release_edge = false; return e;
}

bool eui_input_edge_was_clicked(eui_input_edge_t *in, uint8_t id)
{
    if (id >= EUI_KEY_ID_MAX) return false;
    bool e = in->key[id].click_edge; in->key[id].click_edge = false; return e;
}

bool eui_input_edge_was_hold(eui_input_edge_t *in, uint8_t id)
{
    if (id >= EUI_KEY_ID_MAX) return false;
    bool e = in->key[id].hold_edge; in->key[id].hold_edge = false; return e;
}

bool eui_input_edge_is_released(const eui_input_edge_t *in, uint8_t id)
{
    return id >= EUI_KEY_ID_MAX || !in->key[id].pressed;
}
