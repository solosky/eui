/* examples/cross/basic_label/basic_label.c */
#include "eui/eui.h"
#include "eui/eui_port_bootstrap.h"

void eui_example_setup(const eui_example_config_t *cfg) {
    (void)cfg;

    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();

    eui_widget_t *label = eui_label_create("Hello EUI!", 10, 5);
    eui_view_dispatcher_add(vd, 1, &label->view);
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);
}
