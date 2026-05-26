# EUI Raylib Port (Desktop Simulator)

基于 Raylib 的 EUI 桌面模拟器端口。在 PC 上以窗口方式运行 EUI UI，无需硬件开发板即可开发和调试 UI 代码。

## 实现的 HAL

| HAL | 实现 | 驱动 |
|-----|------|------|
| `eui_display_drv_t` | 窗口渲染 OLED 模拟 | `eui_drv_raylib` |
| `eui_input_drv_t` | 键盘/鼠标输入映射 | `eui_drv_raylib` |

> 此端口 **不** 实现 I2C/SPI/GPIO HAL——它使用 `eui_drv_raylib` 直接渲染 framebuffer 到 Raylib 窗口。

## 依赖

| 依赖 | 说明 |
|------|------|
| **Raylib** | 自带于 `third_party/raylib/`，自动编译 |
| **OpenGL / Metal** | Raylib 后端 (macOS/Linux/Windows) |
| **Host GCC/Clang** | 桌面本地编译器 |

## 快速开始

```bash
# 方式 1: 使用 PC 预设
cmake --preset pc -B build_pc
cmake --build build_pc

# 方式 2: 手动指定
cmake -DEUI_BUILD_CROSS_EXAMPLES=ON \
      -DEUI_TARGET_PORT=raylib \
      -B build_raylib
cmake --build build_raylib

# 方式 3: 使用旧版示例
cmake -DEUI_BUILD_EXAMPLES=ON -B build
cmake --build build
```

## CMake 变量

| 变量 | 默认值 | 说明 |
|------|--------|------|
| `EUI_TARGET_PORT` | `raylib` | 目标端口 (使用 raylib) |
| `EUI_BUILD_CROSS_EXAMPLES` | `OFF` | 构建跨平台示例 |
| `EUI_BUILD_EXAMPLES` | `ON` | 构建旧版 raylib 示例 |
| `EUI_COLOR_DEPTH` | `1` | 色深: 1, 4, 8, 16 |

## CMake 预设

预设 `pc` 定义在 `CMakePresets.json` 中：

```json
{
    "name": "pc",
    "displayName": "PC (Raylib)",
    "cacheVariables": {
        "EUI_TARGET_PORT": "raylib",
        "EUI_BUILD_CROSS_EXAMPLES": "ON",
        "EUI_BUILD_EXAMPLES": "OFF",
        "EUI_BUILD_TESTS": "OFF"
    }
}
```

## 运行架构

```
main()
  └─ eui_allocator_init_tlsf()              # 分配 24KB 内存池
  └─ eui_drv_raylib_create_display()        # 创建模拟 OLED 窗口
  └─ eui_drv_raylib_create_input()          # 创建键盘/鼠标输入
  └─ eui_init()                             # 初始化 EUI
  └─ eui_set_tick_callback()                # 基于 GetTime() 的 tick
  └─ display->init()                        # 初始化显示
  └─ eui_example_setup()                    # 构建 UI
  └─ while(!close) {
       eui_tick();
       eui_drv_raylib_refresh();            # 绘制 framebuffer 到窗口
     }
  └─ 清理资源 (deinit display, destroy input, eui_deinit)
```

- Tick 通过 raylib 的 `GetTime()` 获取运行时间 (秒)，转为毫秒
- 窗口关闭时正常清理所有资源
- 运行 PC 本地编译器 (clang/gcc)，无需交叉编译工具链

## 键盘映射

| 按键 | EUI 事件 |
|------|----------|
| ↑ / W | `EUI_INPUT_UP` |
| ↓ / S | `EUI_INPUT_DOWN` |
| ← / A | `EUI_INPUT_LEFT` |
| → / D | `EUI_INPUT_RIGHT` |
| Enter / Space | `EUI_INPUT_CONFIRM` |
| Backspace / Esc | `EUI_INPUT_BACK` |
| Tab | `EUI_INPUT_NEXT` |

## 构建跨平台示例

### 使用 CMake 预设（推荐）

```bash
# 构建所有跨平台示例（默认 128x64, 1bpp）
cmake --preset pc -B build_pc
cmake --build build_pc

# 构建特定分辨率示例（如 desktop_launcher 需要 400x300, 2bpp）
cmake --preset pc -B build_pc \
      -DEUI_COLOR_DEPTH=2 \
      -DEUI_MEM_POOL_SIZE=65536 \
      -DEUI_MAX_VIEWS=16
cmake --build build_pc
```

### 使用配置 profile

配置 profile 文件位于 `configs/pc/`，预设了显示尺寸、色深等参数：

```bash
# 标准 128x64, 1bpp
cmake -B build_pc \
      -DEUI_BUILD_CROSS_EXAMPLES=ON \
      -DEUI_CONFIG_PROFILE="configs/pc/raylib_128x64_1bpp.cmake"
cmake --build build_pc

# 桌面启动器 400x300, 2bpp
cmake -B build_pc \
      -DEUI_BUILD_CROSS_EXAMPLES=ON \
      -DEUI_CONFIG_PROFILE="configs/pc/raylib_400x300_2bpp.cmake"
cmake --build build_pc
```

### 完整 profile 列表

| Profile | 尺寸 | 色深 | 用途 |
|---------|------|------|------|
| `configs/pc/raylib_128x64_1bpp.cmake` | 128×64 | 1bpp | 通用 OLED 模拟 |
| `configs/pc/raylib_400x300_2bpp.cmake` | 400×300 | 2bpp | 桌面启动器 |
| `configs/pc/raylib_240x240_16bpp.cmake` | 240×240 | 16bpp | 彩色 TFT 模拟 |

### 可用示例

| 示例 | 要求 | 说明 |
|------|------|------|
| `basic_label` | — | 最小 widget 示例 |
| `button_test` | — | 按钮交互 |
| `list_nav` | — | 列表导航 |
| `menu_system` | — | 菜单系统（含子菜单） |
| `dialog_overlay` | — | 对话框覆盖层 |
| `animation_demo` | — | 框架动画 API |
| `custom_widget` | — | 自定义 widget |
| `benchmark` | — | 性能测试 |
| `page_buffer` | — | 分页缓冲模式 |
| `scene_view_demo` | — | 场景管理器 |
| `color_demo` | ≥16bpp | 彩色渲染 |
| `amiibo_demo` | ≥16bpp | Amiibo 轮播 |
| `desktop_launcher` | ≥2bpp, 400×300 | 桌面启动器（弹簧动画） |

所有示例代码完全跨平台，通过修改 `EUI_CONFIG_PROFILE` 或 `EUI_TARGET_PORT`
选择目标，无需修改源文件。

## 与其他端口对比

| 特性 | Raylib | ESP-IDF | nRF5 |
|------|--------|---------|------|
| 目标 | 桌面 PC | ESP32 | nRF52832 |
| RTOS | 无 | FreeRTOS | Bare-metal |
| 显示 | 真实窗口 | OLED 硬件 | OLED 硬件 |
| 编译工具链 | Host GCC/Clang | Xtensa/RISC-V GCC | ARM GCC |
| 输入 | 键盘/鼠标 | GPIO 按键 | GPIO 按键 |
| 用途 | 开发调试/演示 | 生产部署 | 生产部署 |
