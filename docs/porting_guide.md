# EUI 移植指南

将 EUI 移植到新 MCU/屏幕仅需实现两个 HAL 接口。UI 代码无需修改。

## 推荐工作流

1. **桌面开发** — 先用 raylib 模拟层在 PC 上完成全部 UI 开发 (1小时)
2. **单元测试** — 运行 `ctest` 验证核心功能
3. **硬件移植** — 实现 Display HAL + Input HAL (3-6小时)
4. **交叉编译** — 链接 `libeui.a`，UI 代码零修改

---

## 步骤 1：实现 Display HAL

### 最小接口

至少实现 `init`、`write_buffer`（FULL/PAGE 模式）或 `draw_pixel`（DIRECT 模式）：

```c
#include "eui.h"

/* ===== SSD1306 I2C 示例 ===== */

static uint8_t page_buf[128];  /* PAGE 模式：128×8 条带缓冲 */

static int ssd1306_init(void *ud) {
    (void)ud;
    /* 发送 SSD1306 初始化命令序列 */
    ssd1306_write_cmd(0xAE);  /* 关闭显示 */
    ssd1306_write_cmd(0x20);  /* 水平寻址模式 */
    ssd1306_write_cmd(0x00);
    ssd1306_write_cmd(0xAF);  /* 开启显示 */
    return 0;
}

static void ssd1306_flush(const uint8_t *buf, const eui_rect_t *r, void *ud) {
    (void)ud;
    /* 逐页写入 */
    uint8_t page_start = r->y / 8;
    uint8_t page_end   = (r->y + r->h - 1) / 8;
    for (uint8_t p = page_start; p <= page_end; p++) {
        ssd1306_set_cursor(r->x, p);
        ssd1306_write_data(buf + (p - page_start) * r->w, r->w);
    }
}

eui_display_hal_t ssd1306_hal = {
    .caps = {
        .width       = 128,
        .height      = 64,
        .color_depth = 1,              /* 单色 */
        .buffer_mode = EUI_BUFFER_PAGE, /* 条带模式，节省内存 */
        .has_gram    = true,
    },
    .init         = ssd1306_init,
    .write_buffer = ssd1306_flush,
    .set_contrast = ssd1306_set_contrast,
    .user_data    = NULL,
};
```

### ST7735 SPI TFT 示例 (16bpp FULL 模式)

```c
eui_display_hal_t st7735_hal = {
    .caps = {
        .width       = 128,
        .height      = 160,
        .color_depth = 16,             /* RGB565 */
        .buffer_mode = EUI_BUFFER_FULL, /* 完整帧缓冲 */
        .has_gram    = true,
    },
    .init         = st7735_init,
    .write_buffer = st7735_flush,
    .user_data    = NULL,
};
```

### 显示能力说明

| 字段 | 说明 |
|------|------|
| `width / height` | 屏幕分辨率（像素） |
| `color_depth` | 1 / 4 / 8 / 16 |
| `buffer_mode` | `EUI_BUFFER_FULL` \| `PAGE` \| `DIRECT` |
| `has_gram` | 显示控制器是否有内置 GRAM |
| `hw_scroll` | 是否支持硬件滚动（可选） |

### 缓冲模式选择

| 模式 | 内存 | 性能 | 建议 |
|------|------|------|------|
| FULL | W×H×bpp/8 | 最快（一次 DMA） | 有 GRAM 或外接 SRAM |
| PAGE | W×8×bpp/8 (~1KB) | 多次遍历 | 小内存 + 大分辨率 |
| DIRECT | 0 | 逐像素慢 | 极高内存约束 |

---

## 步骤 2：实现 Input HAL

### 按键示例

按键编号（`key_id`）由**项目自定义**，eui 不定义语义。先在项目里给物理按键分配编号，再在驱动的 poll 里发出：

```c
/* 项目层定义编号（示例）：0=上键，4=确认键 */
#define MY_KEY_UP  0u
#define MY_KEY_OK  4u

static int keypad_poll(eui_event_t *evt, void *ud) {
    (void)ud;

    /* 扫描矩阵键盘 */
    uint8_t pressed = scan_keypad();

    if (pressed & KEY_UP) {
        evt->type = EUI_EVT_KEY_PRESS;
        evt->data.key_id = MY_KEY_UP;
        return 1;
    }
    if (pressed & KEY_OK) {
        evt->type = EUI_EVT_KEY_PRESS;
        evt->data.key_id = MY_KEY_OK;
        return 1;
    }
    /* ... 其他按键 ... */

    return 0;  /* 无事件 */
}

eui_input_drv_t keypad_hal = {
    .poll      = keypad_poll,
    .user_data = NULL,
};
```

