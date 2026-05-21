#include "eui/eui.h"
#include "eui/hal/eui_hal_raylib.h"
#include <raylib.h>
#include <stdio.h>
#include <string.h>

#define W 128
#define H 64
#define POOL_SIZE 16384
static uint8_t mem_pool[POOL_SIZE];
static uint32_t get_tick(void) { return (uint32_t)(GetTime() * 1000.0); }

static void on_dialog_done(eui_dialog_result_t result, void *ctx) {
    printf("[DIALOG] Result: %s\n", result == EUI_DIALOG_YES ? "YES" : "NO");
    (void)ctx;
}

int main(void) {
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);
    eui_display_hal_t *display = eui_hal_raylib_create_display(W, H, EUI_COLOR_DEPTH);
    eui_input_hal_t *input = eui_hal_raylib_create_input();
    eui_config_t cfg = { .display=display, .input=input };
    eui_init(&cfg);
    eui_set_tick_callback(get_tick);
    display->init(display->user_data);
    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();

    eui_view_t placeholder;
    eui_view_init(&placeholder, NULL, NULL);
    eui_view_dispatcher_add(vd, 0, &placeholder);
    eui_view_dispatcher_switch_to(vd, 0, EUI_ANIM_NONE);

    eui_widget_t *dialog = eui_dialog_create("Confirm", "Are you sure?");
    eui_dialog_add_button(dialog, "Yes", EUI_DIALOG_YES);
    eui_dialog_add_button(dialog, "No",  EUI_DIALOG_NO);
    eui_dialog_show(dialog, vd, on_dialog_done);

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
