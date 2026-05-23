# EUI Raylib Port (Desktop Simulator)

基于 Raylib 的 EUI 桌面模拟器端口。在 PC 上以窗口方式运行 EUI UI，无需硬件开发板即可开发和调试 UI 代码。

## 实现的 HAL

| HAL | 实现 | 驱动 |
|-----|------|------|
| `eui_display_hal_t` | 窗口渲染 OLED 模拟 | `eui_drv_raylib` |
| `eui_input_hal_t` | 键盘/鼠标输入映射 | `eui_drv_raylib` |

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

## 跨平台示例

以下示例可在所有三个端口 (raylib / esp-idf / nrf5) 上使用相同代码运行：

| 示例 | 目录 |
|------|------|
| basic_label | `examples/cross/basic_label/` |
| button_test | `examples/cross/button_test/` |
| list_nav | `examples/cross/list_nav/` |

通过修改 `EUI_TARGET_PORT` 选择目标平台，无需修改示例代码。

## 与其他端口对比

| 特性 | Raylib | ESP-IDF | nRF5 |
|------|--------|---------|------|
| 目标 | 桌面 PC | ESP32 | nRF52832 |
| RTOS | 无 | FreeRTOS | Bare-metal |
| 显示 | 真实窗口 | OLED 硬件 | OLED 硬件 |
| 编译工具链 | Host GCC/Clang | Xtensa/RISC-V GCC | ARM GCC |
| 输入 | 键盘/鼠标 | GPIO 按键 | GPIO 按键 |
| 用途 | 开发调试/演示 | 生产部署 | 生产部署 |
