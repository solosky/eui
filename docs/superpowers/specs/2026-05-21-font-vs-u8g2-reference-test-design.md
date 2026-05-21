# 字体渲染对比验证测试 — 设计文档

## 概述

以 u8g2 库的字体渲染结果为基准，编写自动化测试验证 eui 库的字体渲染功能是否与之一致。
测试覆盖多种场景：纯英文、中文、带符号的中文、大小写、kerning 等。

## 架构

单一测试可执行文件 `test_font_vs_u8g2`，同时链接 eui 和 u8g2 两个库。
测试分层两阶段，由场景数据表统一驱动。

```
┌──────────────────────────────────────────┐
│         场景数据表                        │
│  描述, 字符串, 字体, 预期宽度             │
└──────────────┬───────────────────────────┘
               │
    ┌──────────┴──────────┐
    ▼                     ▼
┌────────────┐       ┌────────────┐
│ 第一层     │       │ 第二层     │
│ 字形级对比 │       │ Canvas级   │
│ (逐字)     │       │ (全缓冲)   │
└─────┬──────┘       └─────┬──────┘
      │                    │
      ▼                    ▼
┌─────────────────────────────────┐
│  PASS / FAIL (含差异详情)       │
└─────────────────────────────────┘
```

## 测试场景

| # | 场景 | 字体 | 字符串 |
|---|------|------|--------|
| 1 | 纯英文 ASCII | profont10 | `"Hello World"` |
| 2 | 全大写 | profont10 | `"ABCDEFGHIJKLM"` |
| 3 | 全小写 | profont10 | `"abcdefghijklm"` |
| 4 | 大小写混合 | profont10 | `"HeLlO wOrLd"` |
| 5 | 数字 | profont10 | `"0123456789"` |
| 6 | 标点符号 | profont10 | `"!@#$%^&*()-+="` |
| 7 | 单个字符 | profont10 | `"A"` |
| 8 | 纯空格 | profont10 | `"     "` |
| 9 | 空字符串 | profont10 | `""` |
| 10 | Amiibo | profont10 | `"Amiibo"` |
| 11 | 纯中文 | wqy12 | `"你好世界"` |
| 12 | 中英混合 | wqy12 | `"Hello你好World世界"` |
| 13 | 中文+Unicode符号 | wqy12 | `"温度:25℃ ★评级A"` |
| 14 | 全中文多种 | wqy12 | `"中文测试验证"` |
| 15 | 中文标点 | wqy12 | `"「你好」『世界』"` |
| 16 | Kerning 配对 | profont10 | `"TAVAWA"` |

## 第一层：字形级对比 (Glyph-level)

对场景字符串中的每个 UTF-8 码点，逐字渲染并对比。

**流程：**
1. 通过 u8g2 渲染单个字形到其 tile buffer
2. 将 u8g2 的垂直列主序（vertical LSB-top）像素格式转换为水平行主序（horizontal MSB-left）
3. 通过 `eui_font_u8g2_draw_glyph()` 渲染同一字形到独立缓冲区
4. `memcmp` 逐字节对比两个字形缓冲区
5. 验证 `x_advance`（字形宽度）一致

**对比条件：**
- 仅对比双方都能渲染的码点（若某方无该字形，记录警告并跳过）
- 缓冲区大小：`glyph_height × ((max_char_width + 7) / 8)` 字节
- 每次绘制前将缓冲区清零

**像素格式转换（u8g2 → eui 水平格式）：**
```c
// u8g2: 垂直列主序, LSB 顶部
pixel = (tile_buf[(y/8) * tile_width + x] >> (y & 7)) & 1;

// eui 字形缓冲: 水平行主序, MSB 左侧
if (pixel) buf[y * stride + x/8] |= (1u << (7 - (x % 8)));
```

## 第二层：Canvas 级对比

通过完整的 Canvas / 字符串绘制 API 渲染整段字符串，像素级对比。

**u8g2 侧：**
- `u8g2_SetupBitmap()` 创建 128×64 内存缓冲区
- ASCII 场景用 `u8g2_DrawStr()`，中文场景用 `u8g2_DrawUTF8()`
- `u8g2_SendBuffer()` 将 tile 缓冲刷入像素缓冲

**eui 侧：**
- `eui_canvas_create()` 创建 128×64 full-buffer canvas
- `eui_canvas_draw_str()` 绘制字符串（内部处理 UTF-8 解码和字形分发）
- `eui_canvas_commit()` 刷入 mock buffer

