#ifndef EUI_INPUT_H
#define EUI_INPUT_H

#include "eui_input_drv.h"
#include "eui_event.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief High-level input manager with debounce, long-press and repeat.
 *
 * Wraps an eui_input_drv_t and provides polling, debouncing, key repeat,
 * and long-press detection.  Consumed events are placed into an internal
 * eui_event_queue_t for the application to read.
 */
typedef struct {
    eui_input_drv_t *hal;
    eui_event_queue_t queue;
    uint32_t last_poll_ms;
    uint32_t debounce_ms;
    uint32_t long_press_ms;
    uint32_t repeat_interval_ms;
    /* Debounce tracking for each key */
    struct {
        uint32_t last_change_ms;
        bool     pressed;
        bool     long_press_fired;
        uint32_t next_repeat_ms;
    } key_state[EUI_KEY_COUNT];
} eui_input_manager_t;

/**
 * @brief Initialize the input manager and bind it to a HAL backend.
 *
 * @param mgr  Pointer to the input manager to initialize.
 * @param hal  Pointer to the input HAL implementation.
 */
void eui_input_init(eui_input_manager_t *mgr, eui_input_drv_t *hal);

/**
 * @brief Set the debounce interval for key inputs.
 *
 * Consecutive state changes within this window are suppressed.
 *
 * @param mgr  Pointer to the input manager.
 * @param ms   Debounce time in milliseconds.
 */
void eui_input_set_debounce(eui_input_manager_t *mgr, uint32_t ms);

/**
 * @brief Set the long-press trigger interval.
 *
 * When a key is held longer than this duration, an EUI_EVT_KEY_REPEAT
 * event is generated.
 *
 * @param mgr  Pointer to the input manager.
 * @param ms   Long-press threshold in milliseconds.
 */
void eui_input_set_long_press(eui_input_manager_t *mgr, uint32_t ms);

/**
 * @brief Set the key repeat interval.
 *
 * After the long-press threshold, subsequent repeat events are generated
 * at this interval.
 *
 * @param mgr  Pointer to the input manager.
 * @param ms   Repeat interval in milliseconds.
 */
void eui_input_set_repeat_interval(eui_input_manager_t *mgr, uint32_t ms);

/**
 * @brief Poll the input HAL and update internal state.
 *
 * Should be called once per frame.  Reads raw input from the HAL,
 * applies debounce/long-press/repeat logic, and pushes resulting
 * events into the internal queue.
 *
 * @param mgr    Pointer to the input manager.
 * @param now_ms Current monotonic timestamp in milliseconds.
 */
void eui_input_update(eui_input_manager_t *mgr, uint32_t now_ms);

/**
 * @brief Dequeue a single processed input event.
 *
 * @param mgr   Pointer to the input manager.
 * @param event [out] Pointer to receive the dequeued event.
 * @return true if an event was dequeued, false if the queue was empty.
 */
bool eui_input_get_event(eui_input_manager_t *mgr, eui_event_t *event);

#endif /* EUI_INPUT_H */
