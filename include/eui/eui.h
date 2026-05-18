#ifndef EUI_H
#define EUI_H

#include "eui/eui_config.h"
#include "eui/eui_types.h"
#include "eui/eui_allocator.h"
#include "eui/eui_display_hal.h"
#include "eui/eui_input_hal.h"
#include "eui/eui_event.h"
#include "eui/eui_input.h"
#include "eui/eui_canvas.h"
#include "eui/eui_font.h"
#include "eui/eui_view.h"
/* TODO: #include "eui/eui_view_dispatcher.h" */
/* TODO: #include "eui/eui_scene.h" */
/* TODO: #include "eui/eui_anim.h" */
/* TODO: #include "eui/eui_widget.h" */
/* TODO: #include "eui/eui_widget_label.h" */
/* TODO: #include "eui/eui_widget_button.h" */
/* TODO: #include "eui/eui_widget_list.h" */
/* TODO: #include "eui/eui_widget_menu.h" */
/* TODO: #include "eui/eui_widget_progress.h" */
/* TODO: #include "eui/eui_widget_slider.h" */
/* TODO: #include "eui/eui_widget_scroll.h" */
/* TODO: #include "eui/eui_widget_dialog.h" */

#ifdef __cplusplus
extern "C" {
#endif

struct eui_display_hal_t;
struct eui_input_hal_t;

typedef struct {
    uint8_t  *mem_pool_buffer;
    size_t    mem_pool_size;
    struct eui_display_hal_t *display;
    struct eui_input_hal_t   *input;
    uint16_t  fps_target;
    uint8_t   max_views;
    uint8_t   max_animations;
    uint8_t   max_widgets;
} eui_config_t;

int  eui_init(const eui_config_t *config);
void eui_deinit(void);
void eui_tick(void);
bool eui_is_running(void);
void eui_set_fps(uint16_t fps);
uint16_t eui_get_fps(void);

#ifdef __cplusplus
}
#endif

#endif /* EUI_H */
