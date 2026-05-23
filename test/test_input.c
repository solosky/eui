#include "eui/eui_input.h"
#include <stdio.h>
#include <string.h>

static int tests_run = 0;
static int tests_passed = 0;
#define TEST(n) do { printf("  %s... ", n); tests_run++; } while(0)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(m) do { printf("FAIL: %s\n", m); return; } while(0)

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
    mock_events[0] = (eui_event_t){ .type = EUI_EVT_KEY_PRESS, .data.key = EUI_KEY_OK, .timestamp = 0 };
    mock_events[1] = (eui_event_t){ .type = EUI_EVT_KEY_PRESS, .data.key = EUI_KEY_OK, .timestamp = 5 };
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
    mock_events[0] = (eui_event_t){ .type = EUI_EVT_KEY_PRESS, .data.key = EUI_KEY_OK, .timestamp = 0 };
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

    mock_events[0] = (eui_event_t){ .type = EUI_EVT_KEY_PRESS, .data.key = EUI_KEY_OK, .timestamp = 0 };
    mock_events[1] = (eui_event_t){ .type = EUI_EVT_KEY_RELEASE, .data.key = EUI_KEY_OK, .timestamp = 300 };
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

int main(void) {
    printf("=== Input Manager Tests ===\n");

    test_debounce();
    test_long_press();
    test_key_release();

    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
