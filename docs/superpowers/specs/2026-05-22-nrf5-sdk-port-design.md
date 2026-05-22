# nRF5 SDK 平台移植设计文档

**版本**: 1.0  
**日期**: 2026-05-22  
**状态**: 设计中

## 1. 概述

为 EUI 框架添加 nRF5 SDK v17.x 平台移植，封装 nRF5 SDK 的 TWI (I2C)、SPI 和 GPIO 外设 API，补齐 `eui_hal_i2c_t`、`eui_hal_spi_t`、`eui_hal_gpio_t` 三个传输层回调，使得内置的 SSD1306、SH1106、ST7735、ILI9341 等芯片驱动可直接在 nRF52 系列芯片上运行。

设计完全镜像现有的 `port/esp-idf/` 移植结构，保持一致的 API 模式和代码风格。

### 术语约定

| 层级 | 含义 | 示例 |
|------|------|------|
| **transport** | 传输回调类型定义（I2C/SPI/GPIO 函数指针） | `eui_hal_i2c_t`、`eui_hal_spi_t` |
| **port** | 平台适配层——用特定平台 SDK 填充传输回调 | `eui_port_nrf5_i2c_create()` |
| **driver** | 芯片协议驱动——通过传输层与硬件通信 | `eui_drv_ssd1306_create()` |

### 范围

- **传输层适配**：I2C master (TWI)、SPI master、GPIO 输入/输出
- **平台**：nRF5 SDK v17.x（nRF52832 / nRF52840）
- **构建系统**：双模 CMake——nRF5 SDK 集成模式 + 独立 CMake 编译
- **不包含**：nRF Connect SDK / Zephyr（后续按需添加）、示例项目、触摸屏专用工厂

## 2. 文件结构

```
port/nrf5/
├── CMakeLists.txt                    # 双模：nRF5 SDK 集成 + 独立 CMake
├── eui_port_nrf5.h                   # 公共头文件（配置结构体 + 工厂声明）
└── eui_port_nrf5.c                   # 实现（依赖 nrf_drv_twi、nrf_drv_spi、nrf_gpio）
```

严格镜像 `port/esp-idf/` 的文件结构，一个目录三个文件。

## 3. 传输层适配 API

### 3.1 I2C (TWI)

```c
typedef struct {
    uint8_t     sda;
    uint8_t     scl;
    uint8_t     addr;       // 7-bit device address
    uint32_t    freq;       // 100000 or 400000
    uint8_t     instance_id; // 0 = TWI0, 1 = TWI1
} nrf5_i2c_config_t;

eui_hal_i2c_t* eui_port_nrf5_i2c_create(const nrf5_i2c_config_t *cfg);
void eui_port_nrf5_i2c_destroy(eui_hal_i2c_t *hal);
```

内部行为：
- 使用 nRF5 SDK legacy 层 `nrf_drv_twi` API
- `nrf_drv_twi_init()` 初始化 TWI 外设，传入 `nrf_drv_twi_config_t`（含 sda, scl, frequency, interrupt_priority）
- `nrf_drv_twi_enable()` 使能外设
- `write_cmd` 回调：构建 2 字节缓冲 `{0x00, cmd}`，调用 `nrf_drv_twi_tx(addr, buf, 2, false)`。控制字节 0x00 = Co=0, D/C#=0
- `write_data` 回调：构建 N+1 字节缓冲 `{0x40, data[0..N-1]}`，调用 `nrf_drv_twi_tx(addr, buf, len+1, false)`。控制字节 0x40 = Co=0, D/C#=1
- `delay_ms` 回调：调用 nRF5 SDK 的 `nrf_delay_ms(ms)`
- 私有结构 `i2c_priv_t` 存储 `twi_instance` 句柄和设备地址

与 ESP-IDF TWI 的关键差异：nRF5 SDK 的 `nrf_drv_twi_tx()` 在每次调用时传入 7-bit 地址；控制字节需手动拼入数据缓冲。

### 3.2 SPI

```c
typedef struct {
    uint8_t     mosi;
    uint8_t     miso;       // NRF_DRV_SPI_PIN_NOT_USED if unused
    uint8_t     sck;
    uint8_t     cs;
    uint8_t     dc;
    uint8_t     rst;
    uint32_t    freq;       // e.g. 8000000
    uint8_t     mode;       // SPI mode 0-3
    uint8_t     instance_id;
    uint8_t     orc;        // over-run character, typically 0xFF
} nrf5_spi_config_t;

eui_hal_spi_t* eui_port_nrf5_spi_create(const nrf5_spi_config_t *cfg);
void eui_port_nrf5_spi_destroy(eui_hal_spi_t *hal);
```

