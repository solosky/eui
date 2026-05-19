#ifndef EUI_WIDGET_DIALOG_H
#define EUI_WIDGET_DIALOG_H

#include "eui/eui_widget.h"
#include "eui/eui_view_dispatcher.h"

typedef enum {
    EUI_DIALOG_OK,
    EUI_DIALOG_CANCEL,
    EUI_DIALOG_YES,
    EUI_DIALOG_NO
} eui_dialog_result_t;

typedef void (*eui_dialog_callback_t)(eui_dialog_result_t result, void *ctx);

#define EUI_DIALOG_MAX_BUTTONS 3

typedef struct {
    eui_widget_t widget;
    const char *title, *message;
    struct {
        const char *label;
        eui_dialog_result_t result;
    } buttons[EUI_DIALOG_MAX_BUTTONS];
    uint8_t button_count, focused_button;
    eui_dialog_callback_t callback;
    void *callback_ctx;
    eui_view_dispatcher_t *vd;
} eui_dialog_t;

eui_widget_t* eui_dialog_create(const char *title, const char *msg);
void eui_dialog_add_button(eui_widget_t *dlg, const char *label, eui_dialog_result_t result);
void eui_dialog_show(eui_widget_t *dlg, eui_view_dispatcher_t *vd, eui_dialog_callback_t cb);

#endif /* EUI_WIDGET_DIALOG_H */
