#include "eui/eui.h"
#include <stdio.h>
#include <string.h>

static int tests_run = 0, tests_passed = 0;
#define TEST(n) do { printf("  %s... ", n); tests_run++; } while(0)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(m) do { printf("FAIL: %s\n", m); return; } while(0)

/* Test handler that counts events */
typedef struct {
    int draw_count;
    int input_count;
    int enter_count;
    int exit_count;
    int nav_count;
    void *received_model;
    const eui_event_t *received_input;
} test_handler_data_t;

static bool test_handler(eui_view_event_t *evt, void *ctx) {
    test_handler_data_t *d = (test_handler_data_t*)ctx;
    switch (evt->type) {
        case EUI_VIEW_EVT_DRAW:  d->draw_count++;  d->received_model = evt->event.draw.model; break;
        case EUI_VIEW_EVT_INPUT: d->input_count++; d->received_input = evt->event.input.input; break;
        case EUI_VIEW_EVT_ENTER: d->enter_count++; break;
        case EUI_VIEW_EVT_EXIT:  d->exit_count++; break;
        case EUI_VIEW_EVT_NAVIGATE: d->nav_count++; break;
        default: break;
    }
    return true;
}

static void test_lifecycle(void) {
    TEST("view lifecycle sequence");
    test_handler_data_t data = {0};
    eui_view_t view;
    eui_view_init(&view, test_handler, &data);

    eui_view_send_enter(&view);
    eui_view_send_draw(&view, NULL);
    eui_view_send_exit(&view);

    if (data.enter_count != 1) FAIL("enter not called");
    if (data.draw_count != 1) FAIL("draw not called");
    if (data.exit_count != 1) FAIL("exit not called");
    PASS();
}

static void test_model_binding(void) {
    TEST("model binding passes model to draw");
    test_handler_data_t data = {0};
    eui_view_t view;
    eui_view_init(&view, test_handler, &data);

    int model_value = 42;
    eui_view_set_model(&view, &model_value);
    eui_view_send_draw(&view, NULL);

    if (data.received_model != &model_value) FAIL("model pointer mismatch");
    PASS();
}

static void test_input_event(void) {
    TEST("input event passes through");
    test_handler_data_t data = {0};
    eui_view_t view;
    eui_view_init(&view, test_handler, &data);

    eui_event_t evt = { .type = EUI_EVT_KEY_PRESS, .data.key = EUI_KEY_OK, .timestamp = 100 };
    eui_view_send_input(&view, &evt);

    if (data.input_count != 1) FAIL("input not received");
    if (data.received_input->data.key != EUI_KEY_OK) FAIL("key mismatch");
    PASS();
}

static void test_dirty_flag(void) {
    TEST("dirty flag clears after draw");
    eui_view_t view;
    eui_view_init(&view, test_handler, &(test_handler_data_t){0});
    eui_view_mark_dirty(&view);
    if (!(view.flags & EUI_VIEW_FLAG_DIRTY)) FAIL("dirty flag not set");
    eui_view_send_draw(&view, NULL);
    if (view.flags & EUI_VIEW_FLAG_DIRTY) FAIL("dirty flag not cleared");
    PASS();
}

static void test_null_handler(void) {
    TEST("null handler returns false safely");
    eui_view_t view;
    eui_view_init(&view, NULL, NULL);
    bool result = eui_view_send_enter(&view);
    if (result) FAIL("null handler should return false");
    PASS();
}

int main(void) {
    printf("=== View Tests ===\n");
    test_lifecycle();
    test_model_binding();
    test_input_event();
    test_dirty_flag();
    test_null_handler();
    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
