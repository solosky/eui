#include "eui/eui.h"
#include <stdio.h>
#include <string.h>

#define W 128
#define H 64
#define POOL_SIZE 32768
static uint8_t mem_pool[POOL_SIZE];

static uint8_t mock_buf[W * H / 8];

static void mock_write_buffer(const uint8_t *b, const eui_rect_t *r, void *ud) {
    (void)ud;
    int bpr = r->w / 8;
    for (int row = 0; row < (int)r->h; row++)
        memcpy(mock_buf + ((r->y + row) * (W / 8) + r->x / 8),
               b + row * bpr, bpr);
}

static eui_display_hal_t mock_display = {
    .caps = { .width = W, .height = H, .color_depth = 1, .buffer_mode = EUI_BUFFER_FULL },
    .init = NULL, .deinit = NULL, .write_buffer = mock_write_buffer,
    .user_data = NULL
};

static int mock_poll(eui_event_t *e, void *d) { (void)e; (void)d; return 0; }
static eui_input_hal_t mock_input = {
    .poll = mock_poll, .user_data = NULL
};

static int btna_clicks = 0, btnb_clicks = 0;

static void on_btn_a(void *ctx) { btna_clicks++; (void)ctx; }
static void on_btn_b(void *ctx) { btnb_clicks++; (void)ctx; }

/* ---- Container widget that recurses into children ---- */
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

    eui_canvas_t *canvas = eui_canvas_create(&mock_display);
    eui_view_dispatcher_t vd;
    eui_view_dispatcher_init(&vd, canvas);

    eui_widget_t root;
    eui_widget_init(&root, &container_vt, 0, 0, W, H);

    eui_widget_t *btn_a = eui_button_create("Btn A", 10, 20, 50, 20);
    eui_button_set_callback(btn_a, on_btn_a, NULL);
    eui_widget_add_child(&root, btn_a);

    eui_widget_t *btn_b = eui_button_create("Btn B", 68, 20, 50, 20);
    eui_button_set_callback(btn_b, on_btn_b, NULL);
    eui_widget_add_child(&root, btn_b);

    eui_view_dispatcher_add(&vd, 1, &root.view);
    eui_view_dispatcher_switch_to(&vd, 1, EUI_ANIM_NONE);

    eui_event_t evt;

    evt.type = EUI_EVT_KEY_PRESS;
    evt.data.key = EUI_KEY_RIGHT;
    eui_view_dispatcher_send_input(&vd, &evt);

    evt.type = EUI_EVT_KEY_PRESS;
    evt.data.key = EUI_KEY_OK;
    eui_view_dispatcher_send_input(&vd, &evt);

    evt.type = EUI_EVT_KEY_RELEASE;
    evt.data.key = EUI_KEY_OK;
    eui_view_dispatcher_send_input(&vd, &evt);
    eui_view_dispatcher_tick(&vd);

    evt.type = EUI_EVT_KEY_PRESS;
    evt.data.key = EUI_KEY_LEFT;
    eui_view_dispatcher_send_input(&vd, &evt);

    evt.type = EUI_EVT_KEY_PRESS;
    evt.data.key = EUI_KEY_OK;
    eui_view_dispatcher_send_input(&vd, &evt);

    evt.type = EUI_EVT_KEY_RELEASE;
    evt.data.key = EUI_KEY_OK;
    eui_view_dispatcher_send_input(&vd, &evt);
    eui_view_dispatcher_tick(&vd);

    for (int i = 0; i < 5; i++)
        eui_view_dispatcher_tick(&vd);

    if (btna_clicks != 1) { printf("[FAIL] Btn A clicks=%d expected=1\n", btna_clicks); return 1; }
    if (btnb_clicks != 1) { printf("[FAIL] Btn B clicks=%d expected=1\n", btnb_clicks); return 1; }

    eui_canvas_destroy(canvas);
    printf("[PASS] button_test\n");
    return 0;
}
