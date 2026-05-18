#include "eui/eui_allocator.h"
#include "tlsf.h"

#include <string.h>

static tlsf_t g_tlsf = NULL;
static eui_allocator_t g_alloc = { NULL, NULL, NULL };
static size_t g_alloc_count = 0;
static size_t g_free_count = 0;
static size_t g_peak_used = 0;
static size_t g_current_used = 0;

typedef struct {
    size_t total;
    size_t used;
} walk_ctx_t;

static void stats_walker(void *ptr, size_t size, int used, void *user)
{
    (void)ptr;
    walk_ctx_t *ctx = (walk_ctx_t *)user;
    ctx->total += size;
    if (used) {
        ctx->used += size;
    }
}

void eui_allocator_init_tlsf(uint8_t *buffer, size_t size)
{
    g_tlsf = tlsf_create_with_pool(buffer, size);
    memset(&g_alloc, 0, sizeof(g_alloc));
    g_alloc_count = 0;
    g_free_count = 0;
    g_peak_used = 0;
    g_current_used = 0;
}

void eui_set_allocator(const eui_allocator_t *allocator)
{
    if (allocator) {
        g_alloc = *allocator;
    } else {
        memset(&g_alloc, 0, sizeof(g_alloc));
    }
}

void* eui_malloc(size_t size)
{
    void *ptr;

    if (g_alloc.alloc) {
        ptr = g_alloc.alloc(size, g_alloc.ctx);
        if (ptr) {
            g_alloc_count++;
        }
        return ptr;
    }

    ptr = tlsf_malloc(g_tlsf, size);
    if (ptr) {
        g_alloc_count++;
        g_current_used += tlsf_block_size(ptr);
        if (g_current_used > g_peak_used) {
            g_peak_used = g_current_used;
        }
    }

    return ptr;
}

void eui_free(void *ptr)
{
    if (!ptr) {
        return;
    }

    g_free_count++;

    if (g_alloc.free) {
        g_alloc.free(ptr, g_alloc.ctx);
        return;
    }

    g_current_used -= tlsf_block_size(ptr);
    tlsf_free(g_tlsf, ptr);
}

void eui_allocator_get_stats(eui_allocator_stats_t *stats)
{
    stats->alloc_count = g_alloc_count;
    stats->free_count = g_free_count;

    if (g_tlsf) {
        pool_t pool = tlsf_get_pool(g_tlsf);
        walk_ctx_t ctx = { 0, 0 };
        tlsf_walk_pool(pool, stats_walker, &ctx);
        stats->total = ctx.total;
        stats->used = ctx.used;
    } else {
        stats->total = 0;
        stats->used = 0;
    }

    stats->peak = g_peak_used;
}
