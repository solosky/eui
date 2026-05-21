# Amiibo Demo — 240x240 16bpp 示例程序设计

## 概述

为 EUI 框架创建一个 240x240 16bpp 全彩示例程序，展示框架的 view 调度、动画引擎、Canvas 绘图、UTF-8 中文字体支持等核心能力。程序包含三个页面：应用列表、Amiibo 列表、Amiibo 详情，通过场景管理器导航。

## 页面与导航

```
AppList (SCENE_APP_LIST)
  │  [选中 Amiibo → SLIDE_LEFT]
  ▼
AmiiboList (SCENE_AMIIBO_LIST)
  │  [选中某个 Amiibo → SLIDE_LEFT]
  ▼
AmiiboDetail (SCENE_AMIIBO_DETAIL)
  │  [BACK → SLIDE_RIGHT]
  └──────────────────────┘
```

- NFC 和设置按钮进入占位页面（显示功能名 + BACK 返回）
- 所有页面通过 `eui_scene_manager_t` + `eui_view_dispatcher_t` 管理

## 可复用 Carousel 组件

水平滑动选择器，支持弹簧动画，通过 `mc_spring_step` 驱动：

```c
typedef enum { CAROUSEL_HORIZONTAL, CAROUSEL_VERTICAL } carousel_dir_t;

typedef struct {
    const char *label;
    void (*draw_icon)(eui_canvas_t *c, int16_t x, int16_t y, uint16_t size);
    void *user_data;
} item_t;

typedef struct {
    const item_t *items;
    uint8_t count;
    int8_t selected;
    mc_real_t scroll_pos;
    mc_real_t target_pos;
    mc_spring_state_t spring_state;
    mc_spring_params_t spring_params;
    carousel_dir_t dir;
    uint16_t item_step;
    uint8_t icon_size;
} carousel_t;
```

- API: `carousel_init`, `carousel_update(dt_ms)`, `carousel_draw(canvas, area)`, `carousel_input(event)`
- 弹簧参数: stiffness=180, damping=12, mass=1
- 选中项居中高亮（金色边框），实时弹簧动画平滑过渡
- 支持水平/垂直两种方向（通过 `carousel_dir_t` 配置）

## 图标方案

每个 Amiibo 角色使用 Canvas 绘图原语绘制彩色 120x120 图标：

| 角色 | 绘制方式 |
|------|---------|
| 马力欧 | 红帽子 + 肤色脸 + 眼睛 + 胡子 |
| 林克 | 绿衣 + 金发 + 金色三角帽 |
| 塞尔达 | 紫衣 + 金发 |
| 皮卡丘 | 黄身 + 红脸颊 + 黑眼睛 |
| 卡比 | 粉圆 + 眼睛 + 红脸颊 |
| 萨姆斯 | 橙甲 + 面罩 |
| 耀西 | 绿身 + 白眼睛 + 红鞋 |
| 森喜刚 | 棕脸 + 红领带 |

应用图标：Amiibo（红底白字）、NFC（蓝底白字）、设置（灰底齿轮图案）。

## 数据

8 个 Amiibo 角色，每个包含中文名、系列名、4 个游戏掉落物品：

| 名称 | 系列 | 掉落物品 |
|------|------|---------|
| 马力欧 | 超级马力欧系列 | 超级蘑菇, 火焰花, 无敌星, 超级铃铛 |
| 林克 | 塞尔达传说系列 | 大师剑, 海利亚盾, 滑翔伞, 时之笛 |
| 塞尔达 | 塞尔达传说系列 | 光之箭, 时之笛, 三角力量, 塞尔达盾 |
| 皮卡丘 | 宝可梦系列 | 电气球, 雷之石, 十万伏特, 电珠 |
| 卡比 | 星之卡比系列 | 复制能力, 星星杖, 番茄, M番茄 |
| 萨姆斯 | 银河战士系列 | 能量罐, 导弹, 超炸, 加速器 |
| 耀西 | 耀西系列 | 耀西蛋, 水果, 快乐花, 星星 |
| 森喜刚 | 大金刚系列 | 大金刚桶, 香蕉, 矿车, DK徽章 |

## 字体

使用 u8g2 格式的 wqy13 WenQuanYi 中文字体（13px，574 glyphs），来自 u8g2 仓库的 `u8g2_font_wqy13_t_chinese3`。字体数据嵌入为 C 数组。

## UTF-8 支持

在 `eui_canvas.c` 中添加了 UTF-8 解码支持：
- `utf8_decode_next()` — 从字符串中解码下一个 UTF-8 编码点
- `draw_str()` — 自动解码 UTF-8，对 > 0xFF 的编码点使用 u8g2 的 Unicode 查找路径
- `str_width()` — 同样支持 UTF-8 编码点宽度计算
- 完全向后兼容：ASCII 字符串行为不变

## 详情页滚动

详情页通过自定义 `g_detail_scroll` 偏移和 `g_detail_max_scroll` 限制实现上下滚动。UP/DOWN 键以 12px 步进滚动，显示 Amiibo 名、大图标、系列名、游戏掉落列表。

## 输入映射

| 按键 | 功能 |
|------|------|
| LEFT/RIGHT | Carousel 切换选中项 |
| OK | 进入子页面 |
| BACK | 返回上一页面 |
| UP/DOWN | 详情页滚动 |
