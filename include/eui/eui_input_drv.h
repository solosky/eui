#ifndef EUI_INPUT_DRV_H
#define EUI_INPUT_DRV_H

#include <stdint.h>

/**
 * @brief Low-level input event types reported by the HAL.
 */
typedef enum {
    EUI_EVT_KEY_PRESS,     /**< A key was pressed. */
    EUI_EVT_KEY_RELEASE,   /**< A key was released. */
    EUI_EVT_KEY_REPEAT,    /**< A key auto-repeat event. */
    EUI_EVT_ENCODER_CW,    /**< Rotary encoder turned clockwise. */
    EUI_EVT_ENCODER_CCW,   /**< Rotary encoder turned counter-clockwise. */
    EUI_EVT_ENCODER_CLICK, /**< Rotary encoder button click. */
    EUI_EVT_TOUCH_DOWN,    /**< Touch press detected. */
    EUI_EVT_TOUCH_UP,      /**< Touch release detected. */
    EUI_EVT_TOUCH_MOVE,    /**< Touch position changed. */
} eui_event_type_t;

/**
 * @brief Standard key identifiers.
 */
typedef enum {
    EUI_KEY_UP = 0,   /**< Up / previous. */
    EUI_KEY_DOWN,     /**< Down / next. */
    EUI_KEY_LEFT,     /**< Left / decrease. */
    EUI_KEY_RIGHT,    /**< Right / increase. */
    EUI_KEY_OK,       /**< OK / confirm / select. */
    EUI_KEY_BACK,     /**< Back / cancel / return. */
    EUI_KEY_COUNT     /**< Internal: number of key IDs (not a key). */
} eui_key_t;

/**
 * @brief Unified input event structure.
 *
 * Carries one HAL-level input event with a timestamp.
 * The @p data union contains event-type-specific payload.
 */
typedef struct {
    eui_event_type_t type; /**< Event type. */
    union {
        eui_key_t key;             /**< Key identifier (key events). */
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
