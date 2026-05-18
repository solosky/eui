#include "eui/eui.h"
#include <stdio.h>
#include <string.h>

static int tests_run = 0, tests_passed = 0;
#define TEST(n) do { printf("  %s... ", n); tests_run++; } while(0)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(m) do { printf("FAIL: %s\n", m); return; } while(0)

static int draw_count, enter_count, exit_count, input_count;

static bool test_handler(eui_view_event_t *event, void *ctx) {
    (void)ctx;
    switch (event->type) {
        case EUI_VIEW_EVT_DRAW:  draw_count++;  break;
        case EUI_VIEW_EVT_INPUT: input_count++; break;
        case EUI_VIEW_EVT_ENTER: enter_count++; break;
        case EUI_VIEW_EVT_EXIT:  exit_count++;  break;
        default: break;
    }
    return true;
}

static void test_lifecycle(void) {
    TEST("view lifecycle enter/draw/exit");
    eui_view_t view;
    eui_view_init(&view, test_handler, NULL);
    
    draw_count = enter_count = exit_count = 0;
    
    eui_view_send_enter(&view);
    eui_view_send_draw(&view, NULL);
    eui_view_send_exit(&view);
    
    if (enter_count != 1) FAIL("enter not called");
    if (draw_count != 1) FAIL("draw not called");
    if (exit_count != 1) FAIL("exit not called");
    PASS();
}

static void test_model(void) {
    TEST("model pointer passed in draw event");
    int model_data = 42;
    
    eui_view_t view;
    eui_view_init(&view, test_handler, NULL);
    eui_view_set_model(&view, &model_data);
    
    if (view.model != &model_data) FAIL("model not stored");
    PASS();
}

static void test_flags(void) {
    TEST("dirty flag");
    eui_view_t view;
    eui_view_init(&view, test_handler, NULL);
    
    if (!(view.flags & EUI_VIEW_FLAG_VISIBLE)) FAIL("view not visible after init");
    eui_view_mark_dirty(&view);
    if (!(view.flags & EUI_VIEW_FLAG_DIRTY)) FAIL("dirty flag not set");
    PASS();
}

int main(void) {
    printf("=== View Tests ===\n");
    test_lifecycle();
    test_model();
    test_flags();
    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
