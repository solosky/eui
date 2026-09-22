#ifndef EUI_SCENE_H
#define EUI_SCENE_H

#include "eui_view.h"
#include <stdint.h>

struct eui_view_dispatcher_t;

/**
 * @brief Lifecycle callback invoked when a scene is entered.
 *
 * @param context  User context pointer associated with the scene.
 */
typedef void (*eui_scene_on_enter_t)(void *context);

/**
 * @brief Lifecycle callback invoked when a scene is exited.
 *
 * @param context  User context pointer associated with the scene.
 */
typedef void (*eui_scene_on_exit_t)(void *context);

/**
 * @brief Custom event handler for a scene.
 *
 * @param context  User context pointer.
 * @param event_id Application-defined event identifier.
 * @return true if the event was handled.
 */
typedef bool (*eui_scene_on_event_t)(void *context, uint32_t event_id);

/**
 * @brief Lifecycle callback invoked when a scene is re-exposed by pop().
 *
 * After pop() removes the top-of-stack scene, the newly exposed scene
 * below gets its view ENTER event (from the dispatcher or standalone
 * path) and then this callback — the place to replay an entrance
 * animation or refresh content without a full re-enter.
 *
 * @param context  User context pointer associated with the scene.
 */
typedef void (*eui_scene_on_resume_t)(void *context);

/**
 * @brief Descriptor for a single scene.
 *
 * A scene bundles a view with optional lifecycle callbacks.
 */
typedef struct {
    uint32_t scene_id;             /**< Unique numeric scene identifier. */
    eui_view_t *view;              /**< The view associated with this scene. */
    eui_scene_on_enter_t on_enter; /**< Called when the scene becomes active (may be NULL). */
    eui_scene_on_exit_t  on_exit;  /**< Called when the scene becomes inactive (may be NULL). */
    eui_scene_on_event_t on_event; /**< Called for custom events when this scene is active (may be NULL). */
    void *context;                 /**< User context passed to the callbacks (may be NULL). */
    eui_scene_on_resume_t on_resume; /**< Called when pop() re-exposes this scene (may be NULL). */
} eui_scene_t;

/** @brief Maximum number of scenes a scene manager can hold. */
#define EUI_SCENE_MAX 16

/** @brief Maximum navigation stack depth (root scene + pushed levels). */
#define EUI_SCENE_STACK_MAX EUI_SCENE_MAX

/**
 * @brief Scene navigation stack manager.
 *
 * The manager owns a stack of active scenes: stack[0] is the root scene,
 * each push() adds one level on top, pop() removes it and re-exposes the
 * level below (firing its on_resume). switch() resets the stack to a
 * single root scene.
 */
typedef struct {
    eui_scene_t scenes[EUI_SCENE_MAX];
    uint8_t count;
    int8_t current;              /**< Index of the active (top-of-stack) scene, -1 if none. */
    int8_t previous;             /**< Legacy: root scene before the last switch() (informational). */
    /** Optional dispatcher: when attached, switch()/push()/pop() route the
     *  view changes through the dispatcher (switch_to / push_overlay /
     *  pop_overlay) so the dispatcher stays in sync and can play transition
     *  animations; pushed scenes' views must be usable as overlays. Without
     *  an attached dispatcher the manager sends the views' ENTER/EXIT
     *  events directly. */
    struct eui_view_dispatcher_t *dispatcher;
    /** Navigation stack: indices into scenes[], stack[0] = root.
     *  depth == 1 means only the root scene is active. */
    uint8_t stack[EUI_SCENE_STACK_MAX];
    uint8_t depth;               /**< Number of scenes on the stack (0 before the first switch). */
} eui_scene_manager_t;

/**
 * @brief Register an array of scenes with the scene manager.
 *
 * The scenes array is copied into the manager's internal table.
 *
 * @param sm     Pointer to the scene manager.
 * @param scenes Array of eui_scene_t descriptors to register.
 * @param count  Number of elements in @p scenes.
 * @return 0 on success, or a negative error code if the table is full.
 */
