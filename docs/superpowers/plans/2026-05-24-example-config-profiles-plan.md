# 示例配置描述文件 实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将 EUI 示例系统改造为平台 × 驱动 × 色深的可组合架构——引入配置描述文件（config profiles）、迁移全部 legacy 示例到 cross/、实现 EXAMPLE+PROFILE 约束检查。

**Architecture:** 配置描述文件是 .cmake 片段，打包平台/驱动/色深/分辨率/引脚变量。构建系统 include() 配置文件后，将变量注入 eui_config.h 和 eui_profile_config.h。示例通过 requirements.cmake 声明约束，构建系统自动跳过不兼容组合。

**Tech Stack:** CMake 3.22+, C99, Kconfig (esp-idf/nrf5)

---

### Task 1: Create configs/ directory and PC raylib profiles

**Files:**
- Create: `configs/pc/raylib_128x64_1bpp.cmake`
- Create: `configs/pc/raylib_240x240_16bpp.cmake`

- [ ] **Step 1: Create configs/pc/ directory**

```bash
mkdir -p configs/pc configs/esp32 configs/nrf52
```

- [ ] **Step 2: Create configs/pc/raylib_128x64_1bpp.cmake**

```cmake
# configs/pc/raylib_128x64_1bpp.cmake
set(EUI_TARGET_PORT      "raylib")
set(EUI_COLOR_DEPTH      1)
set(EUI_DISPLAY_DRIVER   "raylib")
set(EUI_DISPLAY_WIDTH    128)
set(EUI_DISPLAY_HEIGHT   64)
set(EUI_BUFFER_MODE      "full")
set(EUI_MEM_POOL_SIZE    32768)
set(EUI_INPUT_DRIVER     "raylib")
```

- [ ] **Step 3: Create configs/pc/raylib_240x240_16bpp.cmake**

```cmake
# configs/pc/raylib_240x240_16bpp.cmake
set(EUI_TARGET_PORT      "raylib")
set(EUI_COLOR_DEPTH      16)
set(EUI_DISPLAY_DRIVER   "raylib")
set(EUI_DISPLAY_WIDTH    240)
set(EUI_DISPLAY_HEIGHT   240)
set(EUI_BUFFER_MODE      "full")
set(EUI_MEM_POOL_SIZE    262144)
set(EUI_INPUT_DRIVER     "raylib")
```

- [ ] **Step 4: Commit**

```bash
git add configs/
git commit -m "feat(configs): add PC raylib config profiles"
```

---

### Task 2: Create ESP32 config profiles

**Files:**
- Create: `configs/esp32/ssd1306_i2c_128x64_1bpp.cmake`
- Create: `configs/esp32/st7735_spi_240x240_16bpp.cmake`
- Create: `configs/esp32/ili9341_spi_320x240_16bpp.cmake`

- [ ] **Step 1: Create configs/esp32/ssd1306_i2c_128x64_1bpp.cmake**

```cmake
# configs/esp32/ssd1306_i2c_128x64_1bpp.cmake
set(EUI_TARGET_PORT      "esp-idf")
set(EUI_COLOR_DEPTH      1)
set(EUI_DISPLAY_DRIVER   "ssd1306")
set(EUI_DISPLAY_WIDTH    128)
set(EUI_DISPLAY_HEIGHT   64)
set(EUI_BUFFER_MODE      "page")
set(EUI_MEM_POOL_SIZE    32768)

# I2C pins
set(EUI_PIN_I2C_PORT     0)
set(EUI_PIN_SDA          21)
set(EUI_PIN_SCL          22)
set(EUI_PIN_I2C_FREQ     400000)
set(EUI_PIN_I2C_ADDR     0x3C)

# Input
set(EUI_INPUT_DRIVER     "buttons")
set(EUI_PIN_BTN_UP       34)
set(EUI_PIN_BTN_DOWN     35)
set(EUI_PIN_BTN_OK       32)
set(EUI_PIN_BTN_BACK     33)
```

- [ ] **Step 2: Create configs/esp32/st7735_spi_240x240_16bpp.cmake**

```cmake
# configs/esp32/st7735_spi_240x240_16bpp.cmake
set(EUI_TARGET_PORT      "esp-idf")
set(EUI_COLOR_DEPTH      16)
set(EUI_DISPLAY_DRIVER   "st7735")
set(EUI_DISPLAY_WIDTH    240)
set(EUI_DISPLAY_HEIGHT   240)
set(EUI_BUFFER_MODE      "page")
set(EUI_MEM_POOL_SIZE    262144)

# SPI pins
set(EUI_PIN_SPI_HOST     1)
set(EUI_PIN_MOSI         23)
set(EUI_PIN_SCLK         18)
set(EUI_PIN_CS           5)
set(EUI_PIN_DC           16)
set(EUI_PIN_RST          17)
set(EUI_PIN_BL           4)

# Input
set(EUI_INPUT_DRIVER     "buttons")
set(EUI_PIN_BTN_UP       34)
set(EUI_PIN_BTN_DOWN     35)
set(EUI_PIN_BTN_OK       32)
set(EUI_PIN_BTN_BACK     33)
```

- [ ] **Step 3: Create configs/esp32/ili9341_spi_320x240_16bpp.cmake**

```cmake
# configs/esp32/ili9341_spi_320x240_16bpp.cmake
set(EUI_TARGET_PORT      "esp-idf")
set(EUI_COLOR_DEPTH      16)
set(EUI_DISPLAY_DRIVER   "ili9341")
set(EUI_DISPLAY_WIDTH    320)
set(EUI_DISPLAY_HEIGHT   240)
set(EUI_BUFFER_MODE      "page")
set(EUI_MEM_POOL_SIZE    262144)

# SPI pins
set(EUI_PIN_SPI_HOST     1)
set(EUI_PIN_MOSI         23)
set(EUI_PIN_SCLK         18)
set(EUI_PIN_CS           5)
set(EUI_PIN_DC           16)
set(EUI_PIN_RST          17)
set(EUI_PIN_BL           4)

# Input
set(EUI_INPUT_DRIVER     "buttons")
set(EUI_PIN_BTN_UP       34)
set(EUI_PIN_BTN_DOWN     35)
set(EUI_PIN_BTN_OK       32)
set(EUI_PIN_BTN_BACK     33)
```

- [ ] **Step 4: Commit**

```bash
git add configs/esp32/
git commit -m "feat(configs): add ESP32 config profiles (ssd1306, st7735, ili9341)"
```

---

### Task 3: Create NRF52 config profiles

**Files:**
- Create: `configs/nrf52/ssd1306_i2c_128x64_1bpp.cmake`
- Create: `configs/nrf52/st7735_spi_240x240_16bpp.cmake`

- [ ] **Step 1: Create configs/nrf52/ssd1306_i2c_128x64_1bpp.cmake**

```cmake
# configs/nrf52/ssd1306_i2c_128x64_1bpp.cmake
set(EUI_TARGET_PORT      "nrf5")
set(EUI_COLOR_DEPTH      1)
set(EUI_DISPLAY_DRIVER   "ssd1306")
set(EUI_DISPLAY_WIDTH    128)
set(EUI_DISPLAY_HEIGHT   64)
set(EUI_BUFFER_MODE      "page")
set(EUI_MEM_POOL_SIZE    32768)

# I2C pins (nRF52 DK defaults)
set(EUI_PIN_I2C_PORT     0)
set(EUI_PIN_SDA          26)
set(EUI_PIN_SCL          27)
set(EUI_PIN_I2C_FREQ     400000)
set(EUI_PIN_I2C_ADDR     0x3C)

# Input
set(EUI_INPUT_DRIVER     "buttons")
set(EUI_PIN_BTN_UP       13)
set(EUI_PIN_BTN_DOWN     14)
set(EUI_PIN_BTN_OK       11)
set(EUI_PIN_BTN_BACK     12)
```

- [ ] **Step 2: Create configs/nrf52/st7735_spi_240x240_16bpp.cmake**

```cmake
# configs/nrf52/st7735_spi_240x240_16bpp.cmake
set(EUI_TARGET_PORT      "nrf5")
set(EUI_COLOR_DEPTH      16)
set(EUI_DISPLAY_DRIVER   "st7735")
set(EUI_DISPLAY_WIDTH    240)
set(EUI_DISPLAY_HEIGHT   240)
set(EUI_BUFFER_MODE      "page")
set(EUI_MEM_POOL_SIZE    262144)

# SPI pins
set(EUI_PIN_SPI_HOST     0)
set(EUI_PIN_MOSI         23)
set(EUI_PIN_SCLK         24)
set(EUI_PIN_CS           22)
set(EUI_PIN_DC           25)
set(EUI_PIN_RST          19)
set(EUI_PIN_BL           20)

# Input
set(EUI_INPUT_DRIVER     "buttons")
set(EUI_PIN_BTN_UP       13)
set(EUI_PIN_BTN_DOWN     14)
set(EUI_PIN_BTN_OK       11)
set(EUI_PIN_BTN_BACK     12)
```

- [ ] **Step 3: Commit**

```bash
git add configs/nrf52/
git commit -m "feat(configs): add NRF52 config profiles (ssd1306, st7735)"
```

---

### Task 4: Wire EUI_CONFIG_PROFILE into root CMakeLists.txt

**Files:**
- Modify: `CMakeLists.txt:66-92` (cross examples section)
- Create: `tools/config_to_header.cmake`

- [ ] **Step 1: Create tools/config_to_header.cmake**

```cmake
# tools/config_to_header.cmake
# Generates eui_profile_config.h from CMake variables set by config profile.
# Called as: cmake -DCONFIG_VARS="VAR1;VAR2;..." -DOUTPUT_HEADER=<path> -P tools/config_to_header.cmake

if(NOT DEFINED CONFIG_VARS OR NOT DEFINED OUTPUT_HEADER)
    message(FATAL_ERROR "Usage: cmake -DCONFIG_VARS=... -DOUTPUT_HEADER=... -P tools/config_to_header.cmake")
endif()

file(WRITE "${OUTPUT_HEADER}" "/* Auto-generated by tools/config_to_header.cmake — DO NOT EDIT */\n")
file(APPEND "${OUTPUT_HEADER}" "#ifndef EUI_PROFILE_CONFIG_H\n")
file(APPEND "${OUTPUT_HEADER}" "#define EUI_PROFILE_CONFIG_H\n\n")

foreach(var_name ${CONFIG_VARS})
    if(DEFINED ${var_name})
        set(val "${${var_name}}")
        # Quote string values, pass numeric values as-is
        if(val MATCHES "^[0-9]+$" OR val MATCHES "^0x[0-9a-fA-F]+$")
            file(APPEND "${OUTPUT_HEADER}" "#define ${var_name} ${val}\n")
        else()
            file(APPEND "${OUTPUT_HEADER}" "#define ${var_name} \"${val}\"\n")
        endif()
    endif()
endforeach()

file(APPEND "${OUTPUT_HEADER}" "\n#endif /* EUI_PROFILE_CONFIG_H */\n")
message(STATUS "Generated ${OUTPUT_HEADER}")
```

