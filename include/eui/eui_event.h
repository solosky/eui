#ifndef EUI_EVENT_H
#define EUI_EVENT_H

#include "eui/eui_input_hal.h"
#include <stdint.h>
#include <stdbool.h>
#include "eui/eui_config.h"

/**
 * @brief Fixed-size, lock-free event queue.
 *
 * Used to decouple input processing from event consumption.
 * Supports optional overflow callback and overwrite policy.
 */
typedef struct {
    eui_event_t buffer[EUI_EVENT_QUEUE_SIZE]; /**< Circular buffer for events. */
    uint8_t head;       /**< Read index (dequeue from here). */
    uint8_t tail;       /**< Write index (enqueue to here). */
    uint8_t count;      /**< Number of events currently in the queue. */
    void (*overflow_callback)(void); /**< Called when a push fails due to full queue (may be NULL). */
    bool overwrite;     /**< If true, overwrite oldest event when full. */
} eui_event_queue_t;

/**
 * @brief Initialize an event queue to empty.
 *
 * @param q  Pointer to the queue to initialize.
 */
void eui_event_queue_init(eui_event_queue_t *q);

/**
 * @brief Push an event onto the queue.
 *
 * @param q   Pointer to the queue.
 * @param evt Pointer to the event to enqueue.
 * @return true on success, false if the queue was full (and overwrite is false).
 */
bool eui_event_queue_push(eui_event_queue_t *q, const eui_event_t *evt);

/**
 * @brief Pop an event from the queue (FIFO order).
 *
 * @param q   Pointer to the queue.
 * @param evt [out] Pointer to receive the dequeued event.
 * @return true if an event was dequeued, false if the queue was empty.
 */
bool eui_event_queue_pop(eui_event_queue_t *q, eui_event_t *evt);

/**
 * @brief Get the number of events currently in the queue.
 *
 * @param q  Pointer to the queue.
 * @return The number of queued events.
 */
uint8_t eui_event_queue_count(const eui_event_queue_t *q);

/**
 * @brief Check whether the queue is full.
 *
 * @param q  Pointer to the queue.
 * @return true if no more events can be pushed.
 */
bool eui_event_queue_is_full(const eui_event_queue_t *q);

/**
 * @brief Check whether the queue is empty.
 *
 * @param q  Pointer to the queue.
 * @return true if there are no events to pop.
 */
bool eui_event_queue_is_empty(const eui_event_queue_t *q);

/**
 * @brief Set a callback that fires when the queue overflows.
 *
 * The callback is invoked when eui_event_queue_push() fails (queue full
 * and overwrite is false), or when an existing event is discarded
 * (overwrite is true).
 *
 * @param q  Pointer to the queue.
 * @param cb Pointer to the overflow callback function (may be NULL).
 */
void eui_event_queue_set_overflow_callback(eui_event_queue_t *q, void (*cb)(void));

#endif /* EUI_EVENT_H */
