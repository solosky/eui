# EUI 2bpp（每像素2位）支持

**日期：** 2026-05-24
**状态：** 设计（实现前）

## 概述

为 EUI 添加编译时 2bpp 颜色深度支持，实现面向显示器和内存受限目标的 4 级灰度渲染。

## 像素格式

- **4 级灰度：** 0 = 黑色，1 = 深灰，2 = 浅灰，3 = 白色
- **打包：** 每字节 4 像素，MSB 优先（与现有 1bpp 约定一致）
  - 像素 0 → 位 [7:6]，像素 1 → 位 [5:4]，像素 2 → 位 [3:2]，像素 3 → 位 [1:0]
  - 字节索引：`(y * (width / 4)) + (x / 4)`
  - 移位：`6 - 2 * (x % 4)`
- **`eui_color_t`：** `uint8_t`（取值范围 0-3）
- **`EUI_COLOR_WHITE`：** 3（2 位最大值）

## 配置

- `CMakeLists.txt`：在缓存字符串中接受 `EUI_COLOR_DEPTH 2`
- `eui_config.h.in`：更新 `#error` 以包含 `2`
- `test/config_2bpp.h`：测试覆盖配置（类似现有的 `config_16bpp.h`）

## 颜色转换（`eui_types.c`）

- `eui_color_from_rgb()`：加权灰度 → `gray * 3 / 255`
- `eui_color_from_gray()`：`gray * 3 / 255`

## Canvas 修改（`eui_canvas.c`）

### `canvas_set_pixel()`
新增 `#elif EUI_COLOR_DEPTH == 2` 分支：将 2 位值打包到字节内 MSB 对齐的位对中。

### `canvas_buf_size()`
`pixels / 4` — 每字节容纳 4 个像素。

### `eui_canvas_clear()`、`begin_page()`、`next_page()`
新增 `#elif EUI_COLOR_DEPTH == 2` 分支。将 `bg_color`（0-3）复制到填充字节的每个 2 位槽中：
```c
uint8_t fill = (uint8_t)(canvas->bg_color & 3u);
fill |= fill << 2;
fill |= fill << 4;
fill |= fill << 6;
memset(canvas->buffer, fill, size);
```
bg_color=0 时：fill=0x00。bg_color=3 时：fill=0xFF。bg_color=1 时：fill=0x55。bg_color=2 时：fill=0xAA。

### `eui_canvas_invert_rect()`
新增分支：每个 2 位像素与 `0x03` 进行异或（黑色 ↔ 白色切换）。

### `eui_canvas_draw_bitmap()`
在像素读取循环中添加 `depth == 2` 分支：从打包的源数据中提取 2 位值。

## 字体引擎

- `canvas_set_pixel()` 处理打包 → 通过 `draw_glyph()` 实现的字形渲染透明工作
- 原始缓冲区 `draw_char()` API（BDF、VLW、U8G2）：使用相同的 MSB 优先打包添加 2bpp 分支

## 显示驱动

### Raylib（`eui_drv_raylib.c`）
在 `write_buffer()` 中新增 `color_depth == 2` 分支：
- 0 → (0, 0, 0)，1 → (85, 85, 85)，2 → (170, 170, 170)，3 → (255, 255, 255)

### Web（`eui_drv_web.c`）
新增 `color_depth == 2` 分支 → 调用相应的 JS 渲染函数。

### 内置驱动（SSD1306、ST7735 等）
不做修改——它们硬编码了自身的颜色深度。未来的 2bpp 显示驱动会设置 `caps.color_depth = 2`。

## 测试

- `test/config_2bpp.h`：预定义 `EUI_COLOR_DEPTH 2` 并设置合适的池大小
- 测试用例：set_pixel 回读、clear、invert_rect、使用 2bpp 源绘制位图、颜色转换（0-3 范围）、缓冲区大小计算
- CMake 测试：添加 2bpp 配置测试目标（或复用现有的多配置测试模式）

## 修改的文件

| 文件 | 修改内容 |
|------|----------|
| `CMakeLists.txt` | 在深度列表中接受 `2` |
| `include/eui/eui_config.h.in` | 更新 `#error` |
| `include/eui/eui_types.h` | `eui_color_t` 分支、`EUI_COLOR_WHITE` 分支 |
| `src/eui_types.c` | `from_rgb` / `from_gray` 2bpp 分支 |
| `src/eui_canvas.c` | `set_pixel`、`buf_size`、`invert_rect`、`draw_bitmap` |
| `src/eui_font_bdf.c` | `draw_char` 2bpp |
| `src/eui_font_vlw.c` | `draw_char` 2bpp |
| `src/eui_font_u8g2.c` | `draw_glyph` / `rle_decode` 2bpp |
| `src/driver/eui_drv_raylib.c` | `write_buffer` 2bpp |
| `src/driver/eui_drv_web.c` | `write_buffer` 2bpp |
| `test/config_2bpp.h` | 新建测试配置 |
| `test/` | 2bpp 测试用例 |
