# EUI API 参考

## 框架生命周期

### eui_init

```c
int eui_init(const eui_config_t *config);
```

初始化 EUI 框架。必须在任何其他 API 调用之前调用。

**参数：**

| 字段 | 类型 | 说明 |
|------|------|------|
| `config->mem_pool_buffer` | `uint8_t*` | TLSF 内存池缓冲区指针 |
| `config->mem_pool_size` | `size_t` | 内存池大小（字节） |
| `config->display` | `eui_display_hal_t*` | 显示 HAL 接口 |
| `config->input` | `eui_input_hal_t*` | 输入 HAL 接口 |
| `config->fps_target` | `uint16_t` | 目标帧率，默认 30 |
| `config->max_views` | `uint8_t` | 最大 View 数，默认 8 |
| `config->max_widgets` | `uint8_t` | 最大 Widget 数，默认 32 |

**返回：** 成功返回 0，失败返回 -1。

### eui_tick

```c
void eui_tick(void);
```

框架主循环心跳。每帧调用一次，驱动动画更新、输入轮询、事件分发和渲染。

### eui_deinit

```c
void eui_deinit(void);
```

反初始化框架，释放资源。

### eui_set_tick_callback

```c
void eui_set_tick_callback(uint32_t (*tick_fn)(void));
```

设置毫秒级定时器回调。框架内部通过此回调获取时间。

```c
// STM32 示例
eui_set_tick_callback(HAL_GetTick);
```

---

## 内存管理

### eui_malloc / eui_free

```c
void* eui_malloc(size_t size);
void  eui_free(void *ptr);
```

框架统一内存分配/释放。默认使用 TLSF 分配器。

### eui_set_allocator

```c
void eui_set_allocator(const eui_allocator_t *allocator);
```

替换全局分配器为自定义实现。传入 NULL 恢复 TLSF 默认。

```c
typedef struct {
    void* (*alloc)(size_t size, void *ctx);
    void  (*free)(void *ptr, void *ctx);
    void* ctx;
} eui_allocator_t;
```

---

## Canvas 绘图

### 创建与销毁

```c
eui_canvas_t* eui_canvas_create(eui_display_hal_t *display);
void eui_canvas_destroy(eui_canvas_t *canvas);
void eui_canvas_reset(eui_canvas_t *canvas);
void eui_canvas_commit(eui_canvas_t *canvas);
```

`commit` 将缓冲区内容刷新到显示设备。

### 属性设置

```c
void eui_canvas_set_color(eui_canvas_t *canvas, eui_color_t color);
void eui_canvas_set_bg_color(eui_canvas_t *canvas, eui_color_t color);
void eui_canvas_set_font(eui_canvas_t *canvas, const eui_font_t *font);
void eui_canvas_set_clip(eui_canvas_t *canvas, const eui_rect_t *rect);
void eui_canvas_clear_clip(eui_canvas_t *canvas);
void eui_canvas_save(eui_canvas_t *canvas);
void eui_canvas_restore(eui_canvas_t *canvas);
```

`save/restore` 保存/恢复当前裁剪区域和颜色状态（栈深度 4）。

### 绘图原语

```c
void eui_canvas_clear(eui_canvas_t *canvas);
void eui_canvas_draw_dot(eui_canvas_t *canvas, int16_t x, int16_t y);
void eui_canvas_draw_line(eui_canvas_t *canvas, int16_t x1, int16_t y1,
                          int16_t x2, int16_t y2);
void eui_canvas_draw_rect(eui_canvas_t *canvas, int16_t x, int16_t y,
                          uint16_t w, uint16_t h);
void eui_canvas_fill_rect(eui_canvas_t *canvas, int16_t x, int16_t y,
                          uint16_t w, uint16_t h);
void eui_canvas_draw_circle(eui_canvas_t *canvas, int16_t x, int16_t y, uint16_t r);
void eui_canvas_fill_circle(eui_canvas_t *canvas, int16_t x, int16_t y, uint16_t r);
void eui_canvas_draw_triangle(eui_canvas_t *canvas, int16_t x1, int16_t y1,
                              int16_t x2, int16_t y2, int16_t x3, int16_t y3);
void eui_canvas_draw_round_rect(eui_canvas_t *canvas, int16_t x, int16_t y,
                                uint16_t w, uint16_t h, uint16_t r);
void eui_canvas_fill_round_rect(eui_canvas_t *canvas, int16_t x, int16_t y,
                                uint16_t w, uint16_t h, uint16_t r);
```

