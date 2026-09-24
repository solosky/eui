#include "eui/eui_selector.h"
#include <string.h>

void eui_selector_init(eui_selector_t *s)
{
    memset(s, 0, sizeof(*s));
    s->config.move_in_loop = true;
    s->config.read_input_interval = 20;
    s->config.render_interval = 15;
    mc_transition2d_init(&s->selector_position);
    mc_transition2d_init(&s->selector_shape);
    mc_transition2d_init(&s->camera_offset);
    /* view 进入时 selector 从全屏收拢到选项的动画来源 */
    s->is_changed = true;
}

bool eui_selector_add_option(eui_selector_t *s, const eui_selector_option_t *opt)
{
    if (opt == NULL || s->option_count >= EUI_SELECTOR_MAX_OPTIONS)
        return false;
    s->options[s->option_count++] = opt;
    return true;
}

void eui_selector_clear_options(eui_selector_t *s) { s->option_count = 0; }

const eui_selector_option_t *eui_selector_get(const eui_selector_t *s, int idx)
{
    if (idx < 0 || idx >= s->option_count) return NULL;
    return s->options[idx];
}
const eui_selector_option_t *eui_selector_get_selected(const eui_selector_t *s)
{
    return eui_selector_get(s, s->selected_index);
}

void eui_selector_go_next(eui_selector_t *s)
{
    if (s->option_count == 0) return;
    s->selected_index++;
    if (s->selected_index >= s->option_count)
        s->selected_index = s->config.move_in_loop ? 0 : s->option_count - 1;
    /* 原版 SmoothSelector::goNext():_data.is_changed = true,让 update
     * 把 selector 移向新选项(简报 Step4 代码块漏了这句) */
    s->is_changed = true;
    if (s->on_go_next) s->on_go_next(s);
}

void eui_selector_go_last(eui_selector_t *s)
{
    if (s->option_count == 0) return;
    s->selected_index--;
    if (s->selected_index < 0)
        s->selected_index = s->config.move_in_loop ? s->option_count - 1 : 0;
    /* 原版 SmoothSelector::goLast():同上 */
    s->is_changed = true;
    if (s->on_go_last) s->on_go_last(s);
}

void eui_selector_glide_to(eui_selector_t *s, int idx)
{
    if (idx < 0 || idx >= s->option_count) return;
    s->selected_index = idx;
    const eui_selector_option_t *o = s->options[idx];
    mc_transition2d_glide_to(&s->selector_position, o->x, o->y);
    mc_transition2d_glide_to(&s->selector_shape, o->w, o->h);
}

void eui_selector_snap_to(eui_selector_t *s, int idx)
{
    if (idx < 0 || idx >= s->option_count) return;
    s->selected_index = idx;
    const eui_selector_option_t *o = s->options[idx];
    mc_transition2d_snap_to(&s->selector_position, o->x, o->y);
    mc_transition2d_snap_to(&s->selector_shape, o->w, o->h);
}

void eui_selector_press(eui_selector_t *s, const eui_selector_option_t *kf)
{
    s->is_pressing = true;
    mc_transition2d_glide_to(&s->selector_position, kf->x, kf->y);
    mc_transition2d_glide_to(&s->selector_shape, kf->w, kf->h);
}

void eui_selector_release(eui_selector_t *s)
{
    /* 原版:is_changed=true + was_released=true;is_pressing 保持到
     * update 里 transition 完成后随 onClick 一起清除 */
    s->is_changed = true;
    s->was_released = true;
}

void eui_selector_open(eui_selector_t *s, const eui_selector_option_t *kf)
{
    s->is_opening = true;
    s->was_opened = true;
    mc_transition2d_glide_to(&s->selector_position, kf->x, kf->y);
    mc_transition2d_glide_to(&s->selector_shape, kf->w, kf->h);
}

void eui_selector_close(eui_selector_t *s)
{
    s->is_changed = true; /* 对齐原版 close() */
    s->is_opening = false;
}

void eui_selector_update(eui_selector_t *s, uint32_t now_ms)
{
    if (s->on_update) s->on_update(s);

    if (now_ms - s->read_input_time_count > s->config.read_input_interval) {
        if (s->on_read_input) s->on_read_input(s);
        s->read_input_time_count = now_ms;
    }

    if (s->is_changed) {
        s->is_changed = false;
        const eui_selector_option_t *o = eui_selector_get_selected(s);
        if (o) {
            mc_transition2d_glide_to(&s->selector_position, o->x, o->y);
            mc_transition2d_glide_to(&s->selector_shape, o->w, o->h);
        }
        if (s->on_update_camera_keyframe) s->on_update_camera_keyframe(s);
    }
    mc_transition2d_update(&s->selector_position, now_ms);
    mc_transition2d_update(&s->selector_shape, now_ms);
    mc_transition2d_update(&s->camera_offset, now_ms);

    if (s->was_released) {
        if (mc_transition2d_is_finish(&s->selector_position) &&
            mc_transition2d_is_finish(&s->selector_shape)) {
            s->was_released = false;
            s->is_pressing = false;
            if (s->on_click) s->on_click(s);
        }
    }
    if (s->was_opened) {
        if (mc_transition2d_is_finish(&s->selector_position) &&
            mc_transition2d_is_finish(&s->selector_shape)) {
            s->was_opened = false;
            if (s->on_open_end) s->on_open_end(s);
        }
    }
}

void eui_selector_current_frame(const eui_selector_t *s, int *x, int *y, int *w, int *h)
{
    if (x) *x = mc_transition2d_x(&s->selector_position);
    if (y) *y = mc_transition2d_y(&s->selector_position);
    if (w) *w = mc_transition2d_x(&s->selector_shape);
    if (h) *h = mc_transition2d_y(&s->selector_shape);
}

int eui_selector_camera_x(const eui_selector_t *s)
{
    return mc_transition2d_x(&s->camera_offset);
}

int eui_selector_camera_y(const eui_selector_t *s)
{
    return mc_transition2d_y(&s->camera_offset);
}

void eui_selector_set_duration(eui_selector_t *s, uint32_t ms)
{
    mc_transition2d_set_duration(&s->selector_position, ms);
    mc_transition2d_set_duration(&s->selector_shape, ms);
}

void eui_selector_set_path(eui_selector_t *s, mc_easing_fn_t path)
{
    mc_transition2d_set_path(&s->selector_position, path);
    mc_transition2d_set_path(&s->selector_shape, path);
}
