# EUI nRF5 Port

EUI 硬件抽象层 (HAL) 移植到 Nordic nRF5 系列 SoC（nRF52832 / nRF52 系列），使用 Nordic nRF5 SDK v17.x。

## 实现的 HAL

| HAL | 实现 | 驱动 |
|-----|------|------|
| `eui_hal_i2c_t` | I2C/TWI 通信 | `nrf_drv_twi` (legacy) → `nrfx_twim` (EasyDMA) |
| `eui_hal_spi_t` | SPI 通信 | `nrf_drv_spi` (legacy) → `nrfx_spim` (EasyDMA) |
| `eui_hal_gpio_t` | GPIO 输入/输出 | `nrf_gpio` |

## 依赖

| 依赖 | 说明 |
|------|------|
| **nRF5 SDK v17.1.0+** | Nordic 官方 SDK，通过环境变量 `NRF5_SDK_ROOT` 指定路径 |
| **ARM GCC 工具链** | `arm-none-eabi-gcc` (Arm GNU Toolchain 12/13) |

## 快速开始

```bash
# 设置 nRF5 SDK 路径
export NRF5_SDK_ROOT=/path/to/nRF5_SDK_17.1.0_ddde560

# 配置并编译
cmake --preset nrf52 -B build_nrf5
cmake --build build_nrf5
```

## 构建详情

### 工具链文件

`toolchain.cmake` 配置了 ARM Cortex-M4 FPU 交叉编译：

| 参数 | 值 |
|------|-----|
| CPU | `-mcpu=cortex-m4` |
| 指令集 | `-mthumb -mabi=aapcs` |
| 浮点 | `-mfloat-abi=hard -mfpu=fpv4-sp-d16` |
| 优化 | `-ffunction-sections -fdata-sections -fno-strict-aliasing -fno-builtin -fshort-enums` |
| 链接 | `--specs=nosys.specs -Wl,--gc-sections` |
| 系统 | Bare-metal (`CMAKE_SYSTEM_NAME Generic`) |

### CMake 预设

预设 `nrf52` 定义在 `CMakePresets.json` 中：

```json
{
    "name": "nrf52",
    "toolchainFile": "${sourceDir}/port/nrf5/toolchain.cmake",
    "cacheVariables": {
        "EUI_TARGET_PORT": "nrf5",
        "EUI_BUILD_CROSS_EXAMPLES": "ON",
        "EUI_BUILD_EXAMPLES": "OFF",
        "EUI_BUILD_TESTS": "OFF"
    }
}
```

### CMake 变量

| 变量 | 默认值 | 说明 |
|------|--------|------|
| `NRF5_SDK_ROOT` | (环境变量) | nRF5 SDK 根目录 |
| `NRF5_CHIP` | `NRF52832_XXAA` | 目标芯片型号 |
| `NRF5_FAMILY` | `NRF52` | 目标芯片系列 |

### 编译的 SDK 源文件

构建系统将以下 nRF5 SDK 源文件编译为 `libnrf5_sdk.a`：

- `system_nrf52.c`, `gcc_startup_nrf52.S` — 系统启动
- `nrfx_twi.c`, `nrfx_twim.c`, `nrfx_spim.c` — 通信驱动
- `nrfx_clock.c`, `nrfx_power.c`, `nrfx_timer.c`, `nrfx_rtc.c` — 时钟与电源
- `nrfx_prs.c`, `nrfx_atomic.c` — 外设资源共享 & 原子操作
- `nrf_drv_twi.c`, `nrf_drv_spi.c`, `nrf_drv_clock.c`, `nrf_drv_power.c` — Legacy 封装
- `app_timer.c`, `app_error.c`, `app_util_platform.c` — 应用工具
- `nrf_assert.c`, `nrf_atomic.c`, `nrf_balloc.c`, `nrf_memobj.c`, `nrf_ringbuf.c`, `nrf_strerror.c` — 基础库
- `nrf_fprintf.c`, `nrf_fprintf_format.c` — 格式化输出
- `boards.c` — 板级支持

## 配置

### sdk_config.h

`port/nrf5/sdk_config.h` 为最简配置，启用了：

- TWI0/1 (EasyDMA: NRF_DRV_TWI_USE_TWIM=1)
- SPI0/1/2 (EasyDMA: NRF_DRV_SPI_USE_SPIM=1)
- GPIO + GPIOTE
- APP_TIMER (使用 RTC1)
- CLOCK (LF 时钟源: RC 振荡器)
- POWER (DCDC 禁用)
- 所有中断优先级 = 6 (APP_IRQ_PRIORITY_LOWEST)
- 日志功能: 关闭

