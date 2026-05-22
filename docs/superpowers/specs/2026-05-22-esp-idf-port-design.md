# ESP-IDF 平台移植设计文档

**版本**: 1.0  
**日期**: 2026-05-22  
**状态**: 设计中

## 1. 概述

为 EUI 框架移植第一个嵌入式平台——ESP-IDF（Espressif IoT Development Framework）。通过封装 ESP-IDF 的 I2C、SPI、GPIO 驱动 API，补齐 `eui_hal_i2c_t`、`eui_hal_spi_t`、`eui_hal_gpio_t` 三个传输层回调，使得内置的 SSD1306、ST7735 等芯片驱动可直接在 ESP32 系列芯片上运行。

同时提供两个示例项目：SSD1306 I2C OLED 演示（1bpp PAGE 模式）和 ST7735 SPI TFT 演示（16bpp FULL/PAGE 可选模式）。

### 术语约定

| 层级 | 含义 | 示例 |
|------|------|------|
| **transport** | 传输回调类型定义（I2C/SPI/GPIO 函数指针） | `eui_hal_i2c_t`、`eui_hal_spi_t` |
| **port** | 平台适配层——用特定平台 SDK 填充传输回调 | `eui_port_esp_idf_i2c_create()` |
| **driver** | 芯片协议驱动——通过传输层与硬件通信 | `eui_drv_ssd1306_create()` |

### 范围

- **传输层适配**：I2C master、SPI master、GPIO 输入
- **平台**：ESP-IDF v5.x（ESP32 / ESP32-S3 / ESP32-C3）
- **示例**：SSD1306 (128x64, 1bpp, I2C)、ST7735 (240x240, 16bpp, SPI)
- **构建系统**：双模 CMake——同时支持 ESP-IDF 组件和独立 CMake 编译

## 2. 文件结构

```
port/esp-idf/
├── CMakeLists.txt                    # 双模：idf_component_register + 独立 CMake
├── eui_port_esp_idf.h                # 公共头文件
└── eui_port_esp_idf.c                # 实现（依赖 esp-idf driver API）

examples/esp-idf/
├── ssd1306/
│   ├── CMakeLists.txt                # ESP-IDF 项目构建
│   └── main/
│       └── main.c                    # SSD1306 128x64 1bpp 演示
└── st7735/
    ├── CMakeLists.txt                # ESP-IDF 项目构建
    └── main/
        └── main.c                    # ST7735 240x240 16bpp 演示
```

## 3. 传输层适配 API

### 3.1 I2C

```c
typedef struct {
    i2c_port_t  port;
    gpio_num_t  sda;
    gpio_num_t  scl;
    uint32_t    freq;
    uint16_t    timeout_ms;
} esp_idf_i2c_config_t;

eui_hal_i2c_t* eui_port_esp_idf_i2c_create(const esp_idf_i2c_config_t *cfg);
void eui_port_esp_idf_i2c_destroy(eui_hal_i2c_t *hal);
```

内部行为：
- 调用 `i2c_master_bus_config()` + `i2c_new_master_bus()` 初始化 I2C 总线
- `write_cmd` 回调：启动 I2C 写事务，先发控制字节（Co=0, D/C=0），再发命令字节
- `write_data` 回调：启动 I2C 写事务，先发控制字节（Co=0, D/C=1），再发数据
- `delay_ms` 回调：调用 `vTaskDelay(pdMS_TO_TICKS(ms))`
- 总线句柄存储在 `user_data` 中

### 3.2 SPI

```c
typedef struct {
    spi_host_device_t host;
    gpio_num_t        mosi;
    gpio_num_t        sclk;
    gpio_num_t        cs;
    gpio_num_t        dc;
    gpio_num_t        rst;
    int               freq;
    int               queue_size;
} esp_idf_spi_config_t;

eui_hal_spi_t* eui_port_esp_idf_spi_create(const esp_idf_spi_config_t *cfg);
void eui_port_esp_idf_spi_destroy(eui_hal_spi_t *hal);
```

内部行为：
- 调用 `spi_bus_initialize()` 初始化 SPI 总线
- 调用 `spi_bus_add_device()` 添加设备（用于 CS 自动管理）
- `write_cmd` 回调：设置 DC=0，`spi_device_transmit()` 发送命令
- `write_data` 回调：设置 DC=1，`spi_device_transmit()` 发送数据
- `set_dc` / `set_cs` / `set_rst`：直接操作 GPIO
- `delay_ms` 回调：调用 `vTaskDelay(pdMS_TO_TICKS(ms))`
- `read_data` 回调：标准 SPI 全双工读取（ST7735 不使用，XPT2046 需要）

### 3.3 GPIO

```c
typedef struct {
    uint32_t pin_mask;
    bool     pull_up;
} esp_idf_gpio_config_t;

eui_hal_gpio_t* eui_port_esp_idf_gpio_create(const esp_idf_gpio_config_t *cfg);
void eui_port_esp_idf_gpio_destroy(eui_hal_gpio_t *hal);
```

内部行为：
- 调用 `gpio_config()` 初始化引脚为输入模式
- `read_pin` 回调：调用 `gpio_get_level(pin_id)`
- `delay_us` 回调：调用 `esp_rom_delay_us(us)`

## 4. 示例设计

### 4.1 SSD1306 示例 (examples/esp-idf/ssd1306/main/main.c)

```
演示内容：显示 "Hello EUI!" 标签 + 电池电量进度条
硬件：SSD1306 128x64 I2C OLED (地址 0x3C)
颜色：1bpp（黑白）
缓冲：PAGE 模式（128 x 8 / 8 = 128 字节页缓冲）
```