### 文本

```c
uint16_t eui_canvas_draw_str(eui_canvas_t *canvas, int16_t x, int16_t y,
                             const char *str);
uint16_t eui_canvas_draw_str_aligned(eui_canvas_t *canvas, int16_t x, int16_t y,
                                     eui_align_t h_align, eui_align_t v_align,
                                     const char *str);
uint16_t eui_canvas_str_width(const eui_canvas_t *canvas, const char *str);
uint16_t eui_canvas_font_height(const eui_canvas_t *canvas);
```

对齐值组合示例：`EUI_ALIGN_CENTER | EUI_ALIGN_MIDDLE`

### 图像

```c
void eui_canvas_draw_xbm(eui_canvas_t *canvas, int16_t x, int16_t y,
                         uint16_t w, uint16_t h, const uint8_t *data);
void eui_canvas_draw_bitmap(eui_canvas_t *canvas, int16_t x, int16_t y,
                            const eui_bitmap_t *bmp);
void eui_canvas_invert_rect(eui_canvas_t *canvas, int16_t x, int16_t y,
                            uint16_t w, uint16_t h);
```

---

## 显示 HAL

```c
typedef struct {
    eui_display_caps_t caps;
    int  (*init)(void *user_data);
    int  (*deinit)(void *user_data);
    void (*draw_pixel)(int16_t x, int16_t y, eui_color_t color, void *user_data);
    void (*write_buffer)(const uint8_t *buffer, const eui_rect_t *rect, void *user_data);
    void (*set_contrast)(uint8_t level, void *user_data);
    void (*set_power)(bool on, void *user_data);
    void (*set_invert)(bool invert, void *user_data);
    void (*fill_rect)(int16_t x, int16_t y, uint16_t w, uint16_t h,
                      eui_color_t color, void *user_data);
    void *user_data;
} eui_display_hal_t;
```

缓冲模式：

| 模式 | 值 | 内存需求 | 适用场景 |
|------|---|---------|---------|
| `EUI_BUFFER_FULL` | 1 | W×H×bpp/8 | 有 GRAM 的屏幕或外接 SRAM |
| `EUI_BUFFER_PAGE` | 2 | W×8×bpp/8 (~1KB) | 小内存 MCU + 大分辨率 |
| `EUI_BUFFER_DIRECT` | 4 | 0 | 逐像素直接写入 |

---

## 输入 HAL

```c
typedef enum {
    EUI_KEY_UP, EUI_KEY_DOWN, EUI_KEY_LEFT, EUI_KEY_RIGHT,
    EUI_KEY_OK, EUI_KEY_BACK
} eui_key_t;

typedef struct {
    eui_event_type_t type;
    union {
        eui_key_t key;                   // KEY 事件
        int16_t   enc_delta;             // ENCODER 事件
        struct { int16_t x, y; } touch;  // TOUCH 事件
    } data;
    uint32_t timestamp;
} eui_event_t;

typedef struct {
    int  (*init)(void *user_data);
    int  (*deinit)(void *user_data);
    int  (*poll)(eui_event_t *event, void *user_data);
    void (*set_callback)(void (*cb)(const eui_event_t *), void *user_data);
    void *user_data;
} eui_input_hal_t;
```

事件类型：`EUI_EVT_KEY_PRESS` / `KEY_RELEASE` / `KEY_REPEAT` / `ENCODER_CW` / `ENCODER_CCW` / `TOUCH_DOWN` / `TOUCH_UP` / `TOUCH_MOVE`

---

## View 系统

```c
typedef struct {
    eui_view_event_type_t type;
    union {
        struct { eui_canvas_t *canvas; void *model; } draw;
        struct { const eui_event_t *input; } input;
        uint32_t nav_id;                    // navigate
        struct { uint32_t id; void *data; } custom;
    } event;
} eui_view_event_t;

typedef bool (*eui_view_handler_t)(eui_view_event_t *event, void *context);
```

