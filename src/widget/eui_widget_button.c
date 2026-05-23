#include "eui/widget/eui_widget_button.h"
#include "eui/eui_allocator.h"
#include "eui/eui_font_builtin.h"
#include <string.h>

static void button_draw(eui_widget_t *self, eui_canvas_t *canvas) {
    eui_button_t *b = (eui_button_t*)self;

    if (!canvas->font) {
        eui_canvas_set_font(canvas, &eui_font_builtin);
    }

    eui_color_t fg = self->style & EUI_STYLE_PRESSED
        ? EUI_COLOR_BLACK : EUI_COLOR_WHITE;
    eui_color_t bg = self->style & EUI_STYLE_PRESSED
        ? EUI_COLOR_WHITE : EUI_COLOR_BLACK;

    eui_canvas_set_bg_color(canvas, bg);
    eui_canvas_set_color(canvas, fg);

    if (self->style & EUI_STYLE_PRESSED) {
        eui_canvas_fill_round_rect(canvas, self->area.x, self->area.y,
                                    self->area.w, self->area.h, 3);
    } else {
        eui_canvas_draw_round_rect(canvas, self->area.x, self->area.y,
                                    self->area.w, self->area.h, 3);
    }

    if (b->icon) {
        int16_t icon_x = self->area.x + (self->area.w - b->icon->width) / 2;
        int16_t icon_y = self->area.y + (self->area.h - b->icon->height) / 2;
        if (!eui_str_empty(&b->label)) {
            icon_x = self->area.x + 4;
        }
        eui_canvas_draw_bitmap(canvas, icon_x, icon_y, b->icon);
    }

    if (!eui_str_empty(&b->label)) {
        int16_t tx = self->area.x + self->area.w / 2;
        int16_t ty = self->area.y + self->area.h / 2;
        if (b->icon) {
            tx = self->area.x + b->icon->width + 8
                + (self->area.w - b->icon->width - 8) / 2;
        }
        eui_canvas_draw_str_aligned(canvas, tx, ty,
            EUI_ALIGN_CENTER, EUI_ALIGN_MIDDLE, eui_str_cstr(&b->label));
    }

    if (self->style & EUI_STYLE_FOCUSED) {
        eui_canvas_draw_rect(canvas, self->area.x - 1, self->area.y - 1,
                              self->area.w + 2, self->area.h + 2);
    }
}

static bool button_input(eui_widget_t *self, const eui_event_t *evt) {
    eui_button_t *b = (eui_button_t*)self;
    if (evt->type == EUI_EVT_KEY_PRESS && evt->data.key == EUI_KEY_OK) {
        self->style |= EUI_STYLE_PRESSED;
        self->style |= EUI_STYLE_DIRTY;
        return true;
    }
    if (evt->type == EUI_EVT_KEY_RELEASE && evt->data.key == EUI_KEY_OK) {
        self->style &= ~EUI_STYLE_PRESSED;
        self->style |= EUI_STYLE_DIRTY;
        if (b->callback) b->callback(b->callback_ctx);
        return true;
    }
    return false;
}

static void button_enter(eui_widget_t *self) {
    self->style |= EUI_STYLE_FOCUSED;
    self->style |= EUI_STYLE_DIRTY;
}

static void button_exit(eui_widget_t *self) {
    self->style &= ~(EUI_STYLE_FOCUSED | EUI_STYLE_PRESSED);
    self->style |= EUI_STYLE_DIRTY;
}

static void button_destroy(eui_widget_t *self) {
    eui_button_t *b = (eui_button_t*)self;
    eui_str_clear(&b->label);
    eui_free(self);
}

static const eui_widget_vtable_t button_vtable = {
    .draw = button_draw,
    .input = button_input,
    .enter = button_enter,
    .exit = button_exit,
    .layout = NULL,
    .destroy = button_destroy
};

eui_widget_t* eui_button_create(const char *label, int16_t x, int16_t y,
                                 uint16_t w, uint16_t h) {
    eui_button_t *b = eui_malloc(sizeof(eui_button_t));
    if (!b) return NULL;
    memset(b, 0, sizeof(*b));
    eui_widget_init(&b->widget, &button_vtable, x, y, w, h);
    b->widget.focus_policy = EUI_FOCUS_STRONG;
    eui_str_init(&b->label);
    if (label) eui_str_set(&b->label, label);
    return &b->widget;
}

void eui_button_set_callback(eui_widget_t *btn, eui_button_callback_t cb, void *ctx) {
    if (!btn) return;
    eui_button_t *b = (eui_button_t*)btn;
    b->callback = cb;
    b->callback_ctx = ctx;
}

void eui_button_set_bitmap(eui_widget_t *btn, const eui_bitmap_t *bmp) {
    if (!btn) return;
    ((eui_button_t*)btn)->icon = bmp;
}
