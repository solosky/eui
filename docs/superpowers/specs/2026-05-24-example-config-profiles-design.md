# 示例配置管理：平台 × 驱动 × 色深 三维组织方案

## 问题陈述

EUI 示例横跨三个独立维度：

| 维度 | 可选值 |
|------|--------|
| 平台 | raylib (PC), esp-idf, nrf5 |
| 驱动芯片 | SSD1306, SH1106, ST7735, ILI9341, raylib (模拟) |
| 色深 | 1bpp, 4bpp, 8bpp, 16bpp |

当前设计（参见[跨平台示例设计](2026-05-23-cross-platform-examples-design.md)）通过 `EUI_TARGET_PORT` 处理**平台**维度，通过每个示例目录下的 Kconfig 处理驱动和引脚。但仍存在两个局限：

1. **色深是全局变量** — `EUI_COLOR_DEPTH` 是根 CMake 变量，对构建中的所有示例生效。无法同时构建 `basic_label`（1bpp）和 `color_demo`（16bpp）。
2. **驱动配置散落在示例目录中** — 每个 `cross/` 示例都有各自的 Kconfig，内含 SSD1306 与 ST7735 的引脚定义。这耦合了示例代码和硬件配置，且在每个示例中重复。

`basic_label` 应该在 raylib-128x64-1bpp、esp32-ssd1306-1bpp、esp32-st7735-16bpp、nrf52-ili9341-16bpp 上**无需任何代码改动**即可运行。示例作者只写一次 UI 逻辑；构建系统将它与对应的硬件配置组合。

## 设计方案：配置描述文件（Config Profiles）

引入**配置描述文件** — 独立的配置单元，将完整的硬件环境打包在一起：平台、显示驱动、色深、分辨率、引脚分配。

### 核心思路

```
示例逻辑          配置描述文件           构建产物
(要展示什么)   ×  (什么硬件)       →    (一个可执行文件)
```

示例是一个纯函数 `eui_example_setup(cfg)`。配置描述文件是硬件上下文。构建系统将任意示例与任意兼容的配置文件配对，生成一个可执行文件。

这分离了"我想演示什么 UI"和"我有什么硬件"，彻底消除 N×M×D 的文件膨胀（不再需要 `ssd1306_basic_label`、`st7735_basic_label` 等）。

### 约束检查

示例声明自身需求。构建系统自动跳过不兼容的 示例+配置 组合：

```
示例              需求                     兼容的配置
──────────────────────────────────────────────────────────────
basic_label       无                      所有配置
color_demo        depth>=16               raylib_240x240_16bpp, esp32_st7735_16bpp
amiibo_demo       depth>=16, w>=240       raylib_240x240_16bpp, esp32_st7735_16bpp
list_nav          w>=128                  所有配置
```

## 配置描述文件结构

### 文件格式

每个配置文件是一个 CMake 片段（非 Kconfig），因为配置文件在 CMake 配置阶段被消费，而非嵌入式 menuconfig 工具。Kconfig 保留用于 esp-idf/nrf5 的交互式配置路径。

```
configs/
├── pc/
│   ├── raylib_128x64_1bpp.cmake
│   └── raylib_240x240_16bpp.cmake
├── esp32/
│   ├── ssd1306_i2c_128x64_1bpp.cmake
│   ├── st7735_spi_240x240_16bpp.cmake
│   └── ili9341_spi_320x240_16bpp.cmake
└── nrf52/
    ├── ssd1306_i2c_128x64_1bpp.cmake
    └── st7735_spi_240x240_16bpp.cmake
```

### 配置文件内容

```cmake
# configs/esp32/st7735_spi_240x240_16bpp.cmake
set(EUI_TARGET_PORT      "esp-idf")
set(EUI_COLOR_DEPTH      16)
set(EUI_DISPLAY_DRIVER   "st7735")
set(EUI_DISPLAY_WIDTH    240)
set(EUI_DISPLAY_HEIGHT   240)
set(EUI_BUFFER_MODE      "page")

# SPI 引脚
set(EUI_PIN_SPI_HOST     1)
set(EUI_PIN_MOSI         23)
set(EUI_PIN_SCLK         18)
set(EUI_PIN_CS           5)
set(EUI_PIN_DC           16)
set(EUI_PIN_RST          17)
set(EUI_PIN_BL           4)

# 输入
set(EUI_INPUT_DRIVER     "buttons")
set(EUI_PIN_BTN_UP       34)
set(EUI_PIN_BTN_DOWN     35)
set(EUI_PIN_BTN_OK       32)
set(EUI_PIN_BTN_BACK     33)
```

