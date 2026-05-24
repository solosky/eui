# Web 移植设计

## 概述

基于 Emscripten + HTML5 Canvas 为 EUI 添加 Web 移植层，使 EUI 示例可直接在浏览器中运行。该移植层服务于两个主要目标：(1) 在文档站点上实时展示 EUI 示例，(2) 提供快速开发反馈循环，无需安装原生桌面环境。

**适用范围**：所有迁移至 `examples/cross/` 的示例（与[示例配置描述文件设计](2026-05-24-example-config-profiles-design.md)对齐）。示例仅需实现 `eui_example_setup()`，构建系统将其与 Web 配置配对产出 `.wasm` + `.html`。

## 与配置描述文件系统的关系

Web 移植作为**第四平台**加入配置描述文件系统，与现有的 `pc/`、`esp32/`、`nrf52/` 并列。Web 配置文件位于 `configs/web/`，使用 Emscripten 工具链。

构建时通过 `EUI_CONFIG_PROFILE` 选择：

```bash
emcmake cmake -B build -DEUI_CONFIG_PROFILE=configs/web/canvas_128x64_1bpp.cmake
cmake --build build -j$(nproc)
```

自动构建所有兼容的 cross 示例，每个示例产出独立的 `.wasm` + `.js` + `.html`。

## 方案决策：原生 HTML5 Canvas 驱动

**选定方案**：编写一个新的轻量级 `eui_drv_web` 驱动，将 EUI 帧缓冲直接渲染到 HTML5 `<canvas>` 元素，并从 DOM 捕获键盘/触摸事件。Web 构建不依赖 raylib。

选择此方案而非 Emscripten + Raylib 的原因：

- **体积**：WASM 约 50KB，raylib 方案超过 500KB。对于嵌入文档站点至关重要。
- **Web 原生体验**：Canvas 渲染可无缝集成到文档页面中；raylib Web 构建看起来像是困在浏览器标签页里的桌面应用。
- **依赖干净**：Web 移植不需要 OpenGL、音频或 3D 数学，没必要引入 raylib。
- **实现成本**：EUI 的 HAL 接口极简——`write_buffer` + `poll`——从头编写驱动约 400 行 C + 100 行 JS 胶水代码。

### 被拒绝的替代方案

Emscripten + Raylib：通过 raylib 内置的 emscripten 后端将现有 `eui_drv_raylib` 编译为 WASM。因体积过大且呈现效果类似浏览器中的桌面应用，与展示和开发便捷的目标相冲突而被拒绝。原生方案足够简单，走捷径不值得。

## 架构

### 文件布局

```
configs/web/
├── canvas_128x64_1bpp.cmake      ← 1bpp 单色 Web 配置
└── canvas_240x240_16bpp.cmake    ← 16bpp RGB565 Web 配置

port/web/
├── index.html                    ← 示例导航主页（构建时动态生成）
├── shell.html                    ← 各示例共用的 HTML 模板
├── shell.js                      ← Canvas 初始化、虚拟方向键、键盘捕获、主循环
├── CMakeLists.txt                ← Web bootstrap + 示例构建规则
├── eui_port_bootstrap.c          ← Web 移植 bootstrap（初始化 HAL、主循环、导出 tick）
└── eui_port_bootstrap.h          ← eui_example_setup() 签名

src/driver/eui_drv_web.c          ← Web Canvas 显示 + 输入 HAL 驱动
include/eui/driver/eui_drv_web.h  ← 驱动公开 API 头文件
```

### 配置文件内容

```cmake
# configs/web/canvas_128x64_1bpp.cmake
set(EUI_TARGET_PORT      "web")
set(EUI_COLOR_DEPTH      1)
set(EUI_DISPLAY_DRIVER   "web")
set(EUI_DISPLAY_WIDTH    128)
set(EUI_DISPLAY_HEIGHT   64)
set(EUI_BUFFER_MODE      "full")
set(EUI_INPUT_DRIVER     "web")
set(EUI_CANVAS_SCALE     4)         # CSS 像素放大倍数
```