流程：
1. `eui_allocator_init_tlsf()` — 初始化内存池
2. `eui_port_esp_idf_i2c_create()` — 创建 I2C 传输层
3. `eui_drv_ssd1306_create()` — 创建 SSD1306 显示驱动
4. `eui_init()` + `eui_set_tick_callback()` — 初始化框架
5. `display->init()` — 硬件初始化
6. 创建 Label + Progress 控件
7. 主循环 `eui_tick()` → `vTaskDelay(10)`

### 4.2 ST7735 示例 (examples/esp-idf/st7735/main/main.c)

```
演示内容：彩色标签 + 按钮交互 + 圆形绘制
硬件：ST7735 240x240 SPI TFT
颜色：16bpp (RGB565)
缓冲：通过编译开关选择 FULL 或 PAGE 模式
```

编译开关（在 CMakeLists.txt 中定义）：
- `EUI_ST7735_BUFFER_FULL`：全帧缓冲 240×240×2 = 115,200 字节，需 PSRAM
- `EUI_ST7735_BUFFER_PAGE`：页缓冲 240×8×2 = 3,840 字节，无需 PSRAM

流程：
1. `eui_allocator_init_tlsf()` — 初始化内存池（FULL 模式需在 PSRAM 中分配）
2. `eui_port_esp_idf_spi_create()` — 创建 SPI 传输层
3. `eui_drv_st7735_create()` — 创建 ST7735 显示驱动（variant=1, red tab, 240x240）
4. `eui_init()` + `eui_set_tick_callback()` — 初始化框架
5. `display->init()` — 硬件初始化
6. 创建 Button + Label 控件
7. 主循环

## 5. CMake 构建设计

### port/esp-idf/CMakeLists.txt

双模设计：
```cmake
# ESP-IDF 组件模式检测
if(DEFINED ENV{IDF_PATH} OR DEFINED IDF_PATH)
    idf_component_register(
        SRCS            eui_port_esp_idf.c
        INCLUDE_DIRS    .
        REQUIRES        driver eui
    )
else()
    # 独立 CMake 模式
    add_library(eui_port_esp_idf STATIC eui_port_esp_idf.c)
    target_include_directories(eui_port_esp_idf PUBLIC .)
    target_link_libraries(eui_port_esp_idf PUBLIC eui)
endif()
```

### 示例项目 CMakeLists.txt

标准的 ESP-IDF 项目最小化构建：
```cmake
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(ssd1306_demo)
set(EXTRA_COMPONENT_DIRS ${CMAKE_SOURCE_DIR}/../../../port/esp-idf
                          ${CMAKE_SOURCE_DIR}/../../..)
```

## 6. 内存策略

| 组件 | 内存来源 | 大小 |
|------|----------|------|
| TLSF 池 | SRAM (DRAM) | 8,192 字节（默认） |
| SSD1306 页缓冲 (128B×1) | 由 Canvas 分配 | 128 字节 |
| ST7735 FULL 帧缓冲 (240×240×2) | PSRAM (SPIRAM) | 115,200 字节 |
| ST7735 PAGE 页缓冲 (240×8×2) | SRAM | 3,840 字节 |
| 端口内部状态 | SRAM | < 100 字节 |

ST7735 FULL 模式要求 ESP32 启用 PSRAM（`CONFIG_SPIRAM=y`），并在 `eui_allocator_init_tlsf()` 时传入 PSRAM 地址。

## 7. 错误处理

- `eui_port_esp_idf_i2c_create()` / `eui_port_esp_idf_spi_create()` / `eui_port_esp_idf_gpio_create()` 返回 `NULL` 表示初始化失败（外设冲突、内存不足、引脚非法）
- `_destroy()` 对 `NULL` 参数安全返回
- 所有 GPIO 操作通过 `ESP_ERROR_CHECK` 在 Debug 构建中校验

## 8. 依赖关系

```
examples/esp-idf/ssd1306/main.c
    ├── eui_port_esp_idf_i2c_create() ──→ driver/i2c.h (ESP-IDF)
    ├── eui_drv_ssd1306_create()    ──→ eui_drv_ssd1306.c
    └── eui_init()                   ──→ libeui.a

examples/esp-idf/st7735/main.c
    ├── eui_port_esp_idf_spi_create() ──→ driver/spi_master.h (ESP-IDF)
    ├── eui_port_esp_idf_gpio_create()──→ driver/gpio.h (ESP-IDF)
    ├── eui_drv_st7735_create()      ──→ eui_drv_st7735.c
    └── eui_init()                   ──→ libeui.a
```

端口层不依赖任何 MCU 厂商 SDK 以外的库。

## 9. 变更清单

### 新增文件

| 文件 | 说明 |
|------|------|
| `port/esp-idf/CMakeLists.txt` | 双模构建文件 |
| `port/esp-idf/eui_port_esp_idf.h` | 移植层公共头文件 |
| `port/esp-idf/eui_port_esp_idf.c` | I2C/SPI/GPIO 传输回调实现 |
| `examples/esp-idf/ssd1306/CMakeLists.txt` | SSD1306 示例构建 |
| `examples/esp-idf/ssd1306/main/main.c` | SSD1306 演示程序 |
| `examples/esp-idf/st7735/CMakeLists.txt` | ST7735 示例构建 |
| `examples/esp-idf/st7735/main/main.c` | ST7735 演示程序 |

### 不涉及修改的文件

- 所有现有 `src/`、`include/` 文件均不修改
- 端口层是完全独立的新增模块

## 10. 不包含的内容

- ESP-IDF 以外的平台移植（STM32、nRF 等后续按需添加）
- BLE/WiFi 网络远程显示
- LVGL 对比或兼容层
- I2S 并行 LCD 驱动
- 低功耗休眠管理
