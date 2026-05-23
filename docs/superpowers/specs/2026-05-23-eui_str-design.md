# eui_str_t 字符串类型设计

## 背景

EUI 目前所有字符串使用原始 `const char*`，没有所有权概念，导致以下问题：

- 调用方的栈缓冲区被释放后，widget 持有野指针（use-after-free）
- `eui_list_add_item` 已因该问题紧急修复，将 `const char*` 改为 `char[32]` 固定缓冲区（commit 8967caf）
- 没有统一的格式化、拼接等字符串操作

需要在 EUI 中引入 `eui_str_t` 类型，替代原始 `char*`。

## 需求摘要

- 始终堆分配（动态长度）
- 带容量追踪，减少重分配
- 核心操作：格式化、拼接、赋值、比较、清空、裁剪、子串
- OOM 时直接 assert（嵌入式场景下不可恢复）
- 零值即空字符串：`{NULL, 0, 0}` 行为等价于 `""`

## 文件规划

| 文件 | 说明 |
|------|------|
| `include/eui/eui_str.h` | 公开头文件 |
| `src/eui_str.c` | 实现 |
| `test/test_str.c` | 测试 |

集成改动：
- `include/eui/eui.h` 添加 `#include "eui/eui_str.h"`
- `src/CMakeLists.txt` 添加 `eui_str.c`
- `test/CMakeLists.txt` 添加 `test_str.c`

## 数据结构

```c
typedef struct {
    char  *data;   /* null 结尾，堆分配 */
    size_t len;    /* 字符串长度，不含 '\0' */
    size_t cap;    /* 已分配容量，含 '\0'，始终 >= len+1 */
} eui_str_t;
```

**扩张策略：** 需要扩容量时，取 `max(cap * 2, len + 1 + 16)`，几何增长减少重分配，同时保证最小步长 16 字节。

## API

### 生命周期

```c
void eui_str_init(eui_str_t *s);                  /* 初始化为空字符串 */
void eui_str_clear(eui_str_t *s);                 /* 释放 data，重置为 {NULL,0,0}，保留 s 本身 */
eui_str_t* eui_str_create(void);                  /* 堆分配并初始化 */
void eui_str_destroy(eui_str_t *s);               /* 释放 data 和 s 本身 */
```

注意：没有 `eui_str_free` 方法（容易歧义：释放 data 还是释放结构体？）。`clear` 释放 data 保留结构体；`destroy` 释放结构体本身。

### 赋值

```c
void eui_str_set(eui_str_t *s, const char *src);         /* 从 C 字符串赋值 */
void eui_str_copy(eui_str_t *dst, const eui_str_t *src); /* 从另一个 eui_str_t 拷贝 */
```

### 拼接

```c
void eui_str_append(eui_str_t *s, const char *src);           /* 追加 C 字符串 */
void eui_str_append_str(eui_str_t *s, const eui_str_t *src);  /* 追加 eui_str_t */
void eui_str_append_char(eui_str_t *s, char c);               /* 追加单个字符 */
```

### 格式化

```c
void eui_str_printf(eui_str_t *s, const char *fmt, ...);           /* 格式化写入，覆盖 */
void eui_str_append_printf(eui_str_t *s, const char *fmt, ...);    /* 格式化追加到末尾 */
```

### 访问

```c
const char* eui_str_cstr(const eui_str_t *s);  /* C 字符串指针，空串返回 "" */
size_t eui_str_len(const eui_str_t *s);         /* 长度（不含 '\0'） */
bool eui_str_empty(const eui_str_t *s);         /* 是否为空 */
```

### 比较

```c
bool eui_str_equals(const eui_str_t *a, const char *b);          /* 与 C 字符串比较 */
bool eui_str_equals_str(const eui_str_t *a, const eui_str_t *b); /* 两个 eui_str_t 比较 */
int  eui_str_cmp(const eui_str_t *a, const eui_str_t *b);        /* 字典序，<0/0/>0 */
```

### 裁剪与子串

```c
void eui_str_trim_left(eui_str_t *s);                                       /* 去头部空白 */
void eui_str_trim_right(eui_str_t *s);                                      /* 去尾部空白 */
void eui_str_trim(eui_str_t *s);                                            /* 去两端空白 */
void eui_str_substr(eui_str_t *dst, const eui_str_t *src, size_t start,
                    size_t len);            /* 取子串，start>=s->len 时 assert，len 超出自动截断 */
```

## 内存管理

- 使用 `eui_malloc` / `eui_free`，不修改 `eui_allocator_t` 接口
- 内部实现的 realloc 逻辑：`malloc(new_size)` + `memcpy` + `free(old_ptr)`
- 所有分配操作以 `assert` 检查 OOM

## 错误处理

分配失败时 `assert`。不返回错误码，不静默失败。

## 集成现有代码

第二阶段（本次设计范围外）：将以下 widget 字段从 `const char*` 迁移到 `eui_str_t`：

| Widget | 字段 | 当前类型 |
|--------|------|----------|
| `eui_label_t` | `text` | `const char*` |
| `eui_button_t` | `label` | `const char*` |
| `eui_menu_item_t` | `label` | `const char*` |
| `eui_list_item_t` | `text` | `char[32]` |
| `eui_dialog_t` | `title`, `message` | `const char*` |

公开 API 签名保持不变（仍接受 `const char*`），内部自动拷贝到 `eui_str_t`。

## 测试

`test/test_str.c`，测试项：

- 空字符串行为（cstr 返回 `""`，len=0，empty=true）
- set / copy 正常拷贝
- append / append_char 正常拼接
- printf / append_printf 格式化正确
- 扩容正确（cap 始终 >= len+1，几何增长）
- trim / substr 正确
- compare 正确
- clear 后数据释放，结构体可复用
- 零初始化 `{0}` 可安全使用