- [ ] **Step 2: Add EUI_CONFIG_PROFILE handling to root CMakeLists.txt (before the cross-examples section at line 72)**

Replace lines 72-92 (the `if(EUI_BUILD_CROSS_EXAMPLES)` block) with:

```cmake
# Cross-platform examples
if(EUI_BUILD_CROSS_EXAMPLES)
    # Config profile support: if set, include() the profile which sets
    # EUI_TARGET_PORT, EUI_COLOR_DEPTH, EUI_DISPLAY_DRIVER, etc.
    if(DEFINED EUI_CONFIG_PROFILE)
        if(NOT EXISTS "${CMAKE_SOURCE_DIR}/${EUI_CONFIG_PROFILE}")
            message(FATAL_ERROR "Config profile not found: ${EUI_CONFIG_PROFILE}")
        endif()
        message(STATUS "Using config profile: ${EUI_CONFIG_PROFILE}")
        include("${CMAKE_SOURCE_DIR}/${EUI_CONFIG_PROFILE}")

        # Generate eui_profile_config.h from profile variables
        set(PROFILE_VARS
            EUI_DISPLAY_DRIVER EUI_DISPLAY_WIDTH EUI_DISPLAY_HEIGHT EUI_BUFFER_MODE
            EUI_PIN_SPI_HOST EUI_PIN_MOSI EUI_PIN_SCLK EUI_PIN_CS EUI_PIN_DC EUI_PIN_RST EUI_PIN_BL
            EUI_PIN_I2C_PORT EUI_PIN_SDA EUI_PIN_SCL EUI_PIN_I2C_FREQ EUI_PIN_I2C_ADDR
            EUI_INPUT_DRIVER EUI_PIN_BTN_UP EUI_PIN_BTN_DOWN EUI_PIN_BTN_OK EUI_PIN_BTN_BACK
        )
        set(PROFILE_HEADER "${CMAKE_BINARY_DIR}/include/eui/eui_profile_config.h")
        add_custom_command(
            OUTPUT ${PROFILE_HEADER}
            COMMAND ${CMAKE_COMMAND}
                -DCONFIG_VARS="${PROFILE_VARS}"
                -DOUTPUT_HEADER=${PROFILE_HEADER}
                -P ${CMAKE_SOURCE_DIR}/tools/config_to_header.cmake
            COMMENT "Generating eui_profile_config.h from config profile"
        )
        add_custom_target(gen_profile_config DEPENDS ${PROFILE_HEADER})
    endif()

    if(NOT DEFINED EUI_TARGET_PORT)
        set(EUI_TARGET_PORT "raylib" CACHE STRING "Target port: raylib, esp-idf, nrf5")
    endif()
    set_property(CACHE EUI_TARGET_PORT PROPERTY STRINGS "raylib" "esp-idf" "nrf5")

    if(EUI_TARGET_PORT STREQUAL "raylib")
        set(BOOTSTRAP_LIB eui_port_raylib)
        add_subdirectory(port/raylib)
    elseif(EUI_TARGET_PORT STREQUAL "esp-idf")
        set(BOOTSTRAP_LIB eui_port_esp_idf)
        add_subdirectory(port/esp-idf)
    elseif(EUI_TARGET_PORT STREQUAL "nrf5")
        set(BOOTSTRAP_LIB eui_port_nrf5)
        add_subdirectory(port/nrf5)
    else()
        message(FATAL_ERROR "Unknown EUI_TARGET_PORT: ${EUI_TARGET_PORT}")
    endif()

    add_subdirectory(examples/cross)
endif()
```

- [ ] **Step 3: Verify raylib config profile builds work**

```bash
cmake -B build -DEUI_CONFIG_PROFILE=configs/pc/raylib_128x64_1bpp.cmake -DEUI_BUILD_CROSS_EXAMPLES=ON -DEUI_BUILD_EXAMPLES=OFF -DEUI_BUILD_TESTS=OFF
cmake --build build -j$(nproc)
```

Expected: Build succeeds for existing cross examples (basic_label, button_test, list_nav).

- [ ] **Step 4: Commit**

```bash
git add CMakeLists.txt tools/config_to_header.cmake
git commit -m "feat(build): add EUI_CONFIG_PROFILE support with auto-generated profile header"
```

---

### Task 5: Migrate basic_label to cross/

**Files:**
- Create: `examples/cross/basic_label/requirements.cmake` (empty — no constraints)
- Modify: `examples/basic_label.c` — DELETE after verification
- Modify: `examples/CMakeLists.txt` — remove `basic_label` from EXAMPLE list

- [ ] **Step 1: Create requirements.cmake**

The existing `examples/cross/basic_label/` already has basic_label.c and CMakeLists.txt. Only need to add requirements.cmake for completeness:

```cmake
# examples/cross/basic_label/requirements.cmake
# No requirements — compatible with all config profiles
```

- [ ] **Step 2: Verify cross basic_label builds**

```bash
cmake -B build -DEUI_CONFIG_PROFILE=configs/pc/raylib_128x64_1bpp.cmake -DEUI_BUILD_CROSS_EXAMPLES=ON -DEUI_BUILD_EXAMPLES=OFF -DEUI_BUILD_TESTS=OFF
cmake --build build -j$(nproc)
```

- [ ] **Step 3: Commit**

```bash
git add examples/cross/basic_label/requirements.cmake
git commit -m "feat(cross): add requirements.cmake for basic_label"
```

---

### Task 6: Migrate button_test to cross/

**Files:**
- Create: `examples/cross/button_test/requirements.cmake`
- Modify: `examples/button_test.c` — DELETE after migration verified
- Modify: `examples/CMakeLists.txt` — remove `button_test` from list

- [ ] **Step 1: Create requirements.cmake**

Button_test is already in cross/ with button_test.c and CMakeLists.txt. Add requirements.cmake:

```cmake
# examples/cross/button_test/requirements.cmake
# No requirements — compatible with all config profiles
```

- [ ] **Step 2: Verify cross button_test builds**

```bash
cmake -B build -DEUI_CONFIG_PROFILE=configs/pc/raylib_128x64_1bpp.cmake -DEUI_BUILD_CROSS_EXAMPLES=ON -DEUI_BUILD_EXAMPLES=OFF -DEUI_BUILD_TESTS=OFF
cmake --build build -j$(nproc)
```

- [ ] **Step 3: Commit**

```bash
git add examples/cross/button_test/requirements.cmake
git commit -m "feat(cross): add requirements.cmake for button_test"
```

---

### Task 7: Migrate list_nav to cross/

Same as Task 6 — list_nav already exists in cross/, just add requirements.cmake.

- [ ] **Step 1: Create examples/cross/list_nav/requirements.cmake**

```cmake
# examples/cross/list_nav/requirements.cmake
# No requirements — compatible with all config profiles
```

- [ ] **Step 2: Commit**

```bash
git add examples/cross/list_nav/requirements.cmake
git commit -m "feat(cross): add requirements.cmake for list_nav"
```

---

### Task 8: Migrate menu_system to cross/

**Files:**
- Create: `examples/cross/menu_system/`
- Create: `examples/cross/menu_system/menu_system.c`
- Create: `examples/cross/menu_system/CMakeLists.txt`
- Create: `examples/cross/menu_system/requirements.cmake`
- Modify: `examples/cross/CMakeLists.txt` — add `add_subdirectory(menu_system)`

- [ ] **Step 1: Read existing legacy source**

Read `examples/menu_system.c` to extract UI logic. The extracted eui_example_setup() removes `main()`, allocator init, display/input creation, main loop, and teardown boilerplate.

- [ ] **Step 2: Create examples/cross/menu_system/menu_system.c**

```c
/* examples/cross/menu_system/menu_system.c */
#include "eui/eui.h"
#include "eui/eui_port_bootstrap.h"
#include <stdio.h>

static void on_brightness(void *ctx) { printf("[MENU] Brightness\n"); (void)ctx; }
static void on_volume(void *ctx)     { printf("[MENU] Volume\n"); (void)ctx; }
static void on_about(void *ctx)      { printf("[MENU] About\n"); (void)ctx; }
static void on_quit(void *ctx)       { printf("[MENU] Quit\n"); (void)ctx; }

void eui_example_setup(const eui_example_config_t *cfg) {
    (void)cfg;
    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();

    eui_widget_t *menu = eui_menu_create(0, 0, 128, 64);
    menu->focus_policy = EUI_FOCUS_STRONG;

    eui_menu_item_t *settings = eui_menu_add_submenu(menu, "Settings");
    eui_menu_add_item(menu, "About", on_about);
    eui_menu_add_item(menu, "Exit", on_quit);

    if (settings && settings->submenu) {
        eui_widget_t *sub = (eui_widget_t*)settings->submenu;
        eui_menu_add_item(sub, "Brightness", on_brightness);
        eui_menu_add_item(sub, "Volume", on_volume);
    }

    eui_view_dispatcher_add(vd, 1, &menu->view);
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);
}
```

- [ ] **Step 3: Create examples/cross/menu_system/CMakeLists.txt**

```cmake
# examples/cross/menu_system/CMakeLists.txt
add_executable(menu_system menu_system.c)
target_include_directories(menu_system PRIVATE
    ${CMAKE_BINARY_DIR}/include
)
target_link_libraries(menu_system PRIVATE eui ${BOOTSTRAP_LIB})
```

- [ ] **Step 4: Create requirements.cmake**

```cmake
# examples/cross/menu_system/requirements.cmake
# No requirements — compatible with all config profiles
```

- [ ] **Step 5: Update examples/cross/CMakeLists.txt**

Add to the existing file: `add_subdirectory(menu_system)`

- [ ] **Step 6: Commit**

```bash
git add examples/cross/menu_system/ examples/cross/CMakeLists.txt
git commit -m "feat(cross): migrate menu_system example"
```

---

### Task 9: Migrate dialog_overlay, animation_demo, custom_widget, page_buffer to cross/

**Files:**
- Create: `examples/cross/dialog_overlay/{dialog_overlay.c,CMakeLists.txt,requirements.cmake}`
- Create: `examples/cross/animation_demo/{animation_demo.c,CMakeLists.txt,requirements.cmake}`
- Create: `examples/cross/custom_widget/{custom_widget.c,CMakeLists.txt,requirements.cmake}`
- Create: `examples/cross/page_buffer/{page_buffer.c,CMakeLists.txt,requirements.cmake}`
- Modify: `examples/cross/CMakeLists.txt`

- [ ] **Step 1: Create dialog_overlay migration**

Read `examples/dialog_overlay.c`, extract eui_example_setup():