**像素格式对比策略：**
- eui canvas：水平行主序，LSB 左侧（`bit_pos = x % 8`）
- u8g2 bitmap：垂直列主序，LSB 顶部
- 通过统一的像素读写函数 `get_pixel(buf, x, y, format)` 屏蔽差异
- 遍历 128×64 所有像素，逐点对比

**额外验证：**
- 字符串宽度：`eui_canvas_str_width()` / `eui_font_get_str_width()` vs `u8g2_GetUTF8Width()`

**差异输出：**
- 首个不匹配像素：报告场景名、(x, y)、期望值、实际值
- 统计总不匹配像素数
- 将差异区域写为 BMP 文件：`test_<idx>_<desc>_expected.bmp` / `_actual.bmp`

## 字体数据管理

两端共享同一字体数据数组。

| 字体 | u8g2 源文件 | eui 头文件 | 覆盖 |
|------|------------|------------|------|
| profont10 | `u8g2_font_profont10_tf.c` | `test_u8g2_profont10_data.h` | ASCII 32-127 |
| wqy12_ch1 | `u8g2_font_wqy12_t_chinese1.c` | `test_u8g2_wqy12_ch1_data.h` | ASCII + 中文 + 符号 |

**共享方式**：
- u8g2 侧：直接 `#include` 字体 `.c` 源文件（声明 `u8g2_font_xxx` 结构体），通过 `u8g2_SetFont()` 使用
- eui 侧：通过现有测试头文件 (`test_u8g2_xxx_data.h`) 引用原始字体数据数组（`u8g2_font_xxx_data`），初始化 `eui_font_t.data` 指针
- 两端指向同一底层字节数组，保证字体数据完全一致

## 构建集成

**新增文件：**
- `test/test_font_vs_u8g2.c` — 主测试

**修改文件：**
- `test/CMakeLists.txt` — 添加 test_font_vs_u8g2 目标

**CMake 配置：**
```cmake
set(U8G2_DIR "/home/solosky/u8g2" CACHE PATH "u8g2 source directory")

if(EXISTS ${U8G2_DIR})
    add_executable(test_font_vs_u8g2 test_font_vs_u8g2.c)

    # 直接编译 u8g2 核心源文件
    target_sources(test_font_vs_u8g2 PRIVATE
        ${U8G2_DIR}/csrc/u8g2_setup.c
        ${U8G2_DIR}/csrc/u8g2_buffer.c
        ${U8G2_DIR}/csrc/u8g2_font.c
        ${U8G2_DIR}/csrc/u8g2_ll_hvline.c
        ${U8G2_DIR}/csrc/u8g2_d_memory.c
        ${U8G2_DIR}/csrc/u8x8_capture.c
        ${U8G2_DIR}/sys/bitmap/common/u8x8_d_bitmap.c
        ${U8G2_DIR}/csrc/u8g2_font_profont10_tf.c
        ${U8G2_DIR}/csrc/u8g2_font_wqy12_t_chinese1.c
    )

    target_include_directories(test_font_vs_u8g2 PRIVATE
        ${CMAKE_BINARY_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${U8G2_DIR}/csrc
    )
    target_link_libraries(test_font_vs_u8g2 PRIVATE eui)
    add_test(NAME font_vs_u8g2 COMMAND test_font_vs_u8g2)
endif()
```

## 测试流程

```
1. 初始化 TLSF 内存分配器
2. 遍历场景数据表，对每个场景：
   a. 第一层（字形级）：
      - 对每个码点：u8g2 渲染字形 → 格式转换 → eui 渲染字形 → memcmp
      - 验证 x_advance 一致
   b. 第二层（Canvas 级）：
      - u8g2 全字符串渲染到 128×64 bitmap 缓冲
      - eui canvas 全字符串渲染到 128×64 mock 缓冲
      - 全区域逐像素对比
      - 验证字符串宽度一致
3. 输出：N 个场景，M 个子测试，逐个场景报告 PASS/FAIL
4. 失败时输出 *_expected.bmp 和 *_actual.bmp 供人工检查
5. 返回值：全部通过为 0，任一失败为 1
```

## 边界条件

- **空字符串**：不绘制任何像素，宽度为 0
- **空格**：推进 x 位置但不绘制像素
- **字符缺失**：字体中不存在的码点，双方都应返回 0（不绘制），记录警告
- **Kerning**：字符对的间距调整通过 Canvas 级测试验证
