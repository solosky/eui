#include "eui/eui_widget_list.h"
#include "eui/eui_allocator.h"
#include "eui/eui_font_builtin.h"
#include <string.h>

#define ITEM_H 12

static void list_draw(eui_widget_t *self, eui_canvas_t *canvas) {
    eui_list_t *l = (eui_list_t*)self;
    uint8_t visible = self->area.h / ITEM_H;
    if (visible == 0) visible = 1;

    if (!canvas->font) {
        eui_canvas_set_font(canvas, &eui_font_builtin);
    }

    eui_canvas_save(canvas);
    eui_canvas_set_clip(canvas, &self->area);

    for (uint8_t i = 0; i < visible && (i + l->scroll_offset) < l->item_count; i++) {
        uint8_t idx = i + l->scroll_offset;
        int16_t y = self->area.y + i * ITEM_H;
        if (idx == l->selected_index) {
            eui_canvas_invert_rect(canvas, self->area.x, y, self->area.w, ITEM_H);
        }
        eui_canvas_draw_str(canvas, self->area.x + 2, y + 2, l->items[idx].text ? l->items[idx].text : "");
    }
    eui_canvas_restore(canvas);
}

static bool list_input(eui_widget_t *self, const eui_event_t *evt) {
    eui_list_t *l = (eui_list_t*)self;
    uint8_t visible = self->area.h / ITEM_H;
    if (visible == 0) visible = 1;

    if (evt->type == EUI_EVT_KEY_PRESS) {
        if (evt->data.key == EUI_KEY_DOWN) {
            if (l->selected_index < l->item_count - 1) l->selected_index++;
            if (l->selected_index >= l->scroll_offset + visible) l->scroll_offset++;
            return true;
        }
        if (evt->data.key == EUI_KEY_UP) {
            if (l->selected_index > 0) l->selected_index--;
            if (l->selected_index < l->scroll_offset) l->scroll_offset = l->selected_index;
            return true;
        }
        if (evt->data.key == EUI_KEY_OK) {
            if (l->callback) l->callback(l->selected_index, l->callback_ctx);
            return true;
        }
    }
    if (evt->type == EUI_EVT_ENCODER_CW) {
        if (l->selected_index < l->item_count - 1) l->selected_index++;
        if (l->selected_index >= l->scroll_offset + visible) l->scroll_offset++;
        return true;
    }
    if (evt->type == EUI_EVT_ENCODER_CCW) {
        if (l->selected_index > 0) l->selected_index--;
        if (l->selected_index < l->scroll_offset) l->scroll_offset = l->selected_index;
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

static const eui_widget_vtable_t list_vtable = {
    .draw = list_draw, .input = list_input,
    .enter = list_enter, .exit = list_exit,
    .layout = NULL, .destroy = NULL
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
    l->items[l->item_count].text = text;
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
    l->item_count = 0;
    l->selected_index = 0;
    l->scroll_offset = 0;
    list->style |= EUI_STYLE_DIRTY;
}