> **为什么只用 EasyDMA？** nRF52832 上 SPI0/TWI0 共享同一个中断向量。同时编译 `nrfx_twi.c` 和 `nrfx_spi.c` 会产生重复定义的 IRQ Handler。通过只使用 SPIM/TWIM (EasyDMA) 版本可以避免该冲突。

### 引脚配置

默认引脚定义在 `eui_port_bootstrap.c` 中（可通过 `sdk_config.h` 覆盖）：

| 配置 | 值 | 说明 |
|------|-----|------|
| `CONFIG_EUI_EXAMPLE_I2C_SDA` | 26 | I2C 数据引脚 |
| `CONFIG_EUI_EXAMPLE_I2C_SCL` | 27 | I2C 时钟引脚 |
| `CONFIG_EUI_EXAMPLE_I2C_FREQ` | 400000 | I2C 频率 (Hz) |
| `CONFIG_EUI_EXAMPLE_I2C_ADDR` | 0x3C | I2C 设备地址 |
| `CONFIG_EUI_EXAMPLE_DISPLAY_WIDTH` | 128 | 显示宽度 (像素) |
| `CONFIG_EUI_EXAMPLE_DISPLAY_HEIGHT` | 64 | 显示高度 (像素) |

## 构建跨平台示例

### 使用 CMake 预设（推荐）

```bash
export NRF5_SDK_ROOT=/path/to/nRF5_SDK_17.1.0_ddde560

# 构建所有跨平台示例
cmake --preset nrf52 -B build_nrf5
cmake --build build_nrf5
```

### 使用配置 profile

配置 profile 文件位于 `configs/nrf52/`，预设了显示驱动和引脚配置：

```bash
export NRF5_SDK_ROOT=/path/to/nRF5_SDK_17.1.0_ddde560

# SSD1306 OLED 128x64, 1bpp
cmake -B build_nrf5 \
      -DCMAKE_TOOLCHAIN_FILE=port/nrf5/toolchain.cmake \
      -DEUI_BUILD_CROSS_EXAMPLES=ON \
      -DEUI_CONFIG_PROFILE="configs/nrf52/ssd1306_i2c_128x64_1bpp.cmake"
cmake --build build_nrf5

# ST7735 TFT 240x240, 16bpp
cmake -B build_nrf5 \
      -DCMAKE_TOOLCHAIN_FILE=port/nrf5/toolchain.cmake \
      -DEUI_BUILD_CROSS_EXAMPLES=ON \
      -DEUI_CONFIG_PROFILE="configs/nrf52/st7735_spi_240x240_16bpp.cmake"
cmake --build build_nrf5
```

### 可用 profile

| Profile | 显示 | 尺寸 | 色深 |
|---------|------|------|------|
| `configs/nrf52/ssd1306_i2c_128x64_1bpp.cmake` | SSD1306 (I2C) | 128×64 | 1bpp |
| `configs/nrf52/st7735_spi_240x240_16bpp.cmake` | ST7735 (SPI) | 240×240 | 16bpp |

### 可用示例

与 raylib 和 esp-idf 端口共享完全相同的示例代码。只需修改
`EUI_CONFIG_PROFILE` 即可切换目标显示硬件。示例要求（色深、尺寸）
通过 `requirements.cmake` 自动检查，不满足条件的示例会被跳过。

## 运行架构

```
main()
  └─ eui_allocator_init_tlsf()         # 分配 8KB 内存池
  └─ eui_port_nrf5_i2c_create()        # 创建 I2C 传输层
  └─ eui_drv_ssd1306_create()          # 创建 SSD1306 显示驱动
  └─ eui_init()                        # 初始化 EUI
  └─ eui_set_tick_callback()           # 基于 app_timer RTC 的 tick
  └─ display->init()                   # OLED 初始化
  └─ eui_example_setup()               # 构建 UI
  └─ while(1) { eui_tick(); nrf_delay_ms(16); }
```

- Bare-metal 裸机运行，无 RTOS
- Tick 通过 `app_timer_cnt_get()` + RTC 实现
- 每 16ms 刷新一次 (~62.5 FPS)

## SPI0/TWI0 IRQ 共享问题

nRF52832 上 SPI0 和 TWI0 共享同一个 NVIC 中断向量（同样 SPI1 和 TWI1 也共享）。解决方法：使用 EasyDMA (SPIM/TWIM) 代替非 DMA 版本。此配置已在 `sdk_config.h` 中通过 `NRF_DRV_TWI_USE_TWIM=1` 和 `NRF_DRV_SPI_USE_SPIM=1` 启用。