```cmake
# configs/web/canvas_240x240_16bpp.cmake
set(EUI_TARGET_PORT      "web")
set(EUI_COLOR_DEPTH      16)
set(EUI_DISPLAY_DRIVER   "web")
set(EUI_DISPLAY_WIDTH    240)
set(EUI_DISPLAY_HEIGHT   240)
set(EUI_BUFFER_MODE      "full")
set(EUI_INPUT_DRIVER     "web")
set(EUI_CANVAS_SCALE     2)
```

### 数据流

```
eui_tick() → canvas commit()
    → display->write_buffer(fb)
        → EM_ASM: fb 像素 → RGBA Uint8ClampedArray
        → CanvasRenderingContext2D.putImageData()

eui_input_update()
    → input->poll()
        → 从 WASM 线性内存中的共享环形缓冲区读取
        ← JS keydown/keyup + 虚拟方向键触摸事件写入此处
    → 事件分发到 eui_view_dispatcher → 活跃 view
```

### Bootstrap 模式

遵循 `port/raylib/eui_port_bootstrap.c` 建立的现有跨平台 bootstrap 模式，根据配置描述文件中的 `EUI_DISPLAY_DRIVER=web` 选择 web 驱动：

```c
// 位于 eui_port_bootstrap_main() 中（编译期选择，零运行时开销）：
#if EUI_DISPLAY_DRIVER == EUI_DRIVER_WEB
    eui_display_drv_t *display = eui_drv_web_create_display(
        EUI_DISPLAY_WIDTH, EUI_DISPLAY_HEIGHT, EUI_COLOR_DEPTH);
#endif
```

1. `eui_port_bootstrap_init()` — 调用示例的 `eui_example_setup()`，创建 Canvas 显示 + 键盘输入 HAL，初始化 EUI
2. JS 侧管理 `requestAnimationFrame` 循环，每帧调用 WASM 的 `eui_port_tick()`
3. `eui_port_tick()` 调用 `eui_tick()`，然后触发 Canvas 刷新

## Display HAL（`eui_drv_web.c`）

### `eui_drv_web_create_display(width, height, color_depth)`

创建并返回通过 `eui_malloc()` 分配的 `eui_display_drv_t*`。

**`init(void *user_data)`**：
- 通过 ID 获取 `<canvas>` 元素（由 `user_data` 以 C 字符串形式传入）
- 通过 `EM_ASM` 创建 `CanvasRenderingContext2D`
- 存储上下文引用供 `write_buffer` 后续使用

**`write_buffer(const uint8_t *buffer, const eui_rect_t *rect, void *user_data)`**：
- 主渲染路径。由 `eui_canvas_commit()` 以完整帧缓冲调用。
- 将 EUI 像素格式转换为 RGBA：
  - **1bpp**：每 bit → bit=1 为黑色 RGBA (0,0,0,255)，bit=0 为白色 RGBA (255,255,255,255)。与单色 OLED 惯例一致。
  - **16bpp (RGB565)**：每个 16 位字 → 8 位 R/G/B/A 分量。
- 使用 `EM_ASM` 将转换循环内联为 JS，避免在 C 堆上额外分配。
- 调用 `ctx.putImageData(imageData, 0, 0)` 完成渲染。

**`deinit(void *user_data)`**：
- 释放 JS 侧引用（上下文、图像数据）。

**`draw_pixel`、`set_contrast`、`set_power`、`set_invert`、`fill_rect`**：均为空操作（与 raylib 驱动一致）。

### Buffer 模式

固定为 `EUI_BUFFER_FULL`。浏览器 Canvas 无内存约束也无物理 GRAM，完整帧缓冲是最自然的模型。

### 帧率

Canvas 刷新与 EUI 的 tick 循环同步。JS `requestAnimationFrame` 以约 60fps 驱动循环。无需单独的渲染线程或双缓冲——EUI 的单线程模型在此完美映射。

## Input HAL（`eui_drv_web.c`）

### `eui_drv_web_create_input()`

创建并返回通过 `eui_malloc()` 分配的 `eui_input_drv_t*`。

**键盘映射**（桌面端）：

| 物理按键 | EUI 事件 |
|---|---|
| 方向键 ↑ | `EUI_KEY_UP` 按下/释放 |
| 方向键 ↓ | `EUI_KEY_DOWN` 按下/释放 |
| 方向键 ← | `EUI_KEY_LEFT` 按下/释放 |
| 方向键 → | `EUI_KEY_RIGHT` 按下/释放 |
| Enter | `EUI_KEY_OK` 按下/释放 |
| Backspace | `EUI_KEY_BACK` 按下/释放 |