内部行为：
- 使用 `nrf_drv_spi` API
- `nrf_drv_spi_init()` 初始化 SPI 外设，传入 `nrf_drv_spi_config_t`（含 mosi, miso, sck, frequency, mode, bit_order, irq_priority）
- DC/CS/RST 引脚通过 `nrf_gpio_cfg_output()` 配置为输出
- `write_cmd` 回调：设置 DC=0，构建 `nrf_drv_spi_xfer_desc_t`，调用 `nrf_drv_spi_xfer(&desc, 0)` 发送单字节命令。CS 手动管理：拉低 CS → 传输 → 拉高 CS（如 pin != NRF_DRV_SPI_PIN_NOT_USED）
- `write_data` 回调：设置 DC=1，同上发送多字节数据
- `read_data` 回调：设置 DC=1，构建接收描述符，调用 `nrf_drv_spi_xfer(&desc, 0)` 读取数据
- `set_dc` / `set_cs` / `set_rst`：直接调用 `nrf_gpio_pin_write()`
- `delay_ms` 回调：调用 `nrf_delay_ms(ms)`
- 私有结构 `spi_priv_t` 存储 `nrf_drv_spi_t` 实例和 DC/CS/RST 引脚号

与 ESP-IDF SPI 的关键差异：nRF5 SDK SPI 无设备抽象层，CS 需要手动管理而非通过 `spi_device_transmit()` 自动处理。

### 3.3 GPIO

```c
typedef struct {
    uint32_t pin_mask;      // bitmask of button pins
    uint8_t  buttons[8];    // mapping: index → pin number
    uint8_t  num_buttons;
} nrf5_gpio_config_t;

eui_hal_gpio_t* eui_port_nrf5_gpio_create(const nrf5_gpio_config_t *cfg);
void eui_port_nrf5_gpio_destroy(eui_hal_gpio_t *hal);
```

内部行为：
- 遍历 `cfg->buttons[0..num_buttons-1]`，对每个引脚调用 `nrf_gpio_cfg_input(pin, NRF_GPIO_PIN_PULLUP)` 配置为带上拉的输入
- `read_pin(pin_id, user_data)` 回调：验证 pin_id 在已注册列表中，调用 `nrf_gpio_pin_read(pin_id)` → 返回 `(value == 0)` 表示按下（低电平有效，因为有上拉）
- `delay_us(us, user_data)` 回调：调用 `nrf_delay_us(us)`
- 私有结构 `gpio_priv_t` 存储 `pin_mask`（用于快速合法性检查）

## 4. CMake 构建设计

### port/nrf5/CMakeLists.txt

双模设计，镜像 ESP-IDF 端口：

```cmake
# nRF5 SDK 集成模式：当 NRF5_SDK_ROOT 环境变量存在时
if(DEFINED ENV{NRF5_SDK_ROOT})
    set(NRF5_SDK_ROOT $ENV{NRF5_SDK_ROOT})
    set(NRF5_COMPONENTS nrf_drv_twi nrf_drv_spi nrf_delay nrf_gpio)

    add_library(eui_port_nrf5 STATIC eui_port_nrf5.c)

    target_include_directories(eui_port_nrf5 PUBLIC
        .
        ${NRF5_SDK_ROOT}/components
        ${NRF5_SDK_ROOT}/components/drivers_nrf/twi_master
        ${NRF5_SDK_ROOT}/components/drivers_nrf/spi_master
        ${NRF5_SDK_ROOT}/components/drivers_nrf/delay
        ${NRF5_SDK_ROOT}/components/libraries/util
        ${NRF5_SDK_ROOT}/integration/nrfx
        ${NRF5_SDK_ROOT}/modules/nrfx
        ${NRF5_SDK_ROOT}/modules/nrfx/drivers/include
        ${NRF5_SDK_ROOT}/modules/nrfx/hal
        ${NRF5_SDK_ROOT}/components/toolchain
    )

    target_compile_definitions(eui_port_nrf5 PRIVATE
        NRF52
        NRF52832_XXAA   # 可通过 BUILD_NRF_CHIP 覆盖
    )

    target_link_libraries(eui_port_nrf5 PUBLIC eui)
else()
    # 独立 CMake 模式：用户自行提供 nRF5 SDK 头文件路径
    add_library(eui_port_nrf5 STATIC eui_port_nrf5.c)
    target_include_directories(eui_port_nrf5 PUBLIC .)
    target_link_libraries(eui_port_nrf5 PUBLIC eui)
endif()
```