### 配置 → C 代码

配置描述文件设置 CMake 变量。在配置阶段，这些变量被转换为自动生成的头文件：

```c
// 自动生成：build/include/eui/eui_profile_config.h
#define EUI_DISPLAY_DRIVER    EUI_DRIVER_ST7735
#define EUI_DISPLAY_WIDTH     240
#define EUI_DISPLAY_HEIGHT    240
#define EUI_PIN_MOSI          23
#define EUI_PIN_SCLK          18
// ...
```

端口引导程序包含此头文件，使用宏（而非运行时的字符串比较）来选择驱动：

```c
// 位于 eui_port_bootstrap_main() 中：
#if EUI_DISPLAY_DRIVER == EUI_DRIVER_ST7735
    eui_display_drv_t *display = eui_drv_st7735_create(EUI_PIN_SPI_HOST, EUI_PIN_MOSI, ...);
#elif EUI_DISPLAY_DRIVER == EUI_DRIVER_SSD1306
    eui_display_drv_t *display = eui_drv_ssd1306_create(EUI_PIN_I2C_PORT, EUI_PIN_SDA, ...);
#endif
```

这样驱动选择在编译期完成（零运行时开销），避免在嵌入式代码中使用字符串比较。

### CMake Presets 集成

配置文件可通过 CMake presets 引用，实现一行命令构建：

```json
{
  "configurePresets": [
    { "name": "pc-1bpp",   "cacheVariables": { "EUI_CONFIG_PROFILE": "configs/pc/raylib_128x64_1bpp.cmake" } },
    { "name": "esp-st7735","cacheVariables": { "EUI_CONFIG_PROFILE": "configs/esp32/st7735_spi_240x240_16bpp.cmake" } }
  ]
}
```

使用：`cmake --preset esp-st7735 -B build && cmake --build build`

## 示例约束声明

每个跨平台示例通过 `requirements.cmake` 声明最低需求：

```cmake
# examples/cross/color_demo/requirements.cmake
set(EXAMPLE_REQUIRES_COLOR_DEPTH_MIN 16)
set(EXAMPLE_REQUIRES_WIDTH_MIN       240)
set(EXAMPLE_REQUIRES_HEIGHT_MIN      240)
```

没有 requirements 文件的示例默认兼容所有配置。这种方式避免了注释解析，且可以直接 `include()`。

在配置阶段，构建系统将每个示例的 `requirements.cmake` 与当前配置文件进行校验：

```cmake
# 位于根 CMakeLists.txt 或 cross/CMakeLists.txt 中，针对每个示例：
if(EXISTS "${example_dir}/requirements.cmake")
    include("${example_dir}/requirements.cmake")
    if(DEFINED EXAMPLE_REQUIRES_COLOR_DEPTH_MIN AND EUI_COLOR_DEPTH LESS EXAMPLE_REQUIRES_COLOR_DEPTH_MIN)
        message(WARNING "跳过 ${example_name}：需要色深 >= ${EXAMPLE_REQUIRES_COLOR_DEPTH_MIN}")
        return()  # 跳过此示例
    endif()
    if(DEFINED EXAMPLE_REQUIRES_WIDTH_MIN AND EUI_DISPLAY_WIDTH LESS EXAMPLE_REQUIRES_WIDTH_MIN)
        message(WARNING "跳过 ${example_name}：需要宽度 >= ${EXAMPLE_REQUIRES_WIDTH_MIN}")
        return()
    endif()
endif()
```

## 目录布局（目标状态）

