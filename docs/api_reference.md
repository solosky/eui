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
| `config->input` | `eui_input_drv_t*` | 输入 HAL 接口 |
| `config->fps_target` | `uint16_t` | 目标帧率，默认 30 |
| `config->max_views` | `uint8_t` | 最大 View 数，默认 8 |
| `config->max_widgets` | `uint8_t` | 最大 Widget 数，默认 32 |

**返回：** 成功返回 0，失败返回 -1。

### eui_tick

```c
void eui_tick(void);
```

框架主循环心跳。每帧调用一次，驱动动画更新、输入轮询、事件分发和渲染。

### eui_post / eui_post_cancel

帧尾延迟执行：投递回调，在 `eui_tick()` 渲染完成后的帧尾（Step 6，chrome 层之后）执行。

> **注意（临时）：`eui_post_cancel()` 当前尚未实现，为占位空实现** —— 调用它不会清掉任何条目，也不会报错；`owner` 归属机制待后续提交补齐。在此之前请不要照抄下面的清理模式（否则被取消的条目仍会执行，可能造成 use-after-free）。下面的 `eui_post_cancel` 示例行同样暂时只是占位。

```c
#include <eui/eui_post.h>

static void on_next_frame(void *ud) { /* 帧尾执行 */ }

eui_post(on_next_frame, NULL, NULL);          /* 匿名条目，执行一次 */
eui_post(on_next_frame, ctx, app_handle);      /* 归属 app_handle，可整体取消 */
eui_post_cancel(app_handle);                   /* 清掉该归属未执行的条目 —— 尚未实现，见上方注意 */
```

- 投递序即执行序；**排空到空** —— 回调内继续 `eui_post` 的条目同帧执行（每帧上限 `EUI_POST_DRAIN_MAX`，余量顺延下一帧）。
- 帧尾回调内的绘制只写 canvas、不 commit：本帧上屏内容仍由渲染与 chrome 层决定。
- 队列容量 `EUI_POST_QUEUE_SIZE`（上限 255，超出为编译期错误），满时投递返回 false，失败次数见 `eui_post_dropped()`。
- 投递方负责保证执行时 `fn` / `user_data` 有效；对象销毁前用 `eui_post_cancel(owner)` 清理（`owner == NULL` 的匿名条目不可取消）—— **但 `eui_post_cancel()` 当前尚未实现，请不要依赖这一条**。
- 单线程：必须与 `eui_tick()` 同线程调用。

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
void eui_canvas_draw_arc(eui_canvas_t *canvas, int16_t cx, int16_t cy,
                         uint16_t r, uint16_t thickness,
                         int16_t start_deg, int16_t end_deg);
void eui_canvas_draw_ring(eui_canvas_t *canvas, int16_t cx, int16_t cy,
                          uint16_t r_outer, uint16_t r_inner,
                          int16_t start_deg, int16_t end_deg);
void eui_canvas_fill_pie(eui_canvas_t *canvas, int16_t cx, int16_t cy, uint16_t r,
                         int16_t start_deg, int16_t end_deg);
