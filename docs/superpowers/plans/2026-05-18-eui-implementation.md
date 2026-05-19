# EUI Embedded UI Framework Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement a complete embedded UI framework (EUI) targeting MCUs with <32KB RAM, covering memory management, display/input HAL, Canvas drawing, View management, Widget library, MotionC-based animation, and raylib desktop simulation for testing.

**Architecture:** Four sequential phases. Each phase produces independently compilable and testable output. Phase 1 (Foundation) has zero dependencies. Phase 2 builds on Phase 1. Phase 3 builds on Phase 2. Phase 4 is the integration/testing layer.

**Tech Stack:** C99, CMake, TLSF allocator, MotionC (git submodule), raylib (for desktop simulation only), CTest

---

## File Map

```
eui/
├── CMakeLists.txt                    # Top-level: build libeui.a + examples
├── cmake/
│   └── eui_config.cmake             # CMake configuration helpers
├── include/eui/
│   ├── eui.h                         # Umbrella header
│   ├── eui_config.h                  # Compile-time config template
│   ├── eui_types.h                   # Common types: rect, color, align, bitmap, font
│   ├── eui_allocator.h              # External allocator interface
│   ├── eui_display_hal.h            # Display HAL interface + capabilities
│   ├── eui_input_hal.h              # Input HAL interface + event types
│   ├── eui_event.h                   # Event types + ring buffer event queue
│   ├── eui_input.h                   # InputManager: debounce, long-press, focus chain
│   ├── eui_canvas.h                 # Canvas drawing API
│   ├── eui_font.h                    # FontEngine types + API (bdf + VLW)
│   ├── eui_view.h                    # View struct + handler event types
│   ├── eui_view_dispatcher.h        # ViewDispatcher + internal overlay stack
│   ├── eui_scene.h                   # SceneManager
│   ├── eui_anim.h                    # MotionC adapter (EUI anim API)
│   ├── eui_widget.h                  # Widget base (vtable, tree, style flags)
│   ├── eui_widget_label.h
│   ├── eui_widget_button.h
│   ├── eui_widget_list.h
│   ├── eui_widget_menu.h
│   ├── eui_widget_progress.h
│   ├── eui_widget_slider.h
│   ├── eui_widget_scroll.h
│   ├── eui_widget_dialog.h
│   └── eui_hal_raylib.h             # raylib desktop simulation HAL
├── src/
│   ├── CMakeLists.txt
│   ├── eui.c                        # eui_init / eui_deinit / eui_tick
│   ├── eui_allocator.c             # TLSF allocator wrapper
│   ├── eui_canvas.c                 # Canvas implementation
│   ├── eui_font_bdf.c              # BDF font parser
│   ├── eui_font_vlw.c              # VLW font parser
│   ├── eui_event.c                  # Ring buffer event queue
│   ├── eui_input.c                  # InputManager (debounce, focus chain dispatch)
│   ├── eui_view.c                   # View implementation
│   ├── eui_view_dispatcher.c       # ViewDispatcher + overlay stack
│   ├── eui_scene.c                  # SceneManager
│   ├── eui_anim.c                   # MotionC adapter
│   ├── eui_widget.c                 # Widget base (bridge, tree ops, focus chain)
│   ├── eui_widget_label.c
│   ├── eui_widget_button.c
│   ├── eui_widget_list.c
│   ├── eui_widget_menu.c
│   ├── eui_widget_progress.c
│   ├── eui_widget_slider.c
│   ├── eui_widget_scroll.c
│   ├── eui_widget_dialog.c
│   └── eui_hal_raylib.c            # raylib simulation HAL
├── third_party/
│   ├── tlsf/                        # TLSF allocator source
│   └── motionc/                     # MotionC git submodule
├── test/
│   ├── CMakeLists.txt
│   ├── test_allocator.c
│   ├── test_event.c
│   ├── test_canvas.c
│   ├── test_input.c
│   ├── test_view.c
│   ├── test_widget.c
│   └── test_anim.c
├── examples/
│   ├── CMakeLists.txt
│   ├── basic_label.c
│   ├── button_test.c
│   ├── list_nav.c
│   ├── menu_system.c
│   ├── dialog_overlay.c
│   ├── animation_demo.c
│   ├── custom_widget.c
│   ├── page_buffer.c
│   └── benchmark.c
└── docs/
    ├── eui_framework_design.md
    └── superpowers/
        └── plans/
            └── 2026-05-18-eui-implementation.md
```

---

## Phase 1: Foundation

> Zero external dependencies (beyond C99 stdlib). Produces `libeui_core.a` that compiles and links standalone.

### Task 1.1: Create project scaffolding and build system

**Files:**
- Create: `CMakeLists.txt`
- Create: `cmake/eui_config.cmake`
- Create: `include/eui/eui_config.h`

- [ ] **Step 1: Write top-level CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.16)
project(eui VERSION 1.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(EUI_BUILD_TESTS "Build unit tests" ON)
option(EUI_BUILD_EXAMPLES "Build raylib examples" ON)

# Compile-time config
set(EUI_COLOR_DEPTH 1 CACHE STRING "Color depth: 1, 4, 8, 16")
set(EUI_MEM_POOL_SIZE 8192 CACHE STRING "TLSF memory pool size in bytes")
set(EUI_MAX_VIEWS 8 CACHE STRING "Maximum concurrent views")
set(EUI_MAX_ANIMATIONS 8 CACHE STRING "Maximum concurrent animations")
set(EUI_MAX_WIDGETS 32 CACHE STRING "Maximum widget instances")
set(EUI_EVENT_QUEUE_SIZE 8 CACHE STRING "Event queue capacity")
set(EUI_MAX_OVERLAYS 4 CACHE STRING "Overlay stack depth")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/eui/eui_config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/eui/eui_config.h
)

add_subdirectory(third_party/tlsf)
add_subdirectory(third_party/motionc)
add_subdirectory(src)

if(EUI_BUILD_TESTS)
    enable_testing()
    find_package(raylib QUIET)
    add_subdirectory(test)
endif()

if(EUI_BUILD_EXAMPLES)
    find_package(raylib REQUIRED)
    add_subdirectory(examples)
endif()
```

- [ ] **Step 2: Write eui_config.h.in (config template)**

```c
#ifndef EUI_CONFIG_H
#define EUI_CONFIG_H

#define EUI_COLOR_DEPTH       @EUI_COLOR_DEPTH@
#define EUI_MEM_POOL_SIZE     @EUI_MEM_POOL_SIZE@
#define EUI_MAX_VIEWS         @EUI_MAX_VIEWS@
#define EUI_MAX_ANIMATIONS    @EUI_MAX_ANIMATIONS@
#define EUI_MAX_WIDGETS       @EUI_MAX_WIDGETS@
#define EUI_EVENT_QUEUE_SIZE  @EUI_EVENT_QUEUE_SIZE@
#define EUI_MAX_OVERLAYS      @EUI_MAX_OVERLAYS@
#define EUI_MAX_WIDGET_CHILDREN  8
#define EUI_CANVAS_STATE_STACK   4

