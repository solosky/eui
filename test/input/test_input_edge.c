/*
 * test_input_edge.c — eui_input_edge 手势装配器（push 管线）。
 * 覆盖：
 *   1. click 装配（PRESS/RELEASE/CLICK 顺序断言）
 *   2. hold 吞 click
 *   3. release-before-hold 边界（499ms click / 500ms hold）
 *   4. 任意 key_id 一视同仁；越界 key_id 事件被丢弃
 *   5. CW/CCW → ENC_STEP（delta 保留）；非法 CCW 被忽略
 *   6. flush：队列清空、瞬时边沿清零、按住键 pressed/hold 计时保留
 *   7. 拉取式边沿与事件管线并行不互斥
 */
#include <assert.h>
#include <stdio.h>
#include "eui/eui_input_edge.h"
#include "common/eui_test.h"

#define OUT_OF_RANGE_ID 200u  /* >= EUI_KEY_ID_MAX（默认 8） */
#define VALID_ID        7u    /* 容量内任意编号，装配器一视同仁 */

static void feed_key(eui_input_edge_t *in, eui_event_type_t t, uint8_t id, uint32_t ts)
{
    eui_event_t e = {.type = t, .timestamp = ts};
    e.data.key_id = id;
    eui_input_edge_on_event(in, &e);
}

static bool pop_type(eui_input_edge_t *in, eui_event_t *out, eui_event_type_t want)
{
    if (!eui_input_edge_pop_event(in, out)) return false;
    return out->type == want;
}

/* 1. click 装配：PRESS→RELEASE 弹出 PRESS、RELEASE、CLICK（时序） */
static void test_click_assembly(void)
{
    TEST("input edge: press->release assembles PRESS, RELEASE, CLICK in order");
    eui_input_edge_t in;
    eui_input_edge_init(&in);

    feed_key(&in, EUI_EVT_KEY_PRESS, 0, 0);
    feed_key(&in, EUI_EVT_KEY_RELEASE, 0, 100);

    eui_event_t e;
    assert(pop_type(&in, &e, EUI_EVT_KEY_PRESS));
    assert(e.data.key_id == 0 && e.timestamp == 0);
    assert(pop_type(&in, &e, EUI_EVT_KEY_RELEASE));
    assert(e.data.key_id == 0 && e.timestamp == 100);
    assert(pop_type(&in, &e, EUI_EVT_KEY_CLICK));
    assert(e.data.key_id == 0 && e.timestamp == 100);
    assert(!eui_input_edge_pop_event(&in, &e));
    PASS();
}

/* 2. hold 吞 click：PRESS→tick(500) 出 HOLD，release 不再出 CLICK */
static void test_hold_swallows_click(void)
{
    TEST("input edge: hold at >=500ms fires HOLD and swallows the click");
    eui_input_edge_t in;
    eui_input_edge_init(&in);

    feed_key(&in, EUI_EVT_KEY_PRESS, 1, 1000);
    eui_input_edge_tick(&in, 1499);
    eui_event_t e;
    assert(pop_type(&in, &e, EUI_EVT_KEY_PRESS));
    assert(!eui_input_edge_pop_event(&in, &e));   /* 499ms 未到 hold 阈值 */

    eui_input_edge_tick(&in, 1500);               /* press_ms=1000, +500ms 到期 */
    assert(pop_type(&in, &e, EUI_EVT_KEY_HOLD));
    assert(e.data.key_id == 1);

    feed_key(&in, EUI_EVT_KEY_RELEASE, 1, 1600);
    assert(pop_type(&in, &e, EUI_EVT_KEY_RELEASE));
    assert(!eui_input_edge_pop_event(&in, &e));   /* hold 已吞掉 click */
    PASS();
}

