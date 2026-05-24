#include "eui/eui_event.h"
#include <stdio.h>
#include <string.h>
#include "common/eui_test.h"

static void test_push_pop(void) {
    TEST("push and pop single event");
    eui_event_queue_t q;
    eui_event_queue_init(&q);

    eui_event_t in = { .type = EUI_EVT_KEY_PRESS, .data.key = EUI_KEY_OK, .timestamp = 100 };
    eui_event_t out;

    if (!eui_event_queue_push(&q, &in)) FAIL("push failed");
    if (!eui_event_queue_pop(&q, &out)) FAIL("pop failed");
    if (out.type != in.type) FAIL("type mismatch");
    if (out.data.key != in.data.key) FAIL("key mismatch");
    if (out.timestamp != in.timestamp) FAIL("timestamp mismatch");

    PASS();
}

static void test_full_queue(void) {
    TEST("queue full behavior");
    eui_event_queue_t q;
    eui_event_queue_init(&q);

    eui_event_t evt = { .type = EUI_EVT_KEY_PRESS, .data.key = EUI_KEY_OK, .timestamp = 0 };

    /* Fill the queue */
    for (int i = 0; i < EUI_EVENT_QUEUE_SIZE; i++) {
        evt.timestamp = i;
        if (!eui_event_queue_push(&q, &evt)) FAIL("push failed before full");
    }

    if (!eui_event_queue_is_full(&q)) FAIL("queue should be full");

    /* Try to push when full (non-overwrite mode - default) */
    evt.timestamp = 999;
    if (eui_event_queue_push(&q, &evt)) FAIL("push should fail when full");

    /* Verify queue contents unchanged - oldest event should still be timestamp 0 */
    eui_event_t out;
    eui_event_queue_pop(&q, &out);
    if (out.timestamp != 0) FAIL("oldest event should still be timestamp 0");

    PASS();
}

static void test_overwrite_mode(void) {
    TEST("overwrite mode drops oldest");
    eui_event_queue_t q;
    eui_event_queue_init(&q);
    q.overwrite = true;

    eui_event_t evt = { .type = EUI_EVT_KEY_PRESS, .data.key = EUI_KEY_OK, .timestamp = 0 };

    /* Fill queue */
    for (int i = 0; i < EUI_EVENT_QUEUE_SIZE; i++) {
        evt.timestamp = i;
        eui_event_queue_push(&q, &evt);
    }

    /* Push one more - should overwrite oldest */
    evt.timestamp = 999;
    eui_event_queue_push(&q, &evt);

    /* Oldest should now be timestamp 1 (timestamp 0 was dropped) */
    eui_event_t out;
    eui_event_queue_pop(&q, &out);
    if (out.timestamp != 1) FAIL("oldest event was not dropped");
    if (eui_event_queue_count(&q) != EUI_EVENT_QUEUE_SIZE - 1) FAIL("queue should have one less after pop");

    PASS();
}

static void test_empty_queue(void) {
    TEST("pop from empty queue fails");
    eui_event_queue_t q;
    eui_event_queue_init(&q);

    eui_event_t out;
    if (eui_event_queue_pop(&q, &out)) FAIL("pop from empty should fail");
    if (!eui_event_queue_is_empty(&q)) FAIL("queue should be empty");
    if (eui_event_queue_count(&q) != 0) FAIL("count should be 0");

    PASS();
}

int main(void) {
    printf("=== Event Queue Tests ===\n");

    test_push_pop();
    test_full_queue();
    test_overwrite_mode();
    test_empty_queue();

    return eui_test_summary();
}