```

四个曲线原语（`draw_circle` / `fill_circle` / `draw_round_rect` / `fill_round_rect`）自本版本起输出**抗锯齿**像素：边缘按覆盖度与背景混合，直边部分输出与 `draw_rect` / `fill_rect` 逐字节一致。

- **低色深**：1bpp / 2bpp 用 4×4 Bayer 有序抖动模拟中间灰，抖动阈值相位锚定在屏幕坐标 `(x, y)`（含 PAGE 模式的 `page_y_offset`，跨 band 连续）；**实心内部不抖动**——完全覆盖的像素写入的颜色已在色深量化网格上，输出精确等于前景色，因此填充区零噪点。4bpp 量化到 16 级、8bpp 精确到 256 级、16bpp 在 RGB565 空间逐通道混合，均无抖动。
- **角度**：整数度，0° = 3 点钟方向，顺时针为正（屏幕 y 向下增长）。
- **弧段扫描规则**：`delta = end_deg - start_deg`；`delta == 0` 不画任何像素；`|delta| >= 360` 画整圆；其余折到 1..359°——`delta < 0` 且 `|delta| < 360` 时取 `delta + 360`，此时 `end_deg` 与 `end_deg + 360` 画出的扇形逐像素相同（`start=0, end=-90` 与 `start=0, end=270` 实测一致）。注意 `|delta| >= 360` 那一档优先于该等价关系：`end=-400`（`delta = -400`）画的是整圆，而不是 320° 扇形。端帽为平头（butt），扇形始终从 `start_deg` 沿顺时针量取归一化后的度数。
- **描边宽度**：`eui_canvas_draw_arc` 的描边占据半径区间 `[r - thickness, r]`；`thickness == 0` 不画；`thickness >= r` 时内半径夹到 0，退化为扇形（等价于 `eui_canvas_fill_pie`）。`eui_canvas_draw_ring` 的描边占据 `[r_inner, r_outer]`，`r_inner >= r_outer` 不画。
- **退化情形**：`eui_canvas_fill_pie(cx, cy, 0, ...)` 不画（半径为 0 的扇形没有面积），而 `eui_canvas_fill_circle(cx, cy, 0)` 仍按既有契约画一个点。
- **圆心约定（消费者需注意）**：新内核把圆心放在**连续点** `(cx, cy)`（像素跨 `[px, px+1]`），与画布既有的 `draw_line` / `draw_rect` / `fill_rect` 索引空间一致；legacy 的中点光栅器把圆心放在像素中心 `(x+0.5, y+0.5)`。因此圆与圆角的右/下边缘可能比旧版窄一像素：`eui_canvas_fill_circle(c, 100, 100, 20)` 现在覆盖列 `[80, 119]`（40 列、左右对称），旧版覆盖 `[80, 120]`（41 列、不对称）。这是**修正而非整体位移**（`draw_circle(r == 0)` 的单点位置等既有约定不变），断言图形**内部**像素的代码不受影响，断言图形**边缘**像素的代码会看到 1 px 差。
- **圆角矩形的角心**：四个四分之一圆盘的圆心取**外边界内缩 r**，即 `(x+r, y+r)` / `(x+w-r, y+r)` / `(x+w-r, y+h-r)` / `(x+r, y+h-r)`。因此角弧与直边在相切处严丝合缝（无 1 px 台阶；描边版无 1 px 断口），整个形状落在 `[x, x+w] × [y, y+h]` 内、与 `fill_rect` 的边界约定一致，并满足 180° 旋转对称。`2r == w`（或 `2r == h`）的胶囊形退化配置同样精确：两角盘圆心重合、分界轴落在像素边界上，没有像素被两次混合。

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
void eui_canvas_draw_bitmap_rot(eui_canvas_t *canvas, int16_t x, int16_t y,
                                const eui_bitmap_t *bmp, int16_t deg_cw);
void eui_canvas_draw_bitmap_rot_keyed(eui_canvas_t *canvas, int16_t x, int16_t y,
                                      const eui_bitmap_t *bmp, int16_t deg_cw,
                                      eui_color_t key);
void eui_canvas_invert_rect(eui_canvas_t *canvas, int16_t x, int16_t y,
                            uint16_t w, uint16_t h);
```

**位图数据布局**：`eui_bitmap_t.data` 按**位图自身** `color_depth` 的原生打包解读，与画布色深可以不同（解码出的值交给画布按自身色深落盘）：

| 位图 color_depth | 数据布局 |
|------|---------|
| 1 | 每像素 1 字节，0/1 |
| 2 | 每像素 1 字节，低 2 位有效 |
| 4 | 每像素 1 字节，低 4 位有效 |
| 16 | native `uint16` 序列（与 16bpp 画布帧缓冲同布局，**不是**大端字节对） |

**旋转 blit 用法**：`(x, y)` 是**未旋转**图像的摆放位置（左上角），旋转绕图像中心 `(x+w/2, y+h/2)` 进行；`deg_cw` 为顺时针整数度（负角/超过 360 自动归一化），内部用逐度正弦表实现、不依赖 libm。最近邻采样；采样越界的源像素与旋转后 bbox 外的落点**不写入**（保留画布已有内容）；90°/270° 时可见区域宽高互换。

```c
/* spinner：37x37 图标绕中心每帧 +2°，黑色为透明键（保留表盘背景） */
eui_bitmap_t bmp = { .width = 37, .height = 37, .color_depth = 16,
                     .data = (const uint8_t *)spinner_rgb565 };
eui_canvas_draw_bitmap_rot_keyed(c, 101, 101, &bmp, spinner_deg, 0x0000);
spinner_deg = (spinner_deg + 2) % 360;
```

