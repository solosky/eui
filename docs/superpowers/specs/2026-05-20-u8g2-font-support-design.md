# u8g2 完整字体支持设计

## 概述

重构 EUI 字体系统，使其能直接复用 u8g2 库的 `.c` 字体数据文件，支持 u8g2 字体特性（编码表、Kerning、Unicode），新增多行文本绘制与英文折行、文本对齐/裁剪/截断等操作。同时用编译开关控制功能模块，满足 MCU 低资源场景。测试覆盖率达到行覆盖 90% 以上。

## 字体数据格式

### 当前格式

| format 值 | 宏常量 | 格式 |
|-----------|--------|------|
| 0 | `EUI_FONT_FORMAT_BDF` | 简化 BDF：3 字节头 + LE 偏移表 + 5 字节字形头 + 1bpp bitmap |
| 1 | `EUI_FONT_FORMAT_VLW` | Processing VLW 格式：BE 头 + 28 字节字形表 + 8bpp grayscale bitmap |

### 新增格式

| format 值 | 宏常量 | 格式 |
|-----------|--------|------|
| 2 | `EUI_FONT_FORMAT_U8G2` | u8g2 `.c` 字体原始二进制：位级紧凑编码 + 编码表 |

### u8g2 .c 字体二进制布局

u8g2 字体由 `bdfconv` 工具生成，存储在 `const uint8_t[]` 数组中：

```
┌─────────────────────────────────────────────────────────────────┐
│                    u8g2_font_info_t (位级编码头)                  │
├─────────────────────────────────────────────────────────────────┤
│  glyph_cnt        (1B)   — 字形总数                              │
│  bbx_mode         (1B)   — 包围盒模式                           │
│  bits_per_0       (1B)   — 每组"0"bit 编码位数                   │
│  bits_per_1       (1B)   — 每组"1"bit 编码位数                   │
│  bits_per_char_w  (1B)   — 字符宽度编码位数                      │
│  bits_per_char_h  (1B)   — 字符高度编码位数                      │
│  bits_per_char_x  (1B)   — 字符 x_offset 编码位数                │
│  bits_per_char_y  (1B)   — 字符 y_offset 编码位数                │
│  bits_per_delta_x (1B)   — 字符 delta_x(x_advance) 编码位数      │
│  max_char_width   (2B LE)                                       │
│  max_char_height  (2B LE)                                       │
│  x_offset         (1B signed)                                   │
│  y_offset         (1B signed)                                   │
│  ascent_A         (2B LE signed)                                │
│  ascent_g         (2B LE signed)                                │
│  ascent_para      (2B LE signed)                                │
│  start_pos_unicode(2B LE)  — Unicode 编码表起始偏移（0 表示无）   │
│  start_pos_lower  (2B LE)  — 可选                               │
│  start_pos_upper  (2B LE)  — 可选                               │
├─────────────────────────────────────────────────────────────────┤
│               glyph_bitmap[] (紧凑位编码)                         │
│                                                                  │
│   所有字形数据被编码为连续的位流（bit stream）。每个字形记录：       │
│   [zeros][ones]...[char_w][char_h]...[x_off][y_off][delta]       │
│                                                                  │
│   其中：                                                         │
│   - [zeros]: bits_per_0 位，表示连续"0"位个数，特殊值触发编码跳转  │
│   - [ones]:  bits_per_1 位，表示连续"1"位个数，特殊值触发编码跳转  │
│   - 编码跳转时读取后续的 char_w/char_h 等字段                     │
│   - bitmap 紧随各字形编码之后，为连续 1bpp 位数据                  │
│                                                                  │
│   解码方式：维护一个位指针（字节索引 + bit 偏移），按 bits_per_*    │
│   逐字段读取。位流采用 MSB first（每字节从左到右）。               │
├─────────────────────────────────────────────────────────────────┤
│               encoding_list[] (Unicode 编码表)                    │
│                                                                  │
│   位于 bitmap 区域的 start_pos_unicode 偏移处。                   │
│   编码表将 Unicode code point 映射到 glyph 索引。                  │
│   支持 kerning 时，编码表使用 (prev_code, curr_code) 对进行查找。 │
└─────────────────────────────────────────────────────────────────┘
```

**Glyph 位级编码详解：**

位流解码伪代码：
```
function decode_glyph(bitstream):
    x = read_bits(bits_per_char_x)
    y = read_bits(bits_per_char_y)
    w = read_bits(bits_per_char_w)
    h = read_bits(bits_per_char_h)
    delta = read_bits(bits_per_delta_x)
    bitmap_bits = w * h
    bitmap = read_bits(bitmap_bits)
    return Glyph{x, y, w, h, delta, bitmap}
```

