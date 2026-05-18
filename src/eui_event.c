#include "eui/eui_event.h"
#include <string.h>

void eui_event_queue_init(eui_event_queue_t *q)
{
    memset(q, 0, sizeof(*q));
}

bool eui_event_queue_is_full(const eui_event_queue_t *q)
{
    return q->count == EUI_EVENT_QUEUE_SIZE;
}

bool eui_event_queue_is_empty(const eui_event_queue_t *q)
{
    return q->count == 0;
}

uint8_t eui_event_queue_count(const eui_event_queue_t *q)
{
    return q->count;
}

bool eui_event_queue_push(eui_event_queue_t *q, const eui_event_t *evt)
{
    if (eui_event_queue_is_full(q)) {
        if (!q->overwrite) {
            if (q->overflow_callback) {
                q->overflow_callback();
            }
            return false;
        }
        q->head = (q->head + 1) % EUI_EVENT_QUEUE_SIZE;
    }

    q->buffer[q->tail] = *evt;
    q->tail = (q->tail + 1) % EUI_EVENT_QUEUE_SIZE;

    if (!eui_event_queue_is_full(q)) {
        q->count++;
    }

    return true;
}

bool eui_event_queue_pop(eui_event_queue_t *q, eui_event_t *evt)
{
    if (eui_event_queue_is_empty(q)) {
        return false;
    }

    *evt = q->buffer[q->head];
    q->head = (q->head + 1) % EUI_EVENT_QUEUE_SIZE;
    q->count--;

    return true;
}

void eui_event_queue_set_overflow_callback(eui_event_queue_t *q, void (*cb)(void))
{
    q->overflow_callback = cb;
}
