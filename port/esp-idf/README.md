# EUI ESP-IDF Port

EUI 硬件抽象层 (HAL) 移植到 Espressif ESP32 系列芯片，使用 ESP-IDF 框架。

## 支持的芯片

| 芯片 | 架构 | 状态 |
|------|------|------|
| ESP32 | Xtensa LX6 | 已支持 |
| ESP32-S2 | Xtensa LX7 | 已支持 |
| ESP32-S3 | Xtensa LX7 | 已验证 |
| ESP32-C3 | RISC-V | 理论支持 |

## 实现的 HAL

| HAL | 实现 | 驱动 |
|-----|------|------|
| `eui_hal_i2c_t` | I2C 主机通信 | `driver/i2c.h` |
| `eui_hal_spi_t` | SPI 主机通信 | `driver/spi_master.h` |
| `eui_hal_gpio_t` | GPIO 输入 | `driver/gpio.h`, `esp_rom_gpio.h` |

## 依赖

| 依赖 | 说明 |
|------|------|
| **ESP-IDF** | Espressif IoT Development Framework (v5.0+) |
| **FreeRTOS** | ESP-IDF 内置，提供任务延时 |
| **Xtensa / RISC-V GCC** | ESP-IDF 工具链 |

## 快速开始

### 环境准备

```bash
# 1. 安装 ESP-IDF
git clone -b v5.2.3 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf && ./install.sh esp32s3

# 2. 设置环境变量
. /path/to/esp-idf/export.sh
```

### 构建单个示例

EUI 的 IDE 构建项目位于 `examples/cross/esp_idf_build/`，使用此目录作为 IDF 项目根目录：

```bash
# 设置环境
. $IDF_PATH/export.sh

# 进入 IDF 项目目录
cd examples/cross/esp_idf_build

# 选择芯片 (esp32s3, esp32, esp32s2)
idf.py set-target esp32s3

# 构建指定示例
idf.py -DEUI_EXAMPLE=basic_label build
```

`-DEUI_EXAMPLE` 可选值为：

| 示例 | 说明 | 最低要求 | Profile |
|------|------|----------|---------|
| `basic_label` | 基础文本标签 | 无 | default |
| `button_test` | 按钮交互测试 | 无 | default |
| `list_nav` | 列表导航 | 无 | default |
| `menu_system` | 菜单系统 | 无 | default |
| `dialog_overlay` | 对话框覆盖层 | 无 | default |
| `animation_demo` | 动画效果演示 | 无 | default |
| `custom_widget` | 自定义控件 | 无 | default |
| `page_buffer` | 页缓冲模式 | 无 | default |
| `benchmark` | 性能基准测试 | 无 | default |
| `scene_view_demo` | 场景视图演示 | 无 | default |
| `desktop_launcher` | 桌面启动器 | 2bpp + 400×300 | `st7306_400x300` |
| `amiibo_demo` | Amiibo 风格 UI | 16bpp + 240×240 | tft (待添加) |
| `color_demo` | 色彩演示 | 16bpp + 240×240 | tft (待添加) |

### 使用 Display Profile

高规格示例需要指定 `-DEUI_PROFILE=<name>` 参数选择对应的显示驱动配置：

```bash
. $IDF_PATH/export.sh
cd examples/cross/esp_idf_build

# desktop_launcher 需要 400×300 + 2bpp，使用 ST7306 SPI 屏
idf.py set-target esp32s3
idf.py -DEUI_EXAMPLE=desktop_launcher -DEUI_PROFILE=st7306_400x300 build
```

可用 profile：

| Profile | 驱动 | 尺寸 | 色深 | 接口 |
|---------|------|------|------|------|
| `default` (或不指定) | SSD1306 | 128×64 | 1bpp | I2C |
| `st7306_400x300` | ST7306 | 400×300 | 2bpp | SPI |

对应的 CMake 配置 profile 位于 `configs/esp32/` 目录，可用于 `EUI_BUILD_CROSS_EXAMPLES` 独立构建流程。

### 批量构建所有示例

```bash
. $IDF_PATH/export.sh
cd examples/cross/esp_idf_build
./build_all.sh esp32s3
```