```c
void eui_view_init(eui_view_t *view, eui_view_handler_t handler, void *context);
void eui_view_set_model(eui_view_t *view, void *model);
bool eui_view_send_draw(eui_view_t *view, eui_canvas_t *canvas);
bool eui_view_send_input(eui_view_t *view, const eui_event_t *evt);
bool eui_view_send_enter(eui_view_t *view);
bool eui_view_send_exit(eui_view_t *view);
```

### ViewDispatcher

```c
void eui_view_dispatcher_init(eui_view_dispatcher_t *vd, eui_canvas_t *canvas);
int  eui_view_dispatcher_add(eui_view_dispatcher_t *vd, uint32_t id, eui_view_t *view);
void eui_view_dispatcher_switch_to(eui_view_dispatcher_t *vd, uint32_t id, eui_anim_type_t anim);

/* Overlay 操作 */
int  eui_view_dispatcher_push_overlay(eui_view_dispatcher_t *vd, eui_view_t *overlay,
                                       eui_anim_type_t anim);
void eui_view_dispatcher_pop_overlay(eui_view_dispatcher_t *vd, eui_anim_type_t anim);

/* 事件分发 */
void eui_view_dispatcher_send_input(eui_view_dispatcher_t *vd, const eui_event_t *evt);
void eui_view_dispatcher_tick(eui_view_dispatcher_t *vd);
```

### SceneManager

```c
int  eui_scene_manager_register(eui_scene_manager_t *sm,
                                 const eui_scene_t *scenes, uint8_t count);
void eui_scene_manager_switch(eui_scene_manager_t *sm, uint32_t scene_id);
void eui_scene_manager_back(eui_scene_manager_t *sm);
```

---

## Widget 控件库

### 通用操作

```c
void eui_widget_init(eui_widget_t *w, const eui_widget_vtable_t *vt,
                     int16_t x, int16_t y, uint16_t w, uint16_t h);
void eui_widget_add_child(eui_widget_t *parent, eui_widget_t *child);

/* 焦点 */
eui_widget_t* eui_widget_get_focus(const eui_widget_t *root);
eui_widget_t* eui_widget_focus_next(eui_widget_t *root);
eui_widget_t* eui_widget_focus_prev(eui_widget_t *root);
void eui_widget_set_focus(eui_widget_t *w);
```

样式标志：`EUI_STYLE_VISIBLE` | `ENABLED` | `FOCUSED` | `SELECTED` | `PRESSED` | `DIRTY`

焦点策略：`EUI_FOCUS_NONE` (0) | `EUI_FOCUS_TAB` (1) | `EUI_FOCUS_STRONG` (2)

### Label

```c
eui_widget_t* eui_label_create(const char *text, int16_t x, int16_t y);
void eui_label_set_text(eui_widget_t *label, const char *text);
void eui_label_set_align(eui_widget_t *label, eui_align_t h, eui_align_t v);
```

### Button

```c
typedef void (*eui_button_callback_t)(void *ctx);

eui_widget_t* eui_button_create(const char *label, int16_t x, int16_t y,
                                 uint16_t w, uint16_t h);
void eui_button_set_callback(eui_widget_t *btn, eui_button_callback_t cb, void *ctx);
void eui_button_set_bitmap(eui_widget_t *btn, const eui_bitmap_t *bmp);
```

### List

```c
typedef void (*eui_list_callback_t)(uint8_t index, void *ctx);

eui_widget_t* eui_list_create(int16_t x, int16_t y, uint16_t w, uint16_t h);
int  eui_list_add_item(eui_widget_t *list, const char *text, const eui_bitmap_t *icon);
void eui_list_set_selected(eui_widget_t *list, uint8_t index);
uint8_t eui_list_get_selected(const eui_widget_t *list);
void eui_list_set_callback(eui_widget_t *list, eui_list_callback_t cb, void *ctx);
void eui_list_clear(eui_widget_t *list);
```

### Menu

```c
typedef void (*eui_menu_callback_t)(void *ctx);

eui_widget_t* eui_menu_create(int16_t x, int16_t y, uint16_t w, uint16_t h);
eui_menu_item_t* eui_menu_add_item(eui_widget_t *menu, const char *label,
                                    eui_menu_callback_t cb);
eui_menu_item_t* eui_menu_add_submenu(eui_widget_t *menu, const char *label);
void eui_menu_back(eui_widget_t *menu);
```

