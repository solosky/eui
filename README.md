# EUI — Embedded UI Framework

轻量级嵌入式图形用户界面框架，专为 **RAM < 32KB** 的 MCU 设计。

[![Build & Test](https://github.com/solosky/eui/actions/workflows/build.yml/badge.svg)](https://github.com/solosky/eui/actions/workflows/build.yml)

## 特性

- **TLSF 内存分配器** — O(1) 确定性分配/释放，零碎片
- **多色深支持** — 1bpp / 4bpp / 8bpp / 16bpp 统一颜色抽象
- **多缓冲模式** — FULL / PAGE / DIRECT 自适应
- **事件驱动视图** — 单一 handler 处理绘制、输入、生命周期
- **内置控件库** — Label / Button / List / Menu / Progress / Slider / Scroll / Dialog
- **MotionC 动画** — Q16.16 定点数缓动 + 弹簧物理，无 FPU 依赖
- **raylib 桌面模拟** — 在 PC 上开发调试，再交叉编译到目标板
- **双字体格式** — u8g2 BDF + VLW，Flash 直接寻址
- **外部分配器接口** — 支持用户替换内存分配器

## 快速开始

```c
#include "eui.h"

#define POOL_SIZE 8192
static uint8_t mem_pool[POOL_SIZE];

/* 实现显示 HAL */
static eui_display_hal_t my_display = {
    .caps = { .width = 128, .height = 64, .color_depth = 1, .buffer_mode = EUI_BUFFER_FULL },
    .init = my_display_init,
    .write_buffer = my_display_flush,
};

/* 实现输入 HAL */
static eui_input_hal_t my_input = {
    .poll = my_keypad_poll,
};

int main(void) {
    eui_config_t cfg = {
        .mem_pool_buffer = mem_pool,
        .mem_pool_size = POOL_SIZE,
        .display = &my_display,
        .input = &my_input,
        .fps_target = 30,
    };
    eui_init(&cfg);

    /* 创建 Label */
    eui_widget_t *label = eui_label_create("Hello EUI!", 0, 0);

    while (eui_is_running()) {
        eui_tick();
        HAL_Delay(33);
    }

    eui_deinit();
    return 0;
}
```

> 完整示例见 [示例程序](examples/)

## 构建

### 桌面模拟（raylib）

```bash
git clone --recurse-submodules https://github.com/solosky/eui.git
cd eui

# 安装 raylib
# Ubuntu: sudo apt install libraylib-dev
# macOS: brew install raylib

cmake -B build -DEUI_BUILD_EXAMPLES=ON
cmake --build build
./build/examples/basic_label
```

### 嵌入式目标

```bash
cmake -B build \
    -DCMAKE_TOOLCHAIN_FILE=your_toolchain.cmake \
    -DEUI_BUILD_TESTS=OFF \
    -DEUI_BUILD_EXAMPLES=OFF \
    -DEUI_COLOR_DEPTH=1 \
    -DEUI_MEM_POOL_SIZE=4096
cmake --build build
# 链接 libeui.a 到你的固件
```

## 编译选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `EUI_COLOR_DEPTH` | 1 | 色深：1, 4, 8, 16 |
| `EUI_MEM_POOL_SIZE` | 8192 | TLSF 内存池大小（字节） |
| `EUI_MAX_VIEWS` | 8 | 最大 View 数 |
| `EUI_MAX_WIDGETS` | 32 | 最大 Widget 数 |
| `EUI_EVENT_QUEUE_SIZE` | 8 | 事件队列容量 |
| `EUI_MAX_OVERLAYS` | 4 | Overlay 栈深度 |

## 项目结构

```
eui/
├── include/eui/          # 公共头文件
│   ├── eui.h             # 总入口
│   ├── eui_types.h       # 核心类型 (rect, color, bitmap, font)
│   ├── eui_allocator.h   # 外部分配器接口
│   ├── eui_canvas.h      # Canvas 绘图 API
│   ├── eui_view.h        # View 事件系统
│   ├── eui_widget*.h     # 控件库
│   └── eui_hal_raylib.h  # raylib 桌面模拟
├── src/                  # 框架实现
├── third_party/          # 第三方库
│   ├── tlsf/             # TLSF 分配器
│   └── motionc/          # MotionC 动画引擎
├── examples/             # 示例程序
├── test/                 # 单元测试
└── docs/                 # 文档
```

## 支持平台

- **MCU**: ARM Cortex-M0+/M3/M4, AVR, ESP8266, ESP32
- **屏幕**: SSD1306, SH1106, ST7735, ILI9341 (全部通过 HAL 适配)
- **输入**: 矩阵键盘, 旋转编码器, 触摸屏

## 许可

MIT License

## 文档

- [框架设计文档](docs/eui_framework_design.md)
- [API 参考](docs/api_reference.md)
- [快速入门](docs/quick_start.md)
- [移植指南](docs/porting_guide.md)
