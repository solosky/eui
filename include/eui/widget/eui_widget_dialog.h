#ifndef EUI_WIDGET_DIALOG_H
#define EUI_WIDGET_DIALOG_H

#include "eui/widget/eui_widget.h"
#include "eui/eui_view_dispatcher.h"
#include "eui/eui_str.h"

/**
 * @brief Result value returned when a dialog button is pressed.
 */
typedef enum {
    EUI_DIALOG_OK,     /**< OK / affirmative. */
    EUI_DIALOG_CANCEL, /**< Cancel / dismiss. */
    EUI_DIALOG_YES,    /**< Yes / confirm. */
    EUI_DIALOG_NO      /**< No / decline. */
} eui_dialog_result_t;

/**
 * @brief Dialog dismissal callback.
 *
 * Invoked when the user activates a dialog button.
 *
 * @param result  The result associated with the pressed button.
 * @param ctx     User context pointer.
 */
typedef void (*eui_dialog_callback_t)(eui_dialog_result_t result, void *ctx);

/** @brief Maximum number of buttons a dialog can display. */
#define EUI_DIALOG_MAX_BUTTONS 3

/**
 * @brief Dialog widget: a modal overlay with title, message, and buttons.
 *
 * The dialog is shown as an overlay on a view dispatcher and blocks
 * interaction with the underlying view until dismissed.
 */
typedef struct {
    eui_widget_t widget;
    eui_str_t title;            /**< Dialog title text. */
    eui_str_t message;          /**< Dialog body message text. */
    struct {
        eui_str_t label;
        eui_dialog_result_t result;
    } buttons[EUI_DIALOG_MAX_BUTTONS];
    uint8_t button_count, focused_button;
    eui_dialog_callback_t callback;
    void *callback_ctx;
    eui_view_dispatcher_t *vd;
} eui_dialog_t;

/**
 * @brief Create a dialog widget with a title and message.
 *
 * @param title  Title text (copied internally, may be NULL).
 * @param msg    Body message text (copied internally, may be NULL).
 * @return Pointer to the underlying eui_widget_t, or NULL on failure.
 */
eui_widget_t* eui_dialog_create(const char *title, const char *msg);

/**
 * @brief Add a button to the dialog.
 *
 * Buttons appear in the order they are added.  The dialog must have
 * at least one button to be dismissable.
 *
 * @param dlg    Pointer to the dialog widget.
 * @param label  Button label text (copied internally).
 * @param result Result value returned when this button is pressed.
 */
void eui_dialog_add_button(eui_widget_t *dlg, const char *label, eui_dialog_result_t result);

/**
 * @brief Display the dialog as an overlay on the given view dispatcher.
 *
 * The dialog registers itself as an overlay, receives input and draw
 * events, and invokes the callback when dismissed.
 *
 * @param dlg  Pointer to the dialog widget.
 * @param vd   Pointer to the view dispatcher to overlay onto.
 * @param cb   Dismissal callback (may be NULL).
 */
void eui_dialog_show(eui_widget_t *dlg, eui_view_dispatcher_t *vd, eui_dialog_callback_t cb);

#endif /* EUI_WIDGET_DIALOG_H */