**特殊值语义：**
- `chars_per_0` 编码时，全 1 值表示编码跳转（开始新的字形记录）
- `chars_per_1` 编码时，全 1 值表示该 run 结束

## 数据结构

### eui_font_t 扩展

```c
/* u8g2 font encoding lookup callback */
typedef int16_t (*eui_font_lookup_fn)(const struct eui_font *font,
                                       uint16_t encoding, uint16_t prev);

typedef struct eui_font {
    uint8_t  format;
    uint8_t  line_height;
    uint8_t  baseline;
    uint8_t  flags;
    const uint8_t *data;
#if EUI_FONT_ENABLE_U8G2
    eui_font_lookup_fn lookup_glyph;  /* 仅 U8G2 字体使用 */
#endif
} eui_font_t;
```

### Font flags 扩展

```c
#define EUI_FONT_FIXED_WIDTH   (1u << 0)
#define EUI_FONT_HAS_KERNING   (1u << 1)
#define EUI_FONT_HAS_UNICODE   (1u << 2)
```

### 新增编译配置宏（eui_config.h.in）

```cmake
#cmakedefine EUI_COLOR_DEPTH       @EUI_COLOR_DEPTH@
#cmakedefine EUI_MEM_POOL_SIZE     @EUI_MEM_POOL_SIZE@
#cmakedefine EUI_MAX_VIEWS         @EUI_MAX_VIEWS@
#cmakedefine EUI_MAX_ANIMATIONS    @EUI_MAX_ANIMATIONS@
#cmakedefine EUI_MAX_WIDGETS       @EUI_MAX_WIDGETS@
#cmakedefine EUI_EVENT_QUEUE_SIZE  @EUI_EVENT_QUEUE_SIZE@
#cmakedefine EUI_MAX_OVERLAYS      @EUI_MAX_OVERLAYS@
#cmakedefine EUI_FONT_ENABLE_U8G2     @EUI_FONT_ENABLE_U8G2@      /* 新增 */
#cmakedefine EUI_FONT_ENABLE_KERNING  @EUI_FONT_ENABLE_KERNING@   /* 新增 */
#cmakedefine EUI_FONT_ENABLE_MULTILINE @EUI_FONT_ENABLE_MULTILINE@ /* 新增 */
```

默认值：`EUI_FONT_ENABLE_U8G2=1`, `EUI_FONT_ENABLE_KERNING=1`, `EUI_FONT_ENABLE_MULTILINE=1`。

## 文件结构

```
src/
├── eui_font.c              # 调度层（扩展支持 format=2）
├── eui_font_bdf.c          # 简化 BDF 后端（不变）
├── eui_font_vlw.c          # VLW 后端（不变）
├── eui_font_u8g2.c         # 新增：u8g2 字体后端
├── eui_font_internal.h     # 内部头（新增 u8g2 函数声明）
└── eui_canvas.c            # Canvas 绘制（新增多行/裁剪/截断/kerning 逻辑）

include/eui/
├── eui_font.h              # 公共字体 API（新增 U8G2 格式常量 + lookup 类型）
├── eui_canvas.h            # Canvas API（新增多行/裁剪等函数）
├── eui_types.h             # 扩展 eui_font_t 和 flags
└── eui_config.h.in         # 新增编译宏

test/
├── test_font.c             # 扩展：U8G2 格式 + kerning 测试
├── test_font_multiline.c   # 新增：多行文本 + 折行测试
├── test_font_canvas.c      # 新增：Canvas 级字体绘制测试
├── test_u8g2_font.h        # 新增：测试用 u8g2 格式字体数据
└── test_font_kerning.h     # 新增：带 kerning 的测试字体数据
```

## Kerning 实现

### 原理

u8g2 字体的 encoding_list 支持基于上一字符的 glyph 选择。查表时输入 `(prev_code, curr_code)`，返回更紧凑的字形索引。

### 流程

```
eui_canvas_draw_str() 逐字符绘制流程：

  prev = 0
  for each curr_code in str:
    if font->flags & EUI_FONT_HAS_KERNING and font->lookup_glyph:
        glyph_idx = font->lookup_glyph(font, curr_code, prev)
    else:
        glyph_idx = font->lookup_glyph(font, curr_code, 0)
    
    glyph = decode_glyph_at_index(font, glyph_idx)
    render_glyph(canvas, glyph, x, y)
    x += glyph->delta_x
    prev = curr_code
```

### 编译开关

`#if EUI_FONT_ENABLE_KERNING` 包裹 kerning 相关代码路径。禁用时 `prev` 始终传 0，不查 kerning 编码表。

## 新增绘制 API

