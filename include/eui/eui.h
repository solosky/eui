#ifndef EUI_H
#define EUI_H

#include "eui/eui_config.h"
#include "eui/eui_types.h"
#include "eui/eui_allocator.h"
#include "eui/eui_str.h"
#include "eui/eui_display_hal.h"
#include "eui/eui_input_hal.h"
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

void eui_set_tick_callback(uint32_t (*tick_fn)(void));
uint32_t eui_get_tick_ms(void);

eui_view_dispatcher_t* eui_get_view_dispatcher(void);

#ifdef __cplusplus
}
#endif

#endif /* EUI_H */
