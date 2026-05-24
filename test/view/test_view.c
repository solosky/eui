#include "eui/eui.h"
#include <stdio.h>
#include <string.h>

#include "common/eui_test.h"

/* Mock display for ViewDispatcher tests */
#define MOCK_W 128
#define MOCK_H 64
static uint8_t mock_buf[MOCK_W * MOCK_H / 8];

static void mock_write_buffer(const uint8_t *b, const eui_rect_t *r, void *ud) {
    (void)ud;
    int bpr = r->w / 8;
    for (int row = 0; row < (int)r->h; row++)
        memcpy(mock_buf + ((r->y + row) * (MOCK_W / 8) + r->x / 8),
               b + row * bpr, bpr);
}

static eui_display_drv_t mock_display = {
    .caps = { .width = 128, .height = 64, .color_depth = 1, .buffer_mode = 1 },
    .write_buffer = mock_write_buffer,
};

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

/* ---- ViewDispatcher tests ---- */
static int vd_draw_count, vd_enter_count, vd_exit_count;

static bool vd_handler(eui_view_event_t *event, void *ctx) {
    switch (event->type) {
        case EUI_VIEW_EVT_DRAW: vd_draw_count++; break;
        case EUI_VIEW_EVT_ENTER: vd_enter_count++; break;
        case EUI_VIEW_EVT_EXIT: vd_exit_count++; break;
        default: break;
    }
    return true;
}

static void test_dispatcher_switch(void) {
    TEST("dispatcher switch_to");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    eui_view_dispatcher_t vd;
    eui_view_dispatcher_init(&vd, c, NULL);

    eui_view_t v1, v2;
    eui_view_init(&v1, vd_handler, NULL);
    eui_view_init(&v2, vd_handler, NULL);

    vd_enter_count = vd_exit_count = vd_draw_count = 0;

    eui_view_dispatcher_add(&vd, 1, &v1);
    eui_view_dispatcher_add(&vd, 2, &v2);

    eui_view_dispatcher_switch_to(&vd, 1, EUI_ANIM_NONE);

    eui_view_dispatcher_switch_to(&vd, 2, EUI_ANIM_NONE);

    eui_canvas_destroy(c);
    PASS();
}

static void test_dispatcher_overlay(void) {
    TEST("dispatcher overlay push/pop");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    eui_view_dispatcher_t vd;
    eui_view_dispatcher_init(&vd, c, NULL);

    eui_view_t base, overlay;
    eui_view_init(&base, vd_handler, NULL);
    eui_view_init(&overlay, vd_handler, NULL);

    eui_view_dispatcher_add(&vd, 1, &base);
    eui_view_dispatcher_switch_to(&vd, 1, EUI_ANIM_NONE);

    eui_view_t *active = eui_view_dispatcher_get_active(&vd);
    if (active != &base) FAIL("expected base as active");

    eui_view_dispatcher_push_overlay(&vd, &overlay, EUI_ANIM_NONE);
    active = eui_view_dispatcher_get_active(&vd);
    if (active != &overlay) FAIL("expected overlay as active");

    eui_view_dispatcher_pop_overlay(&vd, EUI_ANIM_NONE);
    active = eui_view_dispatcher_get_active(&vd);
    if (active != &base) FAIL("expected base as active after pop");

    eui_canvas_destroy(c);
    PASS();
}

int main(void) {
    eui_test_init();
    printf("=== View Tests ===\n");
    test_lifecycle();
    test_model();
    test_flags();
    test_dispatcher_switch();
    test_dispatcher_overlay();
    return eui_test_summary();
}
