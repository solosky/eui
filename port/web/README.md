# EUI Web Port (Emscripten)

基于 Emscripten 的 EUI Web 端口。在浏览器中以 Canvas 方式运行 EUI UI，
无需安装任何软件即可演示和测试。

## 实现的 HAL

| HAL | 实现 | 驱动 |
|-----|------|------|
| `eui_display_drv_t` | HTML Canvas 像素渲染 | `eui_drv_web` |
| `eui_input_drv_t` | 键盘/触摸事件映射 | `eui_drv_web` |

显示驱动通过 Emscripten 的 `emscripten_gl*` API 将 framebuffer 渲染到
HTML5 Canvas 上。

## 依赖

| 依赖 | 说明 |
|------|------|
| **Emscripten** | `emcc` 编译器 (>= 3.1.0) |
| **Node.js** | 可选，用于本地运行 HTTP 服务器 |

## 快速开始

### 使用 CMake 预设

```bash
# 标准 128x64, 1bpp
cmake --preset web-1bpp -B build_web
cmake --build build_web

# 彩色 240x240, 16bpp
cmake --preset web-16bpp -B build_web
cmake --build build_web
```

### 使用配置 profile

配置 profile 文件位于 `configs/web/`，预设了 Canvas 尺寸、色深和缩放比：

```bash
# 通用 OLED 模拟 128x64, 1bpp
cmake -B build_web \
      -DEUI_BUILD_CROSS_EXAMPLES=ON \
      -DEUI_CONFIG_PROFILE="configs/web/canvas_128x64_1bpp.cmake"
cmake --build build_web

# 桌面启动器 400x300, 2bpp（Canvas 缩放为 800x600）
cmake -B build_web \
      -DEUI_BUILD_CROSS_EXAMPLES=ON \
      -DEUI_CONFIG_PROFILE="configs/web/canvas_400x300_2bpp.cmake"
cmake --build build_web

# 彩色 240x240, 16bpp
cmake -B build_web \
      -DEUI_BUILD_CROSS_EXAMPLES=ON \
      -DEUI_CONFIG_PROFILE="configs/web/canvas_240x240_16bpp.cmake"
cmake --build build_web
```

> 注意：使用 Emscripten 编译时需要将 `emcc` 加入 PATH。
> 如果系统同时安装了本地编译器和 Emscripten，CMake 会自动检测
> `EMSCRIPTEN` 环境并使用 `Emscripten.cmake` 工具链文件。

### 完整 profile 列表

| Profile | Canvas 尺寸 | 色深 | 缩放 | 用途 |
|---------|------------|------|------|------|
| `configs/web/canvas_128x64_1bpp.cmake` | 128×64 | 1bpp | 4× | 通用 OLED 模拟 |
| `configs/web/canvas_400x300_2bpp.cmake` | 400×300 | 2bpp | 2× | 桌面启动器 |
| `configs/web/canvas_240x240_16bpp.cmake` | 240×240 | 16bpp | 2× | 彩色 TFT 模拟 |

## 输出文件

编译后每个示例生成三个文件：

```
examples/cross/<name>/
├── <name>.html    # 入口页面（自动生成）
├── <name>.js      # Emscripten JavaScript 胶水代码
└── <name>.wasm    # WebAssembly 二进制
```

使用任何 HTTP 服务器打开 `.html` 文件即可运行：

```bash
# Python
python3 -m http.server -d build_web/examples/cross/ 8000

# Node.js (npx)
npx serve build_web/examples/cross/
```

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

## 运行架构

```
main()
  └─ eui_allocator_init_tlsf()              # 分配内存池 (EUI_MEM_POOL_SIZE)
  └─ eui_drv_web_create_display()           # 创建 Canvas 显示驱动
  └─ eui_drv_web_create_input()             # 创建键盘/触摸输入驱动
  └─ eui_init()                             # 初始化 EUI
  └─ eui_set_tick_callback()                # 基于 emscripten_get_now() 的 tick
  └─ display->init()                        # 初始化 Canvas
  └─ eui_example_setup()                    # 构建 UI
  └─ emscripten_set_main_loop(main_loop, 0, 1)
       └─ eui_tick()
```

- Tick 通过 `emscripten_get_now()` 获取高精度时间 (毫秒)
- 浏览器负责帧调度，`emscripten_set_main_loop` 使用 requestAnimationFrame
- Canvas 显示通过 Emscripten GL 接口直接写入像素数据

## 可用示例

与 raylib、esp-idf 和 nrf5 端口共享完全相同的示例代码。通过修改
`EUI_CONFIG_PROFILE` 选择不同的 Canvas 尺寸和色深。
