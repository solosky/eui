#include "eui/eui_widget_dialog.h"
#include "eui/eui_allocator.h"
#include <string.h>

static void dialog_draw(eui_widget_t *self, eui_canvas_t *canvas) {
    eui_dialog_t *d = (eui_dialog_t*)self;
    int16_t dlg_w = 80, dlg_h = 40;
    int16_t dlg_x = self->area.x + (self->area.w - dlg_w) / 2;
    int16_t dlg_y = self->area.y + (self->area.h - dlg_h) / 2;

    /* Semi-transparent overlay: checkerboard pattern */
    for (int16_t yy = 0; yy < (int16_t)self->area.h; yy += 4) {
        for (int16_t xx = 0; xx < (int16_t)self->area.w; xx += 4) {
            if ((xx / 4 + yy / 4) % 2) {
                eui_canvas_draw_dot(canvas, self->area.x + xx, self->area.y + yy);
            }
        }
    }

    /* Dialog box */
    eui_canvas_fill_rect(canvas, dlg_x, dlg_y, dlg_w, dlg_h);
    eui_canvas_invert_rect(canvas, dlg_x, dlg_y, dlg_w, dlg_h);

    if (d->title) {
        eui_canvas_draw_str(canvas, dlg_x + 2, dlg_y + 2, d->title);
    }
    if (d->message) {
        eui_canvas_draw_str(canvas, dlg_x + 2, dlg_y + 14, d->message);
    }

    /* Draw buttons */
    for (uint8_t i = 0; i < d->button_count; i++) {
        int16_t bx = dlg_x + 4 + i * 26;
        int16_t by = dlg_y + dlg_h - 12;
        if (i == d->focused_button) {
            eui_canvas_invert_rect(canvas, bx, by, 22, 10);
        }
        eui_canvas_draw_str(canvas, bx + 2, by + 2, d->buttons[i].label);
    }
}

static bool dialog_input(eui_widget_t *self, const eui_event_t *evt) {
    eui_dialog_t *d = (eui_dialog_t*)self;

    if (evt->type == EUI_EVT_KEY_PRESS) {
        if (evt->data.key == EUI_KEY_LEFT && d->focused_button > 0) {
            d->focused_button--;
            self->style |= EUI_STYLE_DIRTY;
        }
        if (evt->data.key == EUI_KEY_RIGHT && d->focused_button < d->button_count - 1) {
            d->focused_button++;
            self->style |= EUI_STYLE_DIRTY;
        }
        if (evt->data.key == EUI_KEY_OK) {
            eui_dialog_result_t result = d->buttons[d->focused_button].result;
            if (d->vd) {
                eui_view_dispatcher_pop_overlay(d->vd, EUI_ANIM_NONE);
            }
            if (d->callback) {
                d->callback(result, d->callback_ctx);
            }
        }
        return true; /* Dialog intercepts all input */
    }

    return true;
}

static const eui_widget_vtable_t dialog_vtable = {
    .draw = dialog_draw,
    .input = dialog_input,
    .enter = NULL,
    .exit = NULL,
    .layout = NULL,
    .destroy = NULL
};

eui_widget_t* eui_dialog_create(const char *title, const char *msg) {
    eui_dialog_t *d = eui_malloc(sizeof(eui_dialog_t));
    if (!d) return NULL;
    memset(d, 0, sizeof(*d));
    eui_widget_init(&d->widget, &dialog_vtable, 0, 0, 128, 64);
    d->widget.focus_policy = EUI_FOCUS_STRONG;
    d->title = title;
    d->message = msg;
    return &d->widget;
}

void eui_dialog_add_button(eui_widget_t *dlg, const char *label, eui_dialog_result_t result) {
    if (!dlg) return;
    eui_dialog_t *d = (eui_dialog_t*)dlg;
    if (d->button_count >= EUI_DIALOG_MAX_BUTTONS) return;
    d->buttons[d->button_count].label = label;
    d->buttons[d->button_count].result = result;
    d->button_count++;
}

void eui_dialog_show(eui_widget_t *dlg, eui_view_dispatcher_t *vd, eui_dialog_callback_t cb) {
    if (!dlg || !vd) return;
    eui_dialog_t *d = (eui_dialog_t*)dlg;
    d->vd = vd;
    d->callback = cb;
    eui_view_dispatcher_push_overlay(vd, &dlg->view, EUI_ANIM_NONE);
}
