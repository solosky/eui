# Desktop Launcher — 400×300 2bpp 主屏幕示例

## 概述

为 EUI 框架创建一个 400×300 2bpp 桌面启动器示例，展示框架在较大屏幕上的 widget、动画、view 调度能力。程序包含主屏幕（状态栏 + 9宫格应用列表）+ 9个应用模拟页面，通过 spring 动画驱动的选中效果和 view 过渡动画导航。

## 屏幕布局

```
400×300, 2bpp (4级灰度: 0=黑, 1=深灰, 2=浅灰, 3=白)

┌──────────────────────────────────────────────────────┐
│ 09:41                              ▂▃▄▆█  [WiFi][电池] │ ← StatusBar (h=24)
├──────────────────────────────────────────────────────┤
│                                                      │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐              │
│  │ [icon]  │  │ [icon]  │  │ [icon]  │              │
│  │  阅读器  │  │ GB模拟器 │  │ NFC模拟器 │              │
│  └─────────┘  └─────────┘  └─────────┘              │
│                                                      │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐              │
│  │ [icon]  │  │ [icon]  │  │ [icon]  │    3×3 Grid  │
│  │ 音乐播放器│  │  计算器  │  │ 文件管理器│              │
│  └─────────┘  └─────────┘  └─────────┘              │
│                                                      │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐              │
│  │ [icon]  │  │ [icon]  │  │ [icon]  │              │
│  │   日历   │  │amiibo模拟│  │   设置   │              │
│  └─────────┘  └─────────┘  └─────────┘              │
│                                                      │
└──────────────────────────────────────────────────────┘
```

### 尺寸计算

| 元素 | 值 |
|---|---|
| Grid 列数 | 3 |
| Grid 行数 | 3 |
| 每格宽度 | 120px |
| 每格高度 | 80px |
| 格间距 | 16px |
| StatusBar 高 | 24px |
| Grid 左边界 | 8px |
| Grid 上边界 | 26px (statusbar 以下) |
| 图标尺寸 | 32×32 XBM |

### 2bpp 调色板使用

| 颜色 | 用途 |
|---|---|
| 0 (黑) | 背景 |
| 1 (深灰) | 状态栏文字、图标非选中、grid 文字 |
| 2 (浅灰) | 图标选中、highlight 边框 |
| 3 (白) | 选中高亮、状态栏重要元素 |

## 导航架构

```
Desktop View (view_id=1)
   │  [选中应用 → OK → EUI_ANIM_SLIDE_LEFT]
   ▼
App Content View (view_id=2, 复用)
   │  [BACK → EUI_ANIM_SLIDE_RIGHT]
   └─────────────────────┘
```

- 仅使用 2 个 view slot，app 内容 view 根据选中应用索引切换绘制内容
- 通过 `eui_view_dispatcher_switch_to()` 切换

## 动画设计

### 选中高亮动画

使用 `mc_spring_step` 驱动 highlight 矩形坐标的弹簧动画：

```c
static mc_real_t g_hl_x, g_hl_y;           // 当前高亮位置
static mc_real_t g_hl_target_x, g_hl_target_y; // 目标位置
static mc_spring_state_t g_hl_spring;
static mc_spring_params_t g_hl_params = {
    .stiffness = MC_FP_C(200),
    .damping = MC_FP_C(15),
    .mass = MC_FP_C(1)
};
```

- 用户移动选中格时，更新 `g_hl_target_*`
- 每帧 `mc_spring_step(&g_hl_x, &g_hl_spring, &g_hl_params, g_hl_target_x, dt)`
- 绘制时在插值位置绘制 `fill_round_rect` 作为高亮

### 进入应用动画

使用框架内置的 `EUI_ANIM_SLIDE_LEFT` / `EUI_ANIM_SLIDE_RIGHT` 过渡。

### 状态栏时间更新

每约 60 ticks 递增一分钟，模拟实时时钟效果。

## 9 个应用及模拟内容

| # | 应用名 | 模拟内容 |
|---|---|---|
| 1 | 阅读器 | 文字段落 + 滚动条 |
| 2 | GB模拟器 | 模拟 GB 游戏画面 (Title + 像素风) |
| 3 | NFC模拟器 | NFC 标签扫描界面 + 卡片数据 |
| 4 | 音乐播放器 | 播放列表 + 当前播放栏 + 进度条 |
| 5 | 计算器 | 数字按钮布局 + 显示结果 |
| 6 | 文件管理器 | 文件/文件夹列表 |
| 7 | 日历 | 月历网格 |
| 8 | amiibo模拟器 | Amiibo 角色列表 |
| 9 | 设置 | WiFi/蓝牙/亮度 开关列表 |

每个应用页面：全屏绘制 + BACK 返回 + 页面标题。

## 构建

### PC (raylib) 构建

```bash
cmake -B build -DEUI_BUILD_CROSS_EXAMPLES=ON -DEUI_COLOR_DEPTH=2 -DEUI_MAX_VIEWS=16 -DEUI_MEM_POOL_SIZE=32768
cmake --build build -j$(nproc)
./build/examples/cross/desktop_launcher/desktop_launcher

使用 `-DEUI_COLOR_DEPTH=2` 启用 2bpp 渲染。因需要 400×300 屏幕，使用独立 main() 直接链接 raylib。
