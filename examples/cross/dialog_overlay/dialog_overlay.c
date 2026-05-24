#include "eui/eui.h"
#include "eui/eui_port_bootstrap.h"
#include <stdio.h>

static void on_dialog_done(eui_dialog_result_t result, void *ctx) {
    printf("[DIALOG] Result: %s\n", result == EUI_DIALOG_YES ? "YES" : "NO");
    (void)ctx;
}

void eui_example_setup(const eui_example_config_t *cfg) {
    (void)cfg;
    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();

    eui_view_t placeholder;
    eui_view_init(&placeholder, NULL, NULL);
    eui_view_dispatcher_add(vd, 0, &placeholder);
    eui_view_dispatcher_switch_to(vd, 0, EUI_ANIM_NONE);

    eui_widget_t *dialog = eui_dialog_create("Confirm", "Are you sure?");
    eui_dialog_add_button(dialog, "Yes", EUI_DIALOG_YES);
    eui_dialog_add_button(dialog, "No",  EUI_DIALOG_NO);
    eui_dialog_show(dialog, vd, on_dialog_done);
}
