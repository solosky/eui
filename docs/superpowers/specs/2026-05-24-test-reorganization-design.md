# Test Reorganization Design

## Status: Approved

## Problem

EUI 项目测试套件存在 10 个结构性问题：

1. **TEST/PASS/FAIL 宏在 23 个文件中重复定义** — 共 ~230 行重复代码
2. **mock_write_buffer / count_pixels 在 5+ 文件中拷贝** — ~250 行重复
3. **CMakeLists.txt 每个 test 重复 4 行样板** — 120 行
4. **内存池大小不一致**（32768 / 65536 / 524288）
5. **tests_run 递增位置不一致** — 有的在 TEST 宏、有的在 PASS/FAIL 宏
6. **include `../src/*_internal.h` 打破封装边界** — 5 个文件
7. **测试分类散乱** — Font 相关 12 个文件扁平放在 test/ 下，无子目录
8. **固定测试数据 .h 文件与 .c 混杂** — 无 data/ 目录
9. **无断言库 / 无 setup-teardown**
10. **Render 测试仅检查像素数 != 0，不验证正确性** — 保留为视觉确认用途

## Design

### 1. 公共测试基础设施

新增 `test/common/` 目录，包含：

#### `test/common/eui_test.h`

标准化宏和 TLSF 初始化：

- `eui_test_init()` — 使用统一内存池（65536 字节）初始化 TLSF，幂等调用
- `TEST(name)` / `PASS()` / `FAIL(msg)` — 统一断言宏，tests_run 在 TEST 中递增
- `eui_test_summary()` — 打印汇总并返回 exit code

所有测试文件的 `main()` 调用 `eui_test_init()` 替代原有 `eui_allocator_init_tlsf(mem_pool, POOL_SIZE)`。

#### `test/common/eui_test_canvas.h`

便捷函数：
- `eui_test_canvas_new(drv)` — 创建 canvas 并绑定到指定 display driver
- `eui_test_canvas_free(c)` — 销毁 canvas

不包含 mock display HAL — 各模块按需自身定义。

### 2. 目录结构

```
test/
├── common/
│   ├── eui_test.h
│   └── eui_test_canvas.h
├── data/                         # 字体测试数据（从 test/ 根目录移入）
│   ├── test_vlw_font.h
│   ├── test_u8g2_font.h
│   ├── test_u8g2_profont10_data.h
│   ├── test_u8g2_wqy12_ch1_data.h
│   └── test_font_kerning.h
├── allocator/
│   └── test_allocator.c
├── event/
│   └── test_event.c
├── input/
│   └── test_input.c
├── str/
│   └── test_str.c
├── canvas/
│   ├── test_canvas.c
│   ├── test_canvas_16bpp.c
│   └── test_canvas_render.c
├── font/
│   ├── test_font_formats.c           # 合并 test_font.c + test_font_u8g2.c
│   ├── test_font_kerning.c
│   ├── test_wqy13_kerning.c          # 合并 test_wqy13_kerning.c + test_wqy13_u8g2_kerning.c
│   ├── test_font_canvas.c
│   ├── test_font_multiline.c
│   ├── test_font_vs_u8g2.c
│   ├── test_font_wqy13_render.c
│   └── test_font_real_u8g2_render.c  # 合并 test_font_wqy12_chinese.c
├── view/
│   ├── test_view.c
│   ├── test_widget.c
│   ├── test_view_transition.c
│   └── test_view_transition_render.c
├── drivers/
│   └── test_drivers.c
└── CMakeLists.txt
```

### 3. 合并计划

| 原文件 1 | 原文件 2 | 合并为 | 理由 |
|---|---|---|---|
| `test_font.c` (BDF+VLW) | `test_font_u8g2.c` (U8G2) | `font/test_font_formats.c` | 相同 API 测试，仅字体格式不同 |
| `test_font_wqy12_chinese.c` | `test_font_real_u8g2_render.c` | `font/test_font_real_u8g2_render.c` | 后者已包含 wqy12 渲染，前者为子集 |
| `test_wqy13_kerning.c` (诊断打印) | `test_wqy13_u8g2_kerning.c` (u8g2对比) | `font/test_wqy13_kerning.c` | 同一字体的 kerning 测试 |

结果：Font 模块 12 → 9 文件，总测试文件 23 → 20。

### 4. 内部头文件引用清理

| 当前 | 改为 |
|---|---|
| `#include "../src/eui_font_internal.h"` | `#include "eui/eui_font_internal.h"`（已在 include/eui/ 中存在） |
| `#include "../src/eui_font_u8g2_internal.h"` | 将该头文件从 `src/` 移到 `include/eui/` 下 |
| `extern int32_t eui_font_u8g2_lookup_glyph(...)` | 删除 extern 声明，使用头文件中的正式声明 |

### 5. CMakeLists.txt 重构

封装 `eui_add_test()` function：

```cmake
function(eui_add_test name source)
    cmake_parse_arguments(TEST "" "" "INCLUDES;SOURCES;LIBS" ${ARGN})
    add_executable(${name} ${source} ${TEST_SOURCES})
    target_include_directories(${name} PRIVATE
        ${CMAKE_BINARY_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${TEST_INCLUDES}
    )
    target_link_libraries(${name} PRIVATE eui ${TEST_LIBS})
    add_test(NAME ${name} COMMAND ${name})
endfunction()
```

每个测试一行调用。u8g2 对比测试用 `SOURCES` 和 `INCLUDES` 参数传递额外源文件和头文件路径。raylib 渲染测试用 `LIBS` 参数。

### 6. 不做的改动

- 不引入第三方测试框架（保持 C99 + hand-rolled 断言）
- render 测试不加 CRC/哈希验证（保持视觉确认用途，仅 `count_pixels() != 0` 存在性检查）
- 不改变 mock display HAL 的各自定义模式（各模块独立维护 mock）
- 不改变测试断言风格（保持 TEST/PASS/FAIL 宏）

## Test Plan

重构完成后验证：
1. `cmake -B build && cmake --build build -j$(nproc)` 编译通过
2. `cd build && ctest --output-on-failure` 所有 20 个测试 PASS
3. 包含头文件路径检查：无 `../src/` 引用残留