`_keyed` 变体跳过等于 `key` 的源像素——典型用法是图标镂空处透出页面背景。

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
    EUI_EVT_KEY_PRESS = 0,     /* 按键按下（原始） */
    EUI_EVT_KEY_RELEASE = 1,   /* 按键释放（原始） */
    EUI_EVT_KEY_REPEAT = 2,    /* 按键自动重复（原始） */
    EUI_EVT_ENCODER_CW = 3,    /* 编码器顺时针，正 delta（原始） */
    EUI_EVT_ENCODER_CCW = 4,   /* 编码器逆时针，负 delta（原始） */
    EUI_EVT_ENCODER_CLICK = 5, /* 编码器按键 click（原始） */
    EUI_EVT_TOUCH_DOWN = 6,    /* 触摸按下（原始） */
    EUI_EVT_TOUCH_UP = 7,      /* 触摸抬起（原始） */
    EUI_EVT_TOUCH_MOVE = 8,    /* 触摸移动（原始） */
    EUI_EVT_KEY_CLICK = 9,     /* press→release 且未触发 hold（装配手势） */
    EUI_EVT_KEY_HOLD = 10,     /* 按住 >=500ms，吞掉本次 click（装配手势） */
    EUI_EVT_ENC_STEP = 11,     /* 编码器步进，CW 为正（装配手势） */
} eui_event_type_t;

