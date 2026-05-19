# EUI HAL 驱动集合设计文档

**版本**: 1.0  
**日期**: 2026-05-19  
**状态**: 设计中

## 1. 概述

为 EUI 框架新增一套平台无关的硬件驱动模板，覆盖常见嵌入式显示屏控制器（SSD1306、SH1106、ST7735、ILI9341）和输入设备（GPIO 按键、旋转编码器、XPT2046 触摸屏）。所有驱动实现 `eui_display_hal_t` 或 `eui_input_hal_t` 接口，可通过配置结构体初始化，底层 I2C/SPI/GPIO 传输由用户提供回调。

### 术语约定

| 层级 | 含义 | 示例 |
|------|------|------|
| **hal** | 硬件抽象层——底层传输回调类型定义，或平台模拟层 | `eui_hal_i2c_t`、`eui_hal_spi_t`、raylib 模拟 |
| **driver** | 芯片协议驱动——实现 Display/Input HAL 接口，通过传输层与硬件通信 | `eui_drv_ssd1306_*`、`eui_drv_buttons_*` |

### 范围

- **显示驱动**：SSD1306 (I2C)、SH1106 (I2C)、ST7735 (SPI)、ILI9341 (SPI)
- **输入驱动**：GPIO 独立按键、旋转编码器 (EC11)、XPT2046 触摸屏
- **传输抽象**：I2C / SPI 回调结构体定义

## 2. 文件结构

```
include/eui/
├── eui.h                              # 已有，不变
├── eui_display_hal.h                  # 已有，不变
├── eui_input_hal.h                    # 已有，不变
│
├── hal/
│   ├── eui_hal_transport.h            # 新增：I2C/SPI 传输回调类型
│   └── eui_hal_raylib.h               # 从顶层移入
│
└── driver/
    ├── eui_drv_ssd1306.h              # 新增
    ├── eui_drv_sh1106.h               # 新增
    ├── eui_drv_st7735.h               # 新增
    ├── eui_drv_ili9341.h              # 新增
    ├── eui_drv_buttons.h              # 新增
    ├── eui_drv_encoder.h              # 新增
    └── eui_drv_xpt2046.h              # 新增

src/
├── hal/
│   └── eui_hal_raylib.c               # 从 src/ 移入
│
└── driver/
    ├── eui_drv_ssd1306.c              # 新增
    ├── eui_drv_sh1106.c               # 新增
    ├── eui_drv_st7735.c               # 新增
    ├── eui_drv_ili9341.c              # 新增
    ├── eui_drv_buttons.c              # 新增
    ├── eui_drv_encoder.c              # 新增
    └── eui_drv_xpt2046.c              # 新增
```

**规则：**
- 每个驱动一个 `.c` + `.h`，独立可裁剪
- 驱动文件不依赖任何 MCU 平台 SDK
- CMake 编译时按需加入目标

## 3. 传输层抽象

### I2C 传输接口

```c
typedef struct {
    void (*write_cmd)(uint8_t cmd, void *user_data);
    void (*write_data)(const uint8_t *buf, uint32_t len, void *user_data);
    void (*delay_ms)(uint32_t ms, void *user_data);
    void *user_data;
} eui_hal_i2c_t;
```

SSD1306、SH1106 等 I2C OLED 驱动使用。

### SPI 传输接口

```c
typedef struct {
    void (*write_cmd)(uint8_t cmd, void *user_data);
    void (*write_data)(const uint8_t *buf, uint32_t len, void *user_data);
    void (*read_data)(uint8_t *buf, uint32_t len, void *user_data);
    void (*set_dc)(bool data_mode, void *user_data);
    void (*set_cs)(bool active, void *user_data);
    void (*set_rst)(bool active, void *user_data);
    void (*delay_ms)(uint32_t ms, void *user_data);
    void *user_data;
} eui_hal_spi_t;
```

ST7735、ILI9341、XPT2046 使用。`read_data` 仅在 XPT2046 中用于读取触摸坐标，TFT 显示驱动无需实现。

