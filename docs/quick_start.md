# EUI 快速入门

## 1. 环境准备

### 桌面开发（推荐先做）

```bash
git clone --recurse-submodules https://github.com/solosky/eui.git
cd eui

# Ubuntu/Debian
sudo apt install cmake gcc libraylib-dev

# macOS
brew install cmake raylib

# 构建并运行示例
cmake -B build -DEUI_BUILD_EXAMPLES=ON
cmake --build build -j
./build/examples/basic_label
```

### 嵌入式目标

安装交叉编译工具链（以 ARM Cortex-M 为例）：

```bash
sudo apt install gcc-arm-none-eabi
```

---

## 2. 最小可用程序

以下程序在 128x64 OLED (SSD1306) 上显示 "Hello"：

```c
#include "eui.h"
#include <string.h>

/* ===== 内存池 ===== */
#define POOL_SIZE 4096
static uint8_t mem_pool[POOL_SIZE];

/* ===== 显示 HAL (SSD1306 I2C) ===== */
static void ssd1306_flush(const uint8_t *buf, const eui_rect_t *r, void *ud) {
    /* 这里写 SSD1306 的实际 I2C 发送代码 */
}

static eui_display_hal_t display = {
    .caps = { .width = 128, .height = 64, .color_depth = 1,
              .buffer_mode = EUI_BUFFER_FULL },
    .init = ssd1306_init,
    .write_buffer = ssd1306_flush,
};

/* ===== 输入 HAL ===== */
static int keypad_poll(eui_event_t *evt, void *ud) {
    /* 扫描按键，有事件返回 1，无事件返回 0 */
    if (ok_button_pressed()) {
        evt->type = EUI_EVT_KEY_PRESS;
        evt->data.key = EUI_KEY_OK;
        return 1;
    }
    return 0;
}

static eui_input_hal_t input = {
    .poll = keypad_poll,
};

/* ===== View 处理器 ===== */
static bool main_handler(eui_view_event_t *evt, void *ctx) {
    if (evt->type == EUI_VIEW_EVT_DRAW) {
        eui_canvas_draw_str(evt->event.draw.canvas, 20, 28, "Hello EUI!");
        return true;
    }
    return false;
}

/* ===== 主函数 ===== */
int main(void) {
    hardware_init();   /* 初始化 MCU 外设 */
    eui_set_tick_callback(HAL_GetTick);

    eui_config_t cfg = {
        .mem_pool_buffer = mem_pool,
        .mem_pool_size = POOL_SIZE,
        .display = &display,
        .input = &input,
        .fps_target = 30,
    };
    eui_init(&cfg);

    /* 创建 View */
    eui_view_t main_view;
    eui_view_init(&main_view, main_handler, NULL);

    /* 主循环 */
    while (eui_is_running()) {
        eui_tick();
        HAL_Delay(33);  /* ~30 FPS */
    }

    eui_deinit();
    return 0;
}
```

---

## 3. 使用控件

### Label — 静态文本

```c
eui_widget_t *label = eui_label_create("Hello", 10, 10);
eui_label_set_align(label, EUI_ALIGN_CENTER, EUI_ALIGN_MIDDLE);
```

### Button — 可点击按钮

```c
static void on_click(void *ctx) {
    printf("Button clicked!\n");
}

eui_widget_t *btn = eui_button_create("OK", 40, 20, 50, 24);
eui_button_set_callback(btn, on_click, NULL);
```

### List — 选项列表

```c
static void on_select(uint8_t idx, void *ctx) {
    printf("Selected item %d\n", idx);
}

eui_widget_t *list = eui_list_create(0, 0, 128, 64);
eui_list_add_item(list, "Item 1", NULL);
eui_list_add_item(list, "Item 2", NULL);
eui_list_add_item(list, "Item 3", NULL);
eui_list_set_callback(list, on_select, NULL);
```

### Menu — 多级菜单

```c
eui_widget_t *menu = eui_menu_create(0, 0, 128, 64);

static void settings_cb(void *ctx) { /* 进入设置 */ }
eui_menu_add_item(menu, "Settings", settings_cb);

eui_menu_item_t *about = eui_menu_add_submenu(menu, "About");
eui_menu_add_item(/* about子菜单 */, "Version", version_cb);
```

### Dialog — 弹窗对话框