构建完成后，二进制文件输出到 `build_output/` 目录：
```
build_output/
  ├── basic_label_esp32s3.bin
  ├── button_test_esp32s3.bin
  ├── list_nav_esp32s3.bin
  └── ...
```

### 烧录

```bash
# 构建完成后烧录
idf.py -DEUI_EXAMPLE=basic_label build flash monitor

# 或指定端口
idf.py -p /dev/ttyUSB0 flash
```

## 项目结构

```
examples/cross/esp_idf_build/  # IDF 项目根目录
  ├── CMakeLists.txt           # 顶层 CMake，注册 EUI 组件
  ├── build_all.sh             # 批量构建脚本
  ├── main/
  │   └── CMakeLists.txt       # 主组件，根据 EUI_EXAMPLE 选择示例源码
  └── build_output/            # 构建产物输出目录
```

## 组件架构

EUI 作为 IDF 项目中的三个独立组件：

| 组件 | 路径 | 说明 |
|------|------|------|
| `eui` | 项目根目录 | EUI 框架核心（canvas, font, widget, driver） |
| `eui_port` | `port/esp-idf/eui_port` | I2C/SPI/GPIO HAL 实现 |
| `tlsf` | `third_party/tlsf` | TLSF 内存分配器 |
| `motionc` | `third_party/motionc` | MotionC 动画引擎 |

示例源码通过 `main/` 组件的 `EUI_EXAMPLE` CMake 变量选择，与 `eui_port_bootstrap.c`（app_main 入口）编译链接。

## 自定义配置

### I2C 引脚配置

编辑 `port/esp-idf/eui_port_bootstrap.c` 中的默认引脚定义：

```c
// SSD1306 I2C 默认引脚 (ESP32-S3)
#define CONFIG_EUI_EXAMPLE_I2C_PORT   0
#define CONFIG_EUI_EXAMPLE_I2C_SDA    21
#define CONFIG_EUI_EXAMPLE_I2C_SCL    22
#define CONFIG_EUI_EXAMPLE_I2C_FREQ   400000
#define CONFIG_EUI_EXAMPLE_I2C_ADDR   0x3C
```

### SPI 引脚配置

ST7306 等 SPI 设备需要启用 `EUI_DRV_ST7306` 编译宏并配置引脚。

### 显示尺寸

```c
#define CONFIG_EUI_EXAMPLE_DISPLAY_WIDTH  128
#define CONFIG_EUI_EXAMPLE_DISPLAY_HEIGHT 64
```

### 内存池大小

默认 8KB，ST7306 等大尺寸显示自动调整为 64KB。在 `port/esp-idf/eui_port_bootstrap.c` 中通过 `POOL_SIZE` 宏配置。

## 运行架构

```
app_main()  (FreeRTOS 任务入口)
  └─ eui_allocator_init_tlsf()          # 分配 8KB 内存池
  └─ eui_port_esp_idf_i2c_create()      # 创建 I2C 传输层
  └─ eui_drv_ssd1306_create()           # 创建 SSD1306 显示驱动
  └─ eui_init()                         # 初始化 EUI
  └─ eui_set_tick_callback()            # 基于 esp_timer_get_time() 的 tick
  └─ display->init()                    # OLED 初始化
  └─ eui_example_setup()                # 构建 UI
  └─ while(1) { eui_tick(); vTaskDelay(16ms); }
```

- 运行于 FreeRTOS 任务上下文
- Tick 通过 `esp_timer_get_time()` 获取微秒精度时间，转换为毫秒
- 每 16ms 通过 `vTaskDelay` 延时刷新 (~62.5 FPS)

## SPI 模式

SPI 配置为 MODE 0 (CPOL=0, CPHA=0)，DMA 禁用。最大传输大小 4092 字节。

## 与其他端口对比

| 特性 | ESP-IDF | nRF5 | Raylib |
|------|---------|------|--------|
| 目标 | ESP32/S2/S3 | nRF52832 | PC |
| RTOS | FreeRTOS | Bare-metal | 无 |
| 入口点 | `app_main()` | `main()` | `main()` |
| Tick 来源 | `esp_timer_get_time()` | `app_timer` RTC | `GetTime()` |
| 引脚未使用 | `GPIO_NUM_NC` | `NRF5_PIN_NOT_USED` | N/A |