**设计考虑：**
- 所有回调带 `user_data` 指针，用户可将 I2C 句柄/SPI 外设实例等平台对象传入
- `delay_ms` 必须实现，显示初始化命令序列有时间间隔要求
- 按键和编码器驱动不使用传输层——它们只配置引脚编号

## 4. Display 驱动

所有 Display 驱动实现 `eui_display_hal_t` 接口，通过 `xxx_config_t` 结构体初始化。

### 4.1 SSD1306 (I2C OLED)

```c
typedef struct {
    eui_hal_i2c_t i2c;
    uint16_t      width;
    uint16_t      height;
    uint8_t       i2c_addr;     /* 默认 0x3C */
} eui_drv_ssd1306_config_t;

eui_display_hal_t* eui_drv_ssd1306_create(const eui_drv_ssd1306_config_t *cfg);
void eui_drv_ssd1306_destroy(eui_display_hal_t *hal);
```

- **缓冲模式**: PAGE（128x64 时每页 128 字节，共 8 页）
- **色深**: 1bpp
- **实现**: `set_contrast`、`set_invert`、`write_buffer`（PAGE 模式）
- 用 `i2c_addr` 传递显示屏 I2C 地址（0x3C 或 0x3D）

### 4.2 SH1106 (I2C OLED)

```c
typedef struct {
    eui_hal_i2c_t i2c;
    uint16_t      width;
    uint16_t      height;
    uint8_t       i2c_addr;     /* 默认 0x3C */
} eui_drv_sh1106_config_t;

eui_display_hal_t* eui_drv_sh1106_create(const eui_drv_sh1106_config_t *cfg);
void eui_drv_sh1106_destroy(eui_display_hal_t *hal);
```

- **分辨率**: 常见 128×64，硬件实际 132×64（驱动内部处理列偏移 2）
- **缓冲模式**: PAGE，1bpp
- 与 SSD1306 不同的内存寻址方式和初始化序列

### 4.3 ST7735 (SPI TFT)

```c
typedef struct {
    eui_hal_spi_t spi;
    uint16_t      width;
    uint16_t      height;
    uint8_t       variant;      /* 0=green tab, 1=red tab, 2=black tab */
} eui_drv_st7735_config_t;

eui_display_hal_t* eui_drv_st7735_create(const eui_drv_st7735_config_t *cfg);
void eui_drv_st7735_destroy(eui_display_hal_t *hal);
```

- **分辨率**: 常见 128×160
- **缓冲模式**: FULL，16bpp (RGB565)
- **variant**: 适配不同制造商默认方向/偏移，选不同初始化序列
- `write_buffer` 通过 `set_addr_window` + SPI 数据流一次性传输矩形区域

### 4.4 ILI9341 (SPI TFT)

```c
typedef struct {
    eui_hal_spi_t spi;
    uint16_t      width;
    uint16_t      height;
} eui_drv_ili9341_config_t;

eui_display_hal_t* eui_drv_ili9341_create(const eui_drv_ili9341_config_t *cfg);
void eui_drv_ili9341_destroy(eui_display_hal_t *hal);
```

- **分辨率**: 240×320
- **缓冲模式**: FULL，16bpp (RGB565)
- 标准 MIPI DCS 命令集，兼容性好

### 4.5 Display 驱动共同设计

- 所有驱动通过 `eui_malloc` 分配内部状态，`eui_free` 释放
- `caps` 字段根据 `width/height/color_depth` 自动填充
- OLED 驱动（SSD1306/SH1106）内部维护 PAGE 条带缓冲区
- TFT 驱动（ST7735/ILI9341）声明 `has_gram = true`

## 5. Input 驱动

所有 Input 驱动实现 `eui_input_hal_t` 接口，通过 `xxx_config_t` 结构体初始化。

### 5.1 GPIO 独立按键

