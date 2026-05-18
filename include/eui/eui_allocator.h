#ifndef EUI_ALLOCATOR_H
#define EUI_ALLOCATOR_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    void* (*alloc)(size_t size, void *ctx);
    void  (*free)(void *ptr, void *ctx);
    void* ctx;
} eui_allocator_t;

void eui_set_allocator(const eui_allocator_t *allocator);
void* eui_malloc(size_t size);
void  eui_free(void *ptr);

void eui_allocator_init_tlsf(uint8_t *buffer, size_t size);

typedef struct {
    size_t total;
    size_t used;
    size_t peak;
    size_t alloc_count;
    size_t free_count;
} eui_allocator_stats_t;

void eui_allocator_get_stats(eui_allocator_stats_t *stats);

#endif /* EUI_ALLOCATOR_H */
