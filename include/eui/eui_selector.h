#ifndef EUI_SELECTOR_H
#define EUI_SELECTOR_H

#include <stdbool.h>
#include <stdint.h>
#include "mc_transition.h"

/**
 * @file eui_selector.h
 * @brief Animated option carousel/selector (SmoothSelector semantics in C).
 *
 * 选项轮播选择器：选中框 position/shape 与相机 offset 三个 mc_transition2d
 * 驱动，提供 press 挤压 / release 回弹（回弹完成触发 on_click）/ open 全屏 /
 * close 收拢动画与循环滚动。它不画任何东西——通过 current_frame()/camera_*
 * 读取动画值后由调用方在视图绘制回调里自行绘制。
 *
 * 移植自 smooth_ui_toolkit 的 SmoothSelector（C 重实现），动画能力来自
 * motionc 的 mc_transition（eui 静态库已导出其头文件与链接）。
 */

#define EUI_SELECTOR_MAX_OPTIONS 128

typedef struct {
    int x, y, w, h;               /**< 选项内容矩形与目标选中框 */
    void *user_data;
} eui_selector_option_t;

typedef struct {
    bool move_in_loop;            /**< goNext/goLast 回绕，默认 true */
    uint32_t read_input_interval; /**< on_read_input 节流 ms，默认 20 */
    uint32_t render_interval;     /**< 保留字段（渲染由 eui 驱动时无效果），默认 15 */
} eui_selector_config_t;

typedef struct eui_selector {
    /* 存储调用方指针而非拷贝：eui_selector_get_selected() 返回对象同一性
     * （原版 C++ 返回自身 vector 引用）。调用方需保证所指对象生命周期
     * 覆盖 selector 使用期。 */
    const eui_selector_option_t *options[EUI_SELECTOR_MAX_OPTIONS];
    int option_count;
    int selected_index;
    mc_transition2d_t selector_position; /* 选中框 x,y */
    mc_transition2d_t selector_shape;    /* 选中框 w,h */
    mc_transition2d_t camera_offset;     /* 相机偏移 x,y */
    eui_selector_config_t config;
    bool is_pressing, is_opening;
    bool is_changed;            /* 初始 true —— view 进入时收拢动画的来源 */
    bool was_released, was_opened;
    uint32_t read_input_time_count, render_time_count;
    /* 回调（对齐原版虚函数钩子，user_data 为第一参数） */
    void *user_data;
    void (*on_update)(struct eui_selector *s);
    void (*on_read_input)(struct eui_selector *s);
    void (*on_go_next)(struct eui_selector *s);
    void (*on_go_last)(struct eui_selector *s);
    void (*on_click)(struct eui_selector *s);
    void (*on_open_end)(struct eui_selector *s);
    void (*on_update_camera_keyframe)(struct eui_selector *s);
} eui_selector_t;

/**
 * @brief Initialize with defaults (loop on, throttle 20/15 ms, is_changed=true).
 */
void eui_selector_init(eui_selector_t *s);
bool eui_selector_add_option(eui_selector_t *s, const eui_selector_option_t *opt);
void eui_selector_clear_options(eui_selector_t *s);
const eui_selector_option_t *eui_selector_get(const eui_selector_t *s, int idx);
const eui_selector_option_t *eui_selector_get_selected(const eui_selector_t *s);
void eui_selector_go_next(eui_selector_t *s);
void eui_selector_go_last(eui_selector_t *s);
void eui_selector_glide_to(eui_selector_t *s, int idx);
void eui_selector_snap_to(eui_selector_t *s, int idx);
/** @brief 挤压到关键帧 @p kf（按下态）；release 后回弹选中项，落位触发 on_click。 */
void eui_selector_press(eui_selector_t *s, const eui_selector_option_t *kf);
void eui_selector_release(eui_selector_t *s);
/** @brief 展开到全屏关键帧 @p kf；动画落位触发 on_open_end。 */
void eui_selector_open(eui_selector_t *s, const eui_selector_option_t *kf);
void eui_selector_close(eui_selector_t *s);
/** @brief 帧驱动：节流回调 → is_changed 重定向 → 三过渡推进 → 落位回调。 */
void eui_selector_update(eui_selector_t *s, uint32_t now_ms);
/* 便捷读取：当前动画帧的选中框与相机偏移 */
void eui_selector_current_frame(const eui_selector_t *s, int *x, int *y, int *w, int *h);
int  eui_selector_camera_x(const eui_selector_t *s);
int  eui_selector_camera_y(const eui_selector_t *s);
void eui_selector_set_duration(eui_selector_t *s, uint32_t ms);       /* position+shape */
void eui_selector_set_path(eui_selector_t *s, mc_easing_fn_t path);   /* position+shape */

static inline bool eui_selector_is_pressing(const eui_selector_t *s) { return s->is_pressing; }
static inline bool eui_selector_is_opening(const eui_selector_t *s) { return s->is_opening; }

#endif /* EUI_SELECTOR_H */