```c
/* examples/cross/dialog_overlay/dialog_overlay.c */
#include "eui/eui.h"
#include "eui/eui_port_bootstrap.h"
#include <stdio.h>

static void on_dialog_done(eui_dialog_result_t result, void *ctx) {
    printf("[DIALOG] Result: %s\n", result == EUI_DIALOG_YES ? "YES" : "NO");
    (void)ctx;
}

void eui_example_setup(const eui_example_config_t *cfg) {
    (void)cfg;
    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();

    eui_view_t placeholder;
    eui_view_init(&placeholder, NULL, NULL);
    eui_view_dispatcher_add(vd, 0, &placeholder);
    eui_view_dispatcher_switch_to(vd, 0, EUI_ANIM_NONE);

    eui_widget_t *dialog = eui_dialog_create("Confirm", "Are you sure?");
    eui_dialog_add_button(dialog, "Yes", EUI_DIALOG_YES);
    eui_dialog_add_button(dialog, "No",  EUI_DIALOG_NO);
    eui_dialog_show(dialog, vd, on_dialog_done);
}
```

CMakeLists.txt (same pattern as menu_system):

```cmake
# examples/cross/dialog_overlay/CMakeLists.txt
add_executable(dialog_overlay dialog_overlay.c)
target_include_directories(dialog_overlay PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(dialog_overlay PRIVATE eui ${BOOTSTRAP_LIB})
```

requirements.cmake:

```cmake
# examples/cross/dialog_overlay/requirements.cmake
# No requirements
```

- [ ] **Step 2: Create animation_demo migration**

```c
/* examples/cross/animation_demo/animation_demo.c */
#include "eui/eui.h"
#include "eui/eui_port_bootstrap.h"

static void square_draw(eui_widget_t *w, eui_canvas_t *c) {
    eui_canvas_set_color(c, EUI_COLOR_WHITE);
    eui_canvas_fill_rect(c, w->area.x, w->area.y, w->area.w, w->area.h);
}

static bool square_input(eui_widget_t *w, const eui_event_t *e) {
    (void)w; (void)e;
    return false;
}

static eui_widget_vtable_t square_vt = { .draw = square_draw, .input = square_input };

void eui_example_setup(const eui_example_config_t *cfg) {
    (void)cfg;
    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();

    eui_widget_t square;
    eui_widget_init(&square, &square_vt, 0, 30, 16, 16);

    eui_view_dispatcher_add(vd, 1, &square.view);
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);

    eui_anim_init();
    eui_anim_start(&square, EUI_ANIM_TARGET_X, 0, 80, 500, mc_ease_linear, NULL, NULL);
}
```

CMakeLists.txt and requirements.cmake follow same pattern (no requirements).

- [ ] **Step 3: Create custom_widget migration**

```c
/* examples/cross/custom_widget/custom_widget.c */
#include "eui/eui.h"
#include "eui/eui_port_bootstrap.h"
#include <stdio.h>

typedef struct {
    eui_widget_t widget;
    int count;
} counter_t;

static void counter_draw(eui_widget_t *w, eui_canvas_t *c) {
    counter_t *cw = (counter_t*)w;
    eui_canvas_set_color(c, EUI_COLOR_WHITE);
    eui_canvas_draw_round_rect(c, w->area.x, w->area.y, w->area.w, w->area.h, 4);

    char buf[16];
    snprintf(buf, sizeof(buf), "%d", cw->count);
    eui_canvas_draw_str_aligned(c, w->area.x + (int16_t)(w->area.w / 2),
                                  w->area.y + (int16_t)(w->area.h / 2),
                                  EUI_ALIGN_CENTER, EUI_ALIGN_MIDDLE, buf);
}

static bool counter_input(eui_widget_t *w, const eui_event_t *evt) {
    if (evt->type == EUI_EVT_KEY_PRESS && evt->data.key == EUI_KEY_OK) {
        ((counter_t*)w)->count++;
        return true;
    }
    return false;
}

static eui_widget_vtable_t counter_vt = {
    .draw = counter_draw,
    .input = counter_input
};

void eui_example_setup(const eui_example_config_t *cfg) {
    (void)cfg;
    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();

    counter_t cw;
    memset(&cw, 0, sizeof(cw));
    eui_widget_init(&cw.widget, &counter_vt, 40, 20, 48, 24);
    cw.widget.focus_policy = EUI_FOCUS_STRONG;
    eui_view_dispatcher_add(vd, 1, &cw.widget.view);
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);
}
```

- [ ] **Step 4: Create page_buffer migration**

```c
/* examples/cross/page_buffer/page_buffer.c */
#include "eui/eui.h"
#include "eui/eui_port_bootstrap.h"

/* Note: requires eui_get_display() — if this API does not exist,
 * add it to include/eui/eui.h as: eui_display_drv_t* eui_get_display(void);
 * The display is stored internally during eui_init(). */

void eui_example_setup(const eui_example_config_t *cfg) {
    eui_display_drv_t *display = eui_get_display();
    if (!display) return;

    (void)cfg;
    eui_canvas_t *canvas = eui_canvas_create(display);

    eui_canvas_set_color(canvas, EUI_COLOR_WHITE);
    eui_canvas_clear(canvas);
    eui_canvas_fill_rect(canvas, 10, 10, 40, 30);
    eui_canvas_draw_rect(canvas, 60, 15, 50, 30);
    eui_canvas_draw_circle(canvas, 30, 45, 10);
    eui_canvas_commit(canvas);

    eui_canvas_destroy(canvas);
}
```

- [ ] **Step 5: Update examples/cross/CMakeLists.txt**

```cmake
# examples/cross/CMakeLists.txt
add_subdirectory(dialog_overlay)
add_subdirectory(animation_demo)
add_subdirectory(custom_widget)
add_subdirectory(page_buffer)
```

- [ ] **Step 6: Build and verify**

```bash
cmake -B build -DEUI_CONFIG_PROFILE=configs/pc/raylib_128x64_1bpp.cmake -DEUI_BUILD_CROSS_EXAMPLES=ON -DEUI_BUILD_EXAMPLES=OFF -DEUI_BUILD_TESTS=OFF
cmake --build build -j$(nproc)
```

- [ ] **Step 7: Commit**

```bash
git add examples/cross/
git commit -m "feat(cross): migrate dialog_overlay, animation_demo, custom_widget, page_buffer"
```

---

### Task 10: Migrate benchmark and scene_view_demo to cross/

**Files:**
- Create: `examples/cross/benchmark/{benchmark.c,CMakeLists.txt,requirements.cmake}`
- Create: `examples/cross/scene_view_demo/{scene_view_demo.c,CMakeLists.txt,requirements.cmake}`
- Modify: `examples/cross/CMakeLists.txt`

- [ ] **Step 1: Create benchmark migration**

```c
/* examples/cross/benchmark/benchmark.c */
#include "eui/eui.h"
#include "eui/eui_port_bootstrap.h"
#include "eui/driver/eui_drv_raylib.h"
#include <stdio.h>

static void scene_draw(eui_widget_t *w, eui_canvas_t *c) {
    eui_canvas_set_color(c, EUI_COLOR_WHITE);
    eui_canvas_fill_rect(c, w->area.x, w->area.y, w->area.w, w->area.h);
}

static bool scene_input(eui_widget_t *w, const eui_event_t *e) {
    (void)w; (void)e;
    return false;
}

static eui_widget_vtable_t scene_vt = { .draw = scene_draw, .input = scene_input };

void eui_example_setup(const eui_example_config_t *cfg) {
    (void)cfg;
    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();

    eui_widget_t *scene = eui_malloc(sizeof(eui_widget_t));
    eui_widget_init(scene, &scene_vt, 0, 0, 128, 64);

    for (int i = 0; i < 10; i++) {
        eui_widget_t *kid = eui_malloc(sizeof(eui_widget_t));
        eui_widget_init(kid, &scene_vt,
                         (int16_t)((i * 12) % 100), (int16_t)((i * 8) % 50), 16, 10);
        eui_widget_add_child(scene, kid);
    }

    eui_view_dispatcher_add(vd, 1, &scene->view);
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);
}
```

CMakeLists.txt (same pattern). requirements.cmake (no requirements).

- [ ] **Step 2: Create scene_view_demo migration**

