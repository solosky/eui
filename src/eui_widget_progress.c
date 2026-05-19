#include "eui/eui_widget_progress.h"
#include "eui/eui_allocator.h"
#include <string.h>

static void progress_draw(eui_widget_t *self, eui_canvas_t *canvas) {
    eui_progress_t *p = (eui_progress_t*)self;

    eui_canvas_draw_rect(canvas, self->area.x, self->area.y, self->area.w, self->area.h);

    uint16_t fill_w = (uint16_t)((uint32_t)self->area.w * p->value / 100);
    if (fill_w > 0) {
        eui_canvas_fill_rect(canvas, self->area.x + 1, self->area.y + 1,
                              fill_w - 1, self->area.h - 2);
    }
}

static bool progress_input(eui_widget_t *self, const eui_event_t *evt) {
    (void)self;
    (void)evt;
    return false;
}

static const eui_widget_vtable_t progress_vtable = {
    .draw = progress_draw,
    .input = progress_input,
    .enter = NULL,
    .exit = NULL,
    .layout = NULL,
    .destroy = NULL
};

eui_widget_t* eui_progress_create(int16_t x, int16_t y, uint16_t w, uint16_t h) {
    eui_progress_t *p = eui_malloc(sizeof(eui_progress_t));
    if (!p) return NULL;
    memset(p, 0, sizeof(*p));
    eui_widget_init(&p->widget, &progress_vtable, x, y, w, h);
    return &p->widget;
}

void eui_progress_set_value(eui_widget_t *prog, uint8_t percent) {
    if (!prog) return;
    ((eui_progress_t*)prog)->value = (percent > 100) ? 100 : percent;
    prog->style |= EUI_STYLE_DIRTY;
}

void eui_progress_set_indeterminate(eui_widget_t *prog, bool indet) {
    if (!prog) return;
    ((eui_progress_t*)prog)->indeterminate = indet;
}