/* 3. release-before-hold 边界：499ms 成 click；500ms 成 hold 吞 click */
static void test_release_before_hold_boundary(void)
{
    TEST("input edge: release at 499ms clicks, hold at 500ms does not");
    eui_input_edge_t in;
    eui_input_edge_init(&in);

    /* 499ms 释放 → CLICK */
    feed_key(&in, EUI_EVT_KEY_PRESS, 2, 0);
    feed_key(&in, EUI_EVT_KEY_RELEASE, 2, 499);
    eui_event_t e;
    assert(pop_type(&in, &e, EUI_EVT_KEY_PRESS));
    assert(pop_type(&in, &e, EUI_EVT_KEY_RELEASE));
    assert(pop_type(&in, &e, EUI_EVT_KEY_CLICK));
    assert(!eui_input_edge_pop_event(&in, &e));

    /* 500ms 才 tick → HOLD，release 无 CLICK */
    feed_key(&in, EUI_EVT_KEY_PRESS, 3, 2000);
    eui_input_edge_tick(&in, 2500);               /* 恰好 500ms */
    assert(pop_type(&in, &e, EUI_EVT_KEY_PRESS));
    assert(pop_type(&in, &e, EUI_EVT_KEY_HOLD));
    feed_key(&in, EUI_EVT_KEY_RELEASE, 3, 2600);
    assert(pop_type(&in, &e, EUI_EVT_KEY_RELEASE));
    assert(!eui_input_edge_pop_event(&in, &e));
    PASS();
}

/* 4. 任意 key_id 一视同仁；越界 key_id 事件被丢弃 */
static void test_arbitrary_and_out_of_range_key_ids(void)
{
    TEST("input edge: any in-range key_id assembles; out-of-range dropped");
    eui_input_edge_t in;
    eui_input_edge_init(&in);

    /* 7 号合法：完整 click 装配 */
    feed_key(&in, EUI_EVT_KEY_PRESS, VALID_ID, 0);
    feed_key(&in, EUI_EVT_KEY_RELEASE, VALID_ID, 50);
    eui_event_t e;
    assert(pop_type(&in, &e, EUI_EVT_KEY_PRESS));
    assert(e.data.key_id == VALID_ID);
    assert(pop_type(&in, &e, EUI_EVT_KEY_RELEASE));
    assert(pop_type(&in, &e, EUI_EVT_KEY_CLICK));
    assert(e.data.key_id == VALID_ID);
    assert(!eui_input_edge_pop_event(&in, &e));

    /* 越界：不透传、不装配、不锁存边沿 */
    feed_key(&in, EUI_EVT_KEY_PRESS, OUT_OF_RANGE_ID, 100);
    feed_key(&in, EUI_EVT_KEY_RELEASE, OUT_OF_RANGE_ID, 150);
    assert(!eui_input_edge_pop_event(&in, &e));
    assert(!eui_input_edge_was_pressed(&in, OUT_OF_RANGE_ID));
    assert(!eui_input_edge_was_released(&in, OUT_OF_RANGE_ID));
    assert(eui_input_edge_is_released(&in, OUT_OF_RANGE_ID));  /* 越界视作已释放 */
    PASS();
}

/* 5. CW/CCW → ENC_STEP（delta 保留）；CCW 带非负 delta 被忽略 */
static void test_encoder_step_assembly(void)
{
    TEST("input edge: CW/CCW fold into ENC_STEP with delta kept");
    eui_input_edge_t in;
    eui_input_edge_init(&in);

    eui_event_t cw = {.type = EUI_EVT_ENCODER_CW, .data.enc_delta = 2, .timestamp = 10};
    eui_input_edge_on_event(&in, &cw);
    eui_event_t e;
    assert(pop_type(&in, &e, EUI_EVT_ENC_STEP));
    assert(e.data.enc_delta == 2);

    eui_event_t ccw = {.type = EUI_EVT_ENCODER_CCW, .data.enc_delta = -3, .timestamp = 20};
    eui_input_edge_on_event(&in, &ccw);
    assert(pop_type(&in, &e, EUI_EVT_ENC_STEP));
    assert(e.data.enc_delta == -3);

    /* 非法 CCW（delta >= 0）被忽略 */
    eui_event_t bad1 = {.type = EUI_EVT_ENCODER_CCW, .data.enc_delta = 1, .timestamp = 30};
    eui_input_edge_on_event(&in, &bad1);
    eui_event_t bad2 = {.type = EUI_EVT_ENCODER_CCW, .data.enc_delta = 0, .timestamp = 31};
    eui_input_edge_on_event(&in, &bad2);
    assert(!eui_input_edge_pop_event(&in, &e));

    /* ENC_CLICK 透传 */
    eui_event_t click = {.type = EUI_EVT_ENCODER_CLICK, .timestamp = 40};
    eui_input_edge_on_event(&in, &click);
    assert(pop_type(&in, &e, EUI_EVT_ENCODER_CLICK));
    PASS();
}