```c
/* examples/cross/scene_view_demo/scene_view_demo.c */
#include "eui/eui.h"
#include "eui/eui_font_builtin.h"
#include "eui/eui_port_bootstrap.h"
#include <stdio.h>
#include <string.h>

#define W 128
#define H 64

enum { SCENE_MAIN = 1, SCENE_RED, SCENE_BLUE, SCENE_GREEN };
static eui_view_dispatcher_t *g_vd;
static eui_scene_manager_t g_sm;

typedef struct {
    const char *name;
    eui_color_t bg_color;
    void (*draw_pattern)(eui_canvas_t *c, int16_t x, int16_t y, int16_t w, int16_t h);
    eui_anim_type_t anim_in;
    eui_anim_type_t anim_out;
} scene_data_t;

static void draw_vertical(eui_canvas_t *c, int16_t x, int16_t y, int16_t w, int16_t h) {
    for (int16_t px = x + 4; px < x + w; px += 8)
        eui_canvas_draw_line(c, px, y + 2, px, y + h - 2);
}
static void draw_diagonal(eui_canvas_t *c, int16_t x, int16_t y, int16_t w, int16_t h) {
    for (int16_t px = x; px < x + w; px += 6)
        eui_canvas_draw_line(c, px, y, px + 6, y + h);
}
static void draw_horizontal(eui_canvas_t *c, int16_t x, int16_t y, int16_t w, int16_t h) {
    for (int16_t py = y + 2; py < y + h; py += 6)
        eui_canvas_draw_line(c, x + 2, py, x + w - 2, py);
}

static scene_data_t g_red_data   = { "Red Scene",   0, draw_vertical,   EUI_ANIM_SLIDE_LEFT, EUI_ANIM_SLIDE_RIGHT };
static scene_data_t g_blue_data  = { "Blue Scene",  0, draw_diagonal,  EUI_ANIM_SLIDE_UP,   EUI_ANIM_SLIDE_RIGHT };
static scene_data_t g_green_data = { "Green Scene", 0, draw_horizontal,EUI_ANIM_FADE,       EUI_ANIM_SLIDE_RIGHT };

typedef struct { const char *name; uint32_t scene_id; eui_anim_type_t anim; } menu_item_t;
static menu_item_t g_menu_items[] = {
    { "1. Red   (SLIDE_LEFT)", SCENE_RED,   EUI_ANIM_SLIDE_LEFT },
    { "2. Blue  (SLIDE_UP)",   SCENE_BLUE,  EUI_ANIM_SLIDE_UP   },
    { "3. Green (FADE)",       SCENE_GREEN, EUI_ANIM_FADE       },
};
#define MENU_COUNT 3
static int g_menu_sel = 0;

static void navigate_to(uint32_t scene_id, eui_anim_type_t anim) {
    if (g_sm.current >= 0 && g_sm.current < (int8_t)g_sm.count) {
        if (g_sm.scenes[g_sm.current].on_exit)
            g_sm.scenes[g_sm.current].on_exit(NULL);
    }
    eui_view_dispatcher_switch_to(g_vd, scene_id, anim);
    for (uint8_t i = 0; i < g_sm.count; i++) {
        if (g_sm.scenes[i].scene_id == scene_id) {
            if (g_sm.scenes[i].on_enter)
                g_sm.scenes[i].on_enter(NULL);
            g_sm.previous = g_sm.current;
            g_sm.current = (int8_t)i;
            return;
        }
    }
}

static void on_enter_main(void *ctx) { (void)ctx; printf("[SCENE_MGR] Enter Main\n"); }
static void on_exit_main(void *ctx)  { (void)ctx; printf("[SCENE_MGR] Exit Main\n"); }
static void on_enter_red(void *ctx)  { (void)ctx; printf("[SCENE_MGR] Enter Red\n"); }
static void on_exit_red(void *ctx)   { (void)ctx; printf("[SCENE_MGR] Exit Red\n"); }
static void on_enter_blue(void *ctx) { (void)ctx; printf("[SCENE_MGR] Enter Blue\n"); }
static void on_exit_blue(void *ctx)  { (void)ctx; printf("[SCENE_MGR] Exit Blue\n"); }
static void on_enter_green(void *ctx){ (void)ctx; printf("[SCENE_MGR] Enter Green\n"); }
static void on_exit_green(void *ctx) { (void)ctx; printf("[SCENE_MGR] Exit Green\n"); }

static bool main_view_handler(eui_view_event_t *evt, void *context) {
    eui_view_t *view = (eui_view_t*)context;
    switch (evt->type) {
    case EUI_VIEW_EVT_DRAW: {
        eui_canvas_t *c = evt->event.draw.canvas;
        eui_canvas_set_color(c, EUI_COLOR_WHITE);
        eui_canvas_set_font(c, &eui_font_builtin);
        eui_canvas_draw_str(c, view->area.x + 4, view->area.y + 2, "Scene+View Demo");
        eui_canvas_draw_line(c, view->area.x + 4, view->area.y + 12,
                                view->area.x + W - 4, view->area.y + 12);
        for (int i = 0; i < MENU_COUNT; i++) {
            int y = view->area.y + 18 + i * 14;
            if (i == g_menu_sel) {
                eui_canvas_fill_rect(c, view->area.x + 4, y, W - 8, 12);
                eui_canvas_set_color(c, EUI_COLOR_BLACK);
                eui_canvas_draw_str(c, view->area.x + 8, y + 2, g_menu_items[i].name);
                eui_canvas_set_color(c, EUI_COLOR_WHITE);
            } else {
                eui_canvas_draw_rect(c, view->area.x + 4, y, W - 8, 12);
                eui_canvas_draw_str(c, view->area.x + 8, y + 2, g_menu_items[i].name);
            }
        }
        eui_canvas_draw_str(c, view->area.x + 4, view->area.y + H - 10,
                            "UP/DOWN:Nav  OK:Go");
        return true;
    }
    case EUI_VIEW_EVT_INPUT: {
        const eui_event_t *e = evt->event.input.input;
        if (e->type == EUI_EVT_KEY_PRESS) {
            if (e->data.key == EUI_KEY_UP && g_menu_sel > 0) {
                g_menu_sel--; eui_view_mark_dirty(view); return true;
            }
            if (e->data.key == EUI_KEY_DOWN && g_menu_sel < MENU_COUNT - 1) {
                g_menu_sel++; eui_view_mark_dirty(view); return true;
            }
            if (e->data.key == EUI_KEY_OK) {
                menu_item_t *item = &g_menu_items[g_menu_sel];
                navigate_to(item->scene_id, item->anim);
                return true;
            }
        }
        return false;
    }
    case EUI_VIEW_EVT_ENTER: printf("[VIEW] Enter Main\n"); return true;
    case EUI_VIEW_EVT_EXIT:  printf("[VIEW] Exit Main\n");  return true;
    default: return false;
    }
}

static bool detail_view_handler(eui_view_event_t *evt, void *context) {
    eui_view_t *view = (eui_view_t*)context;
    scene_data_t *data = (scene_data_t*)view->model;
    if (!data) return false;
    switch (evt->type) {
    case EUI_VIEW_EVT_DRAW: {
        eui_canvas_t *c = evt->event.draw.canvas;
        eui_canvas_set_color(c, data->bg_color);
        eui_canvas_fill_rect(c, view->area.x, view->area.y,
                                view->area.w, view->area.h);

        int16_t rx = view->area.x + (int16_t)(W / 2) - 16;
        int16_t ry = view->area.y + (int16_t)(H / 2) - 12;
        eui_canvas_set_color(c, eui_color_from_rgb(255, 255, 0));
        eui_canvas_fill_rect(c, rx, ry, 32, 24);

        eui_canvas_set_color(c, EUI_COLOR_WHITE);
        eui_canvas_set_font(c, &eui_font_builtin);
        eui_canvas_draw_str(c, rx + 4, ry + 6, "OK");

        eui_canvas_draw_str(c, view->area.x + 4, view->area.y + H - 10, "ESC/OK:Back");
        return true;
    }
    case EUI_VIEW_EVT_INPUT: {
        const eui_event_t *e = evt->event.input.input;
        if (e->type == EUI_EVT_KEY_PRESS &&
            (e->data.key == EUI_KEY_BACK || e->data.key == EUI_KEY_OK)) {
            navigate_to(SCENE_MAIN, data->anim_out);
            return true;
        }
        return false;
    }
    case EUI_VIEW_EVT_ENTER: printf("[VIEW] Enter %s\n", data->name); return true;
    case EUI_VIEW_EVT_EXIT:  printf("[VIEW] Exit %s\n", data->name);  return true;
    default: return false;
    }
}

void eui_example_setup(const eui_example_config_t *cfg) {
    (void)cfg;

    g_red_data.bg_color   = eui_color_from_rgb(180, 20, 20);
    g_blue_data.bg_color  = eui_color_from_rgb(20, 40, 180);
    g_green_data.bg_color = eui_color_from_rgb(20, 140, 20);

    g_vd = eui_get_view_dispatcher();

    eui_view_t main_view, red_view, blue_view, green_view;

    eui_view_init(&main_view, main_view_handler, &main_view);
    main_view.area = (eui_rect_t){ 0, 0, W, H };

    eui_view_init(&red_view, detail_view_handler, &red_view);
    red_view.area = (eui_rect_t){ 0, 0, W, H };
    red_view.model = &g_red_data;

    eui_view_init(&blue_view, detail_view_handler, &blue_view);
    blue_view.area = (eui_rect_t){ 0, 0, W, H };
    blue_view.model = &g_blue_data;

    eui_view_init(&green_view, detail_view_handler, &green_view);
    green_view.area = (eui_rect_t){ 0, 0, W, H };
    green_view.model = &g_green_data;

    eui_view_dispatcher_add(g_vd, SCENE_MAIN,  &main_view);
    eui_view_dispatcher_add(g_vd, SCENE_RED,   &red_view);
    eui_view_dispatcher_add(g_vd, SCENE_BLUE,  &blue_view);
    eui_view_dispatcher_add(g_vd, SCENE_GREEN, &green_view);

    eui_scene_t scenes[4] = {
        { SCENE_MAIN,  &main_view,  on_enter_main,  on_exit_main,  NULL },
        { SCENE_RED,   &red_view,   on_enter_red,   on_exit_red,   NULL },
        { SCENE_BLUE,  &blue_view,  on_enter_blue,  on_exit_blue,  NULL },
        { SCENE_GREEN, &green_view, on_enter_green, on_exit_green, NULL },
    };
    eui_scene_manager_register(&g_sm, scenes, 4);

    navigate_to(SCENE_MAIN, EUI_ANIM_NONE);
}
```

- [ ] **Step 3: Update examples/cross/CMakeLists.txt**

```cmake
add_subdirectory(benchmark)
add_subdirectory(scene_view_demo)
```

- [ ] **Step 4: Build and verify**

```bash
cmake -B build -DEUI_CONFIG_PROFILE=configs/pc/raylib_128x64_1bpp.cmake -DEUI_BUILD_CROSS_EXAMPLES=ON -DEUI_BUILD_EXAMPLES=OFF -DEUI_BUILD_TESTS=OFF
cmake --build build -j$(nproc)
```

- [ ] **Step 5: Commit**

```bash
git add examples/cross/
git commit -m "feat(cross): migrate benchmark and scene_view_demo"
```

---

### Task 11: Migrate color_demo to cross/ (with requirements)

**Files:**
- Create: `examples/cross/color_demo/{color_demo.c,CMakeLists.txt,requirements.cmake}`
- Modify: `examples/cross/CMakeLists.txt`

- [ ] **Step 1: Create color_demo.c**

```c
/* examples/cross/color_demo/color_demo.c */
#include "eui/eui.h"
#include "eui/eui_font_builtin.h"
#include "eui/eui_port_bootstrap.h"

static void color_draw(eui_widget_t *w, eui_canvas_t *c) {
    int16_t cy;
    int16_t W = w->area.w;
    int16_t H = w->area.h;

    /* Background: dark blue gradient bands */
    for (cy = 0; cy < H; cy += 8) {
        uint8_t b = (uint8_t)(cy * 60 / H);
        eui_canvas_set_color(c, eui_color_from_rgb(0, 0, b));
        eui_canvas_fill_rect(c, 0, cy, W, 8);
    }

    /* Color bars */
    for (cy = 0; cy < W; cy++) {
        eui_canvas_set_color(c, eui_color_from_rgb(cy * 255 / W, 0, 0));
        eui_canvas_draw_dot(c, cy, 6);
        eui_canvas_set_color(c, eui_color_from_rgb(0, cy * 255 / W, 0));
        eui_canvas_draw_dot(c, cy, 14);
        eui_canvas_set_color(c, eui_color_from_rgb(0, 0, cy * 255 / W));
        eui_canvas_draw_dot(c, cy, 22);
        eui_canvas_set_color(c, eui_color_from_rgb(cy * 255 / W, cy * 255 / W, cy * 255 / W));
        eui_canvas_draw_dot(c, cy, 30);
    }

    /* Colored shapes */
    eui_canvas_set_color(c, eui_color_from_rgb(255, 80, 80));
    eui_canvas_fill_rect(c, 6, 44, 50, 50);

    eui_canvas_set_color(c, eui_color_from_rgb(80, 255, 80));
    eui_canvas_fill_circle(c, 125, 69, 26);

    eui_canvas_set_color(c, eui_color_from_rgb(80, 80, 255));
    eui_canvas_draw_triangle(c, 195, 104, 220, 48, 234, 104);

    eui_canvas_set_color(c, eui_color_from_rgb(255, 200, 0));
    eui_canvas_fill_round_rect(c, 6, 110, 90, 36, 6);

    eui_canvas_set_color(c, eui_color_from_rgb(255, 0, 255));
    eui_canvas_draw_rect(c, 120, 104, 48, 48);

    /* Colored text */
    eui_canvas_set_font(c, &eui_font_builtin);

    eui_canvas_set_color(c, eui_color_from_rgb(255, 255, 0));
    eui_canvas_draw_str(c, 6, 164, "Color Demo");

    eui_canvas_set_color(c, eui_color_from_rgb(0, 255, 255));
    eui_canvas_draw_str(c, 6, 178, "16-bit RGB565");

    eui_canvas_set_color(c, eui_color_from_rgb(180, 120, 255));
    eui_canvas_draw_str(c, 6, 192, "Hello Color!");

    /* Heart icon in pink */
    eui_canvas_set_color(c, eui_color_from_rgb(255, 100, 150));
    static const uint8_t heart_icon[] = {
        0x00, 0x00, 0x00,
        0x03, 0x00, 0xC0,
        0x0F, 0x00, 0xF0,
        0xFF, 0xFF, 0xFF,
        0x7F, 0xFF, 0xFE,
        0x3F, 0xFF, 0xFC,
        0x1F, 0xFF, 0xF8,
        0x0F, 0xFF, 0xF0,
        0x00, 0x00, 0x00,
    };
    eui_canvas_draw_xbm(c, 108, 210, 24, 9, heart_icon);
}

static bool color_input(eui_widget_t *w, const eui_event_t *e) {
    (void)w; (void)e;
    return false;
}

static eui_widget_vtable_t color_vt = { .draw = color_draw, .input = color_input };

void eui_example_setup(const eui_example_config_t *cfg) {
    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();

    eui_widget_t *widget = eui_malloc(sizeof(eui_widget_t));
    eui_widget_init(widget, &color_vt, 0, 0, cfg->display_width, cfg->display_height);

    eui_view_dispatcher_add(vd, 1, &widget->view);
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);
}
```