typedef struct {
    eui_event_type_t type;
    union {
        uint8_t   key_id;                // KEY 事件：无语义按键编号（项目自定分配）
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
} eui_input_drv_t;
```

`key_id` 为无语义 `uint8_t` 编号，分配归项目（如 0=确认键、1=侧键）；eui 不定义任何语义映射，驱动 keymap 与项目层各自把物理按键绑定到编号。

事件分两类：**原始事件**（`EUI_EVT_KEY_PRESS` / `KEY_RELEASE` / `KEY_REPEAT` / `ENCODER_CW` / `ENCODER_CCW` / `ENCODER_CLICK` / `TOUCH_DOWN` / `TOUCH_UP` / `TOUCH_MOVE`）由驱动产出，是手势装配器的输入；**装配手势事件**（`EUI_EVT_KEY_CLICK` / `EUI_EVT_KEY_HOLD`，`data.key_id`；`EUI_EVT_ENC_STEP`，`data.enc_delta`）由 core 内置的手势装配器从原始事件流组装，View 一般只消费这三类（见 [eui_input_edge](#eui_input_edge--手势装配器)）。

---

## 内置设备驱动

`src/driver/` 提供一组开箱即用的设备驱动，直接产出 `eui_display_drv_t *` / `eui_input_drv_t *`：

| 类别 | 驱动 | 说明 |
|------|------|------|
| 显示（I2C） | `eui_drv_ssd1306` / `eui_drv_sh1106` | 128x64 OLED，1bpp，PAGE 缓冲 |
| 显示（SPI） | `eui_drv_st7735` / `eui_drv_ili9341` / `eui_drv_st7306` / `eui_drv_st7789` | 彩屏 RGB565，FULL 缓冲 |
| 输入 | `eui_drv_buttons` / `eui_drv_encoder` / `eui_drv_xpt2046` | 按键 / 旋转编码器 / 电阻触摸 |
| 桌面 / Web | `eui_drv_raylib` / `eui_drv_web` | 模拟器窗口（键鼠映射）与 Emscripten |

### ST7789（SPI 彩屏）用法

```c
#include "eui/driver/eui_drv_st7789.h"

static eui_drv_st7789_config_t lcd_cfg = {
    .spi = {
        .write_cmd = my_spi_write_cmd,      /* 实现 eui_hal_spi_t 五个回调 */
        .write_data = my_spi_write_data,
        .read_data  = my_spi_read_data,
        .set_dc = my_spi_set_dc, .set_cs = my_spi_set_cs, .set_rst = my_spi_set_rst,
        .delay_ms = my_delay_ms, .user_data = NULL,
    },
    .width = 240, .height = 240,
    .col_offset = 0, .row_offset = 0,       /* 135x240 类面板需 40/53 */
    .madctl = 0x00,                          /* 方向/RGB 顺序；旋转改此值 */
    .invert = true,                          /* 多数 IPS 面板需要 INVON */
};

eui_display_drv_t *lcd = eui_drv_st7789_create(&lcd_cfg);
eui_config_t cfg = { .display = lcd, /* ... */ };
eui_init(&cfg);

lcd->init(lcd->user_data);     /* 硬复位 → SLPOUT → COLMOD 16bit → MADCTL
                                  → INVON/INVOFF → NORON → DISPON */
/* 运行期可用 lcd->set_invert(lcd->user_data, true/false) 切换反色 */
/* eui_drv_st7789_destroy(lcd); 释放 */
```

初始化序列对齐 LovyanGFX `Panel_ST7789` 的默认行为（240x240 真机配置为参照）；运行期反色经 `set_invert` 回调发送 `INVON(0x21)`/`INVOFF(0x20)`。

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

场景管理器持有**导航栈**：`switch()` 把栈重置为单个根场景，`push()` 在栈顶压入一层（attached 模式下经 dispatcher 的 overlay 栈上屏），`pop()` 弹出一层并对露出的场景回调 `on_resume`。

```c
int  eui_scene_manager_register(eui_scene_manager_t *sm,
                                 const eui_scene_t *scenes, uint8_t count);
void eui_scene_manager_attach(eui_scene_manager_t *sm, struct eui_view_dispatcher_t *vd);

/* 导航 */
void eui_scene_manager_switch(eui_scene_manager_t *sm, uint32_t scene_id);  /* 清栈 → 单根场景 */
int  eui_scene_manager_push(eui_scene_manager_t *sm, uint32_t scene_id);    /* 0 成功 / -1 栈满 */
int  eui_scene_manager_pop(eui_scene_manager_t *sm);                        /* 0 成功 / -1 已在根 */
void eui_scene_manager_back(eui_scene_manager_t *sm);                       /* pop() 别名 */

/* 栈查询 */
uint8_t eui_scene_manager_depth(const eui_scene_manager_t *sm);             /* >1 即有 overlay 层 */
uint32_t eui_scene_manager_current_id(const eui_scene_manager_t *sm);       /* 栈顶场景 id，空为 (uint32_t)-1 */
const eui_scene_t *eui_scene_manager_scene_at(const eui_scene_manager_t *sm, uint8_t level);
```

用法示例（根场景 + 压入子菜单 + 返回）：

```c
eui_scene_manager_register(&sm, scenes, scene_count);
eui_scene_manager_attach(&sm, &dispatcher);
eui_scene_manager_switch(&sm, SCENE_HOME);        /* 栈 = [HOME] */
eui_scene_manager_push(&sm, SCENE_SUBMENU);       /* 栈 = [HOME, SUBMENU] */
if (eui_scene_manager_depth(&sm) > 1)
    eui_scene_manager_pop(&sm);                   /* 回到 HOME，HOME.on_resume 触发 */
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

## 应用组件

不属于 Widget 树的独立 UI 组件：直接在 View 的绘制回调里消费，只依赖画布与 motionc。

### eui_input_edge — 手势装配器

把驱动产出的原始输入事件流（`KEY_PRESS/RELEASE`、`ENCODER_CW/CCW` 等）装配成通用手势事件。eui core 持有唯一实例（内嵌于 view dispatcher，可用 `eui_get_input_edge()` 取到）：`eui_tick` 每帧把 input manager 的去抖原始事件喂入装配器、推进 hold 计时，再把组装完成的手势事件推给当前 View 的 INPUT 回调。app 一般不直接操作该模块——在 View 里消费手势事件即可：

```c
#define MY_KEY_OK   0u    /* 编号分配归项目，eui 不定义语义 */
#define MY_KEY_SIDE 1u

/* View 输入回调里消费装配好的手势事件 */
bool my_view_handler(eui_view_event_t *e, void *ctx) {
    if (e->type == EUI_VIEW_EVT_INPUT) {
        const eui_event_t *evt = e->event.input;
        switch (evt->type) {
        case EUI_EVT_KEY_CLICK:              /* press→release 且未长按 */
            if (evt->data.key_id == MY_KEY_OK)   { /* 确认 */ }
            if (evt->data.key_id == MY_KEY_SIDE) { /* 侧键短按 */ }
            break;
        case EUI_EVT_KEY_HOLD:               /* 按住 >=500ms，吞掉本次 click */
            if (evt->data.key_id == MY_KEY_OK)   { /* 长按 */ }
            break;
        case EUI_EVT_ENC_STEP:               /* 编码器步进，CW 为正 */
            if (evt->data.enc_delta > 0)         { /* 顺时针 */ }
            break;
        default: break;
        }
    }
    return false;
}
```

语义：

- **key_id 无语义**：`uint8_t` 编号，分配归项目；装配器对各 key_id 一视同仁，物理按键 → 编号的映射在驱动 keymap / 项目层完成。
- **hold 500ms**（`EUI_INPUT_EDGE_DEFAULT_HOLD_MS`）：按住过阈值产生一次 `EUI_EVT_KEY_HOLD`，并**吞掉**本次 click——同一次按压 hold 与 click 互斥。press/release 原样透传（`EUI_EVT_KEY_PRESS`/`KEY_RELEASE`），选择器类交互需要 press/release 时序。
- **编码器**：`ENCODER_CW`/`ENCODER_CCW` 组装为 `EUI_EVT_ENC_STEP`（CW 为正）；方向键折算编码器归驱动层键位绑定（见移植指南）。契约：`ENCODER_CCW` 事件须携带负 `enc_delta`，非法 CCW（delta >= 0）被忽略。
- **视图切换**：core 在切视图时自动 `eui_input_edge_flush()`，清掉未派发事件与瞬时边沿；仍按住的键保留按下态与 hold 计时。

拉取式一次性边沿 API 仍保留（消费即清零；key_id 越界返回 false）——仅供 eui 单测/兼容，app 一般走事件：

```c
bool eui_input_edge_was_pressed (eui_input_edge_t *in, uint8_t key_id);
bool eui_input_edge_was_released(eui_input_edge_t *in, uint8_t key_id);
bool eui_input_edge_was_clicked (eui_input_edge_t *in, uint8_t key_id);
bool eui_input_edge_was_hold    (eui_input_edge_t *in, uint8_t key_id);
/* 电平查询，不清锁存；key_id 越界视作已释放 */
bool eui_input_edge_is_released (const eui_input_edge_t *in, uint8_t key_id);
```

### eui_selector — 动效选项轮播

SmoothSelector 语义的选项选择器：选中框 position/shape 与相机 offset 三个 `mc_transition2d` 驱动，支持挤压/回弹/开合/循环滚动。**不负责绘制**——调用方读取动画值后自行画。

```c
#include "eui/eui_selector.h"

eui_selector_t sel;
eui_selector_init(&sel);                      /* move_in_loop=true, is_changed=true */
eui_selector_set_duration(&sel, 300);         /* position+shape 过渡时长 */
eui_selector_set_path(&sel, mc_ease_back_out);

static const eui_selector_option_t opts[] = { /* 注意：selector 存指针！生命周期须覆盖使用期 */
    { .x = 20,  .y = 56, .w = 128, .h = 128 },
    { .x = 190, .y = 56, .w = 128, .h = 128 },
};
eui_selector_add_option(&sel, &opts[0]);
eui_selector_add_option(&sel, &opts[1]);

/* 回调钩子（对齐原版虚函数），user_data 为第一参数 */
sel.on_read_input = my_read_input;              /* 20ms 节流：在此喂输入、go_next/go_last */
sel.on_click = my_click;                        /* release 回弹落位后触发一次 */
sel.on_open_end = my_open_end;                  /* open 全屏动画落位后触发一次 */
sel.on_update_camera_keyframe = my_camera_kf;   /* 选中项变化时：在此 move_to 相机目标 */

/* 帧驱动（View 绘制回调内） */
eui_selector_update(&sel, now_ms);
int x, y, w, h;
eui_selector_current_frame(&sel, &x, &y, &w, &h);
int cam_x = eui_selector_camera_x(&sel);
/* 按 (x - cam_x, y, w, h) 画选中框，按各选项与相机的相对位置画内容 */

/* 交互 */
eui_selector_go_next(&sel);                    /* 循环滚动（可关 move_in_loop） */
eui_selector_press(&sel, &squeeze_kf);         /* 按下挤压到关键帧 */
eui_selector_release(&sel);                    /* 释放：回弹选中项 → 落位触发 on_click */
eui_selector_open(&sel, &fullscreen_kf);       /* 展开到全屏 → 落位触发 on_open_end */
eui_selector_close(&sel);                      /* 收拢回选中项（view 进入时的入场来源） */
```

时序要点：`is_changed` 初始为 true——第一次 `update` 就会把选中框移向 option 0（可作 view 进入的收拢动画）；`release` 后 `is_pressing` 保持到过渡完成才随 `on_click` 一起清除；`open` 后需等落位才有 `on_open_end`。

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
