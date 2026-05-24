#ifndef EUI_TEST_CANVAS_H
#define EUI_TEST_CANVAS_H

#include "eui/eui_canvas.h"
#include "eui/eui_display_drv.h"
#include "eui/eui_font_builtin.h"

static inline eui_canvas_t *eui_test_canvas_new(eui_display_drv_t *drv) {
    return eui_canvas_create(drv);
}

static inline void eui_test_canvas_free(eui_canvas_t *c) {
    eui_canvas_destroy(c);
}

#endif