```
eui/
├── configs/                              # 硬件配置描述文件（新增）
│   ├── pc/
│   │   ├── raylib_128x64_1bpp.cmake
│   │   └── raylib_240x240_16bpp.cmake
│   ├── esp32/
│   │   ├── ssd1306_i2c_128x64_1bpp.cmake
│   │   ├── st7735_spi_240x240_16bpp.cmake
│   │   └── ili9341_spi_320x240_16bpp.cmake
│   └── nrf52/
│       ├── ssd1306_i2c_128x64_1bpp.cmake
│       └── st7735_spi_240x240_16bpp.cmake
│
├── examples/
│   ├── cross/                            # 从 legacy 迁移而来（全部 11+ 示例）
│   │   ├── basic_label/
│   │   │   ├── basic_label.c             # 仅实现 eui_example_setup()
│   │   │   └── CMakeLists.txt
│   │   ├── button_test/
│   │   ├── list_nav/
│   │   ├── menu_system/
│   │   ├── dialog_overlay/
│   │   ├── animation_demo/
│   │   ├── custom_widget/
│   │   ├── page_buffer/
│   │   ├── scene_view_demo/
│   │   ├── benchmark/
│   │   ├── color_demo/
│   │   │   ├── color_demo.c
│   │   │   ├── CMakeLists.txt
│   │   │   └── requirements.cmake        # depth>=16, w>=240, h>=240
│   │   └── amiibo_demo/
│   │       ├── amiibo_demo.c
│   │       ├── CMakeLists.txt
│   │       └── requirements.cmake        # depth>=16, w>=240, h>=240
│   │
│   ├── esp-idf/                          # 废弃（由配置描述文件统一替代）
│   ├── icons/                            # 共享资源（不变）
│   └── render_16bpp.c                    # 独立离线工具（不变）
│
├── port/
│   ├── raylib/eui_port_bootstrap.c       # 读取配置描述文件，创建驱动
│   ├── esp-idf/eui_port_bootstrap.c
│   └── nrf5/eui_port_bootstrap.c
│
└── include/eui/
    └── eui_port_bootstrap.h              # 引导 API（不变）
```

### 待删除文件

| 路径 | 原因 |
|------|------|
| `examples/basic_label.c` 到 `examples/scene_view_demo.c`（10 个文件） | 已迁移到 `cross/` |
| `examples/amiibo_demo.c` | 已迁移到 `cross/amiibo_demo/` |
| `examples/amiibo_font.h` | 移入 `cross/amiibo_demo/` |
| `examples/esp-idf/ssd1306/` | 由 `configs/esp32/ssd1306_i2c_128x64_1bpp.cmake` + cross 示例替代 |
| `examples/esp-idf/st7735/` | 由 `configs/esp32/st7735_spi_240x240_16bpp.cmake` + cross 示例替代 |

### .cmake 与 .Kconfig：同一配置的两种视图

每个硬件配置有两套表示：

| 文件 | 用途 | 使用者 |
|------|------|--------|
| `*.cmake` | 预设引脚值、驱动选择、色深。在 CMake 配置阶段一次性读取。 | CI、快速桌面构建、已确定接线的用户 |
| `*.Kconfig` | 交互式 menuconfig 菜单树，描述相同变量。用户可在构建前调整引脚。 | esp-idf 的 `idf.py menuconfig`、nrf5 的 nRF Connect Kconfig |

两套文件描述的是同一个设备。在 esp-idf 构建时，CMake 配置提供默认值，用户可运行 `menuconfig` 覆盖引脚。在 PC（raylib）上，直接使用 `.cmake` 的值 — 不存在 menuconfig 工具。

## 构建工作流

### 单次构建（一个配置，所有示例）

```bash
cmake -B build \
    -DEUI_CONFIG_PROFILE=configs/esp32/st7735_spi_240x240_16bpp.cmake
cmake --build build
```

这将构建所有兼容的 cross 示例，目标为 ESP32 上的 ST7735、16bpp。`color_demo` 和 `amiibo_demo` 会被包含；其他示例也会被包含（不需要 16bpp 的示例在 16bpp 模式下也能正常渲染）。

### 多配置构建（CI 矩阵）

```bash
# 为所有配置构建所有示例
for profile in configs/**/*.cmake; do
    name=$(basename $profile .cmake)
    cmake -B build_$name -DEUI_CONFIG_PROFILE=$profile
    cmake --build build_$name -j$(nproc)
done
```

### PC 端快速迭代

```bash
cmake -B build -DEUI_CONFIG_PROFILE=configs/pc/raylib_128x64_1bpp.cmake
cmake --build build -j$(nproc)
./build/examples/cross/basic_label/basic_label
```

## 色深处理

### 配置描述文件决定色深

`EUI_COLOR_DEPTH` 从根 CMake 缓存变量转变为由配置文件设定的值。每个配置文件指定一种色深。为 ST7735 构建时设 depth=16；为 SSD1306 构建时设 depth=1。

### 同一次构建中不支持多种色深

单个 CMake 构建树只有一种 `EUI_COLOR_DEPTH`。如需同时测试 1bpp 和 16bpp，使用两个构建目录和不同的配置。这是正确的取舍：框架配置头文件是每构建一份的，不同色深意味着不同的 `eui_config.h`。

### 示例适配

为 1bpp 编写的示例在 16bpp 下也应能正常工作（框架已处理 — `eui_color_1bpp_to_native()` 将黑/白映射到对应色深的原生颜色）。反之，16bpp 示例在 depth=1 时不会构建（由 `requirements.cmake` 拦截）。