**实现方式**：
- JS 侧在 `document` 上注册 `keydown`/`keyup` 事件监听
- 过滤 6 个相关按键，将 `eui_event_t` 结构体写入 WASM 线性内存中的**共享环形缓冲区**
- 避免每次事件都跨越 JS→WASM 边界；C 侧直接从自身内存读取
- 环形缓冲区大小：32 个事件（对键盘输入足够宽裕）

**C 侧 `poll(eui_event_t *event, void *user_data)`**：
- 从环形缓冲区出队到 `*event`
- 返回 0（有事件）或 1（无事件）
- EUI 的 `eui_input_update()` 处理去抖动和长按重复——驱动无需变更

### 移动端虚拟方向键（`shell.js`）

在 Canvas 下方渲染为一组 HTML 按钮：
```
        [↑]
   [←]  [OK]  [→]
        [↓]
             [BACK]
```

- 这些按钮的触摸事件映射为 `EUI_KEY_*` 事件并入队到同一个环形缓冲区
- 按钮使用 `touchstart`/`touchend` 以避免 300ms 点击延迟
- Canvas 本身为触摸透传模式（无缩放/滚动干扰）
- 布局使用 CSS Flexbox，适配 ≥320px 宽度的屏幕

## 构建系统集成

### 构建命令

```bash
# 单个配置，所有兼容示例
emcmake cmake -B build -DEUI_CONFIG_PROFILE=configs/web/canvas_128x64_1bpp.cmake
cmake --build build -j$(nproc)

# 16bpp 配置
emcmake cmake -B build_16bpp -DEUI_CONFIG_PROFILE=configs/web/canvas_240x240_16bpp.cmake
cmake --build build_16bpp -j$(nproc)
```

### CMake 变更

**顶层 `CMakeLists.txt`**：
- 当 `EUI_TARGET_PORT=web`（由配置描述文件设定）：
  - 检测 Emscripten 工具链（检查 `EMSCRIPTEN` 环境变量或 PATH 中的 `emcc`）
  - 设置 `EMSCRIPTEN` 编译定义
  - 跳过 raylib 子模块构建（Web 不需要）
  - 跳过 `EUI_BUILD_EXAMPLES`（旧版示例）
  - `EUI_BUILD_CROSS_EXAMPLES` 默认启用
  - 示例约束检查：`requirements.cmake` 中声明 `width>=240` 的示例在 128x64 配置下自动跳过

**`src/CMakeLists.txt`**：
- 当 `EUI_TARGET_PORT=web`：将 `driver/eui_drv_web.c` 加入 `eui` 静态库
- 驱动编译进 `libeui.a` 而非独立库，以便 Emscripten 对整个调用图进行链接时优化

**`port/web/CMakeLists.txt`**（新建）：
```
eui_port_bootstrap.c  →  eui_port_bootstrap.o
  + libeui.a
  → basic_label.wasm + basic_label.js
  → button_test.wasm + button_test.js
  → list_nav.wasm + list_nav.js
  → ... (所有兼容的 cross 示例)
```

- 遍历 `examples/cross/` 下所有示例目录
- 对每个兼容的示例：链接 `eui_example_setup()` + bootstrap + `libeui.a`
- Emscripten 标志：`-s WASM=1`、`-s EXPORTED_FUNCTIONS='["_eui_port_init","_eui_port_tick","_malloc","_free"]'`、`-s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]'`
- `add_custom_command` 复制 `shell.html` → `<example>.html`，注入正确的 `.js` 脚本引用和示例标题
- 生成 `index.html` 导航页，列出所有已构建的示例链接

### 输出结构

```
build/web/
├── index.html              ← 导航页，链接到所有已构建示例
├── shell.html              ← 共享模板（不直接提供访问）
├── shell.js                ← 共享 JS 运行时
├── basic_label.html        ← basic_label 示例页面
├── basic_label.js          ← Emscripten JS 胶水代码
├── basic_label.wasm        ← 编译产物
├── button_test.html
├── button_test.js
├── button_test.wasm
├── list_nav.html
├── list_nav.js
├── list_nav.wasm
├── menu_system.html        ← 其他兼容示例…
├── ...
```

