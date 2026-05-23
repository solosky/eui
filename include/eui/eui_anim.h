#ifndef EUI_ANIM_H
#define EUI_ANIM_H

#include "eui/widget/eui_widget.h"
#include "mc.h"
#include <stdint.h>

/**
 * @brief Opaque handle representing a running animation instance.
 *
 * Returned by eui_anim_start() and eui_anim_start_spring().
 * Can be used to query or stop a specific animation.
 */
typedef uint8_t eui_anim_handle_t;

/**
 * @brief Identifies which widget property an animation should interpolate.
 */
typedef enum {
    EUI_ANIM_TARGET_X,        /**< Animate widget X position. */
    EUI_ANIM_TARGET_Y,        /**< Animate widget Y position. */
    EUI_ANIM_TARGET_WIDTH,    /**< Animate widget width. */
    EUI_ANIM_TARGET_HEIGHT,   /**< Animate widget height. */
    EUI_ANIM_TARGET_OPACITY,  /**< Animate widget opacity (0-255). */
    EUI_ANIM_TARGET_PROGRESS, /**< Animate progress bar value (0-100). */
    EUI_ANIM_TARGET_CUSTOM,   /**< User-defined property (callback-driven). */
} eui_anim_target_t;

/**
 * @brief Initialize the animation sub-system.
 *
 * Must be called once before any animation functions are used.
 * Typically invoked internally by eui_init().
 */
void eui_anim_init(void);

/**
 * @brief Start a tween (interpolated) animation on a widget property.
 *
 * The animation transitions the selected property from @p from to @p to
 * over the specified duration using the given easing function.
 *
 * @param target      The widget whose property will be animated.
 * @param prop        The property to animate (e.g. EUI_ANIM_TARGET_X).
 * @param from        Starting value of the property.
 * @param to          Ending value of the property.
 * @param duration_ms Duration of the animation in milliseconds.
 * @param easing      Easing function (from the mc easing library).
 *                    Determines the acceleration curve.
 * @param ctx         Opaque user pointer passed to @p on_done.
 * @param on_done     Optional callback invoked when the animation completes.
 *                    May be NULL.
 * @return A handle that can be used to stop or query the animation.
 *
 * @see eui_anim_stop()
 * @see eui_anim_is_running()
 * @see eui_anim_start_spring()
 */
eui_anim_handle_t eui_anim_start(eui_widget_t *target,
                                  eui_anim_target_t prop,
                                  int16_t from, int16_t to,
                                  uint16_t duration_ms,
                                  mc_easing_fn_t easing,
                                  void *ctx,
                                  void (*on_done)(void *ctx));

/**
 * @brief Start a spring-physics animation on a widget property.
 *
 * Instead of a fixed duration, the animation uses a damped spring model.
 * The widget property oscillates towards the target value and settles
 * based on the stiffness and damping parameters.
 *
 * @param target     The widget whose property will be animated.
 * @param prop       The property to animate.
 * @param to         Target value (resting position).
 * @param stiffness  Spring stiffness constant. Higher values = faster/stiffer.
 * @param damping    Damping coefficient. Higher values = less oscillation.
 * @param on_done    Optional completion callback (may be NULL).
 * @return A handle that can be used to stop or query the animation.
 *
 * @see eui_anim_stop()
 * @see eui_anim_start()
 */
eui_anim_handle_t eui_anim_start_spring(eui_widget_t *target,
                                         eui_anim_target_t prop,
                                         int16_t to,
                                         float stiffness,
                                         float damping,
                                         void (*on_done)(void *ctx));

/**
 * @brief Stop a running animation by handle.
 *
 * The animation is immediately terminated. The @p on_done callback
 * is NOT invoked.
 *
 * @param handle  The handle returned by eui_anim_start() or
 *                eui_anim_start_spring().  If the handle is invalid
 *                or the animation already finished, this is a no-op.
 */
void eui_anim_stop(eui_anim_handle_t handle);

/**
 * @brief Stop all animations currently running on the specified widget.
 *
 * @param target  The widget whose animations should be cancelled.
 */
void eui_anim_stop_all(eui_widget_t *target);

/**
 * @brief Check whether an animation is still running.
 *
 * @param handle  The animation handle to query.
 * @return true if the animation is active, false if it has completed
 *         or the handle is invalid.
 */
bool eui_anim_is_running(eui_anim_handle_t handle);

/**
 * @brief Advance all active animations by the given time delta.
 *
 * Must be called once per frame from the main loop.
 *
 * @param delta_ms  Elapsed time in milliseconds since the last update.
 */
void eui_anim_update(uint32_t delta_ms);

#endif /* EUI_ANIM_H */