16bpp 示例可在运行时查询色深以自适应：

```c
void eui_example_setup(const eui_example_config_t *cfg) {
    if (cfg->color_depth >= 16) {
        // 使用 RGB565 颜色
    } else {
        // 单色回退（或直接退出）
    }
}
```

## 迁移路径

### 第一阶段：创建配置描述文件（不改示例代码）

1. 创建 `configs/` 目录和 CMake 配置描述文件
2. 新增 `EUI_CONFIG_PROFILE` CMake 变量；设置后跳过 `EUI_TARGET_PORT`，改为 `include()` 配置描述文件
3. 配置文件设置向后兼容的变量（`EUI_TARGET_PORT`、`EUI_COLOR_DEPTH` 等），现有 cross 示例无需改动即可继续构建

### 第二阶段：将剩余 legacy 示例迁移到 cross/

1. 将每个 `examples/*.c` 的纯 UI 逻辑提取到 `examples/cross/<name>/<name>.c`
2. 添加 `requirements.cmake` 声明色深/分辨率约束
3. 删除原始 `examples/*.c` 文件和 `examples/CMakeLists.txt` 中的对应条目
4. 移除 `EUI_BUILD_EXAMPLES` CMake 选项（只保留 `EUI_BUILD_CROSS_EXAMPLES`）

### 第三阶段：将 Kconfig 移出示例目录

1. 为每个配置创建 `configs/<platform>/*.Kconfig` 文件
2. 更新端口引导程序，从 config 目录读取 Kconfig，而非示例目录
3. 从各个示例目录中移除 Kconfig

### 第四阶段：废弃平台专用示例

1. 验证 `configs/esp32/ssd1306_*.cmake` + `cross/basic_label` 产出与 `examples/esp-idf/ssd1306/` 一致
2. 删除 `examples/esp-idf/` 目录

## 范围边界

**范围内：**
- `configs/` 目录，包含所有当前平台和驱动的 CMake 配置描述文件
- `EUI_CONFIG_PROFILE` CMake 变量，用于选择配置
- `requirements.cmake` 每示例约束声明
- 构建系统过滤不兼容的 示例+配置 组合
- 将全部 11+ legacy 示例迁移到 `cross/`
- 将 esp-idf 示例迁移到 配置描述文件 + cross 示例
- 删除 legacy 示例目录

**范围外：**
- 在单个 CMake 构建树中支持多个配置
- 在配置阶段自动检测可用硬件
- 每示例自定义 Kconfig（超出共享硬件 Kconfig 的范围）
- 引导程序支持 `configs/` 树之外的用户自定义配置描述文件
- CI 矩阵生成脚本（使用简单的 shell 循环即可）

## CI 适配

### 测试矩阵（保持不变）

测试不依赖配置描述文件，直接使用 `EUI_COLOR_DEPTH` 矩阵覆盖三种色深：

```yaml
test:
  strategy:
    matrix:
      color_depth: [1, 8, 16]
```

### 示例构建（新增）

新增 `examples` job，矩阵覆盖两种色深，在 raylib 桌面环境下编译验证所有 cross 示例：

```yaml
examples:
  strategy:
    matrix:
      color_depth: [1, 16]
  steps:
    - name: Configure
      run: cmake -B build -DEUI_BUILD_CROSS_EXAMPLES=ON -DEUI_TARGET_PORT=raylib -DEUI_COLOR_DEPTH=${{ matrix.color_depth }}
```

### 过渡计划

当前 CI 使用 `EUI_TARGET_PORT` + `EUI_COLOR_DEPTH` 直接构建。配置描述文件系统实现后（第一阶段），CI 示例矩阵切换为：

```yaml
examples:
  strategy:
    matrix:
      profile: [raylib_128x64_1bpp, raylib_240x240_16bpp]
  steps:
    - name: Configure
      run: cmake -B build -DEUI_CONFIG_PROFILE=configs/pc/${{ matrix.profile }}.cmake
```

## 参考

- 跨平台引导设计：[2026-05-23-cross-platform-examples-design.md](2026-05-23-cross-platform-examples-design.md)
- HAL 驱动头文件：`include/eui/driver/eui_drv_ssd1306.h`、`eui_drv_st7735.h` 等
- 框架配置：`include/eui/eui_config.h.in`
- 根构建文件：`CMakeLists.txt`（EUI_TARGET_PORT、EUI_BUILD_CROSS_EXAMPLES）
- CI 工作流：`.github/workflows/build.yml`
