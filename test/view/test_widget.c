#include "eui/eui.h"
#include <stdio.h>
#include <string.h>

#include "common/eui_test.h"

static void stub_draw(eui_widget_t *w, eui_canvas_t *c) { (void)w; (void)c; }
static bool stub_input(eui_widget_t *w, const eui_event_t *e) { (void)w; (void)e; return false; }

static eui_widget_vtable_t test_vt = { .draw = stub_draw, .input = stub_input };

static void test_focus_chain(void) {
    TEST("focus chain traversal");
    eui_widget_t root, a, b;
    eui_widget_init(&root, &test_vt, 0, 0, 100, 100);
    eui_widget_init(&a, &test_vt, 0, 0, 20, 20);
    eui_widget_init(&b, &test_vt, 30, 0, 20, 20);
    a.focus_policy = EUI_FOCUS_STRONG;
    b.focus_policy = EUI_FOCUS_STRONG;
    root.focus_policy = EUI_FOCUS_NONE;
    eui_widget_add_child(&root, &a);
    eui_widget_add_child(&root, &b);
    eui_widget_t *f = eui_widget_get_focus(&root);
    if (f != &a) FAIL("first focus should be a");
    eui_widget_t *n = eui_widget_focus_next(&root);
    if (n != &b) FAIL("focus_next should be b");
    eui_widget_t *p = eui_widget_focus_prev(&root);
    if (p != &a) FAIL("focus_prev from b should be a");
    PASS();
}

static void test_bridge(void) {
    TEST("widget bridge sets view handler");
    eui_widget_t w;
    eui_widget_init(&w, &test_vt, 0, 0, 50, 50);
    if (w.view.handler == NULL) FAIL("view handler should be set by widget_init");
    if (w.style != (EUI_STYLE_VISIBLE | EUI_STYLE_ENABLED)) FAIL("default style wrong");
    PASS();
}

int main(void) {
    printf("=== Widget Tests ===\n");
    test_focus_chain();
    test_bridge();
    return eui_test_summary();
}