### Progress / Slider / Scroll / Dialog

```c
// Progress
eui_widget_t* eui_progress_create(int16_t x, int16_t y, uint16_t w, uint16_t h);
void eui_progress_set_value(eui_widget_t *prog, uint8_t percent);
void eui_progress_set_indeterminate(eui_widget_t *prog, bool indet);

// Slider
eui_widget_t* eui_slider_create(int16_t x, int16_t y, uint16_t w, uint16_t h);
void eui_slider_set_range(eui_widget_t *slider, int16_t min, int16_t max);
void eui_slider_set_value(eui_widget_t *slider, int16_t value);
int16_t eui_slider_get_value(const eui_widget_t *slider);

// ScrollContainer
eui_widget_t* eui_scroll_create(int16_t x, int16_t y, uint16_t w, uint16_t h);
void eui_scroll_set_content_size(eui_widget_t *scroll, uint16_t cw, uint16_t ch);
void eui_scroll_add_child(eui_widget_t *scroll, eui_widget_t *child);

// Dialog
eui_widget_t* eui_dialog_create(const char *title, const char *msg);
void eui_dialog_add_button(eui_widget_t *dlg, const char *label, eui_dialog_result_t result);
void eui_dialog_show(eui_widget_t *dlg, eui_view_dispatcher_t *vd, eui_dialog_callback_t cb);
```

Dialog 回调类型：`EUI_DIALOG_OK` / `CANCEL` / `YES` / `NO`

---

## 动画

```c
void eui_anim_init(void);

eui_anim_handle_t eui_anim_start(eui_widget_t *target,
                                  eui_anim_target_t prop,
                                  int16_t from, int16_t to,
                                  uint16_t duration_ms,
                                  mc_easing_fn_t easing,
                                  void *ctx,
                                  void (*on_done)(void *ctx));

eui_anim_handle_t eui_anim_start_spring(eui_widget_t *target,
                                         eui_anim_target_t prop,
                                         int16_t to,
                                         float stiffness,
                                         float damping,
                                         void (*on_done)(void *ctx));

void eui_anim_stop(eui_anim_handle_t handle);
void eui_anim_stop_all(eui_widget_t *target);
bool eui_anim_is_running(eui_anim_handle_t handle);
```

可动画属性：`EUI_ANIM_TARGET_X` / `Y` / `WIDTH` / `HEIGHT` / `OPACITY` / `PROGRESS` / `CUSTOM`

MotionC 预定义缓动函数（30个）：`mc_ease_linear`, `mc_ease_cubic_in`, `mc_ease_cubic_out`, `mc_ease_bounce_out`, `mc_ease_elastic_out` 等。

---

## 核心类型

```c
// 矩形
typedef struct { int16_t x, y; uint16_t w, h; } eui_rect_t;

// 对齐
EUI_ALIGN_LEFT | CENTER | RIGHT | TOP | MIDDLE | BOTTOM

// 位图
typedef struct { uint16_t w, h; uint8_t color_depth; const uint8_t *data; } eui_bitmap_t;

// 字体
typedef struct { uint8_t format, line_height, baseline, flags; const uint8_t *data; } eui_font_t;

// 转场动画类型
EUI_ANIM_NONE | SLIDE_LEFT | SLIDE_RIGHT | FADE | SCALE | SLIDE_UP
```

---

## 自定义 Widget

继承 `eui_widget_t` 并实现虚函数表：

```c
typedef struct {
    eui_widget_t widget;   // 必须为第一个成员
    /* 自定义数据 */
    int16_t count;
} my_counter_t;

static void counter_draw(eui_widget_t *self, eui_canvas_t *c) { /* ... */ }
static bool counter_input(eui_widget_t *self, const eui_event_t *e) { /* ... */ }

static const eui_widget_vtable_t counter_vtable = {
    .draw = counter_draw,
    .input = counter_input,
};

eui_widget_t* my_counter_create(void) {
    my_counter_t *cnt = eui_malloc(sizeof(my_counter_t));
    eui_widget_init(&cnt->widget, &counter_vtable, 0, 0, 48, 24);
    cnt->widget.focus_policy = EUI_FOCUS_STRONG;
    return &cnt->widget;
}
```