```c
typedef struct {
    bool (*read_pin)(uint8_t pin_id, void *user_data);
    void (*delay_us)(uint32_t us, void *user_data);
    void *user_data;
} eui_drv_buttons_gpio_t;

typedef struct {
    uint8_t   pin_id;
    eui_key_t key;
} eui_drv_button_map_t;

typedef struct {
    eui_drv_buttons_gpio_t     gpio;
    const eui_drv_button_map_t *map;
    uint8_t                     count;
} eui_drv_buttons_config_t;

eui_input_hal_t* eui_drv_buttons_create(const eui_drv_buttons_config_t *cfg);
void eui_drv_buttons_destroy(eui_input_hal_t *hal);
```

- `pin_id` 是抽象序号，由用户 `read_pin` 内部映射到实际 GPIO
- 驱动内部维护按键前一状态，在 `poll` 中完成软件消抖和边沿检测
- 支持部分映射——不需要 6 个键全接
- 消抖时间默认 20ms

**使用示例：**

```c
const eui_drv_button_map_t map[] = {
    { PA0, EUI_KEY_UP }, { PA1, EUI_KEY_DOWN }, { PA2, EUI_KEY_OK },
};
eui_drv_buttons_config_t cfg = {
    .gpio = { .read_pin = my_gpio_read, .delay_us = my_delay_us },
    .map = map, .count = 3,
};
eui_input_hal_t *in = eui_drv_buttons_create(&cfg);
```

### 5.2 旋转编码器 (EC11)

```c
typedef struct {
    bool (*read_pin)(uint8_t pin_id, void *user_data);
    void (*delay_us)(uint32_t us, void *user_data);
    void *user_data;
} eui_drv_encoder_gpio_t;

typedef struct {
    eui_drv_encoder_gpio_t gpio;
    uint8_t                pin_a;
    uint8_t                pin_b;
    uint8_t                pin_sw;
    uint32_t               poll_interval_us;  /* 推荐 1000~2000 */
} eui_drv_encoder_config_t;

eui_input_hal_t* eui_drv_encoder_create(const eui_drv_encoder_config_t *cfg);
void eui_drv_encoder_destroy(eui_input_hal_t *hal);
```

- 内部用格雷码状态机解码 CW/CCW，比简单读取更可靠
- `poll_interval_us` 控制采样频率
- 按键部分复用消抖逻辑
- `poll` 返回 `EUI_EVT_ENCODER_CW` / `EUI_EVT_ENCODER_CCW` / `EUI_EVT_ENCODER_CLICK`

### 5.3 XPT2046 触摸屏

```c
typedef struct {
    bool (*read_irq)(void *user_data);
    void *user_data;
} eui_drv_xpt2046_irq_t;

typedef struct {
    eui_hal_spi_t           spi;
    eui_drv_xpt2046_irq_t   irq;
    uint16_t                screen_width;
    uint16_t                screen_height;
} eui_drv_xpt2046_config_t;

eui_input_hal_t* eui_drv_xpt2046_create(const eui_drv_xpt2046_config_t *cfg);
void eui_drv_xpt2046_destroy(eui_input_hal_t *hal);
```

- 内部实现 XPT2046 SPI 命令协议（读取 X/Y 坐标）
- `poll` 返回 `EUI_EVT_TOUCH_DOWN` / `EUI_EVT_TOUCH_UP` / `EUI_EVT_TOUCH_MOVE`
- 坐标已映射到屏幕坐标系
- 内置中值滤波（多次采样取中位数）降噪
- `irq.read_irq` 检测 PENIRQ 引脚（低电平有触摸）

### 5.4 Input 驱动共同设计

- 每个驱动实现完整 `eui_input_hal_t`（init / deinit / poll / set_callback）
- 内部结构体通过 `eui_malloc` 分配
- 消抖、状态机等逻辑内置于驱动，用户只需提供硬件读写回调

## 6. 错误处理

- `create()` 返回 `NULL` 表示分配失败（内存不足或参数非法）
- `destroy()` 对 `NULL` 参数安全返回
- `write_buffer` / `poll` 不返回错误码——硬件错误由用户回调内部处理