在独立 CMake 模式下，用户需要在调用 `add_subdirectory()` 前手动设置 `target_include_directories` 指向 nRF5 SDK 头文件。

## 5. 内存策略

| 组件 | 内存来源 | 大小 |
|------|----------|------|
| TLSF 池 | SRAM（由 `eui_allocator_init_tlsf()` 传入） | 8,192 字节（默认） |
| 显示帧缓冲（FULL 模式） | SRAM (nRF52840) 或无 (nRF52832 32KB 不足) | W×H×bpp/8 |
| 显示页缓冲（PAGE 模式） | SRAM | W×8×bpp/8 (~1KB) |
| 端口内部状态 | SRAM | < 100 字节 |
| nRF5 SDK 驱动内部状态 | SRAM（nrf_drv_twi/nrf_drv_spi 在 init 时由 SDK 静态分配） | ~200 字节 |

nRF52832 仅有 32KB SRAM（含 SoftDevice 占用），PAGE 模式为推荐选项。nRF52840 有 256KB SRAM，可支持 FULL 模式小尺寸显示。

## 6. 错误处理

- `eui_port_nrf5_i2c_create()` / `spi_create()` / `gpio_create()` 返回 `NULL` 表示初始化失败（外设冲突、eui_malloc 失败、引脚非法）
- `_destroy()` 对 `NULL` 参数安全返回
- nRF5 SDK 错误码 `NRF_SUCCESS` vs `NRF_ERROR_*` 检查：不传播 nRF 错误码到调用者，仅用以判断成功/失败
- 所有 `nrf_drv_*_init()` 返回非 `NRF_SUCCESS` 时立即返回 `NULL`

## 7. 依赖关系

```
用户 main.c
    ├── eui_port_nrf5_i2c_create()  ──→ nrf_drv_twi (nRF5 SDK legacy)
    ├── eui_port_nrf5_spi_create()  ──→ nrf_drv_spi (nRF5 SDK legacy)
    ├── eui_port_nrf5_gpio_create() ──→ nrf_gpio    (nRF5 SDK HAL)
    │
    ├── eui_drv_ssd1306_create()    ──→ eui_drv_ssd1306.c
    ├── eui_drv_st7735_create()     ──→ eui_drv_st7735.c
    └── eui_init()                  ──→ libeui.a
```

端口层仅依赖 nRF5 SDK 的 driver 和 HAL 层，不依赖 SoftDevice、BLE 协议栈或 FreeRTOS。

## 8. nRF5 SDK 前置要求

用户需在 `sdk_config.h` 中启用以下模块：

```c
// TWI
#define TWI_ENABLED       1
#define TWI0_ENABLED      1
#define TWI0_USE_EASY_DMA 0

// SPI
#define SPI_ENABLED       1
#define SPI0_ENABLED      1
#define SPI0_USE_EASY_DMA 0

// GPIO (默认启用，通常无需额外配置)
#define GPIOTE_ENABLED    0   // 不需要 GPIOTE
```

## 9. 变更清单

### 新增文件

| 文件 | 说明 |
|------|------|
| `port/nrf5/CMakeLists.txt` | 双模构建文件（nRF5 SDK 集成 + 独立 CMake） |
| `port/nrf5/eui_port_nrf5.h` | 移植层公共头文件（配置结构体 + 工厂声明） |
| `port/nrf5/eui_port_nrf5.c` | TWI/SPI/GPIO 传输回调实现 |

### 不涉及修改的文件

- 所有现有 `src/`、`include/`、`port/esp-idf/` 文件均不修改
- 端口层是完全独立的新增模块

## 10. 不包含的内容

- nRF Connect SDK / Zephyr 平台移植（后续按需添加为 `port/nrf-connect/`）
- 示例项目（后续按需添加为 `examples/nrf5/ssd1306/`）
- XPT2046 触摸屏专用工厂（SPI + GPIO 已覆盖基础传输，用户可自行组合）
- BLE/NFC 无线功能
- 低功耗休眠管理（System ON/OFF）
- SoftDevice 集成（使用裸机 nRF5 外设驱动）