- [ ] **Step 2: Create CMakeLists.txt**

```cmake
# examples/cross/color_demo/CMakeLists.txt
add_executable(color_demo color_demo.c)
target_include_directories(color_demo PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(color_demo PRIVATE eui ${BOOTSTRAP_LIB})
```

- [ ] **Step 3: Create requirements.cmake**

```cmake
# examples/cross/color_demo/requirements.cmake
set(EXAMPLE_REQUIRES_COLOR_DEPTH_MIN 16)
set(EXAMPLE_REQUIRES_WIDTH_MIN       240)
set(EXAMPLE_REQUIRES_HEIGHT_MIN      240)
```

- [ ] **Step 4: Update examples/cross/CMakeLists.txt** — add `add_subdirectory(color_demo)`

- [ ] **Step 5: Build with both profiles to verify constraint filtering**

```bash
# Should build (16bpp, 240x240):
cmake -B build_16bpp -DEUI_CONFIG_PROFILE=configs/pc/raylib_240x240_16bpp.cmake -DEUI_BUILD_CROSS_EXAMPLES=ON -DEUI_BUILD_EXAMPLES=OFF -DEUI_BUILD_TESTS=OFF
cmake --build build_16bpp -j$(nproc)

# Should skip color_demo (1bpp incompatible):
cmake -B build_1bpp -DEUI_CONFIG_PROFILE=configs/pc/raylib_128x64_1bpp.cmake -DEUI_BUILD_CROSS_EXAMPLES=ON -DEUI_BUILD_EXAMPLES=OFF -DEUI_BUILD_TESTS=OFF
cmake --build build_1bpp -j$(nproc)
```

- [ ] **Step 6: Commit**

```bash
git add examples/cross/color_demo/ examples/cross/CMakeLists.txt
git commit -m "feat(cross): migrate color_demo with 16bpp/240x240 requirements"
```

---

### Task 12: Migrate amiibo_demo to cross/ (with icons)

**Files:**
- Create: `examples/cross/amiibo_demo/{amiibo_demo.c,CMakeLists.txt,requirements.cmake}`
- Copy: `examples/amiibo_font.h` → `examples/cross/amiibo_demo/amiibo_font.h`
- Copy: `examples/amiibo_icons.h` → `examples/cross/amiibo_demo/amiibo_icons.h`
- Modify: `examples/cross/CMakeLists.txt`

- [ ] **Step 1: Copy icon/font assets**

```bash
mkdir -p examples/cross/amiibo_demo
cp examples/amiibo_font.h examples/cross/amiibo_demo/amiibo_font.h
cp examples/amiibo_icons.h examples/cross/amiibo_demo/amiibo_icons.h
```

- [ ] **Step 2: Create amiibo_demo.c**

Read `examples/amiibo_demo.c`. The migration:
- Keep all struct definitions: `item_t`, `carousel_t`, `amiibo_detail_t`, `carousel_dir_t`
- Keep all function implementations: `carousel_init`, `carousel_select`, `carousel_update`, `carousel_draw`, `carousel_input`, `navigate_to`, all view handlers, `scene_enter`, `scene_exit`
- Global state (`g_vd`, `g_sm`, `g_app_carousel`, `g_amiibo_carousel`, `g_detail_index`, `g_detail_scroll`, `g_detail_max_scroll`, `app_items`, `amiibo_data`, `amiibo_items`, `amiibo_icons`) — declare as `static` at file scope
- `eui_example_setup()` contains: carousel init, view creation/registration, scene manager setup, `eui_anim_init()`, initial `navigate_to()`
- The `main()` contains a tick loop with carousel_update and animation dirty-marking. This must move into the port bootstrap's tick callback. Option: register a custom tick via `eui_set_tick_callback()` override or use a view handler that updates per-frame.
- **Simplification**: Since the cross bootstrap handles the main loop, move the per-frame update (carousel_update + dirty marking) into a separate function and call it from `eui_example_setup` via registering a tick callback that wraps it.

The amiibo_demo has per-frame animation updates in its main loop. Cross examples can register a custom tick callback. Let's use `eui_set_tick_callback()` within `eui_example_setup()` to handle this:

```c
/* examples/cross/amiibo_demo/amiibo_demo.c */
#include "eui/eui.h"
#include "eui/eui_font_builtin.h"
#include "eui/eui_port_bootstrap.h"
#include "amiibo_font.h"
#include "amiibo_icons.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#define W 240
#define H 240

/* ─── Carousel direction ─── */
typedef enum { CAROUSEL_HORIZONTAL, CAROUSEL_VERTICAL } carousel_dir_t;

/* ─── Item type ─── */
typedef struct {
    const char *label;
    const eui_bitmap_t *icon;
    void *user_data;
} item_t;

/* ─── Carousel state ─── */
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

static void carousel_init(carousel_t *c, const item_t *items, uint8_t count,
                           carousel_dir_t dir, uint16_t item_step, uint8_t icon_size) {
    c->items = items;
    c->count = count;
    c->selected = 0;
    c->dir = dir;
    c->item_step = item_step;
    c->icon_size = icon_size;
    c->scroll_pos = MC_FP_C(0);
    c->target_pos = MC_FP_C(0);
    memset(&c->spring_state, 0, sizeof(c->spring_state));
    c->spring_params.stiffness = MC_FP_C(180);
    c->spring_params.damping = MC_FP_C(12);
    c->spring_params.mass = MC_FP_C(1);
    c->target_pos = MC_REAL_FROM_INT(c->selected * c->item_step);
    c->scroll_pos = c->target_pos;
}

static void carousel_select(carousel_t *c, int8_t idx) {
    if (idx < 0 || idx >= (int8_t)c->count || idx == c->selected) return;
    c->selected = idx;
    c->target_pos = MC_REAL_FROM_INT(c->selected * c->item_step);
}

static void carousel_update(carousel_t *c, uint32_t dt_ms) {
    mc_real_t dt = MC_FP_C((float)dt_ms / 1000.0f);
    mc_spring_step(&c->scroll_pos, &c->spring_state, &c->spring_params, c->target_pos, dt);
}

static void carousel_draw(carousel_t *c, eui_canvas_t *canvas, eui_rect_t area) {
    int16_t center_x = area.x + (int16_t)(area.w / 2);
    int16_t center_y = area.y + (int16_t)(area.h / 2);
    int16_t screen_center = (c->dir == CAROUSEL_HORIZONTAL) ? center_x : center_y;
    int16_t perp_center = (c->dir == CAROUSEL_HORIZONTAL) ? center_y : center_x;
    int16_t half_step = (int16_t)(c->item_step / 2);
    int16_t half_size = (int16_t)(c->icon_size / 2);
    int16_t center_offset = screen_center - half_step;
    float scroll_f = MC_REAL_TO_FLOAT(c->scroll_pos);

    for (uint8_t i = 0; i < c->count; i++) {
        int16_t item_left = (int16_t)(center_offset + (float)(i * c->item_step) - scroll_f);
        int16_t draw_pos = area.x + item_left;
        int16_t perp_pos = perp_center;

        if (draw_pos + c->item_step < area.x || draw_pos > area.x + (int16_t)area.w)
            continue;

        int16_t ix, iy;
        if (c->dir == CAROUSEL_HORIZONTAL) {
            ix = draw_pos;
            iy = perp_pos - half_size - 10;
        } else {
            ix = perp_pos - half_size;
            iy = draw_pos;
        }

        if (i == c->selected) {
            eui_color_t gold = eui_color_from_rgb(255, 200, 0);
            eui_canvas_set_color(canvas, gold);
            if (c->dir == CAROUSEL_HORIZONTAL) {
                eui_canvas_draw_round_rect(canvas, ix - 4, iy - 4,
                    c->item_step, c->icon_size + 28, 6);
            } else {
                eui_canvas_draw_round_rect(canvas, ix - 4, iy - 4,
                    c->icon_size + 28, c->item_step, 6);
            }
        }

        if (c->items[i].icon) {
            eui_canvas_draw_bitmap(canvas, ix, iy, c->items[i].icon);
        }

        eui_canvas_set_color(canvas, EUI_COLOR_WHITE);
        eui_canvas_set_font(canvas, &wqy13_font);
        if (c->items[i].label) {
            uint16_t lw = eui_canvas_str_width(canvas, c->items[i].label);
            if (c->dir == CAROUSEL_HORIZONTAL) {
                eui_canvas_draw_str(canvas, draw_pos + half_step - (int16_t)(lw / 2),
                                    perp_pos + half_size + 2, c->items[i].label);
            } else {
                eui_canvas_draw_str(canvas, perp_pos + half_size + 4,
                                    draw_pos + half_step - 6, c->items[i].label);
            }
        }
    }
}

static bool carousel_input(carousel_t *c, const eui_event_t *e) {
    if (e->type != EUI_EVT_KEY_PRESS) return false;
    if (c->dir == CAROUSEL_HORIZONTAL) {
        if (e->data.key == EUI_KEY_LEFT && c->selected > 0) {
            carousel_select(c, c->selected - 1); return true;
        }
        if (e->data.key == EUI_KEY_RIGHT && c->selected < (int8_t)(c->count - 1)) {
            carousel_select(c, c->selected + 1); return true;
        }
    } else {
        if (e->data.key == EUI_KEY_UP && c->selected > 0) {
            carousel_select(c, c->selected - 1); return true;
        }
        if (e->data.key == EUI_KEY_DOWN && c->selected < (int8_t)(c->count - 1)) {
            carousel_select(c, c->selected + 1); return true;
        }
    }
    return false;
}

/* ─── Amiibo icon array ─── */
static const eui_bitmap_t* amiibo_icons[] = {
    &g_icon_mario, &g_icon_link, &g_icon_zelda, &g_icon_pikachu,
    &g_icon_kirby, &g_icon_samus, &g_icon_yoshi, &g_icon_dk,
};

/* ─── Amiibo detail data ─── */
typedef struct {
    const char *name;
    const char *series;
    const char *drops[6];
    uint8_t drop_count;
} amiibo_detail_t;

static amiibo_detail_t amiibo_data[] = {
    { "马里奥", "超级马里奥系列", {"超级蘑菇", "火焰花", "无敌星", "超级铃铛"}, 4 },
    { "林克", "塞尔达传说系列", {"大师剑", "海利亚盾", "滑翔伞", "时之笛"}, 4 },
    { "塞尔达", "塞尔达传说系列", {"光之箭", "时之笛", "三角力量", "塞尔达盾"}, 4 },
    { "皮卡丘", "宝可梦系列", {"电气球", "雷之石", "十万伏特", "电珠"}, 4 },
    { "卡比", "星之卡比系列", {"复制能力", "星星杖", "番茄", "M番茄"}, 4 },
    { "萨姆斯", "银河战士系列", {"能量罐", "导弹", "超炸", "加速器"}, 4 },
    { "耀西", "耀西系列", {"耀西蛋", "水果", "快乐花", "星星"}, 4 },
    { "森喜刚", "大金刚系列", {"大金刚桶", "香蕉", "矿车", "DK徽章"}, 4 },
};

/* ─── Navigation ─── */
enum { SCENE_APP_LIST = 1, SCENE_AMIIBO_LIST, SCENE_AMIIBO_DETAIL, SCENE_NFC, SCENE_SETTINGS };
static eui_view_dispatcher_t *g_vd;
static eui_scene_manager_t g_sm;
static carousel_t g_app_carousel, g_amiibo_carousel;
static int8_t g_detail_index = 0;
static int16_t g_detail_scroll = 0;
static int16_t g_detail_max_scroll = 0;

static void navigate_to(uint32_t scene_id, eui_anim_type_t anim) {
    if (g_sm.current >= 0 && g_sm.current < (int8_t)g_sm.count) {
        if (g_sm.scenes[g_sm.current].on_exit)
            g_sm.scenes[g_sm.current].on_exit(NULL);
    }
    eui_view_dispatcher_switch_to(g_vd, scene_id, anim);
    for (uint8_t i = 0; i < g_sm.count; i++) {
        if (g_sm.scenes[i].scene_id == scene_id) {
            if (g_sm.scenes[i].on_enter)
                g_sm.scenes[i].on_enter(NULL);
            g_sm.previous = g_sm.current;
            g_sm.current = (int8_t)i;
            return;
        }
    }
}

/* ─── App list items ─── */
static item_t app_items[] = {
    { "Amiibo", &g_icon_amiibo_app, NULL },
    { "NFC",    &g_icon_nfc,        NULL },
    { "设置",    &g_icon_settings,   NULL },
};

/* ─── Amiibo list items ─── */
static item_t amiibo_items[8];

/* ─── View handlers (same as legacy, with view area references from eui_view_t*) ─── */
static bool app_list_handler(eui_view_event_t *evt, void *context) {
    eui_view_t *view = (eui_view_t*)context;
    switch (evt->type) {
    case EUI_VIEW_EVT_DRAW: {
        eui_canvas_t *c = evt->event.draw.canvas;
        eui_canvas_set_color(c, eui_color_from_rgb(26, 26, 46));
        eui_canvas_fill_rect(c, view->area.x, view->area.y, view->area.w, view->area.h);
        eui_canvas_set_color(c, eui_color_from_rgb(200, 200, 220));
        eui_canvas_set_font(c, &wqy13_font);
        const char *title = "应用列表";
        uint16_t tw = eui_canvas_str_width(c, title);
        uint8_t fh = eui_canvas_font_height(c);
        eui_canvas_draw_str(c, view->area.x + (int16_t)(view->area.w / 2) - (int16_t)(tw / 2),
                             view->area.y + fh, title);
        carousel_draw(&g_app_carousel, c, (eui_rect_t){ 0, 30, 240, 200 });
        return true;
    }
    case EUI_VIEW_EVT_INPUT: {
        const eui_event_t *e = evt->event.input.input;
        if (carousel_input(&g_app_carousel, e)) {
            eui_view_mark_dirty(view); return true;
        }
        if (e->type == EUI_EVT_KEY_PRESS && e->data.key == EUI_KEY_OK) {
            switch (g_app_carousel.selected) {
                case 0: navigate_to(SCENE_AMIIBO_LIST, EUI_ANIM_SLIDE_LEFT); break;
                case 1: navigate_to(SCENE_NFC, EUI_ANIM_SLIDE_LEFT); break;
                case 2: navigate_to(SCENE_SETTINGS, EUI_ANIM_SLIDE_LEFT); break;
            }
            return true;
        }
        return false;
    }
    default: return false;
    }
}

static bool amiibo_list_handler(eui_view_event_t *evt, void *context) {
    eui_view_t *view = (eui_view_t*)context;
    switch (evt->type) {
    case EUI_VIEW_EVT_DRAW: {
        eui_canvas_t *c = evt->event.draw.canvas;
        eui_canvas_set_color(c, eui_color_from_rgb(26, 26, 46));
        eui_canvas_fill_rect(c, view->area.x, view->area.y, view->area.w, view->area.h);
        eui_canvas_set_color(c, eui_color_from_rgb(200, 200, 220));
        eui_canvas_set_font(c, &wqy13_font);
        const char *title = "Amiibo";
        uint16_t tw = eui_canvas_str_width(c, title);
        uint8_t fh = eui_canvas_font_height(c);
        eui_canvas_draw_str(c, view->area.x + (int16_t)(view->area.w / 2) - (int16_t)(tw / 2),
                             view->area.y + fh, title);
        carousel_draw(&g_amiibo_carousel, c, (eui_rect_t){ 0, 30, 240, 200 });
        return true;
    }
    case EUI_VIEW_EVT_INPUT: {
        const eui_event_t *e = evt->event.input.input;
        if (carousel_input(&g_amiibo_carousel, e)) {
            eui_view_mark_dirty(view); return true;
        }
        if (e->type == EUI_EVT_KEY_PRESS && e->data.key == EUI_KEY_OK) {
            g_detail_index = (int8_t)g_amiibo_carousel.selected;
            g_detail_scroll = 0;
            navigate_to(SCENE_AMIIBO_DETAIL, EUI_ANIM_SLIDE_LEFT);
            return true;
        }
        if (e->type == EUI_EVT_KEY_PRESS && e->data.key == EUI_KEY_BACK) {
            navigate_to(SCENE_APP_LIST, EUI_ANIM_SLIDE_RIGHT);
            return true;
        }
        return false;
    }
    default: return false;
    }
}

static bool detail_handler(eui_view_event_t *evt, void *context) {
    eui_view_t *view = (eui_view_t*)context;
    switch (evt->type) {
    case EUI_VIEW_EVT_DRAW: {
        eui_canvas_t *c = evt->event.draw.canvas;
        eui_canvas_set_color(c, eui_color_from_rgb(26, 26, 46));
        eui_canvas_fill_rect(c, view->area.x, view->area.y, view->area.w, view->area.h);

        int16_t y = view->area.y + 10 - g_detail_scroll;
        amiibo_detail_t *d = &amiibo_data[g_detail_index];

        eui_canvas_set_color(c, eui_color_from_rgb(255, 220, 80));
        eui_canvas_set_font(c, &wqy13_font);
        uint16_t tw_name = eui_canvas_str_width(c, d->name);
        eui_canvas_draw_str(c, view->area.x + (int16_t)(view->area.w / 2) - (int16_t)(tw_name / 2), y, d->name);
        y += 18;

        eui_canvas_draw_bitmap(c, view->area.x + 60, y, amiibo_icons[g_detail_index]);
        y += 126;

        eui_canvas_set_color(c, eui_color_from_rgb(180, 180, 200));
        eui_canvas_set_font(c, &wqy13_font);
        eui_canvas_draw_str(c, view->area.x + 10, y, d->series);
        y += 18;

        eui_canvas_set_color(c, eui_color_from_rgb(255, 200, 100));
        eui_canvas_draw_str(c, view->area.x + 10, y, "游戏掉落物品");
        y += 18;

        eui_canvas_set_color(c, eui_color_from_rgb(200, 220, 255));
        for (uint8_t i = 0; i < d->drop_count; i++) {
            eui_canvas_draw_str(c, view->area.x + 20, y, d->drops[i]);
            y += 16;
        }

        g_detail_max_scroll = (y > (int16_t)(view->area.y + (int16_t)view->area.h - 10))
                              ? (y - (view->area.y + (int16_t)view->area.h - 10)) : 0;

        eui_canvas_set_color(c, eui_color_from_rgb(120, 120, 140));
        eui_canvas_draw_str(c, view->area.x + 4, view->area.y + view->area.h - 12, "BACK: 返回");
        return true;
    }
    case EUI_VIEW_EVT_INPUT: {
        const eui_event_t *e = evt->event.input.input;
        if (e->type == EUI_EVT_KEY_PRESS) {
            if (e->data.key == EUI_KEY_BACK) {
                navigate_to(SCENE_AMIIBO_LIST, EUI_ANIM_SLIDE_RIGHT);
                return true;
            }
            if (e->data.key == EUI_KEY_UP && g_detail_scroll > 0) {
                g_detail_scroll -= 12;
                if (g_detail_scroll < 0) g_detail_scroll = 0;
                eui_view_mark_dirty(view); return true;
            }
            if (e->data.key == EUI_KEY_DOWN && g_detail_scroll < g_detail_max_scroll) {
                g_detail_scroll += 12;
                eui_view_mark_dirty(view); return true;
            }
        }
        return false;
    }
    default: return false;
    }
}

static bool placeholder_handler(eui_view_event_t *evt, void *context) {
    eui_view_t *view = (eui_view_t*)context;
    switch (evt->type) {
    case EUI_VIEW_EVT_DRAW: {
        eui_canvas_t *c = evt->event.draw.canvas;
        eui_canvas_set_color(c, eui_color_from_rgb(26, 26, 46));
        eui_canvas_fill_rect(c, view->area.x, view->area.y, view->area.w, view->area.h);
        eui_canvas_set_color(c, eui_color_from_rgb(200, 200, 220));
        eui_canvas_set_font(c, &wqy13_font);
        const char *title = (view->model) ? (const char*)view->model : "功能";
        uint16_t tw = eui_canvas_str_width(c, title);
        eui_canvas_draw_str(c, view->area.x + (int16_t)(view->area.w / 2) - (int16_t)(tw / 2), view->area.y + 100, title);
        eui_canvas_set_color(c, eui_color_from_rgb(120, 120, 140));
        eui_canvas_draw_str(c, view->area.x + 4, view->area.y + view->area.h - 12, "BACK: 返回");
        return true;
    }
    case EUI_VIEW_EVT_INPUT: {
        const eui_event_t *e = evt->event.input.input;
        if (e->type == EUI_EVT_KEY_PRESS && e->data.key == EUI_KEY_BACK) {
            navigate_to(SCENE_APP_LIST, EUI_ANIM_SLIDE_RIGHT);
            return true;
        }
        return false;
    }
    default: return false;
    }
}

static void scene_enter(void *ctx) { (void)ctx; printf("[SCENE] Enter\n"); }
static void scene_exit(void *ctx)  { (void)ctx; printf("[SCENE] Exit\n"); }

/* Views referenced by tick callback */
static eui_view_t *g_app_view;
static eui_view_t *g_amiibo_view;

/* Per-frame animation update for both carousels */
static void amiibo_animate(uint32_t dt) {
    carousel_update(&g_app_carousel, dt);
    carousel_update(&g_amiibo_carousel, dt);

    float app_vel = MC_REAL_TO_FLOAT(g_app_carousel.spring_state.velocity);
    float app_err = MC_REAL_TO_FLOAT(g_app_carousel.scroll_pos - g_app_carousel.target_pos);
    if (fabsf(app_vel) > 0.5f || fabsf(app_err) > 0.5f) {
        eui_view_mark_dirty(g_app_view);
    } else if (fabsf(app_err) > 0.01f) {
        g_app_carousel.scroll_pos = g_app_carousel.target_pos;
        g_app_carousel.spring_state.velocity = 0;
        eui_view_mark_dirty(g_app_view);
    }

    float ami_vel = MC_REAL_TO_FLOAT(g_amiibo_carousel.spring_state.velocity);
    float ami_err = MC_REAL_TO_FLOAT(g_amiibo_carousel.scroll_pos - g_amiibo_carousel.target_pos);
    if (fabsf(ami_vel) > 0.5f || fabsf(ami_err) > 0.5f) {
        eui_view_mark_dirty(g_amiibo_view);
    } else if (fabsf(ami_err) > 0.01f) {
        g_amiibo_carousel.scroll_pos = g_amiibo_carousel.target_pos;
        g_amiibo_carousel.spring_state.velocity = 0;
        eui_view_mark_dirty(g_amiibo_view);
    }
}

static uint32_t g_last_anim_tick = 0;

static uint32_t amiibo_tick(void) {
    /* In a real cross-platform example, replace with platform timer.
     * For raylib: GetTime() returns seconds as double. */
#ifdef __has_include
#if __has_include(<raylib.h>)
    uint32_t now = (uint32_t)(GetTime() * 1000.0);
#else
    uint32_t now = 0;
#endif
#else
    uint32_t now = 0;
#endif
    if (g_last_anim_tick == 0) g_last_anim_tick = now;
    uint32_t dt = now - g_last_anim_tick;
    g_last_anim_tick = now;

    amiibo_animate(dt);
    return now;
}

void eui_example_setup(const eui_example_config_t *cfg) {
    (void)cfg;
    g_vd = eui_get_view_dispatcher();

    /* Init carousels */
    carousel_init(&g_app_carousel, app_items, 3, CAROUSEL_HORIZONTAL, 140, 120);

    for (uint8_t i = 0; i < 8; i++) {
        amiibo_items[i].label = amiibo_data[i].name;
        amiibo_items[i].icon = amiibo_icons[i];
        amiibo_items[i].user_data = NULL;
    }
    carousel_init(&g_amiibo_carousel, amiibo_items, 8, CAROUSEL_HORIZONTAL, 140, 120);

    /* Create views on heap (owned by view dispatcher lifecycle) */
    eui_view_t *app_view = eui_malloc(sizeof(eui_view_t));
    eui_view_t *amiibo_view = eui_malloc(sizeof(eui_view_t));
    eui_view_t *detail_view = eui_malloc(sizeof(eui_view_t));
    eui_view_t *nfc_view = eui_malloc(sizeof(eui_view_t));
    eui_view_t *settings_view = eui_malloc(sizeof(eui_view_t));

    eui_view_init(app_view, app_list_handler, app_view);
    app_view->area = (eui_rect_t){ 0, 0, W, H };

    eui_view_init(amiibo_view, amiibo_list_handler, amiibo_view);
    amiibo_view->area = (eui_rect_t){ 0, 0, W, H };

    eui_view_init(detail_view, detail_handler, detail_view);
    detail_view->area = (eui_rect_t){ 0, 0, W, H };

    eui_view_init(nfc_view, placeholder_handler, nfc_view);
    nfc_view->area = (eui_rect_t){ 0, 0, W, H };
    nfc_view->model = (void*)"NFC 读写器";

    eui_view_init(settings_view, placeholder_handler, settings_view);
    settings_view->area = (eui_rect_t){ 0, 0, W, H };
    settings_view->model = (void*)"设置";

    /* Register views */
    eui_view_dispatcher_add(g_vd, SCENE_APP_LIST, app_view);
    eui_view_dispatcher_add(g_vd, SCENE_AMIIBO_LIST, amiibo_view);
    eui_view_dispatcher_add(g_vd, SCENE_AMIIBO_DETAIL, detail_view);
    eui_view_dispatcher_add(g_vd, SCENE_NFC, nfc_view);
    eui_view_dispatcher_add(g_vd, SCENE_SETTINGS, settings_view);

    /* Register scenes */
    eui_scene_t scenes[] = {
        { SCENE_APP_LIST, app_view, scene_enter, scene_exit, NULL },
        { SCENE_AMIIBO_LIST, amiibo_view, scene_enter, scene_exit, NULL },
        { SCENE_AMIIBO_DETAIL, detail_view, scene_enter, scene_exit, NULL },
        { SCENE_NFC, nfc_view, scene_enter, scene_exit, NULL },
        { SCENE_SETTINGS, settings_view, scene_enter, scene_exit, NULL },
    };
    eui_scene_manager_register(&g_sm, scenes, 5);

    eui_anim_init();
    navigate_to(SCENE_APP_LIST, EUI_ANIM_NONE);

    /* Setup tick for carousel animation */
    eui_set_tick_callback(amiibo_tick);
    g_app_view = app_view;
    g_amiibo_view = amiibo_view;
}
```

