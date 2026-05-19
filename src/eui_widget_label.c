#include "eui/eui_widget_label.h"
#include "eui/eui_allocator.h"
#include "eui/eui_font_builtin.h"
#include <string.h>

static void label_draw(eui_widget_t *self, eui_canvas_t *canvas) {
    eui_label_t *l = (eui_label_t*)self;
    if (!l->text) return;
    if (!canvas->font) {
        eui_canvas_set_font(canvas, &eui_font_builtin);
    }
    eui_canvas_draw_str_aligned(canvas, self->area.x + self->area.w / 2,
                                 self->area.y + self->area.h / 2,
                                 l->h_align, l->v_align, l->text);
}

static bool label_input(eui_widget_t *self, const eui_event_t *evt) {
    (void)self; (void)evt;
    return false;
}

static const eui_widget_vtable_t label_vtable = {
    .draw = label_draw,
    .input = label_input,
    .enter = NULL, .exit = NULL, .layout = NULL, .destroy = NULL
};

eui_widget_t* eui_label_create(const char *text, int16_t x, int16_t y) {
    eui_label_t *l = eui_malloc(sizeof(eui_label_t));
    if (!l) return NULL;
    memset(l, 0, sizeof(*l));
    eui_widget_init(&l->widget, &label_vtable, x, y, 0, 0);
    l->text = text;
    l->h_align = EUI_ALIGN_LEFT;
    l->v_align = EUI_ALIGN_MIDDLE;
    return &l->widget;
}

void eui_label_set_text(eui_widget_t *label, const char *text) {
    if (!label) return;
    ((eui_label_t*)label)->text = text;
    label->style |= EUI_STYLE_DIRTY;
}

void eui_label_set_align(eui_widget_t *label, eui_align_t h, eui_align_t v) {
    if (!label) return;
    eui_label_t *l = (eui_label_t*)label;
    l->h_align = h;
    l->v_align = v;
    label->style |= EUI_STYLE_DIRTY;
}
