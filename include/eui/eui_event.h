#ifndef EUI_EVENT_H
#define EUI_EVENT_H

#include "eui/eui_input_hal.h"
#include <stdint.h>
#include <stdbool.h>
#include "eui/eui_config.h"

typedef struct {
    eui_event_t buffer[EUI_EVENT_QUEUE_SIZE];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
    void (*overflow_callback)(void);
    bool overwrite;
} eui_event_queue_t;

void eui_event_queue_init(eui_event_queue_t *q);
bool eui_event_queue_push(eui_event_queue_t *q, const eui_event_t *evt);
bool eui_event_queue_pop(eui_event_queue_t *q, eui_event_t *evt);
uint8_t eui_event_queue_count(const eui_event_queue_t *q);
bool eui_event_queue_is_full(const eui_event_queue_t *q);
bool eui_event_queue_is_empty(const eui_event_queue_t *q);
void eui_event_queue_set_overflow_callback(eui_event_queue_t *q, void (*cb)(void));

#endif /* EUI_EVENT_H */