### 开发时提供服务

```bash
cd build/web && python3 -m http.server 8000
# 浏览器打开 http://localhost:8000
```

迭代无需额外构建步骤——重新编译 WASM 后刷新浏览器即可。

## 示例页面

### 导航页（`index.html`）

构建时自动生成，列出本次构建中所有兼容的示例链接：
- `basic_label.html` — Basic Label
- `button_test.html` — Button Test
- `list_nav.html` — List Navigation
- `menu_system.html` — Menu System
- `dialog_overlay.html` — Dialog Overlay
- …（取决于配置兼容性）

结构为极简 HTML 页面，用 `<ul>` 链接到各示例。

### 示例页面（`shell.html` 模板）

每个示例页面包含：
- **Canvas 元素**（id=`eui-canvas`，CSS 尺寸为 `配置宽 × 缩放 × 配置高 × 缩放` 像素，缩放值来自 `EUI_CANVAS_SCALE`）
- **虚拟方向键**（id=`eui-dpad`，在非触屏设备上通过 CSS media query 隐藏）
- **示例标题** 用 `<h1>`
- **状态栏** 显示 FPS 计数器（可选，通过 URL 参数 `?fps=1` 开启）

布局：Canvas 居中，方向键在下方，标题在上方。响应式 CSS，移动优先。

## 示例约束检查

Web 配置通过 `requirements.cmake` 自动过滤不兼容的示例。与配置描述文件系统保持一致：

```cmake
# 配置: canvas_128x64_1bpp (w=128, h=64, depth=1)
#   basic_label     ✓ (无约束)
#   button_test     ✓ (无约束)
#   list_nav        ✓ (w>=128)
#   color_demo      ✗ (depth>=16) — 被跳过
#   amiibo_demo     ✗ (depth>=16, w>=240) — 被跳过
#   scene_view_demo ✗ (depth>=16) — 被跳过

# 配置: canvas_240x240_16bpp (w=240, h=240, depth=16)
#   所有示例 ✓
```

## 测试策略

Web 移植不设独立测试套件。验证方式：

1. **构建测试**：`emcmake cmake -B build -DEUI_CONFIG_PROFILE=configs/web/canvas_128x64_1bpp.cmake` 必须成功，无编译错误
2. **手动冒烟测试**：在 Chrome/Firefox/Safari 中打开构建产出的示例页面，验证：
   - Canvas 正确渲染 EUI 控件（标签文字、按钮、列表项）
   - 方向键可在控件间切换焦点
   - Enter/Backspace 触发预期行为（按钮点击、列表选择）
   - 移动端方向键与键盘操作等效
3. **现有 C 测试**：`ctest` 套件继续通过；Web 移植对桌面构建无回退影响

初始实现不包含自动化浏览器测试。

## 非目标（明确排除）

- 4bpp 或 8bpp 色彩深度支持（仅支持 1bpp 和 16bpp）
- 旧版 `examples/*.c` 迁移（配置文件系统迁移完成后 legacy 示例会被删除，Web 仅面向 `examples/cross/`）
- 生产级 Web 应用部署（无离线支持、无 PWA、无 CDN 优化）
- 原生触屏事件 `EUI_EVT_TOUCH_DOWN/UP/MOVE`（仅方向键映射）
- 多点触控或手势支持
- WebGL 渲染后端（仅 Canvas2D）
- 自动化无头浏览器 CI 测试
- 音频输出
- FULL 以外的 EUI buffer 模式（浏览器不需要 PAGE 和 DIRECT）

## 实施依赖

本 spec 依赖[示例配置描述文件设计](2026-05-24-example-config-profiles-design.md)先行完成：
- `configs/` 目录体系已建立
- `EUI_CONFIG_PROFILE` 变量已支持
- 所有 legacy 示例已迁移至 `examples/cross/`
- `requirements.cmake` 约束检查机制已就绪
- Bootstrap 驱动选择（`#if EUI_DISPLAY_DRIVER == EUI_DRIVER_WEB`）模式已建立

Web 移植在配置文件系统就绪后，作为**增量变更**加入：新增 `configs/web/` 目录 + `port/web/` 目录 + `eui_drv_web` 驱动，不修改现有 `pc/`/`esp32/`/`nrf52/` 的任何代码。
