#include "eui/widget/eui_widget_list.h"
#include "eui/eui_allocator.h"
#include "eui/eui_font_builtin.h"
#include "mc.h"
#include <string.h>

#define ITEM_H 12
#define EUI_LIST_ANIM_DURATION 10

static void list_draw(eui_widget_t *self, eui_canvas_t *canvas) {
    eui_list_t *l = (eui_list_t*)self;
    uint8_t visible = self->area.h / ITEM_H;
    if (visible == 0) visible = 1;
    bool animating = l->anim_rem > 0;

    if (!canvas->font) {
        eui_canvas_set_font(canvas, &eui_font_builtin);
    }

    eui_canvas_save(canvas);
    eui_canvas_set_clip(canvas, &self->area);

    /* Draw animation highlight */
    if (animating) {
        l->anim_rem--;
        int16_t from_vis = (int16_t)l->anim_from - (int16_t)l->scroll_offset;
        if (from_vis >= 0 && from_vis < (int16_t)visible) {
            int16_t old_y = self->area.y + from_vis * l->item_height;
            int16_t to_vis = (int16_t)l->selected_index - (int16_t)l->scroll_offset;
            int16_t new_y = self->area.y + to_vis * l->item_height;
            int32_t raw = EUI_LIST_ANIM_DURATION - l->anim_rem;
            mc_real_t t = MC_FP_C((float)raw / EUI_LIST_ANIM_DURATION);
            mc_real_t eased = mc_ease_back_out(t);
            float f = MC_REAL_TO_FLOAT(eased);
            int16_t cur_y = old_y + (int16_t)((new_y - old_y) * f);
            eui_canvas_invert_rect(canvas, self->area.x, cur_y,
                                   self->area.w, l->item_height);
        }
    } else {
        /* Static highlight at selected index */
        if (l->selected_index >= l->scroll_offset
         && l->selected_index < l->scroll_offset + visible) {
            int16_t y = self->area.y
                + (l->selected_index - l->scroll_offset) * l->item_height;
            eui_canvas_invert_rect(canvas, self->area.x, y,
                                   self->area.w, l->item_height);
        }
    }

    /* Draw item text */
    for (uint8_t i = 0; i < visible && (i + l->scroll_offset) < l->item_count; i++) {
        uint8_t idx = i + l->scroll_offset;
        int16_t y = self->area.y + i * l->item_height;
        bool highlighted = (idx == l->selected_index)
                        || (animating && idx == (uint8_t)l->anim_from);
        eui_canvas_set_color(canvas, highlighted
                             ? EUI_COLOR_BLACK : EUI_COLOR_WHITE);
        eui_canvas_draw_str(canvas, self->area.x + 2, y + 2, eui_str_cstr(&l->items[idx].text));
    }
    eui_canvas_restore(canvas);
}

static void list_start_anim(eui_list_t *l, uint8_t old_sel) {
    l->anim_from = (int8_t)old_sel;
    l->anim_rem = EUI_LIST_ANIM_DURATION;
}

static bool list_input(eui_widget_t *self, const eui_event_t *evt) {
    eui_list_t *l = (eui_list_t*)self;
    uint8_t visible = self->area.h / ITEM_H;
    if (visible == 0) visible = 1;

    if (evt->type == EUI_EVT_KEY_PRESS) {
        if (evt->data.key == EUI_KEY_DOWN) {
            if (l->selected_index < l->item_count - 1) {
                uint8_t old = l->selected_index;
                l->selected_index++;
                if (l->selected_index >= l->scroll_offset + visible)
                    l->scroll_offset++;
                list_start_anim(l, old);
            }
            return true;
        }
        if (evt->data.key == EUI_KEY_UP) {
            if (l->selected_index > 0) {
                uint8_t old = l->selected_index;
                l->selected_index--;
                if (l->selected_index < l->scroll_offset)
                    l->scroll_offset = l->selected_index;
                list_start_anim(l, old);
            }
            return true;
        }
        if (evt->data.key == EUI_KEY_OK) {
            if (l->callback) l->callback(l->selected_index, l->callback_ctx);
            return true;
        }
    }
    if (evt->type == EUI_EVT_ENCODER_CW) {
        if (l->selected_index < l->item_count - 1) {
            uint8_t old = l->selected_index;
            l->selected_index++;
            if (l->selected_index >= l->scroll_offset + visible)
                l->scroll_offset++;
            list_start_anim(l, old);
        }
        return true;
    }
    if (evt->type == EUI_EVT_ENCODER_CCW) {
        if (l->selected_index > 0) {
            uint8_t old = l->selected_index;
            l->selected_index--;
            if (l->selected_index < l->scroll_offset)
                l->scroll_offset = l->selected_index;
            list_start_anim(l, old);
        }
        return true;
    }
    return false;
}

static void list_enter(eui_widget_t *self) {
    self->style |= EUI_STYLE_FOCUSED;
}

static void list_exit(eui_widget_t *self) {
    self->style &= ~EUI_STYLE_FOCUSED;
}

static void list_destroy(eui_widget_t *self);

static const eui_widget_vtable_t list_vtable = {
    .draw = list_draw, .input = list_input,
    .enter = list_enter, .exit = list_exit,
    .layout = NULL, .destroy = list_destroy
};

eui_widget_t* eui_list_create(int16_t x, int16_t y, uint16_t w, uint16_t h) {
    eui_list_t *l = eui_malloc(sizeof(eui_list_t));
    if (!l) return NULL;
    memset(l, 0, sizeof(*l));
    eui_widget_init(&l->widget, &list_vtable, x, y, w, h);
    l->widget.focus_policy = EUI_FOCUS_STRONG;
    l->item_height = ITEM_H;
    return &l->widget;
}

int eui_list_add_item(eui_widget_t *list, const char *text, const eui_bitmap_t *icon) {
    if (!list) return -1;
    eui_list_t *l = (eui_list_t*)list;
    if (l->item_count >= EUI_LIST_MAX_ITEMS) return -1;
    eui_str_set(&l->items[l->item_count].text, text ? text : "");
    l->items[l->item_count].icon = icon;
    l->item_count++;
    list->style |= EUI_STYLE_DIRTY;
    return l->item_count - 1;
}

void eui_list_set_selected(eui_widget_t *list, uint8_t index) {
    if (!list) return;
    eui_list_t *l = (eui_list_t*)list;
    if (index < l->item_count) l->selected_index = index;
}

uint8_t eui_list_get_selected(const eui_widget_t *list) {
    if (!list) return 0;
    return ((eui_list_t*)list)->selected_index;
}

void eui_list_set_callback(eui_widget_t *list, eui_list_callback_t cb, void *ctx) {
    if (!list) return;
    eui_list_t *l = (eui_list_t*)list;
    l->callback = cb;
    l->callback_ctx = ctx;
}

void eui_list_clear(eui_widget_t *list) {
    if (!list) return;
    eui_list_t *l = (eui_list_t*)list;
    for (uint8_t i = 0; i < l->item_count; i++) {
        eui_str_clear(&l->items[i].text);
    }
    l->item_count = 0;
    l->selected_index = 0;
    l->scroll_offset = 0;
    list->style |= EUI_STYLE_DIRTY;
}

static void list_destroy(eui_widget_t *self) {
    eui_list_t *l = (eui_list_t*)self;
    for (uint8_t i = 0; i < l->item_count; i++) {
        eui_str_clear(&l->items[i].text);
    }
    eui_free(l);
}