## 7. 测试策略

- **单元测试**：用 mock transport 验证各驱动的 HAL 接口实现（init/write_buffer/destroy 流程、编码器状态机、按键消抖逻辑）
- **集成测试**：在 raylib 模拟环境中用虚拟显示屏和键盘验证驱动端到端行为
- **覆盖率目标**：核心协议逻辑 80%+

## 8. 依赖关系

```
eui_drv_ssd1306 ──→ eui_hal_transport (I2C) ──→ 用户 I2C 实现
eui_drv_sh1106  ──→ eui_hal_transport (I2C) ──→ 用户 I2C 实现
eui_drv_st7735  ──→ eui_hal_transport (SPI) ──→ 用户 SPI 实现
eui_drv_ili9341 ──→ eui_hal_transport (SPI) ──→ 用户 SPI 实现
eui_drv_buttons  ──→ (无 transport 依赖) ──→ 用户 GPIO 实现
eui_drv_encoder  ──→ (无 transport 依赖) ──→ 用户 GPIO 实现
eui_drv_xpt2046  ──→ eui_hal_transport (SPI) ──→ 用户 SPI 实现

所有驱动 ──→ eui_display_hal.h / eui_input_hal.h (框架核心 HAL 接口)
所有驱动 ──→ eui_allocator.h (eui_malloc / eui_free)
```

## 9. 变更清单

### 新增文件

| 文件 | 说明 |
|------|------|
| `include/eui/hal/eui_hal_transport.h` | I2C/SPI 传输回调类型定义 |
| `include/eui/driver/eui_drv_ssd1306.h` | SSD1306 驱动头文件 |
| `include/eui/driver/eui_drv_sh1106.h` | SH1106 驱动头文件 |
| `include/eui/driver/eui_drv_st7735.h` | ST7735 驱动头文件 |
| `include/eui/driver/eui_drv_ili9341.h` | ILI9341 驱动头文件 |
| `include/eui/driver/eui_drv_buttons.h` | GPIO 按键驱动头文件 |
| `include/eui/driver/eui_drv_encoder.h` | 编码器驱动头文件 |
| `include/eui/driver/eui_drv_xpt2046.h` | XPT2046 触摸驱动头文件 |
| `src/driver/eui_drv_ssd1306.c` | SSD1306 驱动实现 |
| `src/driver/eui_drv_sh1106.c` | SH1106 驱动实现 |
| `src/driver/eui_drv_st7735.c` | ST7735 驱动实现 |
| `src/driver/eui_drv_ili9341.c` | ILI9341 驱动实现 |
| `src/driver/eui_drv_buttons.c` | GPIO 按键驱动实现 |
| `src/driver/eui_drv_encoder.c` | 编码器驱动实现 |
| `src/driver/eui_drv_xpt2046.c` | XPT2046 触摸驱动实现 |

### 移动文件

| 原路径 | 新路径 | 说明 |
|--------|--------|------|
| `include/eui/eui_hal_raylib.h` | `include/eui/hal/eui_hal_raylib.h` | 移入 hal 子目录 |
| `src/eui_hal_raylib.c` | `src/hal/eui_hal_raylib.c` | 移入 hal 子目录 |

### 修改文件

| 文件 | 修改内容 |
|------|---------|
| `include/eui/eui.h` | 更新 raylib HAL 的 include 路径 |
| `src/CMakeLists.txt` | 新增 driver 子目录的编译目标 |
| `cmake/eui_config.cmake` | 新增头文件搜索路径 |
| 所有使用 `eui_hal_raylib.h` 的文件 | 更新 include 路径 |

## 10. 不包含的内容

- ST7789 驱动（后续按需添加）
- 矩阵键盘驱动（GPIO 按键驱动可覆盖大多数场景）
- I2C 触摸屏驱动（如 FT6x06）
- SPI 模式 SSD1306/SH1106（当前仅支持 I2C）
- 硬件 SPI DMA 零拷贝模式（后续优化）
