#ifndef EUI_WIDGET_H
#define EUI_WIDGET_H

#include "eui/eui_view.h"
#include "eui/eui_canvas.h"
#include "eui/eui_config.h"
#include <stdint.h>
#include <stdbool.h>

#define EUI_FOCUS_NONE    0
#define EUI_FOCUS_TAB     1
#define EUI_FOCUS_STRONG  2

#define EUI_STYLE_VISIBLE   (1u << 0)
#define EUI_STYLE_ENABLED   (1u << 1)
#define EUI_STYLE_FOCUSED   (1u << 2)
#define EUI_STYLE_SELECTED  (1u << 3)
#define EUI_STYLE_PRESSED   (1u << 4)
#define EUI_STYLE_CHECKED   (1u << 5)
#define EUI_STYLE_DIRTY     (1u << 6)

typedef struct eui_widget_t eui_widget_t;
typedef struct eui_widget_vtable eui_widget_vtable_t;

struct eui_widget_vtable {
    void (*draw)(eui_widget_t *self, eui_canvas_t *canvas);
    bool (*input)(eui_widget_t *self, const eui_event_t *evt);
    void (*enter)(eui_widget_t *self);
    void (*exit)(eui_widget_t *self);
    void (*layout)(eui_widget_t *self);
    void (*destroy)(eui_widget_t *self);
};

struct eui_widget_t {
    eui_view_t view;
    eui_rect_t area;
    uint32_t   style;
    uint8_t    focus_policy;
    const eui_widget_vtable_t *vt;
    eui_widget_t *parent;
    eui_widget_t *children[EUI_MAX_WIDGET_CHILDREN];
    uint8_t    child_count;
    uint8_t    focus_index;
};

void eui_widget_init(eui_widget_t *w, const eui_widget_vtable_t *vt,
                     int16_t x, int16_t y, uint16_t ww, uint16_t hh);
void eui_widget_deinit(eui_widget_t *w);
void eui_widget_add_child(eui_widget_t *parent, eui_widget_t *child);
void eui_widget_remove_child(eui_widget_t *parent, eui_widget_t *child);
eui_widget_t* eui_widget_get_focus(const eui_widget_t *root);
eui_widget_t* eui_widget_focus_next(eui_widget_t *root);
eui_widget_t* eui_widget_focus_prev(eui_widget_t *root);
void eui_widget_set_focus(eui_widget_t *w);

static inline bool eui_widget_is_visible(const eui_widget_t *w) { return w->style & EUI_STYLE_VISIBLE; }
static inline bool eui_widget_is_enabled(const eui_widget_t *w) { return w->style & EUI_STYLE_ENABLED; }
static inline void eui_widget_set_visible(eui_widget_t *w, bool v) { if (v) w->style |= EUI_STYLE_VISIBLE; else w->style &= ~EUI_STYLE_VISIBLE; }
static inline void eui_widget_set_enabled(eui_widget_t *w, bool e) { if (e) w->style |= EUI_STYLE_ENABLED; else w->style &= ~EUI_STYLE_ENABLED; }

#define eui_widget_from_view(v) ((eui_widget_t*)(v))

#endif
