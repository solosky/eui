#ifndef EUI_SCENE_H
#define EUI_SCENE_H

#include "eui_view.h"
#include <stdint.h>

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
} eui_scene_t;

/** @brief Maximum number of scenes a scene manager can hold. */
#define EUI_SCENE_MAX 16

/**
 * @brief Simple scene stack / history manager.
 *
 * Tracks a current and previous scene to support back-navigation.
 */
typedef struct {
    eui_scene_t scenes[EUI_SCENE_MAX];
    uint8_t count;
    int8_t current;
    int8_t previous;
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
 * @brief Switch to a registered scene by ID.
 *
 * The current scene's on_exit callback is invoked (if set), then the
 * new scene's on_enter callback is called.
 *
 * @param sm       Pointer to the scene manager.
 * @param scene_id Numeric ID of the target scene (must be registered).
 */
void eui_scene_manager_switch(eui_scene_manager_t *sm, uint32_t scene_id);

/**
 * @brief Navigate back to the previous scene.
 *
 * @param sm  Pointer to the scene manager.
 */
void eui_scene_manager_back(eui_scene_manager_t *sm);

#endif /* EUI_SCENE_H */
