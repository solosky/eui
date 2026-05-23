#include "eui/widget/eui_widget_label.h"
#include "eui/eui_allocator.h"
#include "eui/eui_font_builtin.h"
#include "eui/eui_str.h"
#include <string.h>

static void label_draw(eui_widget_t *self, eui_canvas_t *canvas) {
    eui_label_t *l = (eui_label_t*)self;
    if (eui_str_empty(&l->text)) return;
    const eui_font_t *use_font = l->font ? l->font : &eui_font_builtin;
    if (canvas->font != use_font) {
        eui_canvas_set_font(canvas, use_font);
    }
    eui_canvas_draw_str_aligned(canvas, self->area.x + self->area.w / 2,
                                 self->area.y + self->area.h / 2,
                                 l->h_align, l->v_align, eui_str_cstr(&l->text));
}

static bool label_input(eui_widget_t *self, const eui_event_t *evt) {
    (void)self; (void)evt;
    return false;
}

static void label_destroy(eui_widget_t *self) {
    eui_label_t *l = (eui_label_t*)self;
    eui_str_clear(&l->text);
    eui_free(self);
}

static const eui_widget_vtable_t label_vtable = {
    .draw = label_draw,
    .input = label_input,
    .enter = NULL, .exit = NULL, .layout = NULL, .destroy = label_destroy
};

eui_widget_t* eui_label_create(const char *text, int16_t x, int16_t y) {
    eui_label_t *l = eui_malloc(sizeof(eui_label_t));
    if (!l) return NULL;
    memset(l, 0, sizeof(*l));
    eui_widget_init(&l->widget, &label_vtable, x, y, 0, 0);
    eui_str_init(&l->text);
    if (text) eui_str_set(&l->text, text);
    l->h_align = EUI_ALIGN_LEFT;
    l->v_align = EUI_ALIGN_MIDDLE;
    return &l->widget;
}

void eui_label_set_text(eui_widget_t *label, const char *text) {
    if (!label) return;
    eui_str_set(&((eui_label_t*)label)->text, text);
    label->style |= EUI_STYLE_DIRTY;
}

void eui_label_set_font(eui_widget_t *label, const eui_font_t *font) {
    if (!label) return;
    ((eui_label_t*)label)->font = font;
    label->style |= EUI_STYLE_DIRTY;
}

void eui_label_set_align(eui_widget_t *label, eui_align_t h, eui_align_t v) {
    if (!label) return;
    eui_label_t *l = (eui_label_t*)label;
    l->h_align = h;
    l->v_align = v;
    label->style |= EUI_STYLE_DIRTY;
}