```c
static void on_dialog_result(eui_dialog_result_t r, void *ctx) {
    if (r == EUI_DIALOG_YES) printf("Confirmed\n");
    else printf("Cancelled\n");
}

void show_confirm(eui_view_dispatcher_t *vd) {
    eui_widget_t *dlg = eui_dialog_create("Confirm", "Are you sure?");
    eui_dialog_add_button(dlg, "Yes", EUI_DIALOG_YES);
    eui_dialog_add_button(dlg, "No", EUI_DIALOG_NO);
    eui_dialog_show(dlg, vd, on_dialog_result);
}
```

### Progress — 进度条

```c
eui_widget_t *bar = eui_progress_create(10, 20, 100, 16);
eui_progress_set_value(bar, 75);  /* 75% */
```

### Slider — 滑块

```c
eui_widget_t *slider = eui_slider_create(10, 20, 100, 20);
eui_slider_set_range(slider, 0, 100);
eui_slider_set_value(slider, 50);
```

---

## 4. View 导航

### 基本切换

```c
/* 注册多个 View */
eui_view_dispatcher_add(&vd, VIEW_MAIN,   &main_view);
eui_view_dispatcher_add(&vd, VIEW_MENU,   &menu_view);
eui_view_dispatcher_add(&vd, VIEW_SETTINGS, &settings_view);

/* 切换 */
eui_view_dispatcher_switch_to(&vd, VIEW_MENU, EUI_ANIM_SLIDE_LEFT);
```

### 弹窗覆盖

```c
/* 在主界面上弹出对话框 */
eui_view_dispatcher_push_overlay(&vd, &dialog_view, EUI_ANIM_SLIDE_UP);

/* 关闭对话框 */
eui_view_dispatcher_pop_overlay(&vd, EUI_ANIM_NONE);
```

---

## 5. 动画

使用 MotionC 的 30 个缓动函数：

```c
#include "mc.h"

/* 按钮从左移到右边，500ms */
eui_anim_start(btn, EUI_ANIM_TARGET_X, 0, 100, 500,
               mc_ease_cubic_out, NULL, NULL);

/* 弹簧动画 */
eui_anim_start_spring(btn, EUI_ANIM_TARGET_Y, 40,
                       120.0f, 10.0f, NULL);
```

---

## 6. 自定义控件

```c
typedef struct {
    eui_widget_t widget;   /* 必须为第一个成员 */
    int count;
} counter_t;

static void counter_draw(eui_widget_t *w, eui_canvas_t *c) {
    counter_t *cnt = (counter_t*)w;
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", cnt->count);
    eui_canvas_fill_round_rect(c, w->area.x, w->area.y, w->area.w, w->area.h, 3);
    eui_canvas_draw_str(c, w->area.x + 8, w->area.y + 4, buf);
}

static bool counter_input(eui_widget_t *w, const eui_event_t *evt) {
    if (evt->type == EUI_EVT_KEY_PRESS && evt->data.key == EUI_KEY_OK) {
        ((counter_t*)w)->count++;
        w->style |= EUI_STYLE_DIRTY;
        return true;
    }
    return false;
}

static const eui_widget_vtable_t counter_vt = {
    .draw = counter_draw,
    .input = counter_input,
};

eui_widget_t* counter_create(int16_t x, int16_t y) {
    counter_t *c = eui_malloc(sizeof(counter_t));
    eui_widget_init(&c->widget, &counter_vt, x, y, 48, 24);
    c->widget.focus_policy = EUI_FOCUS_STRONG;
    return &c->widget;
}
```

---

## 7. 移植到新平台

只需实现两个 HAL 接口：

1. **Display HAL** — 至少实现 `init` + `write_buffer` (FULL/PAGE) 或 `draw_pixel` (DIRECT)
2. **Input HAL** — 至少实现 `poll`

详见 [移植指南](porting_guide.md)

---

## 8. 示例程序

| 示例 | 说明 | 运行 |
|------|------|------|
| `basic_label` | 文本显示 | `./build/examples/basic_label` |
| `button_test` | 按钮交互 | `./build/examples/button_test` |
| `list_nav` | 列表导航 | `./build/examples/list_nav` |
| `menu_system` | 多级菜单 | `./build/examples/menu_system` |
| `dialog_overlay` | 弹窗对话框 | `./build/examples/dialog_overlay` |
| `animation_demo` | 动画效果 | `./build/examples/animation_demo` |
| `custom_widget` | 自定义控件 | `./build/examples/custom_widget` |
| `page_buffer` | PAGE 模式 | `./build/examples/page_buffer` |
| `benchmark` | 性能测试 | `./build/examples/benchmark` |
