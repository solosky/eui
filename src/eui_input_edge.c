#include "eui/eui_input_edge.h"
#include <string.h>

void eui_input_edge_init(eui_input_edge_t *in)
{
    memset(in, 0, sizeof(*in));
    in->hold_ms = EUI_INPUT_EDGE_DEFAULT_HOLD_MS;
}

void eui_input_edge_on_event(eui_input_edge_t *in, const eui_event_t *evt)
{
    switch (evt->type) {
    case EUI_EVT_ENCODER_CW:
        in->encoder_count += evt->data.enc_delta > 0 ? evt->data.enc_delta : 1;
        break;
    case EUI_EVT_ENCODER_CCW:
        /* 契约：CCW 事件携带负 enc_delta（raylib 驱动 CCW 时 wheel<0）。
         * delta >= 0 的 CCW 属非法事件，忽略之。 */
        if (evt->data.enc_delta < 0)
            in->encoder_count -= -evt->data.enc_delta;
        break;
    case EUI_EVT_KEY_PRESS:
        if (evt->data.key == EUI_KEY_RIGHT) in->encoder_count++;
        else if (evt->data.key == EUI_KEY_LEFT) in->encoder_count--;
        /* 扩展：方向键折算编码器，UP/RIGHT 同向，DOWN/LEFT 同向 */
        else if (evt->data.key == EUI_KEY_UP) in->encoder_count++;
        else if (evt->data.key == EUI_KEY_DOWN) in->encoder_count--;
        else if (evt->data.key == EUI_KEY_OK) {
            if (!in->ok_pressed) {
                in->ok_pressed = true;
                in->ok_press_edge = true;
                /* 扩展：OK 侧 hold 计时，与侧键同构 */
                in->ok_press_ms = evt->timestamp;
                in->ok_hold_fired = false;
            }
        } else if (evt->data.key == EUI_KEY_BACK) {
            if (!in->side_pressed) {
                in->side_pressed = true;
                in->side_press_ms = evt->timestamp;
                in->side_hold_fired = false;
            }
        }
        break;
    case EUI_EVT_KEY_RELEASE:
        if (evt->data.key == EUI_KEY_OK) {
            if (in->ok_pressed) { in->ok_pressed = false; in->ok_release_edge = true; }
        } else if (evt->data.key == EUI_KEY_BACK) {
            in->side_pressed = false;
        }
        break;
    default:
        break;
    }
}

void eui_input_edge_tick(eui_input_edge_t *in, uint32_t now_ms)
{
    /* hold 触发时吞掉尚未消费的 press 边沿，保证同一次按压 hold 与
     * 短按互斥（侧键侧等价抑制见 release-before-hold 的 click 转换）。 */
    if (in->ok_pressed && !in->ok_hold_fired &&
        now_ms - in->ok_press_ms >= in->hold_ms) {
        in->ok_hold_fired = true;
        in->ok_hold_edge = true;
        in->ok_press_edge = false;
    }
    if (in->side_pressed && !in->side_hold_fired &&
        now_ms - in->side_press_ms >= in->hold_ms) {
        in->side_hold_fired = true;
        in->side_hold_edge = true;
    }
    /* release before hold threshold => click */
    if (!in->side_pressed && in->side_press_ms != 0 && !in->side_hold_fired) {
        in->side_press_ms = 0;
        in->side_click_edge = true;
    }
}

int eui_input_edge_encoder_count(const eui_input_edge_t *in) { return in->encoder_count; }

void eui_input_edge_reset_edges(eui_input_edge_t *in)
{
    in->ok_press_edge = false;
    in->ok_release_edge = false;
    in->ok_hold_edge = false;
    in->side_click_edge = false;
    in->side_hold_edge = false;
    /* 未决的"已释放待转 click"状态一并清除，避免 reset 后首个 tick
     * 重新推导出 click 边沿（幽灵输入）。仍在按住的键保留按下状态与
     * hold 计时，与 Button_Class 持续跟踪原始状态一致。 */
    if (!in->ok_pressed) {
        in->ok_press_ms = 0;
        in->ok_hold_fired = false;
    }
    if (!in->side_pressed) {
        in->side_press_ms = 0;
        in->side_hold_fired = false;
    }
}

bool eui_input_edge_ok_was_pressed(eui_input_edge_t *in)
{
    bool e = in->ok_press_edge; in->ok_press_edge = false; return e;
}
bool eui_input_edge_ok_was_released(eui_input_edge_t *in)
{
    bool e = in->ok_release_edge; in->ok_release_edge = false; return e;
}
bool eui_input_edge_ok_is_released(const eui_input_edge_t *in) { return !in->ok_pressed; }
bool eui_input_edge_ok_was_hold(eui_input_edge_t *in)
{
    bool e = in->ok_hold_edge; in->ok_hold_edge = false; return e;
}
bool eui_input_edge_side_was_clicked(eui_input_edge_t *in)
{
    bool e = in->side_click_edge; in->side_click_edge = false; return e;
}
bool eui_input_edge_side_was_hold(eui_input_edge_t *in)
{
    bool e = in->side_hold_edge; in->side_hold_edge = false; return e;
}