int  eui_scene_manager_register(eui_scene_manager_t *sm, const eui_scene_t *scenes, uint8_t count);

/**
 * @brief Attach a view dispatcher to the scene manager.
 *
 * When attached, eui_scene_manager_switch() performs the actual view
 * change through eui_view_dispatcher_switch_to() (keeping the
 * dispatcher's routing state and transition logic in sync) and only
 * runs the scene-level on_enter/on_exit callbacks itself. push() and
 * pop() likewise route through eui_view_dispatcher_push_overlay() /
 * pop_overlay(). Without an attached dispatcher the manager sends the
 * views' ENTER/EXIT events directly.
 */
void eui_scene_manager_attach(eui_scene_manager_t *sm, struct eui_view_dispatcher_t *vd);

/**
 * @brief Switch to a registered scene, resetting the navigation stack.
 *
 * Any scenes pushed above the current root are torn down (top-down:
 * on_exit, then the view is popped/EXITed), then the current root's
 * on_exit runs, the new scene's on_enter runs, and the stack becomes
 * exactly [target]. With a dispatcher attached (see
 * eui_scene_manager_attach) the on_enter callback runs just before the
 * dispatcher makes the scene's view active so scene state is fresh for
 * the dispatcher's immediate draw.
 *
 * @param sm       Pointer to the scene manager.
 * @param scene_id Numeric ID of the target scene (must be registered).
 */
void eui_scene_manager_switch(eui_scene_manager_t *sm, uint32_t scene_id);

/**
 * @brief Push a scene on top of the navigation stack.
 *
 * The current top-of-stack scene gets on_exit (+ view EXIT); the pushed
 * scene's view is activated as the new top (through the dispatcher's
 * overlay stack when attached) and its on_enter runs. Typical use:
 * modal menus, dialogs, sub-screens that return with pop().
 *
 * @param sm       Pointer to the scene manager.
 * @param scene_id Numeric ID of the scene to push (must be registered).
 * @return 0 on success, or -1 if the manager stack or the attached
 *         dispatcher's overlay stack is full, or the scene is unknown.
 */
int  eui_scene_manager_push(eui_scene_manager_t *sm, uint32_t scene_id);

/**
 * @brief Pop the top-of-stack scene and re-expose the level below.
 *
 * The popped scene gets on_exit (+ view EXIT); the newly exposed scene
 * gets its view ENTER, then its on_resume callback runs. No-op when
 * only the root scene remains.
 *
 * @param sm  Pointer to the scene manager.
 * @return 0 on success, -1 if the stack was already at the root.
 */
int  eui_scene_manager_pop(eui_scene_manager_t *sm);

/**
 * @brief Navigate back: pop the top-of-stack scene.
 *
 * Legacy single-level back navigation, now an alias for
 * eui_scene_manager_pop().
 *
 * @param sm  Pointer to the scene manager.
 */
void eui_scene_manager_back(eui_scene_manager_t *sm);

/**
 * @brief Number of scenes currently on the navigation stack.
 *
 * depth() > 1 means at least one scene is pushed above the root.
 */
uint8_t eui_scene_manager_depth(const eui_scene_manager_t *sm);

/**
 * @brief Scene id of the current top-of-stack scene.
 *
 * @return The scene id, or (uint32_t)-1 if no scene is active.
 */
uint32_t eui_scene_manager_current_id(const eui_scene_manager_t *sm);

/**
 * @brief Scene descriptor at a given stack level.
 *
 * @param sm    Pointer to the scene manager.
 * @param level 0 = root scene, depth()-1 = top.
 * @return The scene descriptor, or NULL if out of range.
 */
const eui_scene_t *eui_scene_manager_scene_at(const eui_scene_manager_t *sm, uint8_t level);

#endif /* EUI_SCENE_H */
