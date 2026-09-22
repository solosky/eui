/*
 * test_input_edge.c — eui_input_edge（编码器 + 双键边沿锁存状态机）。
 * 用例移植自 VAMeter app-eui 的 test_app_input.c。
 */
#include <assert.h>
#include <stdio.h>
#include "eui/eui_input_edge.h"
#include "common/eui_test.h"

static void key(eui_input_edge_t *in, eui_event_type_t t, eui_key_t k)
{
    eui_event_t e = {.type = t, .timestamp = 0};
    e.data.key = k;
    eui_input_edge_on_event(in, &e);
}

static void test_encoder_and_arrow_keys(void)
{
    TEST("input edge: encoder + arrow keys both drive the virtual counter");
    eui_input_edge_t in;
    eui_input_edge_init(&in);

    eui_event_t e = {.type = EUI_EVT_ENCODER_CW, .data.enc_delta = 1, .timestamp = 0};
    eui_input_edge_on_event(&in, &e);
    key(&in, EUI_EVT_KEY_PRESS, EUI_KEY_RIGHT);
    key(&in, EUI_EVT_KEY_RELEASE, EUI_KEY_RIGHT);
    key(&in, EUI_EVT_KEY_PRESS, EUI_KEY_LEFT);
    key(&in, EUI_EVT_KEY_RELEASE, EUI_KEY_LEFT);
    e.type = EUI_EVT_ENCODER_CCW;
    eui_input_edge_on_event(&in, &e);
    assert(eui_input_edge_encoder_count(&in) == 1); /* +1 +1 -1 -1 +1 */

    /* 非法 CCW（delta >= 0）被忽略 */
    e.data.enc_delta = 1;
    eui_input_edge_on_event(&in, &e);
    assert(eui_input_edge_encoder_count(&in) == 1);
    PASS();
}

static void test_ok_press_release_edges(void)
{
    TEST("input edge: OK press/release edges are one-shot latches");
    eui_input_edge_t in;
    eui_input_edge_init(&in);
    key(&in, EUI_EVT_KEY_PRESS, EUI_KEY_OK);
    key(&in, EUI_EVT_KEY_RELEASE, EUI_KEY_OK);
    assert(eui_input_edge_ok_was_pressed(&in));
    assert(eui_input_edge_ok_was_released(&in));
    assert(eui_input_edge_ok_is_released(&in));
    assert(!eui_input_edge_ok_was_pressed(&in)); /* cleared after read */
    PASS();
}

static void test_side_click_and_hold(void)
{
    TEST("input edge: side click vs hold are mutually exclusive");
    eui_input_edge_t in;
    eui_input_edge_init(&in);

    /* side click: press + release within 500ms */
    eui_event_t p = {.type = EUI_EVT_KEY_PRESS, .timestamp = 1000};
    p.data.key = EUI_KEY_BACK;
    eui_event_t r = {.type = EUI_EVT_KEY_RELEASE, .timestamp = 1200};
    r.data.key = EUI_KEY_BACK;
    eui_input_edge_on_event(&in, &p);
    eui_input_edge_on_event(&in, &r);
    eui_input_edge_tick(&in, 1200);
    assert(eui_input_edge_side_was_clicked(&in));
    assert(!eui_input_edge_side_was_hold(&in));

    /* side hold: still pressed at >= 500ms；hold 后释放不再是 click */
    p.timestamp = 2000; r.timestamp = 2600;
    eui_input_edge_on_event(&in, &p);
    eui_input_edge_tick(&in, 2510);
    assert(eui_input_edge_side_was_hold(&in));   /* fired while held */
    eui_input_edge_on_event(&in, &r);
    eui_input_edge_tick(&in, 2600);
    assert(!eui_input_edge_side_was_clicked(&in));
    PASS();
}

static void test_ok_hold_and_reset_edges(void)
{
    TEST("input edge: OK hold fires at threshold; reset_edges clears ghost edges");
    eui_input_edge_t in;
    eui_input_edge_init(&in);

    eui_input_edge_on_event(&in, &(eui_event_t){.type = EUI_EVT_KEY_PRESS,
                                                .data.key = EUI_KEY_OK, .timestamp = 1000});
    eui_input_edge_tick(&in, 1499);
    assert(!eui_input_edge_ok_was_hold(&in));
    eui_input_edge_tick(&in, 1500);
    assert(eui_input_edge_ok_was_hold(&in));
    eui_input_edge_on_event(&in, &(eui_event_t){.type = EUI_EVT_KEY_RELEASE,
                                                .data.key = EUI_KEY_OK, .timestamp = 1600});
    eui_input_edge_tick(&in, 1601);
    assert(eui_input_edge_ok_was_released(&in));
    assert(!eui_input_edge_ok_was_pressed(&in)); /* hold 吞掉了 press 边沿 */

    /* UP/DOWN 折算编码器 + reset_edges 清幽灵边沿 */
    eui_input_edge_on_event(&in, &(eui_event_t){.type = EUI_EVT_KEY_PRESS,
                                                .data.key = EUI_KEY_UP});
    assert(eui_input_edge_encoder_count(&in) == 1);

    eui_input_edge_reset_edges(&in);
    assert(!eui_input_edge_ok_was_released(&in));
    assert(!eui_input_edge_side_was_clicked(&in));
    assert(!eui_input_edge_side_was_hold(&in));
    /* 释放后 reset：pending click 状态被清，后续 tick 不再推导出边沿 */
    eui_input_edge_tick(&in, 1700);
    assert(!eui_input_edge_side_was_clicked(&in));
    PASS();
}

int main(void)
{
    eui_test_init();
    test_encoder_and_arrow_keys();
    test_ok_press_release_edges();
    test_side_click_and_hold();
    test_ok_hold_and_reset_edges();
    return eui_test_summary();
}
