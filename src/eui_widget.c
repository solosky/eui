#include "eui/eui_widget.h"
#include <string.h>
#include <stddef.h>

static bool eui_widget_event_bridge(eui_view_event_t *evt, void *ctx);

void eui_widget_init(eui_widget_t *w, const eui_widget_vtable_t *vt,
                     int16_t x, int16_t y, uint16_t ww, uint16_t hh) {
    memset(w, 0, sizeof(*w));
    w->vt = vt;
    w->area = (eui_rect_t){x, y, ww, hh};
    w->style = EUI_STYLE_VISIBLE | EUI_STYLE_ENABLED;
    w->focus_policy = EUI_FOCUS_NONE;
    eui_view_init(&w->view, eui_widget_event_bridge, w);
}

void eui_widget_deinit(eui_widget_t *w) {
    if (w->vt && w->vt->destroy) w->vt->destroy(w);
}

static bool eui_widget_event_bridge(eui_view_event_t *evt, void *ctx) {
    eui_widget_t *w = (eui_widget_t*)ctx;
    if (!w || !w->vt) return false;
    switch (evt->type) {
        case EUI_VIEW_EVT_DRAW:
            if (w->vt->draw) { w->vt->draw(w, evt->event.draw.canvas); return true; }
            break;
        case EUI_VIEW_EVT_INPUT:
            if (w->vt->input) return w->vt->input(w, evt->event.input.input);
            break;
        case EUI_VIEW_EVT_ENTER:
            if (w->vt->enter) { w->vt->enter(w); return true; }
            break;
        case EUI_VIEW_EVT_EXIT:
            if (w->vt->exit) { w->vt->exit(w); return true; }
            break;
        default: break;
    }
    return false;
}

static bool any_focused(const eui_widget_t *w) {
    if (!w) return false;
    if (w->style & EUI_STYLE_FOCUSED) return true;
    for (uint8_t i = 0; i < w->child_count; i++) {
        if (any_focused(w->children[i])) return true;
    }
    return false;
}

void eui_widget_add_child(eui_widget_t *parent, eui_widget_t *child) {
    if (!parent || !child || parent->child_count >= EUI_MAX_WIDGET_CHILDREN) return;
    child->parent = parent;
    parent->children[parent->child_count++] = child;
    if (child->focus_policy == EUI_FOCUS_STRONG) {
        eui_widget_t *root = parent;
        while (root->parent) root = root->parent;
        if (!any_focused(root)) {
            eui_widget_set_focus(child);
        }
    }
}

void eui_widget_remove_child(eui_widget_t *parent, eui_widget_t *child) {
    if (!parent || !child) return;
    for (uint8_t i = 0; i < parent->child_count; i++) {
        if (parent->children[i] == child) {
            for (uint8_t j = i; j < parent->child_count - 1; j++)
                parent->children[j] = parent->children[j + 1];
            parent->child_count--;
            child->parent = NULL;
            if (child->style & EUI_STYLE_FOCUSED) child->style &= ~EUI_STYLE_FOCUSED;
            return;
        }
    }
}

static void collect_focusable(const eui_widget_t *root, eui_widget_t **list, uint8_t *count) {
    if (!root) return;
    if (root->focus_policy == EUI_FOCUS_STRONG) {
        list[(*count)++] = (eui_widget_t*)root;
    }
    for (uint8_t i = 0; i < root->child_count; i++) {
        collect_focusable(root->children[i], list, count);
    }
}

static int find_focus_index(const eui_widget_t *root, eui_widget_t **list, uint8_t count) {
    (void)root;
    for (int i = 0; i < (int)count; i++) {
        if (list[i]->style & EUI_STYLE_FOCUSED) return i;
    }
    return -1;
}

static void clear_all_focus(eui_widget_t *root) {
    if (!root) return;
    root->style &= ~EUI_STYLE_FOCUSED;
    for (uint8_t i = 0; i < root->child_count; i++)
        clear_all_focus(root->children[i]);
}

eui_widget_t* eui_widget_get_focus(const eui_widget_t *root) {
    eui_widget_t *list[64];
    uint8_t count = 0;
    collect_focusable(root, list, &count);
    for (uint8_t i = 0; i < count; i++) {
        if (list[i]->style & EUI_STYLE_FOCUSED) return list[i];
    }
    return (count > 0) ? list[0] : NULL;
}

eui_widget_t* eui_widget_focus_next(eui_widget_t *root) {
    eui_widget_t *list[64];
    uint8_t count = 0;
    collect_focusable(root, list, &count);
    if (count == 0) return NULL;
    int cur = find_focus_index(root, list, count);
    int next = (cur + 1) % (int)count;
    clear_all_focus(root);
    list[next]->style |= EUI_STYLE_FOCUSED;
    return list[next];
}

eui_widget_t* eui_widget_focus_prev(eui_widget_t *root) {
    eui_widget_t *list[64];
    uint8_t count = 0;
    collect_focusable(root, list, &count);
    if (count == 0) return NULL;
    int cur = find_focus_index(root, list, count);
    int prev = (cur - 1 + count) % count;
    clear_all_focus(root);
    list[prev]->style |= EUI_STYLE_FOCUSED;
    return list[prev];
}

void eui_widget_set_focus(eui_widget_t *w) {
    if (!w) return;
    eui_widget_t *root = w;
    while (root->parent) root = root->parent;
    clear_all_focus(root);
    w->style |= EUI_STYLE_FOCUSED;
}
