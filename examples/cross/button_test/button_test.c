/* examples/cross/button_test/button_test.c */
#include "eui/eui.h"
#include "eui/eui_port_bootstrap.h"
#include <stdio.h>

static void on_btn_a(void *ctx) {
    (void)ctx;
    printf("[BTN] A clicked\n");
}

static void on_btn_b(void *ctx) {
    (void)ctx;
    printf("[BTN] B clicked\n");
}

static void container_draw(eui_widget_t *w, eui_canvas_t *c) {
    for (uint8_t i = 0; i < w->child_count; i++) {
        eui_widget_t *child = w->children[i];
        if (eui_widget_is_visible(child) && child->vt && child->vt->draw)
            child->vt->draw(child, c);
    }
}

static bool container_input(eui_widget_t *w, const eui_event_t *evt) {
    if (evt->type == EUI_EVT_KEY_PRESS) {
        if (evt->data.key == EUI_KEY_RIGHT) { eui_widget_focus_next(w); return true; }
        if (evt->data.key == EUI_KEY_LEFT)  { eui_widget_focus_prev(w); return true; }
        if (evt->data.key == EUI_KEY_OK) {
            eui_widget_t *focused = eui_widget_get_focus(w);
            if (focused && focused->vt && focused->vt->input)
                return focused->vt->input(focused, evt);
        }
    }
    if (evt->type == EUI_EVT_KEY_RELEASE && evt->data.key == EUI_KEY_OK) {
        eui_widget_t *focused = eui_widget_get_focus(w);
        if (focused && focused->vt && focused->vt->input)
            return focused->vt->input(focused, evt);
    }
    return false;
}

static eui_widget_vtable_t container_vt = {
    .draw = container_draw,
    .input = container_input,
};

void eui_example_setup(const eui_example_config_t *cfg) {
    (void)cfg;

    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();

    eui_widget_t *root = eui_malloc(sizeof(eui_widget_t));
    eui_widget_init(root, &container_vt, 0, 0, 128, 64);

    eui_widget_t *btn_a = eui_button_create("Btn A", 10, 20, 50, 20);
    eui_button_set_callback(btn_a, on_btn_a, NULL);
    eui_widget_add_child(root, btn_a);

    eui_widget_t *btn_b = eui_button_create("Btn B", 68, 20, 50, 20);
    eui_button_set_callback(btn_b, on_btn_b, NULL);
    eui_widget_add_child(root, btn_b);

    eui_view_dispatcher_add(vd, 1, &root->view);
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);
}
