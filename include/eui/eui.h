#ifndef EUI_H
#define EUI_H

#include "eui/eui_config.h"
#include "eui/eui_types.h"
#include "eui/eui_allocator.h"
#include "eui/eui_str.h"
#include "eui/eui_display_drv.h"
#include "eui/eui_input_drv.h"
#include "eui/eui_event.h"
#include "eui/eui_post.h"
#include "eui/eui_input.h"
#include "eui/eui_input_edge.h"
#include "eui/eui_canvas.h"
#include "eui/eui_font.h"
#include "eui/eui_view.h"
#include "eui/eui_view_dispatcher.h"
#include "eui/eui_scene.h"
#include "eui/eui_anim.h"
#include "eui/widget/eui_widget.h"
#include "eui/widget/eui_widget_label.h"
#include "eui/widget/eui_widget_button.h"
#include "eui/widget/eui_widget_list.h"
#include "eui/widget/eui_widget_menu.h"
#include "eui/widget/eui_widget_progress.h"
#include "eui/widget/eui_widget_slider.h"
#include "eui/widget/eui_widget_scroll.h"
#include "eui/widget/eui_widget_dialog.h"
#include "eui/driver/eui_drv_raylib.h"

#ifdef __cplusplus
extern "C" {
#endif

struct eui_display_drv_t;
struct eui_input_drv_t;

/**
 * @brief Library-level configuration parameters.
 *
 * Passed to eui_init() to bootstrap the entire UI framework.
 * All fields must be populated before calling eui_init().
 */
typedef struct {
    uint8_t  *mem_pool_buffer;   /**< Pointer to the memory pool for internal allocations. */
    size_t    mem_pool_size;      /**< Size in bytes of the memory pool buffer. */
    struct eui_display_drv_t *display;  /**< Display HAL implementation (cannot be NULL). */
    struct eui_input_drv_t   *input;    /**< Input HAL implementation (cannot be NULL; a board with several input devices must combine them with eui_input_mux). */
    uint16_t  fps_target;        /**< Desired frame rate in frames-per-second. */
    uint8_t   max_views;         /**< Maximum number of concurrent views. */
    uint8_t   max_animations;    /**< Maximum number of simultaneous animations. */
    uint8_t   max_widgets;       /**< Maximum number of widgets across all views. */
} eui_config_t;

/**
 * @brief Initialize the EUI library.
 *
 * Must be called once before any other EUI function.  Sets up the memory
 * allocator, display, input system, animation engine and view dispatcher
 * according to the provided configuration.
 *
 * @param config  Pointer to a fully populated eui_config_t structure.
 * @return 0 on success, or a negative error code on failure.
 *
 * @see eui_deinit()
 */
int  eui_init(const eui_config_t *config);

/**
 * @brief Deinitialize the EUI library and release all acquired resources.
 *
 * After this call no EUI function (other than eui_init()) may be used
 * until eui_init() is called again.
 */
void eui_deinit(void);

/**
 * @brief Advance the EUI state machine by one frame.
 *
 * Must be called periodically (typically once per main-loop iteration).
 * Internally processes input events, advances animations, and redraws
 * the active view.
 */
void eui_tick(void);

/**
 * @brief Check whether the main loop is still running.
 *
 * @return true if the dispatcher is active and the application should
 *         continue calling eui_tick(); false otherwise.
 */
bool eui_is_running(void);

/**
 * @brief Set the target frame rate.
 *
 * @param fps  Desired frames per second (e.g. 30, 60).
 *
 * @see eui_get_fps()
 */
void eui_set_fps(uint16_t fps);

/**
 * @brief Get the current target frame rate.
 *
 * @return The target FPS value set by eui_set_fps() or the default.
 *
 * @see eui_set_fps()
 */
uint16_t eui_get_fps(void);

/**
 * @brief Register a custom tick callback that returns the current time.
 *
 * The callback is invoked by eui_get_tick_ms() to obtain a monotonic
 * millisecond timestamp.  If not set, an internal default is used.
 *
 * @param tick_fn  Pointer to a function returning uint32_t milliseconds.
 *                 Passing NULL restores the default.
 *
 * @see eui_get_tick_ms()
 */
void eui_set_tick_callback(uint32_t (*tick_fn)(void));

/**
 * @brief Get the current tick time in milliseconds.
 *
 * @return Monotonic millisecond timestamp from the registered tick source.
 */
uint32_t eui_get_tick_ms(void);

/**
 * @brief Number of raw driver events waiting in the input manager queue.
 *
 * Diagnostic accessor: hosts can tell whether input drivers are producing
 * events (count > 0 right after a poll) versus events being lost further
 * down the pipeline (gesture edge / dispatcher).
 *
 * @return Pending raw event count (0..EUI_EVENT_QUEUE_SIZE).
 */
uint8_t eui_input_pending(void);

/**
 * @brief Get the global view dispatcher instance.
 *
 * @return Pointer to the library's internal eui_view_dispatcher_t.
 */
eui_view_dispatcher_t* eui_get_view_dispatcher(void);

/**
 * @brief core 唯一手势装配器（测试/特殊管线直接喂原始事件用）。
 *
 * @return Pointer to the library's internal eui_input_edge_t.
 */
eui_input_edge_t *eui_get_input_edge(void);

/**
 * @brief Register an "active" view dispatcher that overrides the internal one.
 *
 * Once set, eui_tick() routes input and renders through this dispatcher
 * instead of the library's internal one, and eui_get_input_edge() returns
 * its gesture edge.  Passing NULL restores the default behavior (internal
 * dispatcher).
 *
 * @param vd  Pointer to a caller-owned, fully initialized dispatcher, or NULL.
 *            The registered dispatcher's lifetime must cover the registration
 *            period; unregister it (pass NULL) before it goes out of scope,
 *            otherwise a dangling pointer is left behind.
 *
 * @see eui_get_active_dispatcher()
 */
void eui_set_active_dispatcher(eui_view_dispatcher_t *vd);

/**
 * @brief Get the currently active view dispatcher.
 *
 * @return The dispatcher registered with eui_set_active_dispatcher(), or the
 *         library's internal dispatcher when no active one is registered.
 */
eui_view_dispatcher_t* eui_get_active_dispatcher(void);

/**
 * @brief Get the global canvas owned by the library.
 *
 * The canvas is owned by EUI: it is created in eui_init() and destroyed in
 * eui_deinit().  Callers must not destroy it; external dispatchers may bind
 * to it for rendering.
 *
 * @return Pointer to the global canvas, or NULL if eui_init() has not been called.
 */
eui_canvas_t* eui_get_canvas(void);

/**
 * @brief Register a chrome-layer view dispatcher (e.g. the system status bar).
 *
 * Once set, eui_tick() draws the chrome dispatcher's active view at the very
 * end of the frame, after the main view (and any transition animation), so it
 * floats above them.  The chrome layer never receives input events.
 * Passing NULL hides the chrome layer.
 *
 * @param vd  Pointer to a caller-owned, fully initialized dispatcher, or NULL.
 *
 * @see eui_set_active_dispatcher()
 */
void eui_set_chrome_dispatcher(eui_view_dispatcher_t *vd);

/**
 * @brief Get the display driver instance passed to eui_init().
 *
 * @return Pointer to the display driver, or NULL if eui_init() has not been called.
 */
eui_display_drv_t* eui_get_display(void);

#ifdef __cplusplus
}
#endif

#endif /* EUI_H */
