#include "eui/eui_widget_menu.h"
#include "eui/eui_allocator.h"
#include <string.h>

#define ITEM_H 12

static void menu_draw(eui_widget_t *self, eui_canvas_t *canvas) {
    eui_menu_t *m = (eui_menu_t*)self;
    eui_menu_t *active = m->active_submenu ? m->active_submenu : m;
    uint8_t visible = self->area.h / ITEM_H;
    if (visible == 0) visible = 1;

    eui_canvas_save(canvas);
    eui_canvas_set_clip(canvas, &self->area);

    for (uint8_t i = 0; i < visible && (i + active->scroll_offset) < active->item_count; i++) {
        uint8_t idx = i + active->scroll_offset;
        int16_t y = self->area.y + i * ITEM_H;
        eui_menu_item_t *item = &active->items[idx];

        if (idx == active->selected_index) {
            eui_canvas_invert_rect(canvas, self->area.x, y, self->area.w, ITEM_H);
        }
        eui_canvas_draw_str(canvas, self->area.x + 2, y + 2, item->label ? item->label : "");

        /* Draw submenu indicator */
        if (item->submenu) {
            eui_canvas_draw_str(canvas, self->area.x + self->area.w - 10, y + 2, ">");
        }
    }

    /* Breadcrumb hint at bottom */
    if (m->active_submenu) {
        eui_canvas_draw_str(canvas, self->area.x + 2,
                             self->area.y + self->area.h - ITEM_H, "< Back");
    }

    eui_canvas_restore(canvas);
}

static bool menu_input(eui_widget_t *self, const eui_event_t *evt) {
    eui_menu_t *m = (eui_menu_t*)self;
    eui_menu_t *active = m->active_submenu ? m->active_submenu : m;
    uint8_t visible = self->area.h / ITEM_H;
    if (visible == 0) visible = 1;

    if (evt->type == EUI_EVT_KEY_PRESS) {
        if (evt->data.key == EUI_KEY_DOWN) {
            if (active->selected_index < active->item_count - 1) active->selected_index++;
            if (active->selected_index >= active->scroll_offset + visible)
                active->scroll_offset++;
            return true;
        }
        if (evt->data.key == EUI_KEY_UP) {
            if (active->selected_index > 0) active->selected_index--;
            if (active->selected_index < active->scroll_offset)
                active->scroll_offset = active->selected_index;
            return true;
        }
        if (evt->data.key == EUI_KEY_OK) {
            eui_menu_item_t *item = &active->items[active->selected_index];
            if (item->submenu) {
                m->active_submenu = item->submenu;
                return true;
            }
            if (item->callback) item->callback(item->callback_ctx);
            return true;
        }
        if (evt->data.key == EUI_KEY_BACK) {
            if (m->active_submenu) {
                m->active_submenu = NULL;
                return true;
            }
        }
    }
    if (evt->type == EUI_EVT_ENCODER_CW) {
        if (active->selected_index < active->item_count - 1) active->selected_index++;
        if (active->selected_index >= active->scroll_offset + visible) active->scroll_offset++;
        return true;
    }
    if (evt->type == EUI_EVT_ENCODER_CCW) {
        if (active->selected_index > 0) active->selected_index--;
        if (active->selected_index < active->scroll_offset) active->scroll_offset = active->selected_index;
        return true;
    }
    return false;
}

static void menu_enter(eui_widget_t *self) { self->style |= EUI_STYLE_FOCUSED; }
static void menu_exit(eui_widget_t *self) { self->style &= ~EUI_STYLE_FOCUSED; }

static const eui_widget_vtable_t menu_vtable = {
    .draw = menu_draw, .input = menu_input,
    .enter = menu_enter, .exit = menu_exit,
    .layout = NULL, .destroy = NULL
};

eui_widget_t* eui_menu_create(int16_t x, int16_t y, uint16_t w, uint16_t h) {
    eui_menu_t *m = eui_malloc(sizeof(eui_menu_t));
    if (!m) return NULL;
    memset(m, 0, sizeof(*m));
    eui_widget_init(&m->widget, &menu_vtable, x, y, w, h);
    m->widget.focus_policy = EUI_FOCUS_STRONG;
    m->item_height = ITEM_H;
    return &m->widget;
}

eui_menu_item_t* eui_menu_add_item(eui_widget_t *menu, const char *label, eui_menu_callback_t cb) {
    if (!menu) return NULL;
    eui_menu_t *m = (eui_menu_t*)menu;
    if (m->item_count >= EUI_MENU_MAX_ITEMS) return NULL;
    eui_menu_item_t *item = &m->items[m->item_count++];
    item->label = label;
    item->callback = cb;
    item->submenu = NULL;
    item->callback_ctx = NULL;
    return item;
}

eui_menu_item_t* eui_menu_add_submenu(eui_widget_t *menu, const char *label) {
    if (!menu) return NULL;
    eui_menu_t *m = (eui_menu_t*)menu;
    eui_menu_item_t *item = eui_menu_add_item(menu, label, NULL);
    if (!item) return NULL;
    /* Create sub-menu */
    eui_menu_t *sub = eui_malloc(sizeof(eui_menu_t));
    if (!sub) return NULL;
    memset(sub, 0, sizeof(*sub));
    sub->parent_menu = m;
    sub->item_height = ITEM_H;
    item->submenu = sub;
    return item;
}

void eui_menu_back(eui_widget_t *menu) {
    if (!menu) return;
    eui_menu_t *m = (eui_menu_t*)menu;
    if (m->active_submenu) m->active_submenu = NULL;
}