- [ ] **Step 3: Create CMakeLists.txt**

```cmake
# examples/cross/amiibo_demo/CMakeLists.txt
add_executable(amiibo_demo amiibo_demo.c)
target_include_directories(amiibo_demo PRIVATE
    ${CMAKE_BINARY_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(amiibo_demo PRIVATE eui ${BOOTSTRAP_LIB})
```

- [ ] **Step 4: Create requirements.cmake**

```cmake
# examples/cross/amiibo_demo/requirements.cmake
set(EXAMPLE_REQUIRES_COLOR_DEPTH_MIN 16)
set(EXAMPLE_REQUIRES_WIDTH_MIN       240)
set(EXAMPLE_REQUIRES_HEIGHT_MIN      240)
```

- [ ] **Step 5: Update examples/cross/CMakeLists.txt** — add `add_subdirectory(amiibo_demo)`

- [ ] **Step 6: Build and verify**

```bash
cmake -B build -DEUI_CONFIG_PROFILE=configs/pc/raylib_240x240_16bpp.cmake -DEUI_BUILD_CROSS_EXAMPLES=ON -DEUI_BUILD_EXAMPLES=OFF -DEUI_BUILD_TESTS=OFF
cmake --build build -j$(nproc)
```

- [ ] **Step 7: Commit**

```bash
git add examples/cross/amiibo_demo/ examples/cross/CMakeLists.txt
git commit -m "feat(cross): migrate amiibo_demo with 16bpp/240x240 requirements and assets"
```

---

### Task 13: Add requirements.cmake constraint checking to build system

**Files:**
- Modify: `examples/cross/CMakeLists.txt`

Currently, `examples/cross/CMakeLists.txt` just does `add_subdirectory()` for each example. Add constraint checking that reads `requirements.cmake` and skips incompatible examples.

- [ ] **Step 1: Replace examples/cross/CMakeLists.txt**

```cmake
# examples/cross/CMakeLists.txt
# Cross-platform examples — constraint-checked against active config profile

function(add_cross_example name)
    set(example_dir "${CMAKE_CURRENT_SOURCE_DIR}/${name}")

    # Skip if requirements not met
    if(EXISTS "${example_dir}/requirements.cmake")
        include("${example_dir}/requirements.cmake")

        if(DEFINED EXAMPLE_REQUIRES_COLOR_DEPTH_MIN AND EUI_COLOR_DEPTH LESS EXAMPLE_REQUIRES_COLOR_DEPTH_MIN)
            message(WARNING "Skipping ${name}: requires color_depth >= ${EXAMPLE_REQUIRES_COLOR_DEPTH_MIN} (got ${EUI_COLOR_DEPTH})")
            return()
        endif()
        if(DEFINED EXAMPLE_REQUIRES_WIDTH_MIN AND EUI_DISPLAY_WIDTH LESS EXAMPLE_REQUIRES_WIDTH_MIN)
            message(WARNING "Skipping ${name}: requires width >= ${EXAMPLE_REQUIRES_WIDTH_MIN} (got ${EUI_DISPLAY_WIDTH})")
            return()
        endif()
        if(DEFINED EXAMPLE_REQUIRES_HEIGHT_MIN AND EUI_DISPLAY_HEIGHT LESS EXAMPLE_REQUIRES_HEIGHT_MIN)
            message(WARNING "Skipping ${name}: requires height >= ${EXAMPLE_REQUIRES_HEIGHT_MIN} (got ${EUI_DISPLAY_HEIGHT})")
            return()
        endif()
    endif()

    add_subdirectory(${name})

    # Ensure profile config header is generated before building any cross example
    if(TARGET gen_profile_config)
        add_dependencies(${name} gen_profile_config)
    endif()
endfunction()

add_cross_example(basic_label)
add_cross_example(button_test)
add_cross_example(list_nav)
add_cross_example(menu_system)
add_cross_example(dialog_overlay)
add_cross_example(animation_demo)
add_cross_example(custom_widget)
add_cross_example(page_buffer)
add_cross_example(benchmark)
add_cross_example(scene_view_demo)
add_cross_example(color_demo)
add_cross_example(amiibo_demo)
```

- [ ] **Step 2: Verify constraint filtering**

