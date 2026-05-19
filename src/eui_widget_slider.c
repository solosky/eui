#include "eui/eui_widget_slider.h"
#include "eui/eui_allocator.h"
#include <string.h>

static void slider_draw(eui_widget_t *self, eui_canvas_t *canvas) {
    eui_slider_t *s = (eui_slider_t*)self;
    int16_t range = s->max - s->min;
    int16_t knob_x;

    if (range > 0) {
        knob_x = self->area.x + (int16_t)((int32_t)(s->value - s->min) * self->area.w / range);
    } else {
        knob_x = self->area.x + self->area.w / 2;
    }

    eui_canvas_draw_line(canvas, self->area.x, self->area.y + self->area.h / 2,
                          self->area.x + self->area.w, self->area.y + self->area.h / 2);
    eui_canvas_fill_rect(canvas, knob_x - 2, self->area.y, 5, self->area.h);

    if (self->style & EUI_STYLE_FOCUSED) {
        eui_canvas_draw_rect(canvas, self->area.x - 1, self->area.y - 1,
                              self->area.w + 2, self->area.h + 2);
    }
}

static bool slider_input(eui_widget_t *self, const eui_event_t *evt) {
    eui_slider_t *s = (eui_slider_t*)self;

    if (evt->type == EUI_EVT_KEY_PRESS) {
        if (evt->data.key == EUI_KEY_RIGHT && s->value < s->max) {
            s->value++;
            self->style |= EUI_STYLE_DIRTY;
            return true;
        }
        if (evt->data.key == EUI_KEY_LEFT && s->value > s->min) {
            s->value--;
            self->style |= EUI_STYLE_DIRTY;
            return true;
        }
    }

    if (evt->type == EUI_EVT_ENCODER_CW && s->value < s->max) {
        s->value++;
        self->style |= EUI_STYLE_DIRTY;
        return true;
    }
    if (evt->type == EUI_EVT_ENCODER_CCW && s->value > s->min) {
        s->value--;
        self->style |= EUI_STYLE_DIRTY;
        return true;
    }

    return false;
}

static void slider_enter(eui_widget_t *self) {
    self->style |= EUI_STYLE_FOCUSED;
    self->style |= EUI_STYLE_DIRTY;
}

static void slider_exit(eui_widget_t *self) {
    self->style &= ~EUI_STYLE_FOCUSED;
    self->style |= EUI_STYLE_DIRTY;
}

static const eui_widget_vtable_t slider_vtable = {
    .draw = slider_draw,
    .input = slider_input,
    .enter = slider_enter,
    .exit = slider_exit,
    .layout = NULL,
    .destroy = NULL
};

eui_widget_t* eui_slider_create(int16_t x, int16_t y, uint16_t w, uint16_t h) {
    eui_slider_t *s = eui_malloc(sizeof(eui_slider_t));
    if (!s) return NULL;
    memset(s, 0, sizeof(*s));
    eui_widget_init(&s->widget, &slider_vtable, x, y, w, h);
    s->widget.focus_policy = EUI_FOCUS_STRONG;
    s->min = 0;
    s->max = 100;
    s->value = 50;
    return &s->widget;
}

void eui_slider_set_range(eui_widget_t *slider, int16_t min, int16_t max) {
    if (!slider) return;
    ((eui_slider_t*)slider)->min = min;
    ((eui_slider_t*)slider)->max = max;
}

void eui_slider_set_value(eui_widget_t *slider, int16_t value) {
    if (!slider) return;
    ((eui_slider_t*)slider)->value = value;
    slider->style |= EUI_STYLE_DIRTY;
}

int16_t eui_slider_get_value(const eui_widget_t *slider) {
    return slider ? ((eui_slider_t*)slider)->value : 0;
}
