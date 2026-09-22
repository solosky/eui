#include "eui/eui_input.h"
#include <stdio.h>
#include <string.h>
#include "common/eui_test.h"

/* Mock HAL: returns pre-loaded events one at a time */
static eui_event_t mock_events[10];
static int mock_count;
static int mock_index;

static int mock_poll(eui_event_t *event, void *ud) {
    (void)ud;
    if (mock_index >= mock_count) return 0;
    *event = mock_events[mock_index++];
    return 1;
}

static eui_input_drv_t mock_hal = {
    .poll = mock_poll,
    .init = NULL,
    .deinit = NULL,
    .set_callback = NULL,
    .user_data = NULL
};

static void test_debounce(void) {
    TEST("debounce suppresses rapid press");
    eui_input_manager_t mgr;
    eui_input_init(&mgr, &mock_hal);

    /* Load mock: two rapid PRESS events within debounce window */
    mock_events[0] = (eui_event_t){ .type = EUI_EVT_KEY_PRESS, .data.key_id = 4, .timestamp = 0 };
    mock_events[1] = (eui_event_t){ .type = EUI_EVT_KEY_PRESS, .data.key_id = 4, .timestamp = 5 };
    mock_count = 2;
    mock_index = 0;

    /* Update at time 0 - first press should go through */
    eui_input_update(&mgr, 0);

    /* Check only 1 event queued (second should be debounced out) */
    eui_event_t out;
    int count = 0;
    while (eui_input_get_event(&mgr, &out)) count++;
    if (count != 1) { printf("FAIL: expected 1 event after debounce, got %d\n", count); return; }
    if (out.type != EUI_EVT_KEY_PRESS) FAIL("expected KEY_PRESS");

    PASS();
}

static void test_long_press(void) {
    TEST("long press generates repeat event");
    eui_input_manager_t mgr;
    eui_input_init(&mgr, &mock_hal);

    /* Load mock: one press event */
    mock_events[0] = (eui_event_t){ .type = EUI_EVT_KEY_PRESS, .data.key_id = 4, .timestamp = 0 };
    mock_count = 1;
    mock_index = 0;

    /* Process press at time 0 */
    eui_input_update(&mgr, 0);

    /* Drain initial press event */
    eui_event_t out;
    eui_input_get_event(&mgr, &out);

    /* Update at time 600ms (past long_press_ms of 500) */
    eui_input_update(&mgr, 600);

    /* Should have a repeat event */
    if (!eui_input_get_event(&mgr, &out)) FAIL("expected repeat event");
    if (out.type != EUI_EVT_KEY_REPEAT) FAIL("expected KEY_REPEAT");

    PASS();
}

static void test_key_release(void) {
    TEST("key release generates release event");
    eui_input_manager_t mgr;
    eui_input_init(&mgr, &mock_hal);

    mock_events[0] = (eui_event_t){ .type = EUI_EVT_KEY_PRESS, .data.key_id = 4, .timestamp = 0 };
    mock_events[1] = (eui_event_t){ .type = EUI_EVT_KEY_RELEASE, .data.key_id = 4, .timestamp = 300 };
    mock_count = 2;
    mock_index = 0;

    eui_input_update(&mgr, 0);
    eui_input_update(&mgr, 350);

    eui_event_t out;
    int press_count = 0, release_count = 0;
    while (eui_input_get_event(&mgr, &out)) {
        if (out.type == EUI_EVT_KEY_PRESS) press_count++;
        if (out.type == EUI_EVT_KEY_RELEASE) release_count++;
    }

    if (press_count != 1) FAIL("expected 1 press event");
    if (release_count != 1) FAIL("expected 1 release event");

    PASS();
}

static void test_timestamp_normalization(void) {
    TEST("raw event timestamps are normalized to the eui tick clock");
    eui_input_manager_t mgr;
    eui_input_init(&mgr, &mock_hal);

    /* 驱动不上报时间戳（ESP32 buttons/encoder 不写 timestamp，载体为
     * 未初始化栈内存；web 恒 0）：入队事件必须盖 now_ms 戳 */
    mock_events[0] = (eui_event_t){ .type = EUI_EVT_KEY_PRESS, .data.key_id = 4 };
    mock_events[1] = (eui_event_t){ .type = EUI_EVT_KEY_RELEASE, .data.key_id = 4 };
    mock_count = 2;
    mock_index = 0;

    eui_input_update(&mgr, 7777);

    eui_event_t out;
    int count = 0;
    while (eui_input_get_event(&mgr, &out)) {
        if (out.timestamp != 7777) FAIL("queued event timestamp != now_ms");
        count++;
    }
    if (count != 2) FAIL("expected 2 events");

    PASS();
}

int main(void) {
    printf("=== Input Manager Tests ===\n");

    test_debounce();
    test_long_press();
    test_key_release();
    test_timestamp_normalization();

    return eui_test_summary();
}
