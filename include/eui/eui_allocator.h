#ifndef EUI_ALLOCATOR_H
#define EUI_ALLOCATOR_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Allocator interface (strategy pattern).
 *
 * Allows the application to substitute the memory allocator used
 * by all EUI internal allocations.
 */
typedef struct {
    /**
     * @brief Allocate a block of memory.
     * @param size  Requested size in bytes.
     * @param ctx   User context from eui_allocator_t::ctx.
     * @return Pointer to the allocated memory, or NULL on failure.
     */
    void* (*alloc)(size_t size, void *ctx);
    /**
     * @brief Free a previously allocated block.
     * @param ptr  Pointer returned by the alloc callback.
     * @param ctx  User context from eui_allocator_t::ctx.
     */
    void  (*free)(void *ptr, void *ctx);
    void* ctx; /**< Opaque user context passed to alloc/free. */
} eui_allocator_t;

/**
 * @brief Replace the global allocator with a custom one.
 *
 * Must be called before eui_init() to take effect.  Passing NULL
 * restores the built-in allocator.
 *
 * @param allocator  Pointer to the custom allocator, or NULL to reset.
 */
void eui_set_allocator(const eui_allocator_t *allocator);

/**
 * @brief Allocate memory through the current global allocator.
 *
 * @param size  Requested size in bytes.
 * @return Pointer to the allocated memory, or NULL on failure.
 */
void* eui_malloc(size_t size);

/**
 * @brief Free memory previously returned by eui_malloc().
 *
 * @param ptr  Pointer to free.  NULL is safe (no-op).
 */
void  eui_free(void *ptr);

/**
 * @brief Initialize the built-in TLSF (Two-Level Segregated Fit) allocator.
 *
 * Configures the allocator to manage a fixed-size memory pool, suitable
 * for embedded systems without heap fragmentation guarantees.
 *
 * @param buffer  Pointer to the memory pool region.
 * @param size    Size of the pool in bytes.
 */
void eui_allocator_init_tlsf(uint8_t *buffer, size_t size);

/**
 * @brief Allocation statistics snapshot.
 */
typedef struct {
    size_t total;        /**< Total pool size in bytes. */
    size_t used;         /**< Currently allocated bytes. */
    size_t peak;         /**< Peak allocated bytes since init. */
    size_t alloc_count;  /**< Total number of successful allocations. */
    size_t free_count;   /**< Total number of frees. */
} eui_allocator_stats_t;

/**
 * @brief Retrieve allocator statistics.
 *
 * @param stats  [out] Pointer to receive the statistics snapshot.
 */
void eui_allocator_get_stats(eui_allocator_stats_t *stats);

#endif /* EUI_ALLOCATOR_H */
