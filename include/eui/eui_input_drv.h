#ifndef EUI_INPUT_DRV_H
#define EUI_INPUT_DRV_H

#include <stdint.h>

/**
 * @brief Input event types.
 *
 * EUI_EVT_KEY_PRESS / _RELEASE / _REPEAT / EUI_EVT_ENCODER_* /
 * EUI_EVT_TOUCH_* are raw HAL events (assembler input).  The gesture
 * assembler in eui_input_edge turns raw streams into the assembled
 * gesture events EUI_EVT_KEY_CLICK / EUI_EVT_KEY_HOLD /
 * EUI_EVT_ENC_STEP, which views normally receive.
 *
 * Existing members keep their historical numeric values (the values are
 * part of the library ABI); new gesture types are appended.
 */
typedef enum {
    EUI_EVT_KEY_PRESS = 0,     /**< A key was pressed (raw). */
    EUI_EVT_KEY_RELEASE = 1,   /**< A key was released (raw). */
    EUI_EVT_KEY_REPEAT = 2,    /**< A key auto-repeat event (raw). */
    EUI_EVT_ENCODER_CW = 3,    /**< Rotary encoder turned clockwise, positive delta (raw). */
    EUI_EVT_ENCODER_CCW = 4,   /**< Rotary encoder turned counter-clockwise, negative delta (raw). */
    EUI_EVT_ENCODER_CLICK = 5, /**< Rotary encoder button click (raw). */
    EUI_EVT_TOUCH_DOWN = 6,    /**< Touch press detected (raw). */
    EUI_EVT_TOUCH_UP = 7,      /**< Touch release detected (raw). */
    EUI_EVT_TOUCH_MOVE = 8,    /**< Touch position changed (raw). */
    EUI_EVT_KEY_CLICK = 9,     /**< data.key_id: press->release without hold (assembled). */
    EUI_EVT_KEY_HOLD = 10,     /**< data.key_id: press held >= 500ms; swallows the click (assembled). */
    EUI_EVT_ENC_STEP = 11,     /**< data.enc_delta: encoder step, CW positive (assembled). */
} eui_event_type_t;

/**
 * @brief Unified input event structure.
 *
 * Carries one HAL-level input event with a timestamp.
 * The @p data union contains event-type-specific payload.
 * Key ids are neutral uint8_t numbers assigned by the project
 * (widget defaults and driver keymaps map them to roles).
 */
typedef struct {
    eui_event_type_t type; /**< Event type. */
    union {
        uint8_t   key_id;          /**< Neutral key id, project-defined (key events). */
        int16_t   enc_delta;       /**< Encoder step count (encoder events). */
        struct { int16_t x, y; } touch; /**< Touch coordinates (touch events). */
    } data;
    uint32_t timestamp; /**< Event timestamp in milliseconds. */
} eui_event_t;

/**
 * @brief Input HAL interface (abstracted driver).
 *
 * The application implements these callbacks to provide low-level
 * input from hardware (buttons, encoder, touch) to the EUI framework.
 */
typedef struct eui_input_drv_t {
    /**
     * @brief Initialize the input hardware.
     * @param user_data  The eui_input_drv_t::user_data pointer.
     * @return 0 on success, negative on error.
     */
    int  (*init)(void *user_data);

    /**
     * @brief Deinitialize the input hardware.
     * @param user_data  The eui_input_drv_t::user_data pointer.
     * @return 0 on success, negative on error.
     */
    int  (*deinit)(void *user_data);

    /**
     * @brief Poll for a single input event (non-blocking).
     * @param event     [out] Receives the next pending event.
     * @param user_data The eui_input_drv_t::user_data pointer.
     * @return 0 if an event was written, 1 if no events pending,
     *         negative on error.
     */
    int  (*poll)(eui_event_t *event, void *user_data);

    /**
     * @brief Register a callback-driven notification handler.
     *
     * If the hardware supports interrupts, the callback is invoked
     * from interrupt context when new input is available.
     *
     * @param cb         Callback to invoke with new events.
     * @param user_data  The eui_input_drv_t::user_data pointer.
     */
    void (*set_callback)(void (*cb)(const eui_event_t *evt), void *user_data);

    void *user_data; /**< Application-defined pointer passed to all callbacks. */
} eui_input_drv_t;

#endif /* EUI_INPUT_DRV_H */
