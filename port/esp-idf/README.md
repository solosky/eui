# EUI ESP-IDF Port

EUI 硬件抽象层 (HAL) 移植到 Espressif ESP32 系列芯片，使用 ESP-IDF 框架。

## 实现的 HAL

| HAL | 实现 | 驱动 |
|-----|------|------|
| `eui_hal_i2c_t` | I2C 主机通信 | `driver/i2c.h` |
| `eui_hal_spi_t` | SPI 主机通信 | `driver/spi_master.h` |
| `eui_hal_gpio_t` | GPIO 输入 | `driver/gpio.h`, `esp_rom_gpio.h` |

## 依赖

| 依赖 | 说明 |
|------|------|
| **ESP-IDF** | Espressif IoT Development Framework (v4.0+) |
| **FreeRTOS** | ESP-IDF 内置，提供任务延时 |
| **Xtensa / RISC-V GCC** | ESP-IDF 工具链 |

## 快速开始

### 作为 ESP-IDF 组件使用

1. 将 `port/esp-idf/` 目录添加到项目的 `EXTRA_COMPONENT_DIRS`
2. 使用 `menuconfig` 配置引脚和显示参数
3. 标准 ESP-IDF 构建流程：

```bash
# 设置 IDF 环境
. $IDF_PATH/export.sh
# 或 . /path/to/esp-idf/export.sh

# 配置
idf.py menuconfig
# 在 EUI Example 子菜单中设置 I2C 引脚、地址、显示尺寸

# 编译
idf.py build
```

### 作为独立 CMake 库编译

当 `IDF_PATH` 未设置时，端口的 CMakeLists.txt 会自动切换到独立编译模式，仅依赖 EUI 核心库：

```bash
cmake -DEUI_BUILD_CROSS_EXAMPLES=ON \
      -DEUI_TARGET_PORT=esp-idf \
      -DCMAKE_TOOLCHAIN_FILE=$IDF_PATH/tools/cmake/toolchain-esp32.cmake \
      -B build_esp
cmake --build build_esp
```

## CMake 变量

| 变量 | 默认值 | 说明 |
|------|--------|------|
| `IDF_PATH` | (环境变量) | ESP-IDF 根目录 |

端口 CMakeLists.txt 支持双模式：

- **IDF 模式** (`IDF_PATH` 已设置): 使用 `idf_component_register()` 注册为 ESP-IDF 组件
- **Standalone 模式** (`IDF_PATH` 未设置): 使用 `add_library()` 构建静态库

## 配置 (Kconfig)

通过 ESP-IDF 的 `menuconfig` 系统配置（在 `EUI Example` 子菜单中）：

### I2C 配置

| 配置 | 默认值 | 说明 |
|------|--------|------|
| `CONFIG_EUI_EXAMPLE_I2C_PORT` | 0 | I2C 端口号 |
| `CONFIG_EUI_EXAMPLE_I2C_SDA` | 21 | SDA 引脚 |
| `CONFIG_EUI_EXAMPLE_I2C_SCL` | 22 | SCL 引脚 |
| `CONFIG_EUI_EXAMPLE_I2C_FREQ` | 400000 | I2C 频率 (Hz) |
| `CONFIG_EUI_EXAMPLE_I2C_ADDR` | 0x3C | I2C 设备地址 |
| `CONFIG_EUI_EXAMPLE_I2C_TIMEOUT` | 100 | 超时 (ms) |

### 显示配置

| 配置 | 默认值 | 说明 |
|------|--------|------|
| `CONFIG_EUI_EXAMPLE_DISPLAY_WIDTH` | 128 | 显示宽度 (像素) |
| `CONFIG_EUI_EXAMPLE_DISPLAY_HEIGHT` | 64 | 显示高度 (像素) |

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
| 目标 | ESP32 | nRF52832 | PC |
| RTOS | FreeRTOS | Bare-metal | 无 |
| 入口点 | `app_main()` | `main()` | `main()` |
| Tick 来源 | `esp_timer_get_time()` | `app_timer` RTC | `GetTime()` |
| 引脚未使用 | `GPIO_NUM_NC` | `NRF5_PIN_NOT_USED` | N/A |
