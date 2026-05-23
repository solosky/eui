#include "eui/widget/eui_widget_scroll.h"
#include "eui/eui_allocator.h"
#include <string.h>

static void scroll_draw(eui_widget_t *self, eui_canvas_t *canvas) {
    eui_scroll_t *s = (eui_scroll_t*)self;

    eui_canvas_save(canvas);

    eui_rect_t clip = self->area;
    eui_canvas_set_clip(canvas, &clip);

    for (uint8_t i = 0; i < self->child_count; i++) {
        eui_widget_t *child = self->children[i];
        if (child->vt && child->vt->draw) {
            int16_t orig_x = child->area.x;
            int16_t orig_y = child->area.y;
            child->area.x += s->scroll_x;
            child->area.y += s->scroll_y;
            child->vt->draw(child, canvas);
            child->area.x = orig_x;
            child->area.y = orig_y;
        }
    }

    eui_canvas_restore(canvas);
}

static bool scroll_input(eui_widget_t *self, const eui_event_t *evt) {
    eui_scroll_t *s = (eui_scroll_t*)self;

    if (evt->type == EUI_EVT_ENCODER_CW) {
        s->scroll_y += 8;
        self->style |= EUI_STYLE_DIRTY;
        return true;
    }
    if (evt->type == EUI_EVT_ENCODER_CCW) {
        s->scroll_y -= 8;
        self->style |= EUI_STYLE_DIRTY;
        return true;
    }
    if (evt->type == EUI_EVT_KEY_PRESS) {
        if (evt->data.key == EUI_KEY_DOWN) {
            s->scroll_y += 8;
            self->style |= EUI_STYLE_DIRTY;
            return true;
        }
        if (evt->data.key == EUI_KEY_UP) {
            s->scroll_y -= 8;
            self->style |= EUI_STYLE_DIRTY;
            return true;
        }
    }

    return false;
}

static const eui_widget_vtable_t scroll_vtable = {
    .draw = scroll_draw,
    .input = scroll_input,
    .enter = NULL,
    .exit = NULL,
    .layout = NULL,
    .destroy = NULL
};

eui_widget_t* eui_scroll_create(int16_t x, int16_t y, uint16_t w, uint16_t h) {
    eui_scroll_t *s = eui_malloc(sizeof(eui_scroll_t));
    if (!s) return NULL;
    memset(s, 0, sizeof(*s));
    eui_widget_init(&s->widget, &scroll_vtable, x, y, w, h);
    return &s->widget;
}

void eui_scroll_set_content_size(eui_widget_t *scroll, uint16_t cw, uint16_t ch) {
    if (!scroll) return;
    ((eui_scroll_t*)scroll)->content_width = cw;
    ((eui_scroll_t*)scroll)->content_height = ch;
}

void eui_scroll_add_child(eui_widget_t *scroll, eui_widget_t *child) {
    eui_widget_add_child(scroll, child);
}