/* 6. flush：队列清空、瞬时边沿清零、按住键 pressed/hold 计时保留 */
static void test_flush_clears_queue_and_edges_keeps_held_keys(void)
{
    TEST("input edge: flush drops pending events/edges, keeps held key timing");
    eui_input_edge_t in;
    eui_input_edge_init(&in);

    /* key2: press 后 hold 已触发（HOLD 还压在队列里） */
    feed_key(&in, EUI_EVT_KEY_PRESS, 2, 0);
    eui_input_edge_tick(&in, 600);
    /* key3: 按住未到 hold（press_ms 保留） */
    feed_key(&in, EUI_EVT_KEY_PRESS, 3, 100);
    /* key4: 快速 press+release，锁存 press/release/click 边沿 */
    feed_key(&in, EUI_EVT_KEY_PRESS, 4, 120);
    feed_key(&in, EUI_EVT_KEY_RELEASE, 4, 200);

    eui_input_edge_flush(&in);

    /* 队列清空（含未派发的 HOLD） */
    eui_event_t e;
    assert(!eui_input_edge_pop_event(&in, &e));
    /* 瞬时边沿清零 */
    assert(!eui_input_edge_was_pressed(&in, 4));
    assert(!eui_input_edge_was_released(&in, 4));
    assert(!eui_input_edge_was_clicked(&in, 4));
    assert(!eui_input_edge_was_hold(&in, 2));
    /* 按住键电平保留 */
    assert(!eui_input_edge_is_released(&in, 2));
    assert(!eui_input_edge_is_released(&in, 3));

    /* key3 的 hold 计时保留：flush 后 tick 仍按原 press_ms 推进 */
    eui_input_edge_tick(&in, 650);   /* 650-100=550 >= 500 */
    assert(eui_input_edge_was_hold(&in, 3));
    assert(pop_type(&in, &e, EUI_EVT_KEY_HOLD));
    assert(e.data.key_id == 3);
    /* hold 后释放不再成 click */
    feed_key(&in, EUI_EVT_KEY_RELEASE, 3, 700);
    assert(pop_type(&in, &e, EUI_EVT_KEY_RELEASE));
    assert(!eui_input_edge_pop_event(&in, &e));

    /* flush 后未按住的键：游离 RELEASE 只透传，不派生 click */
    feed_key(&in, EUI_EVT_KEY_RELEASE, 5, 800);
    assert(!eui_input_edge_was_clicked(&in, 5));
    assert(pop_type(&in, &e, EUI_EVT_KEY_RELEASE));
    assert(!eui_input_edge_pop_event(&in, &e));
    PASS();
}

/* 7. 拉取式边沿与事件管线并行不互斥（同一序列两边都能读到） */
static void test_pull_edges_coexist_with_event_pipeline(void)
{
    TEST("input edge: pull edges and event queue observe the same sequence");
    eui_input_edge_t in;
    eui_input_edge_init(&in);

    feed_key(&in, EUI_EVT_KEY_PRESS, 4, 0);
    /* 事件与边沿同时可见 */
    eui_event_t e;
    assert(eui_input_edge_was_pressed(&in, 4));
    assert(!eui_input_edge_was_pressed(&in, 4));   /* 消费即清零 */
    assert(pop_type(&in, &e, EUI_EVT_KEY_PRESS));

    feed_key(&in, EUI_EVT_KEY_RELEASE, 4, 100);
    assert(eui_input_edge_was_released(&in, 4));
    assert(eui_input_edge_was_clicked(&in, 4));
    assert(eui_input_edge_is_released(&in, 4));
    assert(pop_type(&in, &e, EUI_EVT_KEY_RELEASE));
    assert(pop_type(&in, &e, EUI_EVT_KEY_CLICK));
    assert(!eui_input_edge_pop_event(&in, &e));

    /* hold 同样两边可见 */
    feed_key(&in, EUI_EVT_KEY_PRESS, 5, 1000);
    eui_input_edge_tick(&in, 1500);
    assert(eui_input_edge_was_hold(&in, 5));
    assert(pop_type(&in, &e, EUI_EVT_KEY_PRESS));   /* PRESS 透传在前 */
    assert(pop_type(&in, &e, EUI_EVT_KEY_HOLD));
    assert(e.data.key_id == 5);
    PASS();
}

int main(void)
{
    eui_test_init();
    test_click_assembly();
    test_hold_swallows_click();
    test_release_before_hold_boundary();
    test_arbitrary_and_out_of_range_key_ids();
    test_encoder_step_assembly();
    test_flush_clears_queue_and_edges_keeps_held_keys();
    test_pull_edges_coexist_with_event_pipeline();
    return eui_test_summary();
}