也可以直接用内置驱动 + 键位映射表，见下文 [使用 GPIO 按键驱动](#使用-gpio-按键驱动) 与 raylib 的 `eui_raylib_keymap_entry_t`。

### 旋转编码器示例

```c
static int encoder_poll(eui_event_t *evt, void *ud) {
    int delta = read_encoder();
    if (delta > 0) {
        evt->type = EUI_EVT_ENCODER_CW;
        evt->data.enc_delta = 1;
        return 1;
    } else if (delta < 0) {
        evt->type = EUI_EVT_ENCODER_CCW;
        evt->data.enc_delta = 1;
        return 1;
    }
    return 0;
}
```

### 按键映射

eui **不定义语义按键**：`key_id` 只是无语义 `uint8_t` 编号（0 .. 编号上限-1，上限见构建配置），“哪个编号是确认键、哪个是侧键”完全由项目决定（如 0=确认键、1=侧键）。编号 → 角色的绑定有两处，须保持一致：

1. **驱动 keymap**（物理按键 → 编号）：raylib 用 `eui_raylib_keymap_entry_t`（`raylib_key` / `mouse_btn` / `key_id` / `enc_delta`），GPIO 按键驱动用 `eui_drv_buttons_map_t` 的 `uint8_t key` 字段。桌面模拟的默认 keymap 沿用历史绑定：编号 0..5 依次为 上/下/左/右/确认/返回，方向键折算编码器（±1），鼠标左/右键为确认/返回。
2. **widget 导航键位**（编号 → 导航角色）：widget 库默认按 `{up=0, down=1, left=2, right=3, ok=4, back=5}` 解释编号；项目若采用别的编号方案，挂 widget 前必须先 `eui_widget_set_nav_keys()` 显式改绑，否则语义静默错位（不用 widget 库则无需关心）。

```c
/* 示例：项目自定义编号方案（0=确认键、1=侧键），同步改绑 widget 导航 */
#define MY_KEY_OK   0u
#define MY_KEY_SIDE 1u

eui_widget_nav_keys_t nav = {
    .up = 2, .down = 3, .left = 4, .right = 5,
    .ok = MY_KEY_OK, .back = MY_KEY_SIDE,
};
eui_widget_set_nav_keys(&nav);
```

---

## 步骤 3：帧率控制

EUI 不限制帧率。建议静态界面 10-15 FPS，动画场景 30-60 FPS。

```c
int main(void) {
    hardware_init();
    eui_config_t cfg = {
        .mem_pool_buffer = mem_pool,
        .mem_pool_size   = POOL_SIZE,
        .display = &display_hal,
        .input   = &input_hal,
        .fps_target = 30,
    };
    eui_init(&cfg);
    eui_set_tick_callback(HAL_GetTick);   /* 平台毫秒计时器 */

    while (eui_is_running()) {
        eui_tick();
        HAL_Delay(1000 / 30);  /* 30 FPS: 33ms 延迟 */
    }

    eui_deinit();
    return 0;
}
```

**RTOS 任务模式：**

```c
void ui_task(void *param) {
    while (1) {
        eui_tick();
        vTaskDelay(pdMS_TO_TICKS(33));
    }
}
```

---

## 步骤 4：内存配置

### 默认配置 (8KB 池)

```bash
cmake -B build -DEUI_MEM_POOL_SIZE=8192
```

### 极低内存 (4KB 池，1bpp OLED)

```bash
cmake -B build \
    -DEUI_MEM_POOL_SIZE=4096 \
    -DEUI_COLOR_DEPTH=1 \
    -DEUI_MAX_WIDGETS=8
```

### 典型应用内存预算

以 32KB RAM 的 STM32F103C8T6 为例：

| 区域 | 大小 | 说明 |
|------|------|------|
| TLSF 内存池 | 8 KB | 运行时对象分配 |
| 帧缓冲 (PAGE) | 128 B | 128×8/8 (1bpp) |
| 字体 (Flash) | 4 KB | 内置 8×8 字体 |
| 栈空间 | 2 KB | 回调嵌套 |
| 应用数据 | 4 KB | 业务逻辑 |

---

## 步骤 5：验证

### 桌面模拟测试

在移植到硬件前，先用 raylib 验证 UI 逻辑：

```bash
cmake -B build -DEUI_BUILD_EXAMPLES=ON
cmake --build build
./build/examples/menu_system
```

### 嵌入式单元测试

```bash
cmake -B build -DEUI_BUILD_TESTS=ON
cmake --build build
cd build && ctest
```

---

## 常见问题

**Q: 内存不足？**
- 减小 `EUI_MEM_POOL_SIZE`
- 使用 `EUI_BUFFER_PAGE` 缓冲模式
- 减少 `EUI_MAX_WIDGETS` / `EUI_MAX_VIEWS`

**Q: 屏幕不刷新？**
- 确认 `eui_canvas_commit()` 被调用（框架在 `eui_tick()` 中自动调用）
- 确认 `write_buffer` 正确实现了硬件写入

**Q: 按键无响应？**
- 确认 `poll` 返回 1 且有事件填充
- 确认消抖配置正确（默认 20ms）
- 用 `EU I_BUILD_EXAMPLES` 在 PC 上先验证输入逻辑

**Q: 如何添加新屏幕驱动？**
参考 `src/driver/eui_drv_raylib.c` 的 Display HAL 实现模式，实现你自己的 `eui_display_hal_t`。

---

## 使用内置驱动（新）

EUI 现在提供内置的平台无关驱动模板，放在 `include/eui/driver/` 目录下。只需实现传输回调（I2C/SPI/GPIO），即可快速接入：

### 使用 SSD1306 驱动

```c
#include "eui.h"
#include "eui/driver/eui_drv_ssd1306.h"

/* 1. 实现 I2C 传输回调 */
static void my_i2c_write_cmd(uint8_t cmd, void *ud) {
    /* platform-specific: send command byte via I2C */
}
static void my_i2c_write_data(const uint8_t *buf, uint32_t len, void *ud) {
    /* platform-specific: send data bytes via I2C */
}
static void my_delay_ms(uint32_t ms, void *ud) {
    /* platform-specific: delay */
}

/* 2. 创建驱动 */
eui_drv_ssd1306_config_t disp_cfg = {
    .i2c = { my_i2c_write_cmd, my_i2c_write_data, my_delay_ms, NULL },
    .width = 128, .height = 64, .i2c_addr = 0x3C,
};
eui_display_hal_t *display = eui_drv_ssd1306_create(&disp_cfg);
```

### 使用 GPIO 按键驱动

```c
#include "eui/driver/eui_drv_buttons.h"

/* 项目自定义编号（示例）：0=确认键、1=侧键；eui 不定义编号语义 */
#define MY_KEY_OK   0u
#define MY_KEY_SIDE 1u

const eui_drv_buttons_map_t map[] = {
    { .pin_id = 0, .key = MY_KEY_OK },
    { .pin_id = 1, .key = MY_KEY_SIDE },
    { .pin_id = 2, .key = 2 },
};

eui_drv_buttons_config_t btn_cfg = {
    .gpio = { .read_pin = my_gpio_read, .delay_us = my_delay_us },
    .map = map, .count = 3,
};
eui_input_drv_t *input = eui_drv_buttons_create(&btn_cfg);
```

按键按下/释放经 `map[].key` 指定的 `key_id` 原样发出（`EUI_EVT_KEY_PRESS`/`EUI_EVT_KEY_RELEASE`），再由 core 手势装配器组装为 click/hold 手势事件。

### 可用的内置驱动

| 驱动 | 头文件 | 类型 |
|------|--------|------|
| SSD1306 I2C OLED | `eui/driver/eui_drv_ssd1306.h` | Display |
| SH1106 I2C OLED | `eui/driver/eui_drv_sh1106.h` | Display |
| ST7735 SPI TFT | `eui/driver/eui_drv_st7735.h` | Display |
| ILI9341 SPI TFT | `eui/driver/eui_drv_ili9341.h` | Display |
| GPIO 按键 | `eui/driver/eui_drv_buttons.h` | Input |
| 旋转编码器 | `eui/driver/eui_drv_encoder.h` | Input |
| XPT2046 触摸屏 | `eui/driver/eui_drv_xpt2046.h` | Input |

传输层定义见 `eui/hal/eui_hal_types.h`。
