#include "eui/eui.h"
#include <raylib.h>
#include <stdio.h>
#include <string.h>

#define W 128
#define H 64
#define POOL_SIZE 8192
static uint8_t mem_pool[POOL_SIZE];

static uint32_t get_tick(void) { return (uint32_t)(GetTime() * 1000.0); }

static void on_btn_a(void *ctx) { printf("[BTN] A clicked\n"); (void)ctx; }
static void on_btn_b(void *ctx) { printf("[BTN] B clicked\n"); (void)ctx; }

static void container_draw(eui_widget_t *w, eui_canvas_t *c) {
    for (uint8_t i = 0; i < w->child_count; i++) {
        eui_widget_t *child = w->children[i];
        if (eui_widget_is_visible(child) && child->vt && child->vt->draw)
            child->vt->draw(child, c);
    }
}

static bool container_input(eui_widget_t *w, const eui_event_t *evt) {
    if (evt->type == EUI_EVT_KEY_PRESS) {
        if (evt->data.key == EUI_KEY_RIGHT) {
            eui_widget_focus_next(w);
            return true;
        }
        if (evt->data.key == EUI_KEY_LEFT) {
            eui_widget_focus_prev(w);
            return true;
        }
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
    .input = container_input
};

int main(void) {
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);
    eui_display_hal_t *display = eui_hal_raylib_create_display(W, H, 1);
    eui_input_hal_t *input = eui_hal_raylib_create_input();

    eui_config_t cfg = { .display=display, .input=input };
    eui_init(&cfg);
    eui_set_tick_callback(get_tick);
    display->init(display->user_data);

    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();

    eui_widget_t root;
    eui_widget_init(&root, &container_vt, 0, 0, W, H);

    eui_widget_t *btn_a = eui_button_create("Btn A", 10, 20, 50, 20);
    eui_button_set_callback(btn_a, on_btn_a, NULL);
    eui_widget_add_child(&root, btn_a);

    eui_widget_t *btn_b = eui_button_create("Btn B", 68, 20, 50, 20);
    eui_button_set_callback(btn_b, on_btn_b, NULL);
    eui_widget_add_child(&root, btn_b);

    eui_view_dispatcher_add(vd, 1, &root.view);
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);

    while (!eui_hal_raylib_window_should_close()) {
        eui_tick();
        eui_hal_raylib_refresh();
    }

    display->deinit(display->user_data);
    eui_deinit();
    eui_hal_raylib_destroy_input(input);
    eui_hal_raylib_destroy_display(display);
    return 0;
}