```bash
# 1bpp build — should SKIP color_demo and amiibo_demo
cmake -B build_1bpp -DEUI_CONFIG_PROFILE=configs/pc/raylib_128x64_1bpp.cmake -DEUI_BUILD_CROSS_EXAMPLES=ON -DEUI_BUILD_EXAMPLES=OFF -DEUI_BUILD_TESTS=OFF 2>&1 | grep -i skip
cmake --build build_1bpp -j$(nproc)

# 16bpp build — should INCLUDE all examples
cmake -B build_16bpp -DEUI_CONFIG_PROFILE=configs/pc/raylib_240x240_16bpp.cmake -DEUI_BUILD_CROSS_EXAMPLES=ON -DEUI_BUILD_EXAMPLES=OFF -DEUI_BUILD_TESTS=OFF
cmake --build build_16bpp -j$(nproc)
```

Expected: 1bpp build produces WARNING for color_demo and amiibo_demo being skipped. All other examples build.
16bpp build includes all 12 examples.

- [ ] **Step 3: Commit**

```bash
git add examples/cross/CMakeLists.txt
git commit -m "feat(build): add requirements.cmake constraint checking for cross examples"
```

---

### Task 14: Create Kconfig files for config profiles and remove from example dirs

**Files:**
- Create: `configs/esp32/ssd1306_i2c_128x64_1bpp.Kconfig`
- Create: `configs/esp32/st7735_spi_240x240_16bpp.Kconfig`
- Create: `configs/esp32/ili9341_spi_320x240_16bpp.Kconfig`
- Create: `configs/nrf52/ssd1306_i2c_128x64_1bpp.Kconfig`
- Create: `configs/nrf52/st7735_spi_240x240_16bpp.Kconfig`
- Delete: `examples/cross/basic_label/Kconfig`
- Delete: `examples/cross/button_test/Kconfig`
- Delete: `examples/cross/list_nav/Kconfig`

- [ ] **Step 1: Create Kconfig files for ESP32**

```kconfig
# configs/esp32/ssd1306_i2c_128x64_1bpp.Kconfig
menu "Display Configuration"
    config EUI_DISPLAY_WIDTH
        int "Display Width"
        default 128
    config EUI_DISPLAY_HEIGHT
        int "Display Height"
        default 64
endmenu

menu "I2C Configuration"
    config EUI_PIN_I2C_PORT
        int "I2C Port"
        default 0
    config EUI_PIN_SDA
        int "SDA Pin"
        default 21
    config EUI_PIN_SCL
        int "SCL Pin"
        default 22
    config EUI_PIN_I2C_FREQ
        int "I2C Frequency (Hz)"
        default 400000
    config EUI_PIN_I2C_ADDR
        hex "I2C Address"
        default 0x3C
endmenu

menu "Input Configuration"
    config EUI_INPUT_DRIVER
        string "Input Driver"
        default "buttons"
    config EUI_PIN_BTN_UP
        int "Up Button Pin"
        default 34
    config EUI_PIN_BTN_DOWN
        int "Down Button Pin"
        default 35
    config EUI_PIN_BTN_OK
        int "OK Button Pin"
        default 32
    config EUI_PIN_BTN_BACK
        int "Back Button Pin"
        default 33
endmenu
```

```kconfig
# configs/esp32/st7735_spi_240x240_16bpp.Kconfig
menu "Display Configuration"
    config EUI_DISPLAY_WIDTH
        int "Display Width"
        default 240
    config EUI_DISPLAY_HEIGHT
        int "Display Height"
        default 240
endmenu

menu "SPI Configuration"
    config EUI_PIN_SPI_HOST
        int "SPI Host"
        default 1
    config EUI_PIN_MOSI
        int "MOSI Pin"
        default 23
    config EUI_PIN_SCLK
        int "SCLK Pin"
        default 18
    config EUI_PIN_CS
        int "CS Pin"
        default 5
    config EUI_PIN_DC
        int "DC Pin"
        default 16
    config EUI_PIN_RST
        int "RST Pin"
        default 17
    config EUI_PIN_BL
        int "Backlight Pin"
        default 4
endmenu

menu "Input Configuration"
    config EUI_INPUT_DRIVER
        string "Input Driver"
        default "buttons"
    config EUI_PIN_BTN_UP
        int "Up Button Pin"
        default 34
    config EUI_PIN_BTN_DOWN
        int "Down Button Pin"
        default 35
    config EUI_PIN_BTN_OK
        int "OK Button Pin"
        default 32
    config EUI_PIN_BTN_BACK
        int "Back Button Pin"
        default 33
endmenu
```

ili9341 Kconfig follows the same pattern with 320x240 defaults.

- [ ] **Step 2: Create Kconfig files for NRF52** — similar structure, different pin defaults.

- [ ] **Step 3: Remove Kconfig from example directories**

```bash
rm examples/cross/basic_label/Kconfig
rm examples/cross/button_test/Kconfig
rm examples/cross/list_nav/Kconfig
```

- [ ] **Step 4: Commit**

```bash
git add configs/esp32/*.Kconfig configs/nrf52/*.Kconfig
git rm examples/cross/*/Kconfig
git commit -m "refactor(config): extract Kconfig from example dirs to configs/, one per hardware profile"
```

---

### Task 15: Cleanup — remove legacy examples and EUI_BUILD_EXAMPLES

**Files:**
- Delete: `examples/basic_label.c` through `examples/scene_view_demo.c` (10 files)
- Delete: `examples/amiibo_demo.c`, `examples/amiibo_font.h`
- Delete: `examples/button_test.c`
- Delete: `examples/list_nav.c`
- Delete: `examples/CMakeLists.txt`
- Modify: `CMakeLists.txt` — remove `EUI_BUILD_EXAMPLES` option and legacy examples section
- Modify: `.github/workflows/build.yml` — remove `EUI_BUILD_EXAMPLES` references (already handled in CI update)

- [ ] **Step 1: Delete legacy example source files**

```bash
rm examples/basic_label.c examples/button_test.c examples/list_nav.c \
   examples/menu_system.c examples/dialog_overlay.c examples/animation_demo.c \
   examples/custom_widget.c examples/page_buffer.c examples/benchmark.c \
   examples/color_demo.c examples/scene_view_demo.c examples/amiibo_demo.c \
   examples/amiibo_font.h
```

- [ ] **Step 2: Delete legacy examples/CMakeLists.txt**

```bash
rm examples/CMakeLists.txt
```

- [ ] **Step 3: Update root CMakeLists.txt** — remove `EUI_BUILD_EXAMPLES` option and related sections

Remove lines 22 and 46-48 (option declaration), and lines 66-69 (legacy examples `add_subdirectory`). Also remove `EUI_BUILD_EXAMPLES` from the raylib dependency check (line 46).

The raylib dependency check at line 46 should become:

```cmake
if((EUI_BUILD_CROSS_EXAMPLES AND (NOT DEFINED EUI_TARGET_PORT OR EUI_TARGET_PORT STREQUAL "raylib")) OR
   EUI_BUILD_TESTS)
```

- [ ] **Step 4: Verify full build works after removal**

```bash
cmake -B build -DEUI_CONFIG_PROFILE=configs/pc/raylib_128x64_1bpp.cmake -DEUI_BUILD_CROSS_EXAMPLES=ON -DEUI_BUILD_TESTS=ON
cmake --build build -j$(nproc)
cd build && ctest --output-on-failure
```

- [ ] **Step 5: Commit**

```bash
git add -u
git commit -m "chore: remove legacy flat examples, keep only cross/ examples"
```

---

### Task 16: Deprecate examples/esp-idf/

**Files:**
- Modify: `examples/esp-idf/ssd1306/` → add DEPRECATED.md notice
- Modify: `examples/esp-idf/st7735/` → add DEPRECATED.md notice

- [ ] **Step 1: Add deprecation notice to esp-idf examples**

Create `examples/esp-idf/ssd1306/DEPRECATED.md`:

```markdown
# DEPRECATED

This standalone ESP-IDF example is replaced by the config profile system:

```bash
cmake -B build -DEUI_CONFIG_PROFILE=configs/esp32/ssd1306_i2c_128x64_1bpp.cmake -DEUI_BUILD_CROSS_EXAMPLES=ON
```

See `docs/superpowers/specs/2026-05-24-example-config-profiles-design.md` for details.
```

Same for `examples/esp-idf/st7735/DEPRECATED.md`.

The directories and build artifacts remain temporarily for reference but should not be used for new builds.

- [ ] **Step 2: Commit**

```bash
git add examples/esp-idf/*/DEPRECATED.md
git commit -m "docs: deprecate standalone esp-idf examples in favor of config profiles"
```

---

### Task 17: Final verification — full CI matrix

- [ ] **Step 1: Build all PC profiles**

```bash
# Profile 1: 1bpp 128x64
cmake -B build_pc_1bpp -DEUI_CONFIG_PROFILE=configs/pc/raylib_128x64_1bpp.cmake -DEUI_BUILD_CROSS_EXAMPLES=ON -DEUI_BUILD_TESTS=ON
cmake --build build_pc_1bpp -j$(nproc)
cd build_pc_1bpp && ctest --output-on-failure

# Profile 2: 16bpp 240x240
cmake -B build_pc_16bpp -DEUI_CONFIG_PROFILE=configs/pc/raylib_240x240_16bpp.cmake -DEUI_BUILD_CROSS_EXAMPLES=ON -DEUI_BUILD_TESTS=ON
cmake --build build_pc_16bpp -j$(nproc)
cd build_pc_16bpp && ctest --output-on-failure
```

- [ ] **Step 2: Verify example count per profile**

```bash
# 1bpp: 10 examples (all except color_demo, amiibo_demo)
ls build_pc_1bpp/examples/cross/*/basic_label 2>/dev/null  # should exist
ls build_pc_1bpp/examples/cross/*/color_demo 2>/dev/null   # should NOT exist

# 16bpp: all 12 examples
ls build_pc_16bpp/examples/cross/*/amiibo_demo 2>/dev/null # should exist
```

- [ ] **Step 3: Commit any remaining changes**

```bash
git status
git add -A
git diff --cached --stat
git commit -m "chore: final verification — all profiles build successfully"
```

- [ ] **Step 4: Push**

```bash
git push
```

---

## Post-Implementation Notes

- The CI `.github/workflows/build.yml` was updated in a prior commit to add the `examples` job with color_depth matrix [1, 16]. After config profiles land, update it to use `EUI_CONFIG_PROFILE` as described in the spec.
- `examples/icons/` and `examples/render_16bpp.c` remain unchanged — they are assets and standalone tools, not examples in the build system.
- The `tools/config_to_header.cmake` script handles string vs numeric values heuristically. If any profile variable needs explicit type handling, extend the regex match in the script.
- The amiibo_demo uses `eui_set_tick_callback()` to register its own tick for carousel animation updates, replacing the original in-main-loop sync. The raylib port bootstrap also calls `eui_set_tick_callback()`, so the example's call overrides it — this is intentional as the example needs per-frame animation updates beyond the standard tick.