#endif /* EUI_CONFIG_H */
```

- [ ] **Step 3: Write cmake/eui_config.cmake (helper for consumers)**

```cmake
function(eui_target_configure target)
    target_include_directories(${target} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(${target} PUBLIC eui)
endfunction()
```

- [ ] **Step 4: Verify build system works**

Run: `cmake -B build -DEUI_BUILD_TESTS=OFF -DEUI_BUILD_EXAMPLES=OFF`
Expected: Configure succeeds (even though nothing is built yet)

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt cmake/ include/eui/eui_config.h.in
git commit -m "feat: add project scaffolding and build system"
```

---

### Task 1.2: Add TLSF allocator (third-party)

**Files:**
- Create: `third_party/tlsf/tlsf.h`
- Create: `third_party/tlsf/tlsf.c`
- Create: `third_party/tlsf/CMakeLists.txt`

- [ ] **Step 1: Download and place TLSF source**

Download TLSF from https://github.com/mattconte/tlsf (public domain) and place in `third_party/tlsf/`. The TLSF library consists of exactly two files: `tlsf.h` and `tlsf.c`.

- [ ] **Step 2: Write TLSF CMakeLists.txt**

```cmake
add_library(tlsf STATIC tlsf.c)
target_include_directories(tlsf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
```

- [ ] **Step 3: Verify TLSF compiles**

Run: `cmake -B build && cmake --build build --target tlsf`
Expected: TLSF library builds successfully

- [ ] **Step 4: Commit**

```bash
git add third_party/tlsf/
git commit -m "feat: add TLSF allocator (third-party)"
```

---

### Task 1.3: Add MotionC as git submodule

**Files:**
- Create: `.gitmodules`

- [ ] **Step 1: Add MotionC submodule**

```bash
git submodule add https://github.com/solosky/montionc.git third_party/motionc
```

- [ ] **Step 2: Verify MotionC compiles**

Run: `cmake -B build && cmake --build build --target mc`
Expected: MotionC library builds successfully

- [ ] **Step 3: Commit**

```bash
git add .gitmodules third_party/motionc
git commit -m "feat: add MotionC as git submodule"
```

---

### Task 1.4: Define core types (eui_types.h)

**Files:**
- Create: `include/eui/eui_types.h`

- [ ] **Step 1: Write eui_types.h with all shared types**

```c
#ifndef EUI_TYPES_H
#define EUI_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ---- Rectangle ---- */
typedef struct {
    int16_t x, y;
    uint16_t w, h;
} eui_rect_t;

/* ---- Color abstraction ---- */
#if EUI_COLOR_DEPTH == 1
typedef uint8_t  eui_color_t;
#elif EUI_COLOR_DEPTH == 4
typedef uint8_t  eui_color_t;
#elif EUI_COLOR_DEPTH == 8
typedef uint8_t  eui_color_t;
#elif EUI_COLOR_DEPTH == 16
typedef uint16_t eui_color_t;
#else
#error "EUI_COLOR_DEPTH must be 1, 4, 8, or 16"
#endif

typedef enum {
    EUI_COLOR_BLACK = 0,
    EUI_COLOR_WHITE = 1,
} eui_color_id_t;

eui_color_t eui_color_from_rgb(uint8_t r, uint8_t g, uint8_t b);
eui_color_t eui_color_from_gray(uint8_t gray);

/* ---- Alignment ---- */
typedef enum {
    EUI_ALIGN_LEFT   = 0x01,
    EUI_ALIGN_CENTER = 0x02,
    EUI_ALIGN_RIGHT  = 0x04,
    EUI_ALIGN_TOP    = 0x10,
    EUI_ALIGN_MIDDLE = 0x20,
    EUI_ALIGN_BOTTOM = 0x40,
} eui_align_t;

/* ---- Bitmap (merged icon + bitmap) ---- */
typedef struct {
    uint16_t width;
    uint16_t height;
    uint8_t  color_depth;
    const uint8_t *data;
} eui_bitmap_t;

/* ---- Font ---- */
typedef struct {
    uint8_t  format;        /* 0 = u8g2 bdf, 1 = VLW */
    uint8_t  line_height;
    uint8_t  baseline;
    uint8_t  flags;         /* bit0: 0=variable, 1=fixed width */
    const uint8_t *data;
} eui_font_t;

/* Font flags */
#define EUI_FONT_FIXED_WIDTH  (1u << 0)

/* ---- Buffer mode ---- */
typedef enum {
    EUI_BUFFER_FULL   = (1u << 0),
    EUI_BUFFER_PAGE   = (1u << 1),
    EUI_BUFFER_DIRECT = (1u << 2),
} eui_buffer_mode_t;

/* ---- Animation types (placeholder for now) ---- */
typedef enum {
    EUI_ANIM_NONE = 0,
    EUI_ANIM_SLIDE_LEFT,
    EUI_ANIM_SLIDE_RIGHT,
    EUI_ANIM_FADE,
    EUI_ANIM_SCALE,
    EUI_ANIM_SLIDE_UP,
} eui_anim_type_t;

/* ---- Utility functions ---- */
bool eui_rect_intersect(const eui_rect_t *a, const eui_rect_t *b, eui_rect_t *out);
bool eui_rect_contains(const eui_rect_t *rect, int16_t x, int16_t y);
void eui_rect_union(const eui_rect_t *a, const eui_rect_t *b, eui_rect_t *out);

#endif /* EUI_TYPES_H */
```

- [ ] **Step 2: Write eui_types.c with rect utility implementations**

In `src/eui_types.c`:
- `eui_rect_intersect()`: standard AABB intersection, returns false if no overlap
- `eui_rect_contains()`: point-in-rect check
- `eui_rect_union()`: bounding box of two rects
- `eui_color_from_rgb()`: convert RGB888 to current color depth (threshold for 1bpp, grayscale for 4bpp, etc.)
- `eui_color_from_gray()`: convert 0-255 gray to current depth

- [ ] **Step 3: Write src/CMakeLists.txt (add eui_types.c)**

```cmake
add_library(eui STATIC
    eui_types.c
)
target_include_directories(eui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_BINARY_DIR}/../include
)
target_link_libraries(eui PUBLIC tlsf mc)
```

- [ ] **Step 4: Verify types compile**

Run: `cmake -B build && cmake --build build --target eui`
Expected: Library compiles with 1 source file

- [ ] **Step 5: Commit**

```bash
git add include/eui/eui_types.h src/eui_types.c src/CMakeLists.txt
git commit -m "feat: add core types (rect, color, bitmap, font, alignment)"
```

---

### Task 1.5: Implement allocator interface (eui_allocator.h/.c)

**Files:**
- Create: `include/eui/eui_allocator.h`
- Modify: `src/eui_allocator.c` (new)
- Modify: `src/CMakeLists.txt`

- [ ] **Step 1: Write eui_allocator.h**

```c
#ifndef EUI_ALLOCATOR_H
#define EUI_ALLOCATOR_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    void* (*alloc)(size_t size, void *ctx);
    void  (*free)(void *ptr, void *ctx);
    void* ctx;
} eui_allocator_t;

void eui_set_allocator(const eui_allocator_t *allocator);
void* eui_malloc(size_t size);
void  eui_free(void *ptr);

/* Initialize built-in TLSF allocator with a static buffer */
void eui_allocator_init_tlsf(uint8_t *buffer, size_t size);

/* Get allocator stats */
typedef struct {
    size_t total;
    size_t used;
    size_t peak;
    size_t alloc_count;
    size_t free_count;
} eui_allocator_stats_t;

void eui_allocator_get_stats(eui_allocator_stats_t *stats);

#endif /* EUI_ALLOCATOR_H */
```

- [ ] **Step 2: Write eui_allocator.c**

Implement using the TLSF C API (`tlsf.h`):
- `eui_allocator_init_tlsf()`: call `init_memory_pool(size, buffer)`, then store the pool handle
- `eui_malloc()` / `eui_free()`: delegate to current allocator or fall back to TLSF default
- `eui_set_allocator()`: override the global allocator
- `eui_allocator_get_stats()`: track alloc/free counts and total/used via TLSF's `get_used_size(pool)` and `get_free_size(pool)`

- [ ] **Step 3: Update src/CMakeLists.txt to add eui_allocator.c**

```cmake
add_library(eui STATIC
    eui_types.c
    eui_allocator.c
)
```

- [ ] **Step 4: Verify compilation**

Run: `cmake -B build && cmake --build build --target eui`
Expected: Build succeeds, TLSF allocator linked

- [ ] **Step 5: Write unit test (test/test_allocator.c)**

Create a simple test that:
1. Allocates a static buffer of EUI_MEM_POOL_SIZE
2. Calls `eui_allocator_init_tlsf()`
3. Allocates 100 small blocks, verifies they are non-NULL and non-overlapping
4. Frees them all, verifies no crash
5. Checks stats reflect correct usage

- [ ] **Step 6: Write test/CMakeLists.txt stub**

```cmake
add_executable(test_allocator test_allocator.c)
target_link_libraries(test_allocator PRIVATE eui)
add_test(NAME allocator COMMAND test_allocator)
```

- [ ] **Step 7: Run test and verify**

Run: `cmake -B build && cmake --build build && cd build && ctest -R allocator`
Expected: Test passes

- [ ] **Step 8: Commit**

```bash
git add include/eui/eui_allocator.h src/eui_allocator.c test/CMakeLists.txt test/test_allocator.c
git commit -m "feat: add TLSF-based allocator with external allocator interface"
```

---

### Task 1.6: Write umbrella header and eui_init skeleton

**Files:**
- Create: `include/eui/eui.h`
- Modify: `src/eui.c` (new)
- Modify: `src/CMakeLists.txt`

- [ ] **Step 1: Write eui.h umbrella**

```c
#ifndef EUI_H
#define EUI_H

#include "eui_config.h"
#include "eui_types.h"
#include "eui_allocator.h"
#include "eui_display_hal.h"
#include "eui_input_hal.h"
#include "eui_event.h"
#include "eui_canvas.h"
#include "eui_font.h"
#include "eui_view.h"
#include "eui_view_dispatcher.h"
#include "eui_scene.h"
#include "eui_anim.h"
#include "eui_widget.h"
#include "eui_widget_label.h"
#include "eui_widget_button.h"
#include "eui_widget_list.h"
#include "eui_widget_menu.h"
#include "eui_widget_progress.h"
#include "eui_widget_slider.h"
#include "eui_widget_scroll.h"
#include "eui_widget_dialog.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t  *mem_pool_buffer;
    size_t    mem_pool_size;
    struct eui_display_hal_t *display;
    struct eui_input_hal_t   *input;
    uint16_t  fps_target;
    uint8_t   max_views;
    uint8_t   max_animations;
    uint8_t   max_widgets;
} eui_config_t;

int  eui_init(const eui_config_t *config);
void eui_deinit(void);
void eui_tick(void);
bool eui_is_running(void);
void eui_set_fps(uint16_t fps);
uint16_t eui_get_fps(void);

#ifdef __cplusplus
}
#endif

#endif /* EUI_H */
```

- [ ] **Step 2: Write eui.c skeleton**

```c
#include "eui.h"
#include <string.h>

static struct {
    bool initialized;
    eui_config_t config;
} g_eui;

int eui_init(const eui_config_t *config) {
    if (!config || !config->display || !config->input) return -1;
    if (config->mem_pool_buffer) {
        eui_allocator_init_tlsf(config->mem_pool_buffer, config->mem_pool_size);
    }
    g_eui.config = *config;
    g_eui.initialized = true;
    return 0;
}

void eui_deinit(void) {
    g_eui.initialized = false;
}

void eui_tick(void) {
    if (!g_eui.initialized) return;
    /* Will be filled in later phases */
}

bool eui_is_running(void) {
    return g_eui.initialized;
}

void eui_set_fps(uint16_t fps) {
    g_eui.config.fps_target = fps;
}

uint16_t eui_get_fps(void) {
    return g_eui.config.fps_target;
}
```

- [ ] **Step 3: Update src/CMakeLists.txt**

```cmake
add_library(eui STATIC
    eui_types.c
    eui_allocator.c
    eui.c
)
```

- [ ] **Step 4: Compile and verify**

Run: `cmake -B build && cmake --build build --target eui`
Expected: Compile succeeds (all headers exist even if some .c files are missing)

- [ ] **Step 5: Commit**

```bash
git add include/eui/eui.h src/eui.c
git commit -m "feat: add umbrella header and framework lifecycle (init/deinit/tick)"
```

---

## Phase 2: Hardware Abstraction

### Task 2.1: Define Display HAL interface

**Files:**
- Create: `include/eui/eui_display_hal.h`

- [ ] **Step 1: Write eui_display_hal.h**

```c
#ifndef EUI_DISPLAY_HAL_H
#define EUI_DISPLAY_HAL_H

#include "eui_types.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t width;
    uint16_t height;
    uint8_t  color_depth;
    uint8_t  buffer_mode;
    bool     has_gram;
    bool     hw_scroll;
} eui_display_caps_t;

typedef struct eui_display_hal_t {
    eui_display_caps_t caps;

    int  (*init)(void *user_data);
    int  (*deinit)(void *user_data);
    void (*draw_pixel)(int16_t x, int16_t y, eui_color_t color, void *user_data);
    void (*write_buffer)(const uint8_t *buffer, const eui_rect_t *rect, void *user_data);
    void (*set_contrast)(uint8_t level, void *user_data);
    void (*set_power)(bool on, void *user_data);
    void (*set_invert)(bool invert, void *user_data);
    void (*fill_rect)(int16_t x, int16_t y, uint16_t w, uint16_t h,
                      eui_color_t color, void *user_data);

    void *user_data;
} eui_display_hal_t;

#endif /* EUI_DISPLAY_HAL_H */
```

- [ ] **Step 2: Verify header compiles as part of eui.h include chain**

Run: `cmake -B build && cmake --build build --target eui`
Expected: Compile succeeds

- [ ] **Step 3: Commit**

```bash
git add include/eui/eui_display_hal.h
git commit -m "feat: add Display HAL interface"
```

---

### Task 2.2: Define Input HAL interface and event types

**Files:**
- Create: `include/eui/eui_input_hal.h`
- Create: `include/eui/eui_event.h`

- [ ] **Step 1: Write eui_input_hal.h**

```c
#ifndef EUI_INPUT_HAL_H
#define EUI_INPUT_HAL_H

#include <stdint.h>

typedef enum {
    EUI_EVT_KEY_PRESS,
    EUI_EVT_KEY_RELEASE,
    EUI_EVT_KEY_REPEAT,
    EUI_EVT_ENCODER_CW,
    EUI_EVT_ENCODER_CCW,
    EUI_EVT_ENCODER_CLICK,
    EUI_EVT_TOUCH_DOWN,
    EUI_EVT_TOUCH_UP,
    EUI_EVT_TOUCH_MOVE,
} eui_event_type_t;

typedef enum {
    EUI_KEY_UP = 0,
    EUI_KEY_DOWN,
    EUI_KEY_LEFT,
    EUI_KEY_RIGHT,
    EUI_KEY_OK,
    EUI_KEY_BACK,
    EUI_KEY_COUNT
} eui_key_t;

typedef struct {
    eui_event_type_t type;
    union {
        eui_key_t key;
        int16_t   enc_delta;
        struct { int16_t x, y; } touch;
    } data;
    uint32_t timestamp;
} eui_event_t;

typedef struct eui_input_hal_t {
    int  (*init)(void *user_data);
    int  (*deinit)(void *user_data);
    int  (*poll)(eui_event_t *event, void *user_data);
    void (*set_callback)(void (*cb)(const eui_event_t *evt), void *user_data);
    void *user_data;
} eui_input_hal_t;

#endif /* EUI_INPUT_HAL_H */
```

- [ ] **Step 2: Write eui_event.h (event queue)**

```c
#ifndef EUI_EVENT_H
#define EUI_EVENT_H

#include "eui_input_hal.h"
#include <stdint.h>
#include <stdbool.h>
#include "eui_config.h"

typedef struct {
    eui_event_t buffer[EUI_EVENT_QUEUE_SIZE];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
    void (*overflow_callback)(void);
    bool overwrite;
} eui_event_queue_t;

void eui_event_queue_init(eui_event_queue_t *q);
bool eui_event_queue_push(eui_event_queue_t *q, const eui_event_t *evt);
bool eui_event_queue_pop(eui_event_queue_t *q, eui_event_t *evt);
uint8_t eui_event_queue_count(const eui_event_queue_t *q);
bool eui_event_queue_is_full(const eui_event_queue_t *q);
bool eui_event_queue_is_empty(const eui_event_queue_t *q);
void eui_event_queue_set_overflow_callback(eui_event_queue_t *q, void (*cb)(void));

#endif /* EUI_EVENT_H */
```

- [ ] **Step 3: Verify headers compile**

Run: `cmake -B build && cmake --build build --target eui`
Expected: Compile succeeds

- [ ] **Step 4: Commit**

```bash
git add include/eui/eui_input_hal.h include/eui/eui_event.h
git commit -m "feat: add Input HAL interface and event types"
```

---

### Task 2.3: Implement event queue (ring buffer)

**Files:**
- Create: `src/eui_event.c`
- Modify: `src/CMakeLists.txt`

- [ ] **Step 1: Write eui_event.c**

Implement each function:
- `eui_event_queue_init()`: zero out queue struct
- `eui_event_queue_push()`: add at tail; if full and not overwrite, return false (drop newest); if overwrite mode, advance head (drop oldest)
- `eui_event_queue_pop()`: remove from head
- `eui_event_queue_count()`: return count
- `eui_event_queue_is_full()`: count == EUI_EVENT_QUEUE_SIZE
- `eui_event_queue_is_empty()`: count == 0
- `eui_event_queue_set_overflow_callback()`: set the callback pointer

- [ ] **Step 2: Update src/CMakeLists.txt**

Add `eui_event.c` to the eui library sources.

- [ ] **Step 3: Verify compilation**

Run: `cmake -B build && cmake --build build --target eui`
Expected: Compile succeeds

- [ ] **Step 4: Write unit test (test/test_event.c)**

Test cases:
1. Push 1 event, pop it, verify pop returns true and event matches
2. Push EUI_EVENT_QUEUE_SIZE events, verify queue is full
3. Push one more when full (non-overwrite mode), verify push returns false (dropped)
4. Pop all events, verify queue is empty
5. Overwrite mode: push N+1 events, verify oldest was dropped (newest at head)

- [ ] **Step 5: Add test to test/CMakeLists.txt, run**

Run: `cmake -B build && cmake --build build && cd build && ctest -R event`
Expected: All event queue tests pass

- [ ] **Step 6: Commit**

```bash
git add src/eui_event.c test/test_event.c
git commit -m "feat: implement ring buffer event queue"
```

---

### Task 2.4: Implement InputManager (debounce, long-press, focus chain dispatch)

**Files:**
- Create: `include/eui/eui_input.h`
- Create: `src/eui_input.c`
- Modify: `src/CMakeLists.txt`

- [ ] **Step 1: Write eui_input.h**

```c
#ifndef EUI_INPUT_H
#define EUI_INPUT_H

#include "eui_input_hal.h"
#include "eui_event.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    eui_input_hal_t *hal;
    eui_event_queue_t queue;
    uint32_t last_poll_ms;
    uint32_t debounce_ms;
    uint32_t long_press_ms;
    uint32_t repeat_interval_ms;
    /* Debounce tracking for each key */
    struct {
        uint32_t last_change_ms;
        bool     pressed;
        bool     long_press_fired;
        uint32_t next_repeat_ms;
    } key_state[EUI_KEY_COUNT];
} eui_input_manager_t;

void eui_input_init(eui_input_manager_t *mgr, eui_input_hal_t *hal);
void eui_input_set_debounce(eui_input_manager_t *mgr, uint32_t ms);
void eui_input_set_long_press(eui_input_manager_t *mgr, uint32_t ms);
void eui_input_set_repeat_interval(eui_input_manager_t *mgr, uint32_t ms);
void eui_input_update(eui_input_manager_t *mgr, uint32_t now_ms);
bool eui_input_get_event(eui_input_manager_t *mgr, eui_event_t *event);

#endif /* EUI_INPUT_H */
```

- [ ] **Step 2: Write eui_input.c**

Impl eui `eui_input_update()`:
1. Call `hal->poll()` in a loop until no more raw events, collecting them
2. For each raw key event: debounce (ignore if within debounce_ms of last change), detect long-press (if held > long_press_ms, emit EUI_EVT_KEY_REPEAT), push to event queue
3. For encoder events: push directly (no debounce needed for encoder)
4. `eui_input_get_event()`: pop from queue

Default values: `debounce_ms = 20`, `long_press_ms = 500`, `repeat_interval_ms = 50`

- [ ] **Step 3: Update src/CMakeLists.txt**

Add `eui_input.c` to library sources.

- [ ] **Step 4: Verify compilation**

Run: `cmake -B build && cmake --build build --target eui`
Expected: Compile succeeds

- [ ] **Step 5: Write unit test (test/test_input.c)**

Use a mock input HAL (function pointers set to stubs that store events in a pre-filled array, returned one-by-one on poll). Test:
1. Single key press → one EUI_EVT_KEY_PRESS in queue
2. Key held > long_press_ms → EUI_EVT_KEY_REPEAT events at repeat interval
3. Key release within debounce_ms → no event (bounce suppressed)
4. Multiple encoder events → all queued

- [ ] **Step 6: Add test, run**

Run: `cmake -B build && cmake --build build && cd build && ctest -R input`
Expected: All input tests pass

- [ ] **Step 7: Commit**

```bash
git add include/eui/eui_input.h src/eui_input.c test/test_input.c
git commit -m "feat: implement InputManager with debounce and long-press detection"
```

---

### Task 2.5: Implement Canvas core (drawing primitives)

**Files:**
- Create: `include/eui/eui_canvas.h`
- Create: `src/eui_canvas.c`
- Modify: `src/CMakeLists.txt`

- [ ] **Step 1: Write eui_canvas.h**

```c
#ifndef EUI_CANVAS_H
#define EUI_CANVAS_H

#include "eui_types.h"
#include "eui_display_hal.h"
#include "eui_font.h"
#include <stdint.h>
#include <stdbool.h>

/* Forward declaration */
typedef struct eui_canvas_t eui_canvas_t;

struct eui_canvas_t {
    eui_display_hal_t *display;
    uint8_t           *buffer;        /* Frame buffer or page buffer */
    uint16_t           buf_width;     /* Buffer width (screen width or page band width) */
    uint16_t           buf_height;    /* Buffer height (screen height or page band height) */
    eui_color_t        fg_color;
    eui_color_t        bg_color;
    const eui_font_t  *font;
    eui_rect_t         clip;
    uint8_t            state_stack_idx;
    eui_rect_t         state_stack_clip[EUI_CANVAS_STATE_STACK];
    eui_color_t        state_stack_fg[EUI_CANVAS_STATE_STACK];
    eui_color_t        state_stack_bg[EUI_CANVAS_STATE_STACK];
    /* PAGE mode state */
    uint8_t            page_current;
    uint8_t            page_total;
    uint16_t           page_y_offset;
};

/* Lifecycle */
eui_canvas_t* eui_canvas_create(eui_display_hal_t *display);
void eui_canvas_destroy(eui_canvas_t *canvas);
void eui_canvas_reset(eui_canvas_t *canvas);
void eui_canvas_commit(eui_canvas_t *canvas);

/* Properties */
uint16_t eui_canvas_width(const eui_canvas_t *canvas);
uint16_t eui_canvas_height(const eui_canvas_t *canvas);
void eui_canvas_set_color(eui_canvas_t *canvas, eui_color_t color);
void eui_canvas_set_bg_color(eui_canvas_t *canvas, eui_color_t color);
void eui_canvas_set_clip(eui_canvas_t *canvas, const eui_rect_t *rect);
void eui_canvas_clear_clip(eui_canvas_t *canvas);
void eui_canvas_save(eui_canvas_t *canvas);
void eui_canvas_restore(eui_canvas_t *canvas);

/* Drawing primitives */
void eui_canvas_clear(eui_canvas_t *canvas);
void eui_canvas_draw_dot(eui_canvas_t *canvas, int16_t x, int16_t y);
void eui_canvas_draw_line(eui_canvas_t *canvas, int16_t x1, int16_t y1, int16_t x2, int16_t y2);
void eui_canvas_draw_rect(eui_canvas_t *canvas, int16_t x, int16_t y, uint16_t w, uint16_t h);
void eui_canvas_fill_rect(eui_canvas_t *canvas, int16_t x, int16_t y, uint16_t w, uint16_t h);
void eui_canvas_draw_circle(eui_canvas_t *canvas, int16_t x, int16_t y, uint16_t r);
void eui_canvas_fill_circle(eui_canvas_t *canvas, int16_t x, int16_t y, uint16_t r);
void eui_canvas_draw_triangle(eui_canvas_t *canvas, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3);
void eui_canvas_draw_round_rect(eui_canvas_t *canvas, int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t r);
void eui_canvas_fill_round_rect(eui_canvas_t *canvas, int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t r);

/* Text */
void eui_canvas_set_font(eui_canvas_t *canvas, const eui_font_t *font);
uint16_t eui_canvas_draw_str(eui_canvas_t *canvas, int16_t x, int16_t y, const char *str);
uint16_t eui_canvas_draw_str_aligned(eui_canvas_t *canvas, int16_t x, int16_t y,
                                      eui_align_t h_align, eui_align_t v_align, const char *str);
uint16_t eui_canvas_str_width(const eui_canvas_t *canvas, const char *str);
uint16_t eui_canvas_font_height(const eui_canvas_t *canvas);

/* Images */
void eui_canvas_draw_xbm(eui_canvas_t *canvas, int16_t x, int16_t y,
                          uint16_t w, uint16_t h, const uint8_t *data);
void eui_canvas_draw_bitmap(eui_canvas_t *canvas, int16_t x, int16_t y, const eui_bitmap_t *bmp);

/* Advanced */
void eui_canvas_invert_rect(eui_canvas_t *canvas, int16_t x, int16_t y, uint16_t w, uint16_t h);

/* PAGE mode control (internal) */
bool eui_canvas_begin_page(eui_canvas_t *canvas);
bool eui_canvas_next_page(eui_canvas_t *canvas);

#endif /* EUI_CANVAS_H */
```

- [ ] **Step 2: Write eui_canvas.c — pixel setter helper**

First implement a static helper `canvas_set_pixel(c, x, y, color)` that:
1. Checks clip bounds, returns if outside
2. In FULL/PAGE mode: writes to buffer[y * buf_width + x] with appropriate bit/byte packing for the color depth
3. In DIRECT mode: calls `c->display->draw_pixel(x, y, color, c->display->user_data)`

For 1bpp mode: use bit masking operations (1 byte = 8 pixels). Pixel (x, y) = `buffer[y * (buf_width/8) + x/8]`, bit position = `x % 8`.

- [ ] **Step 3: Implement draw primitives in eui_canvas.c**

Implement in order:
- `clear()`: fill buffer with bg_color
- `draw_dot()`: single pixel via canvas_set_pixel
- `draw_line()`: Bresenham's line algorithm
- `draw_rect()`: 4 line segments
- `fill_rect()`: row-by-row fill
- `draw_circle()`: midpoint circle algorithm (outline)
- `fill_circle()`: midpoint circle algorithm (filled, draw horizontal lines between x pairs)
- `draw_round_rect()`: 4 line segments + 4 corner arcs
- `fill_round_rect()`: filled rect + filled corner arcs
- `draw_triangle()`: 3 line segments
- `draw_xbm()`: iterate rows, for each byte expand bits to pixels
- `draw_bitmap()`: iterate pixel data based on bitmap->color_depth
- `invert_rect()`: XOR each pixel in rect with fg_color

- [ ] **Step 4: Implement Canvas lifecycle and state management**

- `eui_canvas_create()`: allocate canvas struct via `eui_malloc()`, allocate buffer based on display caps
  - FULL mode: buffer = W×H×bpp/8 bytes
  - PAGE mode: buffer = W×8×bpp/8 bytes (8 pixel high band)
  - DIRECT mode: buffer = NULL
- `eui_canvas_destroy()`: free buffer, free canvas
- `eui_canvas_reset()`: reset clip to full screen, reset color to default, reset font, pop all state stack entries
- `eui_canvas_save()`: push current clip + fg + bg to stack (up to EUI_CANVAS_STATE_STACK depth)
- `eui_canvas_restore()`: pop from stack
- `eui_canvas_commit()`: in FULL mode, call display->write_buffer(); in PAGE mode, iterate pages via begin_page/next_page

- [ ] **Step 5: Implement PAGE mode begin/next_page**

- `eui_canvas_begin_page()`: reset to page_current = 0, page_total = ceil(height / 8), set page_y_offset = 0, clear band buffer
- `eui_canvas_next_page()`: flush current band to display via write_buffer, advance page_current, set new page_y_offset, clear band buffer, return true if more pages remain
- PAGE mode modifies canvas_set_pixel to use page_y_offset as the virtual Y offset within the screen

- [ ] **Step 6: Update src/CMakeLists.txt**

Add `eui_canvas.c` to library sources.

- [ ] **Step 7: Verify compilation**

Run: `cmake -B build && cmake --build build --target eui`
Expected: Compile succeeds (font functions are stubs that return 0 for now)

- [ ] **Step 8: Write unit test with mock display (test/test_canvas.c)**

Create a mock display HAL that has:
- A static buffer (128x64 bytes for 1bpp test)
- stub functions (init/deinit/set_contrast/set_power do nothing, draw_pixel writes to static buffer, write_buffer copies to static buffer)

Test cases:
1. Create canvas, clear with white → verify all pixels white (0xFF for 1bpp)
2. Draw a horizontal line → verify only those pixels set
3. Draw rect outline → verify correct border pixels set, interior unchanged
4. Fill rect → verify all interior pixels set
5. Canvas save/restore: set color, save, change color, draw, restore, draw → verify correct colors used
6. Clip test: set clip rect, draw line that extends beyond clip → verify only clipped portion drawn
7. PAGE mode: create PAGE canvas, begin_page/next_page loop → verify each band has correct content

- [ ] **Step 9: Add test, run tests**

Run: `cmake -B build && cmake --build build && cd build && ctest -R canvas`
Expected: All canvas tests pass

- [ ] **Step 10: Commit**

```bash
git add include/eui/eui_canvas.h src/eui_canvas.c test/test_canvas.c
git commit -m "feat: implement Canvas with drawing primitives, clip, and PAGE/FULL buffer modes"
```

---

### Task 2.6: Implement Font Engine (BDF + VLW)

**Files:**
- Create: `include/eui/eui_font.h`
- Create: `src/eui_font_bdf.c`
- Create: `src/eui_font_vlw.c`
- Modify: `src/CMakeLists.txt`

- [ ] **Step 1: Update eui_font.h with font API**

```c
#ifndef EUI_FONT_H
#define EUI_FONT_H

#include "eui_types.h"
#include <stdint.h>

/* Font format identifiers */
#define EUI_FONT_FORMAT_BDF  0
#define EUI_FONT_FORMAT_VLW  1

/* Font engine functions */
uint8_t  eui_font_get_char_width(const eui_font_t *font, char c);
uint16_t eui_font_get_str_width(const eui_font_t *font, const char *str);
uint8_t  eui_font_get_height(const eui_font_t *font);
uint8_t  eui_font_get_baseline(const eui_font_t *font);

/* Draw a single character glyph into a buffer (used by Canvas) */
uint8_t eui_font_draw_char(const eui_font_t *font, char c,
                            uint8_t *buf, uint16_t buf_stride,
                            uint8_t color_depth);

#endif /* EUI_FONT_H */
```

- [ ] **Step 2: Implement BDF font parser (eui_font_bdf.c)**

u8g2 BDF format:
- Header contains: first_char, last_char, line_height
- Glyph table: array of {offset: uint16} indexed by (char - first_char)
- Each glyph: {width: u8, height: u8, x_offset: i8, y_offset: i8, x_advance: u8, bitmap_data: ...}
- Bitmap is packed: ceil(width/8) bytes per row × height rows

Implement `eui_font_get_char_width()`: read char table entry at index (c - first_char), read glyph header, return x_advance.

Implement `eui_font_get_str_width()`: sum x_advance for each char.

Implement `eui_font_draw_char()`: decode glyph bitmap, write to output buffer at appropriate position, respecting color_depth.

- [ ] **Step 3: Implement VLW font parser (eui_font_vlw.c)**

VLW (Very Light Weight) format:
- Header: {magic: u32, line_height: u8, baseline: u8, first_char: u8, last_char: u8, bpp: u8}
- Glyph table: array of {offset: u32, width: u8, height: u8, x_advance: u8, ...}
- Glyph bitmap: packed at VLW's bpp level (1, 2, 4, or 8 bpp)

Similar structure to BDF, different byte layout. Implement same interface functions.

- [ ] **Step 4: Write font dispatch logic**

In a shared helper (eui_font.c or inline):
```c
uint8_t eui_font_get_char_width(const eui_font_t *font, char c) {
    if (font->format == EUI_FONT_FORMAT_BDF)
        return eui_font_bdf_get_char_width(font, c);
    else
        return eui_font_vlw_get_char_width(font, c);
}
```
Similarly for get_str_width, get_height, get_baseline, draw_char.

- [ ] **Step 5: Update src/CMakeLists.txt**

Add `eui_font_bdf.c`, `eui_font_vlw.c` to library sources.

- [ ] **Step 6: Create a minimal embedded test font**

Create a tiny 8x8 fixed-width font in BDF format as a C array in `test/test_font.h`:
- Only ASCII 32-126 (printable)
- Each glyph: 8 pixels wide, 8 pixels tall
- Bitmap data: 8 bytes per glyph (8×8 pixels, 1bpp)
- Store as `const uint8_t test_font_data[]` in Flash-emulating `const` array

- [ ] **Step 7: Write unit test (test/test_font.c)**

Test:
1. `eui_font_get_char_width()` returns 8 for any char (fixed width test font)
2. `eui_font_get_str_width("ABC")` returns 24
3. `eui_font_get_height()` returns 8
4. `eui_font_draw_char('A', buf, ...)` writes correct glyph pattern to buffer (verify known bitmap bytes match)

- [ ] **Step 8: Update Canvas text functions to use FontEngine**

In `eui_canvas.c`, update the text drawing functions:
- `eui_canvas_draw_str()`: iterate chars, call `eui_font_draw_char()`, blit glyph buffer to canvas at (x, y), advance x by char width
- `eui_canvas_str_width()`: call `eui_font_get_str_width(canvas->font, str)`
- `eui_canvas_font_height()`: call `eui_font_get_height(canvas->font)`

- [ ] **Step 9: Run all tests**

Run: `cmake -B build && cmake --build build && cd build && ctest`
Expected: All tests (allocator + event + input + canvas + font) pass

- [ ] **Step 10: Commit**

```bash
git add include/eui/eui_font.h src/eui_font_bdf.c src/eui_font_vlw.c test/test_font.h test/test_font.c
git commit -m "feat: implement FontEngine with BDF and VLW format support"
```

---

## Phase 3: UI Layer

### Task 3.1: Implement View system

**Files:**
- Create: `include/eui/eui_view.h`
- Create: `src/eui_view.c`
- Modify: `src/CMakeLists.txt`

- [ ] **Step 1: Write eui_view.h**

```c
#ifndef EUI_VIEW_H
#define EUI_VIEW_H

#include "eui_types.h"
#include "eui_canvas.h"
#include "eui_input_hal.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    EUI_VIEW_EVT_DRAW     = 0,
    EUI_VIEW_EVT_INPUT    = 1,
    EUI_VIEW_EVT_ENTER    = 2,
    EUI_VIEW_EVT_EXIT     = 3,
    EUI_VIEW_EVT_NAVIGATE = 4,
    EUI_VIEW_EVT_CUSTOM   = 0x1000,
} eui_view_event_type_t;

typedef struct {
    eui_view_event_type_t type;
    union {
        struct { eui_canvas_t *canvas; void *model; } draw;
        struct { const eui_event_t *input; } input;
        struct { uint32_t nav_id; } navigate;
        struct { uint32_t id; void *data; } custom;
    } event;
} eui_view_event_t;

typedef bool (*eui_view_handler_t)(eui_view_event_t *event, void *context);

typedef struct eui_view {
    eui_rect_t          area;
    eui_view_handler_t  handler;
    void               *context;
    void               *model;
    uint32_t            flags;
} eui_view_t;

/* View flags */
#define EUI_VIEW_FLAG_VISIBLE  (1u << 0)
#define EUI_VIEW_FLAG_DIRTY    (1u << 1)
#define EUI_VIEW_FLAG_ANIMATING (1u << 2)

/* Lifecycle helpers */
void eui_view_init(eui_view_t *view, eui_view_handler_t handler, void *context);
void eui_view_set_model(eui_view_t *view, void *model);
void eui_view_mark_dirty(eui_view_t *view);

/* Dispatch events */
bool eui_view_send_draw(eui_view_t *view, eui_canvas_t *canvas);
bool eui_view_send_input(eui_view_t *view, const eui_event_t *evt);
bool eui_view_send_enter(eui_view_t *view);
bool eui_view_send_exit(eui_view_t *view);
bool eui_view_send_navigate(eui_view_t *view, uint32_t nav_id);

#endif /* EUI_VIEW_H */
```

- [ ] **Step 2: Write eui_view.c**

Implement each `eui_view_send_*` function:
```c
bool eui_view_send_draw(eui_view_t *view, eui_canvas_t *canvas) {
    eui_view_event_t event = { .type = EUI_VIEW_EVT_DRAW };
    event.event.draw.canvas = canvas;
    event.event.draw.model = view->model;
    return view->handler(&event, view->context);
}
```
Similarly for input, enter, exit, navigate. Each constructs the appropriate event and calls handler.

`eui_view_init()`: set handler, context, default area to full screen placeholder.

- [ ] **Step 3: Update src/CMakeLists.txt**

Add `eui_view.c`.

- [ ] **Step 4: Write unit test (test/test_view.c)**

Test:
1. Create a view with a handler that counts draw/enter/exit calls, verify correct sequence on send_enter → send_draw → send_exit
2. Model binding: set model pointer, verify it's passed in draw event
3. Flags: mark dirty, verify flag set

- [ ] **Step 5: Add test, run**

Run: `cmake -B build && cmake --build build && cd build && ctest -R view`
Expected: View tests pass

- [ ] **Step 6: Commit**

```bash
git add include/eui/eui_view.h src/eui_view.c test/test_view.c
git commit -m "feat: implement View system with single event handler"
```

---

### Task 3.2: Implement ViewDispatcher with internal overlay stack

**Files:**
- Create: `include/eui/eui_view_dispatcher.h`
- Create: `src/eui_view_dispatcher.c`
- Modify: `src/CMakeLists.txt`

- [ ] **Step 1: Write eui_view_dispatcher.h**

```c
#ifndef EUI_VIEW_DISPATCHER_H
#define EUI_VIEW_DISPATCHER_H

#include "eui_view.h"
#include "eui_types.h"
#include "eui_config.h"
#include <stdint.h>

typedef struct eui_view_dispatcher_t {
    /* Registered views */
    struct {
        uint32_t    id;
        eui_view_t *view;
    } views[EUI_MAX_VIEWS];
    uint8_t view_count;

    /* Current bottom-layer view index */
    uint8_t current_view_idx;

    /* Overlay stack */
    eui_view_t *overlays[EUI_MAX_OVERLAYS];
    uint8_t overlay_count;

    /* State */
    bool running;
    eui_canvas_t *canvas;
} eui_view_dispatcher_t;

void eui_view_dispatcher_init(eui_view_dispatcher_t *vd, eui_canvas_t *canvas);
int  eui_view_dispatcher_add(eui_view_dispatcher_t *vd, uint32_t view_id, eui_view_t *view);
void eui_view_dispatcher_switch_to(eui_view_dispatcher_t *vd, uint32_t view_id, eui_anim_type_t anim);
int  eui_view_dispatcher_push_overlay(eui_view_dispatcher_t *vd, eui_view_t *overlay, eui_anim_type_t anim);
void eui_view_dispatcher_pop_overlay(eui_view_dispatcher_t *vd, eui_anim_type_t anim);
eui_view_t* eui_view_dispatcher_get_active(eui_view_dispatcher_t *vd);
void eui_view_dispatcher_tick(eui_view_dispatcher_t *vd);
void eui_view_dispatcher_send_input(eui_view_dispatcher_t *vd, const eui_event_t *evt);
void eui_view_dispatcher_send_custom(eui_view_dispatcher_t *vd, uint32_t event_id);

#endif /* EUI_VIEW_DISPATCHER_H */
```

- [ ] **Step 2: Write eui_view_dispatcher.c**

Key logic:
- `get_active()`: if overlay_count > 0, return overlays[overlay_count-1]; else return views[current_view_idx].view (if any)
- `switch_to()`: if overlay_count == 0: send exit to current, set new current_view_idx, send enter to new, mark dirty. If overlay_count > 0: just update current_view_idx (will take effect when overlays are all popped)
- `push_overlay()`: send exit to current active view, push to overlays[], send enter to new overlay
- `pop_overlay()`: send exit to top overlay, pop, send enter to newly exposed view
- `tick()`: get active view, send EUI_VIEW_EVT_DRAW, then commit canvas
- `send_input()`: get active, send EUI_VIEW_EVT_INPUT, if returns false, try dispatch to lower overlay/base view as fallback

- [ ] **Step 3: Update src/CMakeLists.txt**

Add `eui_view_dispatcher.c`.

- [ ] **Step 4: Write unit test (test/test_view.c, add test cases)**

Add to test_view.c:
1. Register 2 views, switch_to view 0, verify active view is view 0
2. switch_to view 1, verify enter called on view 1, exit on view 0
3. push_overlay, verify overlay is active, base view received exit
4. pop_overlay, verify base view is active again, received enter
5. push_overlay while already on overlay → second overlay is active

- [ ] **Step 5: Run tests**

Run: `cmake -B build && cmake --build build && cd build && ctest -R view`
Expected: All view tests pass

- [ ] **Step 6: Commit**

```bash
git add include/eui/eui_view_dispatcher.h src/eui_view_dispatcher.c
git commit -m "feat: implement ViewDispatcher with internal overlay stack"
```

---

### Task 3.3: Implement Widget base class (bridge + tree + focus chain)

**Files:**
- Create: `include/eui/eui_widget.h`
- Create: `src/eui_widget.c`
- Modify: `src/CMakeLists.txt`

- [ ] **Step 1: Write eui_widget.h**

```c
#ifndef EUI_WIDGET_H
#define EUI_WIDGET_H

#include "eui_view.h"
#include "eui_canvas.h"
#include "eui_config.h"
#include <stdint.h>
#include <stdbool.h>

/* Focus policy */
#define EUI_FOCUS_NONE    0
#define EUI_FOCUS_TAB     1
#define EUI_FOCUS_STRONG  2

/* Style flags */
#define EUI_STYLE_VISIBLE   (1u << 0)
#define EUI_STYLE_ENABLED   (1u << 1)
#define EUI_STYLE_FOCUSED   (1u << 2)
#define EUI_STYLE_SELECTED  (1u << 3)
#define EUI_STYLE_PRESSED   (1u << 4)
#define EUI_STYLE_CHECKED   (1u << 5)
#define EUI_STYLE_DIRTY     (1u << 6)

/* Forward declarations */
typedef struct eui_widget_t eui_widget_t;
typedef struct eui_widget_vtable eui_widget_vtable_t;

struct eui_widget_vtable {
    void (*draw)(eui_widget_t *self, eui_canvas_t *canvas);
    bool (*input)(eui_widget_t *self, const eui_event_t *evt);
    void (*enter)(eui_widget_t *self);
    void (*exit)(eui_widget_t *self);
    void (*layout)(eui_widget_t *self);
    void (*destroy)(eui_widget_t *self);
};

struct eui_widget_t {
    eui_view_t view;                         /* Must be first member (container_of) */
    eui_rect_t area;
    uint32_t   style;
    uint8_t    focus_policy;
    const eui_widget_vtable_t *vt;
    eui_widget_t *parent;
    eui_widget_t *children[EUI_MAX_WIDGET_CHILDREN];
    uint8_t    child_count;
    uint8_t    focus_index;
};

/* Widget lifecycle */
void eui_widget_init(eui_widget_t *w, const eui_widget_vtable_t *vt,
                     int16_t x, int16_t y, uint16_t w, uint16_t h);
void eui_widget_deinit(eui_widget_t *w);

/* Widget tree */
void eui_widget_add_child(eui_widget_t *parent, eui_widget_t *child);
void eui_widget_remove_child(eui_widget_t *parent, eui_widget_t *child);

/* Focus chain */
eui_widget_t* eui_widget_get_focus(const eui_widget_t *root);
eui_widget_t* eui_widget_focus_next(eui_widget_t *root);
eui_widget_t* eui_widget_focus_prev(eui_widget_t *root);
void eui_widget_set_focus(eui_widget_t *w);

/* Style helpers */
static inline bool eui_widget_is_visible(const eui_widget_t *w) { return w->style & EUI_STYLE_VISIBLE; }
static inline bool eui_widget_is_enabled(const eui_widget_t *w) { return w->style & EUI_STYLE_ENABLED; }
static inline void eui_widget_set_visible(eui_widget_t *w, bool v) { if (v) w->style |= EUI_STYLE_VISIBLE; else w->style &= ~EUI_STYLE_VISIBLE; }
static inline void eui_widget_set_enabled(eui_widget_t *w, bool e) { if (e) w->style |= EUI_STYLE_ENABLED; else w->style &= ~EUI_STYLE_ENABLED; }

/* Convert view pointer to widget pointer */
#define eui_widget_from_view(v) ((eui_widget_t*)(v))

#endif /* EUI_WIDGET_H */
```

- [ ] **Step 2: Write eui_widget.c**

Implement:
- `eui_widget_init()`: set area, vt, style = VISIBLE|ENABLED, focus_policy = NONE, parent = NULL, set view.handler to `eui_widget_event_bridge()` (internal static function)
- `eui_widget_event_bridge()`: receives eui_view_event_t*, uses container_of to get widget pointer, dispatches by event type: DRAW→vt->draw, INPUT→vt->input, ENTER→vt->enter, EXIT→vt->exit
- `eui_widget_add_child()`: append to children[], set child->parent = this, if child has STRONG focus and no current focus, set focus_index
- `eui_widget_remove_child()`: find and remove from children[], shift remaining, update focus_index if needed
- `eui_widget_get_focus()`: recursively find the widget with EUI_STYLE_FOCUSED flag set, using pre-order traversal
- `eui_widget_focus_next()`: find current focus, pre-order traversal to find the next STRONG-focus widget, clear old focus style, set new
- `eui_widget_focus_prev()`: same but reverse direction
- `eui_widget_set_focus()`: clear all focused flags in tree, set this widget's FOCUSED style

- [ ] **Step 3: Update src/CMakeLists.txt**

Add `eui_widget.c`.

- [ ] **Step 4: Write unit test (test/test_widget.c)**

Test:
1. Create widget A (STRONG focus), B (NONE focus) → get_focus returns A
2. Create A (STRONG), B (STRONG) → get_focus returns A (first created); focus_next returns B; focus_prev from B returns A
3. Add child widget to parent → child.parent == parent, parent.children[0] == child
4. Widget init with vtable → view.handler is set (non-NULL)
5. Style helpers: set_visible(false), is_visible() returns false

- [ ] **Step 5: Add test, run**

Run: `cmake -B build && cmake --build build && cd build && ctest -R widget`
Expected: All widget tests pass

- [ ] **Step 6: Commit**

```bash
git add include/eui/eui_widget.h src/eui_widget.c test/test_widget.c
git commit -m "feat: implement Widget base class with vtable bridge, tree, and focus chain"
```

---

### Task 3.4: Implement Label widget

**Files:**
- Create: `include/eui/eui_widget_label.h`
- Create: `src/eui_widget_label.c`
- Modify: `src/CMakeLists.txt`

- [ ] **Step 1: Write eui_widget_label.h**

```c
#ifndef EUI_WIDGET_LABEL_H
#define EUI_WIDGET_LABEL_H

#include "eui_widget.h"

typedef struct {
    eui_widget_t widget;
    const char  *text;
    eui_align_t  h_align;
    eui_align_t  v_align;
} eui_label_t;

eui_widget_t* eui_label_create(const char *text, int16_t x, int16_t y);
void eui_label_set_text(eui_widget_t *label, const char *text);
void eui_label_set_align(eui_widget_t *label, eui_align_t h, eui_align_t v);

#endif /* EUI_WIDGET_LABEL_H */
```

- [ ] **Step 2: Write eui_widget_label.c**

Implement vtable methods:
- `draw()`: call `eui_canvas_draw_str_aligned(canvas, area.x + area.w/2, area.y + area.h/2, h_align, v_align, text)`
- `input()`: return false (label doesn't handle input)
- `enter()` / `exit()`: no-op
- `layout()`: no-op (label has fixed position)
- `destroy()`: eui_free(self)

Create function:
- `eui_label_create()`: eui_malloc(sizeof(eui_label_t)), init widget with vtable and area, set text and default alignment (LEFT|MIDDLE)
- `eui_label_set_text()`: update text pointer, set DIRTY flag
- `eui_label_set_align()`: update h_align/v_align, set DIRTY flag

- [ ] **Step 3: Update src/CMakeLists.txt**

Add `eui_widget_label.c`.

- [ ] **Step 4: Commit**

```bash
git add include/eui/eui_widget_label.h src/eui_widget_label.c
git commit -m "feat: implement Label widget"
```

---

### Task 3.5: Implement Button widget

**Files:**
- Create: `include/eui/eui_widget_button.h`
- Create: `src/eui_widget_button.c`
- Modify: `src/CMakeLists.txt`

- [ ] **Step 1: Write eui_widget_button.h**

```c
#ifndef EUI_WIDGET_BUTTON_H
#define EUI_WIDGET_BUTTON_H

#include "eui_widget.h"
#include "eui_types.h"

typedef void (*eui_button_callback_t)(void *ctx);

typedef struct {
    eui_widget_t widget;
    const char  *label;
    const eui_bitmap_t *icon;
    eui_button_callback_t callback;
    void *callback_ctx;
} eui_button_t;

eui_widget_t* eui_button_create(const char *label, int16_t x, int16_t y,
                                 uint16_t w, uint16_t h);
void eui_button_set_callback(eui_widget_t *btn, eui_button_callback_t cb, void *ctx);
void eui_button_set_bitmap(eui_widget_t *btn, const eui_bitmap_t *bmp);

#endif /* EUI_WIDGET_BUTTON_H */
```

- [ ] **Step 2: Write eui_widget_button.c**

Implement vtable:
- `draw()`: draw filled rect (or round rect with r=3) with different style based on state:
  - FOCUSED: draw a dashed/dotted border around button
  - PRESSED: draw with slightly offset text (x+1, y+1) for pushed-in effect
  - Normal: draw centered text label
  - If icon is set, draw icon left of text
- `input()`: on EUI_KEY_OK press → set PRESSED style; on release → call callback if set, return true
- `enter()`: set FOCUSED style
- `exit()`: clear FOCUSED and PRESSED styles

Create function: eui_malloc, init widget with STRONG focus policy, store label.

- [ ] **Step 3: update src/CMakeLists.txt**

Add `eui_widget_button.c`.

- [ ] **Step 4: Commit**

```bash
git add include/eui/eui_widget_button.h src/eui_widget_button.c
git commit -m "feat: implement Button widget with focus/press states"
```

---

### Task 3.6: Implement List widget

**Files:**
- Create: `include/eui/eui_widget_list.h`
- Create: `src/eui_widget_list.c`
- Modify: `src/CMakeLists.txt`

- [ ] **Step 1: Write eui_widget_list.h**

```c
#ifndef EUI_WIDGET_LIST_H
#define EUI_WIDGET_LIST_H

#include "eui_widget.h"

typedef void (*eui_list_callback_t)(uint8_t index, void *ctx);

typedef struct {
    const char         *text;
    const eui_bitmap_t *icon;
} eui_list_item_t;

#define EUI_LIST_MAX_ITEMS 32

typedef struct {
    eui_widget_t   widget;
    eui_list_item_t items[EUI_LIST_MAX_ITEMS];
    uint8_t        item_count;
    uint8_t        selected_index;
    uint8_t        scroll_offset;
    uint16_t       item_height;
    eui_list_callback_t callback;
    void          *callback_ctx;
} eui_list_t;

eui_widget_t* eui_list_create(int16_t x, int16_t y, uint16_t w, uint16_t h);
int  eui_list_add_item(eui_widget_t *list, const char *text, const eui_bitmap_t *icon);
void eui_list_set_selected(eui_widget_t *list, uint8_t index);
uint8_t eui_list_get_selected(const eui_widget_t *list);
void eui_list_set_callback(eui_widget_t *list, eui_list_callback_t cb, void *ctx);
void eui_list_clear(eui_widget_t *list);

#endif /* EUI_WIDGET_LIST_H */
```

- [ ] **Step 2: Write eui_widget_list.c**

Implement vtable:
- `draw()`: clip to area. For each visible item (from scroll_offset to min(item_count, scroll_offset + visible_slots)):
  - Calculate y position
  - If index == selected_index: draw highlighted background (invert rect)
  - Draw item text (left-aligned)
  - Draw item icon if present (to the left of text)
  - Draw scroll indicator if items exceed visible slots
- `input()`:
  - EUI_KEY_DOWN / encoder CW: selected_index++, scroll if needed, return true
  - EUI_KEY_UP / encoder CCW: selected_index--, scroll if needed, return true
  - EUI_KEY_OK: fire callback if set, return true
  - Otherwise: return false (doesn't handle)

- [ ] **Step 3: Update src/CMakeLists.txt**

Add `eui_widget_list.c`.

- [ ] **Step 4: Commit**

```bash
git add include/eui/eui_widget_list.h src/eui_widget_list.c
git commit -m "feat: implement List widget with scrollable item selection"
```

---

### Task 3.7: Implement Menu widget

**Files:**
- Create: `include/eui/eui_widget_menu.h`
- Create: `src/eui_widget_menu.c`
- Modify: `src/CMakeLists.txt`

- [ ] **Step 1: Write eui_widget_menu.h**

```c
#ifndef EUI_WIDGET_MENU_H
#define EUI_WIDGET_MENU_H

#include "eui_widget.h"

typedef void (*eui_menu_callback_t)(void *ctx);

typedef struct eui_menu_item_t {
    const char *label;
    eui_menu_callback_t callback;
    void *callback_ctx;
    struct eui_menu_t *submenu;  /* if non-NULL, this item opens a submenu */
} eui_menu_item_t;

#define EUI_MENU_MAX_ITEMS 32
#define EUI_MENU_MAX_BREADCRUMB 8

typedef struct eui_menu_t {
    eui_widget_t   widget;
    eui_menu_item_t items[EUI_MENU_MAX_ITEMS];
    uint8_t        item_count;
    uint8_t        selected_index;
    uint8_t        scroll_offset;
    /* Breadcrumb for submenu navigation */
    struct {
        uint8_t selected;
        uint8_t scroll;
        eui_menu_item_t *items;
        uint8_t count;
    } breadcrumb[EUI_MENU_MAX_BREADCRUMB];
    uint8_t        breadcrumb_depth;
    /* Current submenu (NULL = top level) */
    struct eui_menu_t *current_submenu;
} eui_menu_t;

eui_widget_t* eui_menu_create(int16_t x, int16_t y, uint16_t w, uint16_t h);
eui_menu_item_t* eui_menu_add_item(eui_widget_t *menu, const char *label, eui_menu_callback_t cb);
eui_menu_item_t* eui_menu_add_submenu(eui_widget_t *menu, const char *label);
void eui_menu_enter_submenu(eui_widget_t *menu, eui_menu_item_t *submenu);
void eui_menu_back(eui_widget_t *menu);

#endif /* EUI_WIDGET_MENU_H */
```

- [ ] **Step 2: Write eui_widget_menu.c**

Menu is built on top of List-like rendering but adds:
- Breadcrumb stack for submenu navigation
- `eui_menu_add_item()`: append to items[], set callback, return &items[item_count-1]
- `eui_menu_add_submenu()`: same as add_item but marks the item as submenu holder (creates a child eui_menu_t internally)
- `eui_menu_enter_submenu()`: push current state to breadcrumb stack, switch to submenu's items
- `eui_menu_back()`: pop breadcrumb stack, restore previous menu state
- `draw()`: render current items (top level or submenu), draw breadcrumb path at top if in submenu
- `input()`: handle UP/DOWN for navigation, OK to trigger callback or enter submenu, BACK to go back in breadcrumb

- [ ] **Step 3: Update src/CMakeLists.txt**

Add `eui_widget_menu.c`.

- [ ] **Step 4: Commit**

```bash
git add include/eui/eui_widget_menu.h src/eui_widget_menu.c
git commit -m "feat: implement Menu widget with submenu breadcrumb navigation"
```

---

### Task 3.8: Implement remaining widgets (Progress, Slider, ScrollContainer, Dialog)

**Files:**
- Create: `include/eui/eui_widget_progress.h` + `src/eui_widget_progress.c`
- Create: `include/eui/eui_widget_slider.h` + `src/eui_widget_slider.c`
- Create: `include/eui/eui_widget_scroll.h` + `src/eui_widget_scroll.c`
- Create: `include/eui/eui_widget_dialog.h` + `src/eui_widget_dialog.c`
- Modify: `src/CMakeLists.txt`

- [ ] **Step 1: Implement Progress widget**

```c
// eui_widget_progress.h
typedef struct {
    eui_widget_t widget;
    uint8_t value;       /* 0-100 */
    bool indeterminate;
} eui_progress_t;

eui_widget_t* eui_progress_create(int16_t x, int16_t y, uint16_t w, uint16_t h);
void eui_progress_set_value(eui_widget_t *prog, uint8_t percent);
void eui_progress_set_indeterminate(eui_widget_t *prog, bool indet);
```

Draw: if indeterminate, draw animated bar that sweeps back and forth. If determinate, draw filled rect for value% of width, outline for remaining.

- [ ] **Step 2: Implement Slider widget**

```c
// eui_widget_slider.h
typedef struct {
    eui_widget_t widget;
    int16_t value;
    int16_t min;
    int16_t max;
} eui_slider_t;

eui_widget_t* eui_slider_create(int16_t x, int16_t y, uint16_t w, uint16_t h);
void eui_slider_set_range(eui_widget_t *slider, int16_t min, int16_t max);
void eui_slider_set_value(eui_widget_t *slider, int16_t value);
int16_t eui_slider_get_value(const eui_widget_t *slider);
```

Draw: horizontal bar with a knob at value position. Input: LEFT/RIGHT to adjust value, OK to confirm (fire callback).

- [ ] **Step 3: Implement ScrollContainer widget**

```c
// eui_widget_scroll.h
typedef struct {
    eui_widget_t widget;
    uint16_t content_width;
    uint16_t content_height;
    int16_t  scroll_x;
    int16_t  scroll_y;
    uint8_t  scrollbar_size;  /* 0 = hidden */
} eui_scroll_t;

eui_widget_t* eui_scroll_create(int16_t x, int16_t y, uint16_t w, uint16_t h);
void eui_scroll_set_content_size(eui_widget_t *scroll, uint16_t cw, uint16_t ch);
void eui_scroll_add_child(eui_widget_t *scroll, eui_widget_t *child);
```

Draw: clip to area, offset children by (-scroll_x, -scroll_y), draw children. Draw scrollbars if needed. Input: if focus child doesn't handle directional input, handle scrolling. ENCODER events: vertical scroll.

- [ ] **Step 4: Implement Dialog widget**

```c
// eui_widget_dialog.h
typedef enum {
    EUI_DIALOG_OK,
    EUI_DIALOG_CANCEL,
    EUI_DIALOG_YES,
    EUI_DIALOG_NO,
} eui_dialog_result_t;

typedef void (*eui_dialog_callback_t)(eui_dialog_result_t result, void *ctx);

#define EUI_DIALOG_MAX_BUTTONS 3

typedef struct {
    eui_widget_t widget;
    const char  *title;
    const char  *message;
    struct {
        const char *label;
        eui_dialog_result_t result;
    } buttons[EUI_DIALOG_MAX_BUTTONS];
    uint8_t button_count;
    uint8_t focused_button;
    eui_dialog_callback_t callback;
    void *callback_ctx;
    eui_view_dispatcher_t *vd;  /* reference to dispatcher for self-close */
} eui_dialog_t;

eui_widget_t* eui_dialog_create(const char *title, const char *msg);
void eui_dialog_add_button(eui_widget_t *dlg, const char *label, eui_dialog_result_t result);
void eui_dialog_show(eui_widget_t *dlg, eui_view_dispatcher_t *vd, eui_dialog_callback_t cb);
```

Draw: semi-transparent overlay background (checkerboard pattern in 1bpp), centered rounded rect with title, message, and buttons. Input: LEFT/RIGHT to switch focused button, OK to select → call callback → auto pop overlay via vd->pop_overlay.

The `eui_dialog_show()` pushes itself as an overlay on the ViewDispatcher.

- [ ] **Step 5: Update src/CMakeLists.txt**

Add all four widget source files.

- [ ] **Step 6: Commit**

```bash
git add include/eui/eui_widget_progress.h src/eui_widget_progress.c \
        include/eui/eui_widget_slider.h src/eui_widget_slider.c \
        include/eui/eui_widget_scroll.h src/eui_widget_scroll.c \
        include/eui/eui_widget_dialog.h src/eui_widget_dialog.c
git commit -m "feat: implement Progress, Slider, ScrollContainer, and Dialog widgets"
```

---

### Task 3.9: Implement SceneManager

**Files:**
- Create: `include/eui/eui_scene.h`
- Create: `src/eui_scene.c`
- Modify: `src/CMakeLists.txt`

- [ ] **Step 1: Write eui_scene.h**

```c
#ifndef EUI_SCENE_H
#define EUI_SCENE_H

#include "eui_view.h"
#include <stdint.h>

typedef void (*eui_scene_on_enter_t)(void *context);
typedef void (*eui_scene_on_exit_t)(void *context);
typedef bool (*eui_scene_on_event_t)(void *context, uint32_t event_id);

typedef struct {
    uint32_t scene_id;
    eui_view_t *view;
    eui_scene_on_enter_t on_enter;
    eui_scene_on_exit_t  on_exit;
    eui_scene_on_event_t on_event;
} eui_scene_t;

#define EUI_SCENE_MAX 16

typedef struct {
    eui_scene_t scenes[EUI_SCENE_MAX];
    uint8_t count;
    uint8_t current;
} eui_scene_manager_t;

int  eui_scene_manager_register(eui_scene_manager_t *sm, const eui_scene_t *scenes, uint8_t count);
void eui_scene_manager_switch(eui_scene_manager_t *sm, uint32_t scene_id);
void eui_scene_manager_back(eui_scene_manager_t *sm);
void eui_scene_manager_send_event(eui_scene_manager_t *sm, uint32_t event_id);

#endif /* EUI_SCENE_H */
```

- [ ] **Step 2: Write eui_scene.c**

- `register()`: copy scenes array to internal array, up to EUI_SCENE_MAX
- `switch()`: find scene by ID → send EUI_VIEW_EVT_ENTER to new scene's view → call scene's on_enter
- `back()`: send on_exit for current → switch to previous (tracked via a simple stack or just keep last scene id)
- Scene callbacks wrap around View events: enter fires after view enter, exit fires before view exit

- [ ] **Step 3: Update src/CMakeLists.txt**

Add `eui_scene.c`.

- [ ] **Step 4: Commit**

```bash
git add include/eui/eui_scene.h src/eui_scene.c
git commit -m "feat: implement SceneManager for declarative scene navigation"
```

---

### Task 3.10: Implement MotionC adapter (eui_anim.h/.c)

**Files:**
- Create: `include/eui/eui_anim.h`
- Create: `src/eui_anim.c`
- Modify: `src/CMakeLists.txt`

- [ ] **Step 1: Write eui_anim.h**

```c
#ifndef EUI_ANIM_H
#define EUI_ANIM_H

#include "eui_widget.h"
#include "mc.h"

typedef uint8_t eui_anim_handle_t;

typedef enum {
    EUI_ANIM_TARGET_X,
    EUI_ANIM_TARGET_Y,
    EUI_ANIM_TARGET_WIDTH,
    EUI_ANIM_TARGET_HEIGHT,
    EUI_ANIM_TARGET_OPACITY,
    EUI_ANIM_TARGET_PROGRESS,
    EUI_ANIM_TARGET_CUSTOM,
} eui_anim_target_t;

void eui_anim_init(void);

eui_anim_handle_t eui_anim_start(eui_widget_t *target,
                                  eui_anim_target_t prop,
                                  int16_t from, int16_t to,
                                  uint16_t duration_ms,
                                  mc_easing_fn_t easing,
                                  void *ctx,
                                  void (*on_done)(void *ctx));

eui_anim_handle_t eui_anim_start_spring(eui_widget_t *target,
                                         eui_anim_target_t prop,
                                         int16_t to,
                                         float stiffness,
                                         float damping,
                                         void (*on_done)(void *ctx));

void eui_anim_stop(eui_anim_handle_t handle);
void eui_anim_stop_all(eui_widget_t *target);
bool eui_anim_is_running(eui_anim_handle_t handle);
void eui_anim_update(uint32_t delta_ms);

#endif /* EUI_ANIM_H */
```

- [ ] **Step 2: Write eui_anim.c**

- `eui_anim_init()`: call `mc_hal_set_tick_callback(eui_get_tick_ms)` to bridge MotionC tick source
- `eui_anim_start()`: allocate an mc_animate_t internally, configure easing mode, call mc_animate_update each frame in eui_anim_update(). Store mapping from handle to (widget, property, mc_animate_t). On each update, read mc_animate_t.current, convert from Q16.16 to int16_t, apply to widget property, mark DIRTY.
- `eui_anim_start_spring()`: same but spring mode
- `eui_anim_update()`: iterate all active animations, call mc_animate_update for each, apply property changes. Called from eui_tick().
- Property application: for X → widget->area.x, Y → widget->area.y, WIDTH → widget->area.w, etc.

- [ ] **Step 3: Integrate eui_anim_update() into eui_tick()**

Update `src/eui.c`:
```c
void eui_tick(void) {
    if (!g_eui.initialized) return;
    uint32_t now = eui_get_tick_ms();
    eui_anim_update(now - g_eui.last_tick_ms);
    g_eui.last_tick_ms = now;
    /* Input and rendering will be added in Phase 4 */
}
```

- [ ] **Step 4: Write unit test (test/test_anim.c)**

Test:
1. Start an easing animation on a widget's X property (0 → 100, 1000ms, linear), call eui_anim_update multiple times, verify widget->area.x approaches 100
2. Check eui_anim_is_running returns true during animation, false after completion
3. eui_anim_stop mid-animation, verify is_running returns false immediately

(Note: requires motionc to be linked)

- [ ] **Step 5: Update src/CMakeLists.txt, run test**

Add `eui_anim.c`, run tests.

- [ ] **Step 6: Commit**

```bash
git add include/eui/eui_anim.h src/eui_anim.c test/test_anim.c
git commit -m "feat: implement MotionC animation adapter (easing + spring)"
```

---

### Task 3.11: Integrate rendering pipeline into eui_tick()

**Files:**
- Modify: `src/eui.c`

- [ ] **Step 1: Update eui_tick() with full rendering pipeline**

First, add `eui_get_tick_ms()` to eui.c for timing. The user must provide a platform-specific tick callback:

```c
// In eui.c, add:
static uint32_t (*g_eui_tick_fn)(void) = NULL;

void eui_set_tick_callback(uint32_t (*tick_fn)(void)) {
    g_eui_tick_fn = tick_fn;
}

uint32_t eui_get_tick_ms(void) {
    return g_eui_tick_fn ? g_eui_tick_fn() : 0;
}
```

Then implement the full tick loop:

```c
void eui_tick(void) {
    if (!g_eui.initialized) return;

    uint32_t now = eui_get_tick_ms();
    uint32_t delta = now - g_eui.last_tick_ms;
    g_eui.last_tick_ms = now;

    /* Step 1: Update animations */
    eui_anim_update(delta);

    /* Step 2: Poll input */
    eui_input_update(&g_eui.input_mgr, now);

    /* Step 3: Process input events */
    eui_event_t evt;
    while (eui_event_queue_pop(&g_eui.input_mgr.queue, &evt)) {
        eui_view_dispatcher_send_input(&g_eui.vd, &evt);
    }

    /* Step 4: Render */
    eui_view_dispatcher_tick(&g_eui.vd);
}
```

Update `eui_init()` to also initialize input manager and view dispatcher:
```c
int eui_init(const eui_config_t *config) {
    /* ... existing allocator init ... */
    eui_input_init(&g_eui.input_mgr, config->input);
    eui_canvas_t *canvas = eui_canvas_create(config->display);
    eui_view_dispatcher_init(&g_eui.vd, canvas);
    eui_anim_init();
    /* ... */
}
```

Update `g_eui` struct to include `eui_input_manager_t input_mgr`, `eui_view_dispatcher_t vd`, `eui_canvas_t *canvas`.

- [ ] **Step 2: Verify compilation**

Also add `void eui_set_tick_callback(uint32_t (*tick_fn)(void));` and `uint32_t eui_get_tick_ms(void);` to `include/eui/eui.h` in the extern "C" block.

Run: `cmake -B build && cmake --build build --target eui`

Run: `cmake -B build && cmake --build build --target eui`
Expected: Compile succeeds

- [ ] **Step 3: Commit**

```bash
git commit -m "feat: integrate full rendering pipeline into eui_tick()"
```

---

## Phase 4: Desktop Simulation & Examples

### Task 4.1: Implement raylib Display HAL

**Files:**
- Create: `include/eui/eui_hal_raylib.h`
- Create: `src/eui_hal_raylib.c`
- Modify: `src/CMakeLists.txt`

- [ ] **Step 1: Write eui_hal_raylib.h**

```c
#ifndef EUI_HAL_RAYLIB_H
#define EUI_HAL_RAYLIB_H

#include "eui_display_hal.h"
#include "eui_input_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

eui_display_hal_t* eui_hal_raylib_create_display(uint16_t width, uint16_t height,
                                                  uint8_t color_depth);
void eui_hal_raylib_destroy_display(eui_display_hal_t *hal);
void eui_hal_raylib_refresh(void);

eui_input_hal_t* eui_hal_raylib_create_input(uint16_t window_scale);
void eui_hal_raylib_destroy_input(eui_input_hal_t *hal);

#ifdef __cplusplus
}
#endif

#endif /* EUI_HAL_RAYLIB_H */
```

- [ ] **Step 2: Write eui_hal_raylib.c — Display HAL**

Implement Display HAL function stubs using raylib:
- `init()`: `InitWindow(width * scale, height * scale, "EUI")`, create a `RenderTexture2D` as the frame buffer
- `draw_pixel()`: write to a local Image buffer (used in DIRECT mode)
- `write_buffer()`: `UpdateTexture(render_texture.texture, buffer)` then `BeginDrawing(); DrawTexturePro(...)` with nearest-neighbor scaling; `EndDrawing()`
- `set_contrast()`, `set_power()`, `set_invert()`: no-op (or toggle color inversion)
- `eui_hal_raylib_refresh()`: call `BeginDrawing()` / `DrawTexturePro()` / `EndDrawing()` (for FULL buffer mode where write_buffer does the texture update)

- [ ] **Step 3: Write eui_hal_raylib.c — Input HAL**

Implement Input HAL:
- `poll()`: check raylib key states:
  - `IsKeyPressed(KEY_UP)` → EUI_KEY_UP
  - `IsKeyPressed(KEY_DOWN)` → EUI_KEY_DOWN
  - `IsKeyPressed(KEY_LEFT)` → EUI_KEY_LEFT
  - `IsKeyPressed(KEY_RIGHT)` → EUI_KEY_RIGHT
  - `IsKeyPressed(KEY_ENTER)` → EUI_KEY_OK
  - `IsKeyPressed(KEY_ESCAPE)` → EUI_KEY_BACK
  - `GetMouseWheelMove()` → encoder delta
  - Mouse position → touch coordinates (scaled to framebuffer size)
  - Return 1 if an event was filled, 0 otherwise

- [ ] **Step 4: Update src/CMakeLists.txt**

Add raylib as a dependency for the HAL:
```cmake
if(RAYLIB_FOUND)
    add_library(eui_hal_raylib STATIC eui_hal_raylib.c)
    target_link_libraries(eui_hal_raylib PUBLIC eui raylib)
endif()
```

- [ ] **Step 5: Verify compilation with raylib**

Run: `cmake -B build && cmake --build build --target eui_hal_raylib`
Expected: Compile succeeds (requires raylib installed)

- [ ] **Step 6: Commit**

```bash
git add include/eui/eui_hal_raylib.h src/eui_hal_raylib.c
git commit -m "feat: implement raylib desktop simulation HAL (display + input)"
```

---

### Task 4.2: Implement example programs

**Files:**
- Create: `examples/CMakeLists.txt`
- Create: `examples/basic_label.c`
- Create: `examples/button_test.c`
- Create: `examples/list_nav.c`
- Create: `examples/menu_system.c`
- Create: `examples/dialog_overlay.c`
- Create: `examples/animation_demo.c`
- Create: `examples/custom_widget.c`
- Create: `examples/page_buffer.c`
- Create: `examples/benchmark.c`

- [ ] **Step 1: Write examples/CMakeLists.txt**

```cmake
set(EXAMPLE_SRCS
    basic_label
    button_test
    list_nav
    menu_system
    dialog_overlay
    animation_demo
    custom_widget
    page_buffer
    benchmark
)

foreach(example ${EXAMPLE_SRCS})
    add_executable(${example} ${example}.c)
    target_link_libraries(${example} PRIVATE eui eui_hal_raylib raylib)
    target_include_directories(${example} PRIVATE ${CMAKE_BINARY_DIR}/include)
endforeach()
```

- [ ] **Step 2: Write basic_label.c**

Minimal 128x64 1bpp example:
1. Create raylib display HAL (128, 64, 1)
2. Create raylib input HAL
3. eui_init with config
4. Create label "Hello EUI!" at (0, 0)
5. Main loop: eui_tick(), PollWindowShouldClose(), raylib_refresh()
6. eui_deinit on exit

- [ ] **Step 3: Write button_test.c**

128x64 1bpp:
1. Create 2 buttons: "Btn A" at (10, 20, 50, 24), "Btn B" at (70, 20, 50, 24)
2. Attach callbacks that print to stdout
3. Register as a View in ViewDispatcher
4. Tab between buttons with LEFT/RIGHT keys
5. Press ENTER to trigger callback

- [ ] **Step 4: Write list_nav.c**

128x64 1bpp:
1. Create a List with 10 items ("Item 0" through "Item 9")
2. Register callback that prints selected index
3. Navigate with UP/DOWN keys
4. Select with ENTER

- [ ] **Step 5: Write menu_system.c**

128x64 1bpp:
1. Create a Menu with items: "Settings", "About", "Exit"
2. "Settings" opens a submenu with: "Brightness", "Volume"
3. "Back" returns to main menu (BACK key)
4. "Exit" prints "Goodbye" and exits

- [ ] **Step 6: Write dialog_overlay.c**

128x64 1bpp:
1. Show a main screen with label "Press OK for dialog"
2. On OK press, create and show a Dialog: title="Confirm", msg="Are you sure?", buttons: [Yes, No]
3. On Yes → print "Confirmed" and close
4. On No → print "Cancelled" and close
5. Verify overlay stack works (dialog appears on top, input goes to dialog)

- [ ] **Step 7: Write animation_demo.c**

128x64 1bpp:
1. Create a moving square widget
2. On LEFT/RIGHT key, animate widget X position using easing functions
3. Cycle through easing functions on each move (linear, quad_in, bounce_out, elastic_out)
4. Print current easing name to stdout

- [ ] **Step 8: Write custom_widget.c**

128x64 1bpp:
1. Implement a custom counter widget (same as Section 12.2 in design doc)
2. Create vtable with custom draw (draws rounded rect + count text) and input (increment on OK)
3. Register as View, test increment with OK key

- [ ] **Step 9: Write page_buffer.c**

Creates 2 canvases side by side:
1. Left: FULL buffer mode (128x64, 1bpp)
2. Right: PAGE buffer mode (128x64, 1bpp, 8px band)

Draw identical content on both (text, lines, rects). Verify visually that output is identical, confirming PAGE mode renders correctly.

- [ ] **Step 10: Write benchmark.c**

128x64 1bpp:
1. Render a complex scene (text, lines, circles, rects) every frame
2. Track frame time using raylib's `GetFrameTime()`
3. Display FPS counter as text on screen
4. Print allocator stats (total/used/peak) every 100 frames
5. Exit after 10 seconds, print summary

- [ ] **Step 11: Build and run all examples**

Run: `cmake -B build -DEUI_BUILD_EXAMPLES=ON && cmake --build build`
Expected: All examples compile. Run each manually to verify.

- [ ] **Step 12: Commit**

```bash
git add examples/
git commit -m "feat: add 9 example programs covering all framework features"
```

---

### Task 4.3: CI/CD configuration

**Files:**
- Create: `.github/workflows/build.yml`
- Create: `.github/workflows/test.yml`

- [ ] **Step 1: Write build workflow**

```yaml
name: Build
on: [push, pull_request]
jobs:
  build:
    strategy:
      matrix:
        os: [ubuntu-latest, macos-latest]
        color_depth: [1, 8, 16]
    runs-on: ${{ matrix.os }}
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive
      - name: Install raylib
        run: |
          sudo apt-get update && sudo apt-get install -y libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev libwayland-dev libxkbcommon-dev
          git clone --depth 1 https://github.com/raysan5/raylib.git /tmp/raylib
          cd /tmp/raylib && mkdir build && cd build && cmake .. && make -j$(nproc) && sudo make install
        if: runner.os == 'Linux'
      - name: Install raylib (macOS)
        run: brew install raylib
        if: runner.os == 'macOS'
      - name: Configure
        run: cmake -B build -DEUI_COLOR_DEPTH=${{ matrix.color_depth }}
      - name: Build
        run: cmake --build build -j
```

- [ ] **Step 2: Write test workflow**

```yaml
name: Test
on: [push, pull_request]
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive
      - name: Configure and Build
        run: cmake -B build -DEUI_BUILD_EXAMPLES=OFF && cmake --build build -j
      - name: Run tests
        run: cd build && ctest --output-on-failure
```

- [ ] **Step 3: Add badges and CI status to README.md**

Create a minimal README.md with project description, build instructions, and CI badges.

- [ ] **Step 4: Commit**

```bash
git add .github/ README.md
git commit -m "ci: add GitHub Actions build and test workflows"
```

---

## Self-Review Checklist

Before declaring the plan complete, I verify:

1. **Spec coverage**: Each section of `eui_framework_design.md` maps to at least one task above:
   - §3 (TLSF) → Task 1.2, 1.5
   - §4 (Canvas) → Task 2.5
   - §5 (Display HAL) → Task 2.1
   - §6 (Input) → Tasks 2.2, 2.3, 2.4
   - §7 (Views) → Tasks 3.1, 3.2, 3.9
   - §8 (Widgets) → Tasks 3.3-3.8
   - §9 (Animation) → Task 3.10
   - §10 (Rendering) → Task 3.11
   - §11 (API Ref) → Covered throughout
   - §12 (Examples) → Task 4.2
   - §14 (Porting) → Task 4.1
   - §15 (Testing) → Tasks 4.2, 4.3
   - §16 (Summary) → No implementation needed

2. **Placeholder scan**: No TBD, TODO, "implement later", "add appropriate error handling" without specifics. All functions described with signatures, all algorithms specified.

3. **Type consistency**: Cross-checked — `eui_view_t`, `eui_widget_t`, `eui_canvas_t`, `eui_event_t`, `eui_config_t`, `eui_anim_handle_t`, `eui_bitmap_t`, `eui_font_t` are consistently defined and used throughout all tasks. All function signatures match between headers and implementation descriptions.

4. **Missing spec items**: The visual companion images referenced in the docs (eui_architecture.png etc.) are already present. No additional assets needed.
