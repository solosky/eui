/* examples/cross/list_nav/list_nav.c */
#include "eui/eui.h"
#include "eui/eui_port_bootstrap.h"
#include <stdio.h>

static void on_select(uint8_t idx, void *ctx) {
    (void)ctx;
    printf("[LIST] Selected item %d\n", idx);
}

void eui_example_setup(const eui_example_config_t *cfg) {
    (void)cfg;

    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();

    eui_widget_t *list = eui_list_create(0, 0, 128, 64);
    list->focus_policy = EUI_FOCUS_STRONG;
    eui_list_set_callback(list, on_select, NULL);

    for (int i = 0; i < 10; i++) {
        char buf[16];
        snprintf(buf, sizeof(buf), "Item %d", i);
        eui_list_add_item(list, buf, NULL);
    }

    eui_view_dispatcher_add(vd, 1, &list->view);
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);
}
