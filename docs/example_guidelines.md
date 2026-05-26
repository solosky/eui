# 跨平台示例编写规范

所有跨平台示例位于 `examples/cross/<name>/` 目录下，通过端口引导
（port bootstrap）系统在 raylib（桌面）和 Emscripten（Web）之间共享
一致的结构。

## 文件结构

```
examples/cross/<name>/
├── CMakeLists.txt        # 构建目标定义
├── requirements.cmake    #（可选）显示/颜色深度约束
└── <name>.c              # 单一源文件
```

## 源文件骨架

每个示例必须包含 `eui/eui_port_bootstrap.h` 并定义 `eui_example_setup()`：

```c
/* examples/cross/<name>/<name>.c */
#include "eui/eui.h"
#include "eui/eui_port_bootstrap.h"

void eui_example_setup(const eui_example_config_t *cfg) {
    (void)cfg;
    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();

    // 在此构建 UI —— views、widgets 等
    eui_view_dispatcher_add(vd, 1, &widget->view);
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);
}
```

**规则：**
- 不能定义 `main()` 函数 —— 端口引导提供了 `main()`。
- 不能使用平台相关的 `#ifdef` 块 —— 引导层处理 raylib 与 web 的差异。
- 不能调用 `eui_allocator_init_tlsf()` 或定义内存池 —— 引导层管理内存池。
- 不能直接创建驱动（如 `eui_drv_raylib_create_display()` 等）。

## CMakeLists.txt

```cmake
# examples/cross/<name>/CMakeLists.txt
add_executable(<name> <name>.c)
target_include_directories(<name> PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(<name> PRIVATE eui ${BOOTSTRAP_LIB})
```

`${BOOTSTRAP_LIB}` 由构建系统根据 `EUI_TARGET_PORT` 设置为
`eui_port_raylib`、`eui_port_web` 或 `eui_port_esp_idf`。

## requirements.cmake（可选）

声明最低显示尺寸和颜色深度要求。当活跃配置不满足需求时，需要特定
显示尺寸或 bpp 的示例会被自动跳过。

```cmake
# examples/cross/<name>/requirements.cmake
set(EXAMPLE_REQUIRES_COLOR_DEPTH_MIN 2)
set(EXAMPLE_REQUIRES_WIDTH_MIN       400)
set(EXAMPLE_REQUIRES_HEIGHT_MIN      300)
```

支持的变量：

| 变量 | 说明 |
|---|---|
| `EXAMPLE_REQUIRES_COLOR_DEPTH_MIN` | 最低颜色深度（1、2、4、8、16） |
| `EXAMPLE_REQUIRES_WIDTH_MIN` | 最低显示宽度（像素） |
| `EXAMPLE_REQUIRES_HEIGHT_MIN` | 最低显示高度（像素） |

## 变体

### 简单 —— 仅使用 widget

使用预构建 widget 的最小示例：

```c
void eui_example_setup(const eui_example_config_t *cfg) {
    (void)cfg;
    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();
    eui_widget_t *label = eui_label_create("Hello EUI!", 10, 5);
    eui_view_dispatcher_add(vd, 1, &label->view);
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);
}
```

### 自定义绘制 —— 原始 canvas

当需要完全控制渲染时使用：

```c
static void my_draw(eui_widget_t *w, eui_canvas_t *c) {
    eui_canvas_set_color(c, EUI_COLOR_WHITE);
    eui_canvas_fill_rect(c, w->area.x, w->area.y, w->area.w, w->area.h);
}

static bool my_input(eui_widget_t *w, const eui_event_t *e) {
    (void)w; (void)e;
    return false;
}

static eui_widget_vtable_t my_vt = { .draw = my_draw, .input = my_input };

void eui_example_setup(const eui_example_config_t *cfg) {
    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();
    eui_widget_t *widget = eui_malloc(sizeof(eui_widget_t));
    eui_widget_init(widget, &my_vt, 0, 0, cfg->display_width, cfg->display_height);
    eui_view_dispatcher_add(vd, 1, &widget->view);
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);
}
```

### 自定义视图处理器

用于复杂导航（多视图、覆盖层）：

```c
static bool my_view_handler(eui_view_event_t *evt, void *context) {
    eui_view_t *view = (eui_view_t*)context;
    switch (evt->type) {
    case EUI_VIEW_EVT_DRAW: { /* ... */ return true; }
    case EUI_VIEW_EVT_INPUT: { /* ... */ return true; }
    default: return false;
    }
}

void eui_example_setup(const eui_example_config_t *cfg) {
    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();
    eui_view_t *view = eui_malloc(sizeof(eui_view_t));
    eui_view_init(view, my_view_handler, view);
    view->area = (eui_rect_t){ 0, 0, cfg->display_width, cfg->display_height };
    eui_view_dispatcher_add(vd, 1, view);
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);
}
```

### 动画 —— 框架动画 API

使用 `eui_anim_start()` 实现内置缓动：

```c
void eui_example_setup(const eui_example_config_t *cfg) {
    (void)cfg;
    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();
    eui_widget_t square;
    eui_widget_init(&square, &square_vt, 0, 30, 16, 16);
    eui_view_dispatcher_add(vd, 1, &square.view);
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);
    eui_anim_init();
    eui_anim_start(&square, EUI_ANIM_TARGET_X, 0, 80, 500, mc_ease_linear, NULL, NULL);
}
```

### 自定义 tick —— MotionC 弹簧动画

覆写 tick 回调以实现逐帧物理模拟（如弹簧动画），仍须返回当前
时间戳（毫秒）。

```c
#include <math.h>
#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#define GET_TICK_MS() ((uint32_t)emscripten_get_now())
#elif defined(__has_include)
#if __has_include(<raylib.h>)
#include <raylib.h>
#define GET_TICK_MS() ((uint32_t)(GetTime() * 1000.0f))
#else
#define GET_TICK_MS() 0
#endif
#else
#define GET_TICK_MS() 0
#endif

static uint32_t g_last_tick = 0;

static uint32_t my_tick(void) {
    uint32_t now = GET_TICK_MS();
    if (g_last_tick == 0) { g_last_tick = now; return now; }
    uint32_t dt = now - g_last_tick;
    g_last_tick = now;
    if (dt > 100) dt = 100;
    // 更新弹簧动画，动画活跃时标记 dirty
    return now;
}

void eui_example_setup(const eui_example_config_t *cfg) {
    // ... 设置 views ...
    eui_set_tick_callback(my_tick);
    eui_anim_init();
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);
}
```

平台相关的 `GET_TICK_MS()` 宏是示例源文件中 **唯一** 允许的 `#ifdef`。

## 反模式

| 不要这样做 | 原因 |
|---|---|
| 定义 `main()` | 引导层已提供；重复定义会导致链接错误。 |
| 直接 `#include <raylib.h>` | 应使用 `eui/eui_port_bootstrap.h`；raylib 是引导层细节。 |
| `#include "eui/driver/eui_drv_raylib.h"` | 驱动创建属于引导层，不属于示例。 |
| 调用 `eui_allocator_init_tlsf()` | 引导层分配内存池。 |
| 用 `#define W 128`、`#define H 64` 硬编码显示尺寸 | 使用 `cfg->display_width/height` 或加注释保留为固定常量；配置系统确保显示匹配。 |
| 调用 `eui_init()` | 引导层处理框架初始化。 |
| 在 `GET_TICK_MS()` 之外使用 `#if defined(__EMSCRIPTEN__)` | 所有平台抽象都应通过引导层。 |
