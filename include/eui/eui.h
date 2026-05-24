#ifndef EUI_H
#define EUI_H

#include "eui/eui_config.h"
#include "eui/eui_types.h"
#include "eui/eui_allocator.h"
#include "eui/eui_str.h"
#include "eui/eui_display_drv.h"
#include "eui/eui_input_drv.h"
#include "eui/eui_event.h"
#include "eui/eui_input.h"
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
    struct eui_input_drv_t   *input;    /**< Input HAL implementation (may be NULL). */
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
 * @brief Get the global view dispatcher instance.
 *
 * @return Pointer to the library's internal eui_view_dispatcher_t.
 */
eui_view_dispatcher_t* eui_get_view_dispatcher(void);

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