### 公共接口（eui_canvas.h）

```c
/* 在矩形内对齐绘制单行文本 */
uint16_t eui_canvas_draw_str_in_rect(eui_canvas_t *canvas,
                                      const eui_rect_t *rect,
                                      const char *str,
                                      eui_align_t h_align,
                                      eui_align_t v_align);

/* 多行文本绘制，自动英文单词折行 */
uint16_t eui_canvas_draw_str_multiline(eui_canvas_t *canvas,
                                        const eui_rect_t *rect,
                                        const char *str,
                                        uint8_t line_height,
                                        eui_align_t h_align);

/* 带裁剪的单行文本绘制 */
uint16_t eui_canvas_draw_str_clipped(eui_canvas_t *canvas,
                                      const eui_rect_t *clip_rect,
                                      int16_t x, int16_t y,
                                      const char *str);

/* 超长文本自动截断加 "..." */
uint16_t eui_canvas_draw_str_ellipsis(eui_canvas_t *canvas,
                                       int16_t x, int16_t y,
                                       const char *str,
                                       uint16_t max_width);

/* 测量多行文本总高度 */
uint16_t eui_canvas_str_multiline_height(const eui_canvas_t *canvas,
                                          const char *str,
                                          uint16_t max_width,
                                          uint8_t line_height);
```

### 英文折行算法

```
word_wrap(str, max_pixel_width, font):
    lines = []
    current_line = ""
    current_pixel_width = 0
    last_space_pos = -1
    
    for each char in str:
        if char == '\n':
            lines.append(current_line)
            current_line = ""; reset width
            continue
        if char == '\r':
            continue
        
        char_width = get_char_width(font, char)
        
        if char == ' ':
            last_space_pos = len(current_line)
        
        if current_pixel_width + char_width > max_pixel_width:
            if last_space_pos >= 0:
                // 在最近空格处断行（单词完整）
                lines.append(current_line[0:last_space_pos])
                remaining = current_line[last_space_pos+1:] + char
                current_line = remaining
            else:
                // 单单词超宽，硬断（字符边界）
                lines.append(current_line)
                current_line = "" + char
            
            recalculate current_pixel_width from current_line
        else:
            current_line += char
            current_pixel_width += char_width
    
    if current_line != "":
        lines.append(current_line)
    
    return lines
```

## 测试计划

### 测试矩阵

| 测试文件 | 覆盖模块 | 预计测试数 |
|---------|---------|-----------|
| test_font.c | BDF/VLW/U8G2 格式解析、字符宽度、字符串宽度、字形绘制 | 35+ |
| test_font_u8g2.c | U8G2 位解码、glyph 遍历、编码表查找、边界条件 | 25+ |
| test_font_kerning.c | Kerning 启用/禁用、kerning 表查找、正确性验证 | 15+ |
| test_font_multiline.c | 多行文本、英文折行、边界（空字符串/超长单词/纯换行） | 20+ |
| test_font_canvas.c | Canvas 级 draw_str 系列、对齐、裁剪、截断 | 25+ |
| test_font_coverage.c | 组合场景、错误输入、NULL 安全 | 10+ |

### 覆盖率目标

- 每行代码覆盖率 >= 90%
- 分支覆盖率 >= 85%
- 所有公开 API 必须被测试
- 所有 `#ifdef` 分支（启用/禁用）均需覆盖

### 测试策略

1. **白盒测试**：对位级解码器做精细化测试（每个 bits_per 组合、边界值）
2. **组合测试**：多行 + kerning、裁剪 + 截断等组合场景
3. **边界测试**：空输入、NULL、超长、仅空格、连续换行
4. **配置测试**：每种 `#ifdef` 组合至少一个 CI 构建验证

## 实现顺序

1. **Phase 1: U8G2 字体格式解析器** — `eui_font_u8g2.c`（位解码 + glyph 遍历）
2. **Phase 2: 编码表与 Kerning** — encoding_list 查找 + kerning 集成
3. **Phase 3: 新增绘制 API** — 多行、裁剪、截断在 `eui_canvas.c` 中实现
4. **Phase 4: 测试** — 按测试矩阵逐个覆盖，确保 90%+ 行覆盖
5. **Phase 5: 覆盖率验证** — 运行 gcov/lcov 验证覆盖率目标

## 向后兼容

- 现有 `EUI_FONT_FORMAT_BDF` (0) 和 `EUI_FONT_FORMAT_VLW` (1) 格式不受影响
- 现有 `eui_canvas_draw_str`、`eui_canvas_draw_str_aligned` 保持不变
- 新增 API 均通过 `#if EUI_FONT_ENABLE_*` 可选编译
- 现有测试全部保持通过
