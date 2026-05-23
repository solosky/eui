# Cross-Platform Examples Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement a port bootstrap layer so that the same example `.c` file compiles and runs on raylib (PC), esp-idf, and nrf5 without per-platform entry files.

**Architecture:** Each port provides `eui_port_bootstrap.c` with the platform's entry point (`main`/`app_main`) that initializes EUI, calls `eui_example_setup()` from the example, and enters the platform main loop. An `EUI_TARGET_PORT` CMake variable selects which bootstrap library to link.

**Tech Stack:** C99, CMake 3.22+, raylib, ESP-IDF, nRF5 SDK

---

## File Map

| File | Action | Purpose |
|------|--------|---------|
| `include/eui/eui_port_bootstrap.h` | Create | Bootstrap API + `eui_example_config_t` |
| `port/raylib/eui_port_bootstrap.c` | Create | PC bootstrap: `main()`, raylib init, main loop, cleanup |
| `port/raylib/CMakeLists.txt` | Create | Build `eui_port_raylib` static library |
| `port/esp-idf/eui_port_bootstrap.c` | Create | ESP32 bootstrap: `app_main()`, I2C/SPI HAL, SSD1306/ST7735 |
| `port/esp-idf/CMakeLists.txt` | Modify | Add `eui_port_bootstrap.c` to esp-idf component |
| `port/nrf5/eui_port_bootstrap.c` | Create | nRF52 bootstrap: `main()`, TWI/SPI HAL, SSD1306/ST7735 |
| `port/nrf5/CMakeLists.txt` | Modify | Add `eui_port_bootstrap.c` to nrf5 library |
| `CMakeLists.txt` (root) | Modify | Add `EUI_BUILD_CROSS_EXAMPLES`, `EUI_TARGET_PORT`, port subdirs |
| `examples/CMakeLists.txt` | Modify | Cross-platform example traversal logic |
| `CMakePresets.json` | Create | Presets for pc, esp32, nrf52 |
| `examples/basic_label/basic_label.c` | Create | First cross-platform example |
| `examples/basic_label/CMakeLists.txt` | Create | Per-example build (links eui + bootstrap) |
| `examples/basic_label/Kconfig` | Create | Hardware config (pins, driver choice) |
| `tools/kconfig_to_header.cmake` | Create | CMake script to parse Kconfig defaults → C header |
| `examples/button_test/` | Create | Second cross-platform example |
| `examples/list_nav/` | Create | Third cross-platform example |

---

### Task 1: Bootstrap API Header

**Files:**
- Create: `include/eui/eui_port_bootstrap.h`

- [ ] **Step 1: Create the header**

```c
#ifndef EUI_PORT_BOOTSTRAP_H
#define EUI_PORT_BOOTSTRAP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t display_width;
    uint16_t display_height;
} eui_example_config_t;

/**
 * Called by the port bootstrap after eui_init() and display init are complete.
 * Each cross-platform example implements this function to build its UI.
 */
void eui_example_setup(const eui_example_config_t *cfg);

#ifdef __cplusplus
}
#endif

#endif /* EUI_PORT_BOOTSTRAP_H */
```

- [ ] **Step 2: Verify header compiles**

```bash
echo '#include "eui/eui_port_bootstrap.h"' | gcc -fsyntax-only -I include -x c -
```

Expected: no errors

- [ ] **Step 3: Commit**

```bash
git add include/eui/eui_port_bootstrap.h
git commit -m "feat: add bootstrap API header for cross-platform examples"
```

---

### Task 2: Raylib Bootstrap (PC)

**Files:**
- Create: `port/raylib/eui_port_bootstrap.c`
- Create: `port/raylib/CMakeLists.txt`

- [ ] **Step 1: Create raylib bootstrap implementation**

```c
/* port/raylib/eui_port_bootstrap.c */
#include "eui/eui.h"
#include "eui/driver/eui_drv_raylib.h"
#include "eui/eui_port_bootstrap.h"
#include <stdio.h>
#include <stdlib.h>

#define POOL_SIZE (128 * 64 * 2 + 8192)
static uint8_t mem_pool[POOL_SIZE];

static uint32_t get_tick_ms(void) {
    return (uint32_t)(GetTime() * 1000.0);
}

int main(void) {
    eui_example_config_t cfg = {
        .display_width  = 128,
        .display_height = 64,
    };

    eui_display_hal_t *display = eui_drv_raylib_create_display(
        cfg.display_width, cfg.display_height, EUI_COLOR_DEPTH);
    if (!display) {
        fprintf(stderr, "Failed to create raylib display\n");
        return 1;
    }

    eui_input_hal_t *input = eui_drv_raylib_create_input();
    if (!input) {
        fprintf(stderr, "Failed to create raylib input\n");
        eui_drv_raylib_destroy_display(display);
        return 1;
    }

    eui_config_t eui_cfg = {
        .mem_pool_buffer = mem_pool,
        .mem_pool_size   = sizeof(mem_pool),
        .display         = display,
        .input           = input,
    };

    if (eui_init(&eui_cfg) != 0) {
        fprintf(stderr, "eui_init failed\n");
        eui_drv_raylib_destroy_input(input);
        eui_drv_raylib_destroy_display(display);
        return 1;
    }

    eui_set_tick_callback(get_tick_ms);
    display->init(display->user_data);

    eui_example_setup(&cfg);

    while (!eui_drv_raylib_window_should_close()) {
        eui_tick();
        eui_drv_raylib_refresh();
    }

    display->deinit(display->user_data);
    eui_deinit();
    eui_drv_raylib_destroy_input(input);
    eui_drv_raylib_destroy_display(display);
    return 0;
}
```

- [ ] **Step 2: Create raylib port CMakeLists.txt**

```cmake
# port/raylib/CMakeLists.txt
add_library(eui_port_raylib STATIC eui_port_bootstrap.c)
target_include_directories(eui_port_raylib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)
target_link_libraries(eui_port_raylib PUBLIC eui eui_drv_raylib)
```

- [ ] **Step 3: Commit**

```bash
git add port/raylib/
git commit -m "feat: add raylib port bootstrap for cross-platform examples"
```

---

### Task 3: ESP-IDF Bootstrap

**Files:**
- Create: `port/esp-idf/eui_port_bootstrap.c`
- Modify: `port/esp-idf/CMakeLists.txt`

- [ ] **Step 1: Create ESP-IDF bootstrap implementation**

```c
/* port/esp-idf/eui_port_bootstrap.c */
#include "eui/eui.h"
#include "eui/driver/eui_drv_ssd1306.h"
#include "eui/driver/eui_drv_st7735.h"
#include "eui/eui_port_bootstrap.h"
#include "eui_port_esp_idf.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include <string.h>

#define POOL_SIZE 8192
static uint8_t mem_pool[POOL_SIZE];

/* Default pin config — override via Kconfig menuconfig */
#ifndef CONFIG_EUI_EXAMPLE_I2C_PORT
#define CONFIG_EUI_EXAMPLE_I2C_PORT   0
#endif
#ifndef CONFIG_EUI_EXAMPLE_I2C_SDA
#define CONFIG_EUI_EXAMPLE_I2C_SDA    21
#endif
#ifndef CONFIG_EUI_EXAMPLE_I2C_SCL
#define CONFIG_EUI_EXAMPLE_I2C_SCL    22
#endif
#ifndef CONFIG_EUI_EXAMPLE_I2C_FREQ
#define CONFIG_EUI_EXAMPLE_I2C_FREQ   400000
#endif
#ifndef CONFIG_EUI_EXAMPLE_I2C_ADDR
#define CONFIG_EUI_EXAMPLE_I2C_ADDR   0x3C
#endif
#ifndef CONFIG_EUI_EXAMPLE_I2C_TIMEOUT
#define CONFIG_EUI_EXAMPLE_I2C_TIMEOUT 100
#endif
#ifndef CONFIG_EUI_EXAMPLE_DISPLAY_WIDTH
#define CONFIG_EUI_EXAMPLE_DISPLAY_WIDTH  128
#endif
#ifndef CONFIG_EUI_EXAMPLE_DISPLAY_HEIGHT
#define CONFIG_EUI_EXAMPLE_DISPLAY_HEIGHT 64
#endif

static uint32_t get_tick_ms(void) {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

void app_main(void) {
    eui_example_config_t cfg = {
        .display_width  = CONFIG_EUI_EXAMPLE_DISPLAY_WIDTH,
        .display_height = CONFIG_EUI_EXAMPLE_DISPLAY_HEIGHT,
    };

    /* Create I2C transport */
    esp_idf_i2c_config_t i2c_cfg = {
        .port       = CONFIG_EUI_EXAMPLE_I2C_PORT,
        .sda        = CONFIG_EUI_EXAMPLE_I2C_SDA,
        .scl        = CONFIG_EUI_EXAMPLE_I2C_SCL,
        .freq       = CONFIG_EUI_EXAMPLE_I2C_FREQ,
        .addr       = CONFIG_EUI_EXAMPLE_I2C_ADDR,
        .timeout_ms = CONFIG_EUI_EXAMPLE_I2C_TIMEOUT,
    };
    eui_hal_i2c_t *i2c = eui_port_esp_idf_i2c_create(&i2c_cfg);

    /* Create SSD1306 display */
    eui_drv_ssd1306_config_t dcfg = {
        .i2c      = *i2c,
        .width    = cfg.display_width,
        .height   = cfg.display_height,
        .i2c_addr = CONFIG_EUI_EXAMPLE_I2C_ADDR,
    };
    eui_display_hal_t *display = eui_drv_ssd1306_create(&dcfg);

    eui_config_t eui_cfg = {
        .mem_pool_buffer = mem_pool,
        .mem_pool_size   = sizeof(mem_pool),
        .display         = display,
        .input           = NULL,
    };

    eui_init(&eui_cfg);
    eui_set_tick_callback(get_tick_ms);
    display->init(display->user_data);

    eui_example_setup(&cfg);

    while (1) {
        eui_tick();
        vTaskDelay(pdMS_TO_TICKS(16));
    }
}
```

- [ ] **Step 2: Modify ESP-IDF port CMakeLists.txt to include bootstrap**

Modify `port/esp-idf/CMakeLists.txt` — add `eui_port_bootstrap.c` to sources:

```cmake
if(DEFINED ENV{IDF_PATH} OR DEFINED IDF_PATH)
    idf_component_register(
        SRCS            eui_port_esp_idf.c eui_port_bootstrap.c
        INCLUDE_DIRS    . ${CMAKE_SOURCE_DIR}/include
        REQUIRES        driver freertos
    )
else()
    add_library(eui_port_esp_idf STATIC eui_port_esp_idf.c eui_port_bootstrap.c)
    target_include_directories(eui_port_esp_idf PUBLIC
        .
        ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(eui_port_esp_idf PUBLIC eui)
endif()
```

- [ ] **Step 3: Commit**

```bash
git add port/esp-idf/eui_port_bootstrap.c
git add port/esp-idf/CMakeLists.txt
git commit -m "feat: add ESP-IDF port bootstrap for cross-platform examples"
```

---

### Task 4: nRF5 Bootstrap

**Files:**
- Create: `port/nrf5/eui_port_bootstrap.c`
- Modify: `port/nrf5/CMakeLists.txt`

- [ ] **Step 1: Create nRF5 bootstrap implementation**

```c
/* port/nrf5/eui_port_bootstrap.c */
#include "eui/eui.h"
#include "eui/driver/eui_drv_ssd1306.h"
#include "eui/eui_port_bootstrap.h"
#include "eui_port_nrf5.h"
#include "nrf_delay.h"
#include "app_timer.h"
#include <string.h>

#define POOL_SIZE 8192
static uint8_t mem_pool[POOL_SIZE];

/* Default pin config — override via Kconfig or sdk_config */
#ifndef CONFIG_EUI_EXAMPLE_I2C_SDA
#define CONFIG_EUI_EXAMPLE_I2C_SDA    26
#endif
#ifndef CONFIG_EUI_EXAMPLE_I2C_SCL
#define CONFIG_EUI_EXAMPLE_I2C_SCL    27
#endif
#ifndef CONFIG_EUI_EXAMPLE_I2C_FREQ
#define CONFIG_EUI_EXAMPLE_I2C_FREQ   400000
#endif
#ifndef CONFIG_EUI_EXAMPLE_I2C_ADDR
#define CONFIG_EUI_EXAMPLE_I2C_ADDR   0x3C
#endif
#ifndef CONFIG_EUI_EXAMPLE_DISPLAY_WIDTH
#define CONFIG_EUI_EXAMPLE_DISPLAY_WIDTH  128
#endif
#ifndef CONFIG_EUI_EXAMPLE_DISPLAY_HEIGHT
#define CONFIG_EUI_EXAMPLE_DISPLAY_HEIGHT 64
#endif

static uint32_t get_tick_ms(void) {
    return app_timer_cnt_get() * (1000 / APP_TIMER_CLOCK_FREQ);
}

int main(void) {
    eui_example_config_t cfg = {
        .display_width  = CONFIG_EUI_EXAMPLE_DISPLAY_WIDTH,
        .display_height = CONFIG_EUI_EXAMPLE_DISPLAY_HEIGHT,
    };

    /* Create TWI (I2C) transport */
    nrf5_i2c_config_t i2c_cfg = {
        .sda         = CONFIG_EUI_EXAMPLE_I2C_SDA,
        .scl         = CONFIG_EUI_EXAMPLE_I2C_SCL,
        .addr        = CONFIG_EUI_EXAMPLE_I2C_ADDR,
        .freq        = CONFIG_EUI_EXAMPLE_I2C_FREQ,
        .instance_id = 0,
    };
    eui_hal_i2c_t *i2c = eui_port_nrf5_i2c_create(&i2c_cfg);

    /* Create SSD1306 display */
    eui_drv_ssd1306_config_t dcfg = {
        .i2c      = *i2c,
        .width    = cfg.display_width,
        .height   = cfg.display_height,
        .i2c_addr = CONFIG_EUI_EXAMPLE_I2C_ADDR,
    };
    eui_display_hal_t *display = eui_drv_ssd1306_create(&dcfg);

    eui_config_t eui_cfg = {
        .mem_pool_buffer = mem_pool,
        .mem_pool_size   = sizeof(mem_pool),
        .display         = display,
        .input           = NULL,
    };

    eui_init(&eui_cfg);
    eui_set_tick_callback(get_tick_ms);
    display->init(display->user_data);

    eui_example_setup(&cfg);

    while (1) {
        eui_tick();
        nrf_delay_ms(16);
    }
}
```

- [ ] **Step 2: Modify nRF5 port CMakeLists.txt to include bootstrap**

In `port/nrf5/CMakeLists.txt`, change `eui_port_nrf5.c` to `eui_port_nrf5.c eui_port_bootstrap.c` in both branches (NRF5_SDK_ROOT and else):

```cmake
if(DEFINED ENV{NRF5_SDK_ROOT})
    set(NRF5_SDK_ROOT $ENV{NRF5_SDK_ROOT})

    add_library(eui_port_nrf5 STATIC eui_port_nrf5.c eui_port_bootstrap.c)
    target_include_directories(eui_port_nrf5 PUBLIC
        .
        ${CMAKE_SOURCE_DIR}/include
        ${NRF5_SDK_ROOT}/components
        ${NRF5_SDK_ROOT}/components/drivers_nrf/twi_master
        ${NRF5_SDK_ROOT}/components/drivers_nrf/spi_master
        ${NRF5_SDK_ROOT}/components/drivers_nrf/delay
        ${NRF5_SDK_ROOT}/components/libraries/util
        ${NRF5_SDK_ROOT}/integration/nrfx
        ${NRF5_SDK_ROOT}/modules/nrfx
        ${NRF5_SDK_ROOT}/modules/nrfx/drivers/include
        ${NRF5_SDK_ROOT}/modules/nrfx/hal
        ${NRF5_SDK_ROOT}/components/toolchain
    )

    set(NRF5_CHIP "NRF52832_XXAA" CACHE STRING "Target nRF5 chip variant")
    set(NRF5_FAMILY "NRF52" CACHE STRING "Target nRF5 family")

    target_compile_definitions(eui_port_nrf5 PRIVATE
        ${NRF5_FAMILY}
        ${NRF5_CHIP}
    )

    target_link_libraries(eui_port_nrf5 PUBLIC eui)
else()
    add_library(eui_port_nrf5 STATIC eui_port_nrf5.c eui_port_bootstrap.c)
    target_include_directories(eui_port_nrf5 PUBLIC
        .
        ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(eui_port_nrf5 PUBLIC eui)
endif()
```

- [ ] **Step 3: Commit**

```bash
git add port/nrf5/eui_port_bootstrap.c port/nrf5/CMakeLists.txt
git commit -m "feat: add nRF5 port bootstrap for cross-platform examples"
```

---

### Task 5: CMake Infrastructure

**Files:**
- Modify: `CMakeLists.txt` (root)
- Modify: `examples/CMakeLists.txt`
- Create: `CMakePresets.json`

- [ ] **Step 1: Modify root CMakeLists.txt**

Add the new option and port subdirectory handling. After line 8 (`option(EUI_BUILD_EXAMPLES ...)`), add:

```cmake
option(EUI_BUILD_CROSS_EXAMPLES "Build cross-platform examples" OFF)

if(NOT DEFINED EUI_TARGET_PORT)
    set(EUI_TARGET_PORT "raylib" CACHE STRING "Target port: raylib, esp-idf, nrf5")
endif()
set_property(CACHE EUI_TARGET_PORT PROPERTY STRINGS "raylib" "esp-idf" "nrf5")

# Platform port bootstrap libraries
if(EUI_TARGET_PORT STREQUAL "raylib")
    set(BOOTSTRAP_LIB eui_port_raylib)
elseif(EUI_TARGET_PORT STREQUAL "esp-idf")
    set(BOOTSTRAP_LIB eui_port_esp_idf)
elseif(EUI_TARGET_PORT STREQUAL "nrf5")
    set(BOOTSTRAP_LIB eui_port_nrf5)
else()
    message(FATAL_ERROR "Unknown EUI_TARGET_PORT: ${EUI_TARGET_PORT}")
endif()

# Build port bootstrap libraries
add_subdirectory(port/raylib)
if(NOT DEFINED ENV{IDF_PATH} AND NOT DEFINED IDF_PATH)
    add_subdirectory(port/esp-idf)
endif()
add_subdirectory(port/nrf5)
```

Before the `add_subdirectory(src)` line, keep all existing options. The port subdirectories need to be added before examples so the bootstrap libraries are available.

Wait — `port/raylib` needs `eui_drv_raylib` which needs `src/` and `third_party/raylib`. And `examples/` needs the bootstrap libraries. Let me be more careful about ordering.

The correct order:
1. Options
2. `third_party/tlsf`, `third_party/motionc`
3. `src/` (builds `eui` + `eui_drv_raylib`)
4. Port subdirectories (builds bootstrap libraries — depends on `eui` + `eui_drv_raylib`)
5. If `EUI_BUILD_EXAMPLES`: `third_party/raylib`, `examples/`

Actually, `port/raylib` depends on `eui_drv_raylib` which depends on `raylib`. And `raylib` is built under `if(EUI_BUILD_EXAMPLES)`. So port/raylib must also be under that guard or have its own raylib dependency.

Let me restructure. The port subdirectories should only be added when cross-platform examples are enabled:

```cmake
if(EUI_BUILD_CROSS_EXAMPLES OR EUI_BUILD_EXAMPLES)
    add_subdirectory(port/raylib)
    add_subdirectory(port/esp-idf)
    add_subdirectory(port/nrf5)
endif()
```

But wait, `port/raylib` depends on `eui_drv_raylib` which depends on `raylib`. Let me handle this properly.

Actually, let me simplify. The port/raylib CMakeLists.txt just creates the library target. If raylib isn't available (because `EUI_BUILD_EXAMPLES` is OFF), the target won't link properly. So we guard it.

Here's the revised plan for root CMakeLists.txt:

After the existing `add_subdirectory(src)` (line 32), and before the existing `if(EUI_BUILD_TESTS)` (line 34), add:

```cmake
# Port bootstrap libraries for cross-platform examples
if(EUI_BUILD_CROSS_EXAMPLES)
    if(NOT DEFINED EUI_TARGET_PORT)
        set(EUI_TARGET_PORT "raylib" CACHE STRING "Target port: raylib, esp-idf, nrf5")
    endif()
    set_property(CACHE EUI_TARGET_PORT PROPERTY STRINGS "raylib" "esp-idf" "nrf5")

    if(EUI_TARGET_PORT STREQUAL "raylib")
        set(BOOTSTRAP_LIB eui_port_raylib)
        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(USE_EXTERNAL_GLFW OFF CACHE BOOL "" FORCE)
        add_subdirectory(third_party/raylib)
        add_subdirectory(port/raylib)
    elseif(EUI_TARGET_PORT STREQUAL "esp-idf")
        set(BOOTSTRAP_LIB eui_port_esp_idf)
        add_subdirectory(port/esp-idf)
    elseif(EUI_TARGET_PORT STREQUAL "nrf5")
        set(BOOTSTRAP_LIB eui_port_nrf5)
        add_subdirectory(port/nrf5)
    endif()

    add_subdirectory(examples/cross)
endif()
```

And keep existing `EUI_BUILD_EXAMPLES` block as-is for legacy examples.

Hmm, actually this is getting complex. Let me simplify the approach. The existing `EUI_BUILD_EXAMPLES` builds the raylib library and old examples. The new `EUI_BUILD_CROSS_EXAMPLES` is independent. Let me restructure:

```cmake
# In root CMakeLists.txt, replace the EUI_BUILD_EXAMPLES block (lines 39-44)
# with this:

if(EUI_BUILD_EXAMPLES OR EUI_BUILD_CROSS_EXAMPLES)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(USE_EXTERNAL_GLFW OFF CACHE BOOL "" FORCE)
    add_subdirectory(third_party/raylib)
endif()

if(EUI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(EUI_BUILD_CROSS_EXAMPLES)
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
    endif()

    add_subdirectory(examples/cross)
endif()
```

Wait, but `BOOTSTRAP_LIB` needs to be available in `examples/cross/CMakeLists.txt`. Since it's set in the parent scope, it should be visible. Let me verify — CMake variables set with `set()` in a parent scope are visible in `add_subdirectory()` children. Good.

But there's an issue: the `EUI_BUILD_EXAMPLES` block in the current root CMakeLists.txt also adds `add_subdirectory(examples)`. If both `EUI_BUILD_EXAMPLES` and `EUI_BUILD_CROSS_EXAMPLES` are ON, we'd have both `examples/` and `examples/cross/` being processed. That's fine.

OK let me now write the exact root CMakeLists.txt changes. Let me re-read the current file:

Current (lines 39-44):
```cmake
if(EUI_BUILD_EXAMPLES)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(USE_EXTERNAL_GLFW OFF CACHE BOOL "" FORCE)
    add_subdirectory(third_party/raylib)
    add_subdirectory(examples)
endif()
```

New version:
```cmake
# Build raylib if needed for either legacy or cross-platform examples
if(EUI_BUILD_EXAMPLES OR EUI_BUILD_CROSS_EXAMPLES)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(USE_EXTERNAL_GLFW OFF CACHE BOOL "" FORCE)
    add_subdirectory(third_party/raylib)
endif()

# Legacy raylib-only examples
if(EUI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Cross-platform examples
if(EUI_BUILD_CROSS_EXAMPLES)
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
    endif()

    add_subdirectory(examples/cross)
endif()
```

But wait, there's a subtlety. `src/CMakeLists.txt` has:
```cmake
if(EUI_BUILD_EXAMPLES)
    add_library(eui_drv_raylib STATIC driver/eui_drv_raylib.c)
    ...
endif()
```

This means `eui_drv_raylib` is only built when `EUI_BUILD_EXAMPLES` is ON. But the raylib bootstrap needs it when `EUI_BUILD_CROSS_EXAMPLES` is ON (with `EUI_TARGET_PORT=raylib`). Let me fix `src/CMakeLists.txt` too:

Change:
```cmake
if(EUI_BUILD_EXAMPLES)
```
to:
```cmake
if(EUI_BUILD_EXAMPLES OR EUI_BUILD_CROSS_EXAMPLES)
```

OK let me finalize the plan for Task 5.

- [ ] **Step 1: Modify root CMakeLists.txt** — the exact code changes

Replace lines 7-8 (the options):
```cmake
option(EUI_BUILD_TESTS "Build unit tests" ON)
option(EUI_BUILD_EXAMPLES "Build raylib examples" ON)
```
With:
```cmake
option(EUI_BUILD_TESTS "Build unit tests" ON)
option(EUI_BUILD_EXAMPLES "Build raylib examples" ON)
option(EUI_BUILD_CROSS_EXAMPLES "Build cross-platform examples" OFF)
```

Replace lines 39-44:
```cmake
if(EUI_BUILD_EXAMPLES)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(USE_EXTERNAL_GLFW OFF CACHE BOOL "" FORCE)
    add_subdirectory(third_party/raylib)
    add_subdirectory(examples)
endif()
```
With:
```cmake
if(EUI_BUILD_EXAMPLES OR EUI_BUILD_CROSS_EXAMPLES)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(USE_EXTERNAL_GLFW OFF CACHE BOOL "" FORCE)
    add_subdirectory(third_party/raylib)
endif()

if(EUI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(EUI_BUILD_CROSS_EXAMPLES)
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

- [ ] **Step 2: Modify src/CMakeLists.txt**

Change line 45:
```cmake
if(EUI_BUILD_EXAMPLES)
```
To:
```cmake
if(EUI_BUILD_EXAMPLES OR EUI_BUILD_CROSS_EXAMPLES)
```

- [ ] **Step 3: Create examples/cross/CMakeLists.txt** (new directory for cross-platform examples)

```cmake
# examples/cross/CMakeLists.txt
add_subdirectory(basic_label)
```

- [ ] **Step 4: Create CMakePresets.json at project root**

```json
{
    "version": 3,
    "configurePresets": [
        {
            "name": "pc",
            "displayName": "PC (Raylib)",
            "cacheVariables": {
                "EUI_TARGET_PORT": "raylib",
                "EUI_BUILD_CROSS_EXAMPLES": "ON",
                "EUI_BUILD_EXAMPLES": "OFF"
            }
        },
        {
            "name": "esp32",
            "displayName": "ESP32 (ESP-IDF)",
            "cacheVariables": {
                "EUI_TARGET_PORT": "esp-idf",
                "EUI_BUILD_CROSS_EXAMPLES": "ON",
                "EUI_BUILD_EXAMPLES": "OFF"
            }
        },
        {
            "name": "nrf52",
            "displayName": "nRF52 (nRF5 SDK)",
            "cacheVariables": {
                "EUI_TARGET_PORT": "nrf5",
                "EUI_BUILD_CROSS_EXAMPLES": "ON",
                "EUI_BUILD_EXAMPLES": "OFF"
            }
        }
    ]
}
```

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt src/CMakeLists.txt examples/cross/ CMakePresets.json
git commit -m "feat: add CMake infrastructure for cross-platform examples"
```

---

### Task 6: Kconfig + PC Config Header Generation

**Files:**
- Create: `examples/cross/basic_label/Kconfig`
- Create: `tools/kconfig_to_header.cmake`

- [ ] **Step 1: Create the Kconfig file**

```kconfig
# examples/cross/basic_label/Kconfig
menu "Display Configuration"
    config EUI_EXAMPLE_DISPLAY_WIDTH
        int "Display Width"
        default 128

    config EUI_EXAMPLE_DISPLAY_HEIGHT
        int "Display Height"
        default 64
endmenu

menu "I2C Configuration"
    config EUI_EXAMPLE_I2C_PORT
        int "I2C Port Number"
        default 0

    config EUI_EXAMPLE_I2C_SDA
        int "SDA Pin"
        default 21

    config EUI_EXAMPLE_I2C_SCL
        int "SCL Pin"
        default 22

    config EUI_EXAMPLE_I2C_FREQ
        int "I2C Frequency (Hz)"
        default 400000

    config EUI_EXAMPLE_I2C_ADDR
        hex "I2C Device Address"
        default 0x3C

    config EUI_EXAMPLE_I2C_TIMEOUT
        int "I2C Timeout (ms)"
        default 100
endmenu
```

- [ ] **Step 2: Create CMake script to parse Kconfig defaults**

```cmake
# tools/kconfig_to_header.cmake
# Usage: cmake -DKCONFIG_FILE=<path> -DOUTPUT_HEADER=<path> -P tools/kconfig_to_header.cmake
#
# Parses a Kconfig file and generates a C header with #define CONFIG_<NAME> <VALUE>
# for each config entry's default value.

file(STRINGS "${KCONFIG_FILE}" lines)
set(current_config "")
set(output "/* Auto-generated from ${KCONFIG_FILE} — do not edit */\n\n")

foreach(line IN LISTS lines)
    string(STRIP "${line}" line)

    # Skip comments and empty lines
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()

    # Match: config FOO
    if(line MATCHES "^config ([A-Za-z0-9_]+)")
        set(current_config "${CMAKE_MATCH_1}")
        set(current_type "")
        set(has_default FALSE)
    endif()

    # Match: int "..." / hex "..." / bool "..." / string "..."
    if(current_config AND line MATCHES "^(int|hex|bool|string)[ \t]+\"")
        set(current_type "${CMAKE_MATCH_1}")
    endif()

    # Match: default VALUE
    if(current_config AND line MATCHES "^default (.+)")
        set(default_val "${CMAKE_MATCH_1}")
        set(has_default TRUE)

        # Remove trailing comment
        string(REGEX REPLACE "[ \t]*#.*$" "" default_val "${default_val}")
        string(STRIP "${default_val}" default_val)

        if(default_val STREQUAL "y")
            set(default_val 1)
        endif()

        string(APPEND output "#define CONFIG_${current_config} ${default_val}\n")
        set(current_config "")
    endif()
endforeach()

file(WRITE "${OUTPUT_HEADER}" "${output}")
message(STATUS "Generated ${OUTPUT_HEADER} from ${KCONFIG_FILE}")
```

- [ ] **Step 3: Integrate Kconfig header generation into basic_label CMakeLists**

This will be done in Task 7 when creating the example's CMakeLists.txt, which will call:
```cmake
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/eui_example_config.h
    COMMAND ${CMAKE_COMMAND}
        -DKCONFIG_FILE=${CMAKE_CURRENT_SOURCE_DIR}/Kconfig
        -DOUTPUT_HEADER=${CMAKE_CURRENT_BINARY_DIR}/eui_example_config.h
        -P ${CMAKE_SOURCE_DIR}/tools/kconfig_to_header.cmake
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Kconfig
    COMMENT "Generating config header from Kconfig"
)
```

- [ ] **Step 4: Commit**

```bash
git add examples/cross/basic_label/Kconfig tools/kconfig_to_header.cmake
git commit -m "feat: add Kconfig and PC config header generation for cross-platform examples"
```

---

### Task 7: Cross-Platform Example — basic_label

**Files:**
- Create: `examples/cross/basic_label/basic_label.c`
- Create: `examples/cross/basic_label/CMakeLists.txt`

- [ ] **Step 1: Create basic_label.c**

```c
/* examples/cross/basic_label/basic_label.c */
#include "eui/eui.h"
#include "eui/eui_port_bootstrap.h"

void eui_example_setup(const eui_example_config_t *cfg) {
    (void)cfg;

    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();

    eui_widget_t *label = eui_label_create("Hello EUI!", 10, 5);
    eui_view_dispatcher_add(vd, 1, &label->view);
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);
}
```

- [ ] **Step 2: Create CMakeLists.txt**

```cmake
# examples/cross/basic_label/CMakeLists.txt

# Generate config header from Kconfig (PC/raylib target)
if(EUI_TARGET_PORT STREQUAL "raylib")
    set(CONFIG_HEADER ${CMAKE_CURRENT_BINARY_DIR}/eui_example_config.h)
    add_custom_command(
        OUTPUT ${CONFIG_HEADER}
        COMMAND ${CMAKE_COMMAND}
            -DKCONFIG_FILE=${CMAKE_CURRENT_SOURCE_DIR}/Kconfig
            -DOUTPUT_HEADER=${CONFIG_HEADER}
            -P ${CMAKE_SOURCE_DIR}/tools/kconfig_to_header.cmake
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Kconfig
        COMMENT "Generating config header from Kconfig defaults"
    )
    set(CONFIG_GEN_TARGET generate_basic_label_config)
    add_custom_target(${CONFIG_GEN_TARGET} DEPENDS ${CONFIG_HEADER})
endif()

add_executable(basic_label basic_label.c)
target_include_directories(basic_label PRIVATE
    ${CMAKE_BINARY_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(basic_label PRIVATE eui ${BOOTSTRAP_LIB})

if(EUI_TARGET_PORT STREQUAL "raylib")
    add_dependencies(basic_label ${CONFIG_GEN_TARGET})
endif()
```

- [ ] **Step 3: Update examples/cross/CMakeLists.txt** (already created in Task 5) — verify it has `add_subdirectory(basic_label)`

- [ ] **Step 4: Build and test on PC**

```bash
mkdir -p build_cross && cd build_cross
cmake .. -DEUI_BUILD_CROSS_EXAMPLES=ON -DEUI_TARGET_PORT=raylib -DEUI_BUILD_EXAMPLES=OFF
cmake --build .
```

Expected: `basic_label` executable built successfully.

- [ ] **Step 5: Run the example on PC**

```bash
cd build_cross && ./examples/cross/basic_label/basic_label
```

Expected: Window opens showing "Hello EUI!" label. Close the window — no crash.

- [ ] **Step 6: Commit**

```bash
git add examples/cross/basic_label/
git commit -m "feat: add cross-platform basic_label example"
```

---

### Task 8: Cross-Platform Example — button_test

**Files:**
- Create: `examples/cross/button_test/button_test.c`
- Create: `examples/cross/button_test/CMakeLists.txt`
- Create: `examples/cross/button_test/Kconfig`
- Modify: `examples/cross/CMakeLists.txt`

- [ ] **Step 1: Read existing button_test.c for reference**

Read `examples/button_test.c` to understand the button test UI logic and adapt it for the cross-platform format.

- [ ] **Step 2: Create button_test.c**

```c
/* examples/cross/button_test/button_test.c */
#include "eui/eui.h"
#include "eui/eui_port_bootstrap.h"
#include <stdio.h>

static void on_btn_a(void *ctx) {
    (void)ctx;
    printf("[BTN] A clicked\n");
}

static void on_btn_b(void *ctx) {
    (void)ctx;
    printf("[BTN] B clicked\n");
}

static void container_draw(eui_widget_t *w, eui_canvas_t *c) {
    for (uint8_t i = 0; i < w->child_count; i++) {
        eui_widget_t *child = w->children[i];
        if (eui_widget_is_visible(child) && child->vt && child->vt->draw)
            child->vt->draw(child, c);
    }
}

static bool container_input(eui_widget_t *w, const eui_event_t *evt) {
    if (evt->type == EUI_EVT_KEY_PRESS) {
        if (evt->data.key == EUI_KEY_RIGHT) { eui_widget_focus_next(w); return true; }
        if (evt->data.key == EUI_KEY_LEFT)  { eui_widget_focus_prev(w); return true; }
        if (evt->data.key == EUI_KEY_OK) {
            eui_widget_t *focused = eui_widget_get_focus(w);
            if (focused && focused->vt && focused->vt->input)
                return focused->vt->input(focused, evt);
        }
    }
    if (evt->type == EUI_EVT_KEY_RELEASE && evt->data.key == EUI_KEY_OK) {
        eui_widget_t *focused = eui_widget_get_focus(w);
        if (focused && focused->vt && focused->vt->input)
            return focused->vt->input(focused, evt);
    }
    return false;
}

static eui_widget_vtable_t container_vt = {
    .draw = container_draw,
    .input = container_input,
};

void eui_example_setup(const eui_example_config_t *cfg) {
    (void)cfg;

    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();

    eui_widget_t root;
    eui_widget_init(&root, &container_vt, 0, 0, 128, 64);

    eui_widget_t *btn_a = eui_button_create("Btn A", 10, 20, 50, 20);
    eui_button_set_callback(btn_a, on_btn_a, NULL);
    eui_widget_add_child(&root, btn_a);

    eui_widget_t *btn_b = eui_button_create("Btn B", 68, 20, 50, 20);
    eui_button_set_callback(btn_b, on_btn_b, NULL);
    eui_widget_add_child(&root, btn_b);

    eui_view_dispatcher_add(vd, 1, &root.view);
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);
}
```

- [ ] **Step 3: Create CMakeLists.txt**

```cmake
# examples/cross/button_test/CMakeLists.txt

if(EUI_TARGET_PORT STREQUAL "raylib")
    set(CONFIG_HEADER ${CMAKE_CURRENT_BINARY_DIR}/eui_example_config.h)
    add_custom_command(
        OUTPUT ${CONFIG_HEADER}
        COMMAND ${CMAKE_COMMAND}
            -DKCONFIG_FILE=${CMAKE_CURRENT_SOURCE_DIR}/Kconfig
            -DOUTPUT_HEADER=${CONFIG_HEADER}
            -P ${CMAKE_SOURCE_DIR}/tools/kconfig_to_header.cmake
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Kconfig
        COMMENT "Generating config header from Kconfig defaults"
    )
    set(CONFIG_GEN_TARGET generate_button_test_config)
    add_custom_target(${CONFIG_GEN_TARGET} DEPENDS ${CONFIG_HEADER})
endif()

add_executable(button_test button_test.c)
target_include_directories(button_test PRIVATE
    ${CMAKE_BINARY_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(button_test PRIVATE eui ${BOOTSTRAP_LIB})

if(EUI_TARGET_PORT STREQUAL "raylib")
    add_dependencies(button_test ${CONFIG_GEN_TARGET})
endif()
```

- [ ] **Step 4: Create Kconfig** (copy from basic_label, same hardware config)

```bash
cp examples/cross/basic_label/Kconfig examples/cross/button_test/Kconfig
```

- [ ] **Step 5: Update examples/cross/CMakeLists.txt**

Add `add_subdirectory(button_test)`.

- [ ] **Step 6: Build and test on PC**

```bash
cd build_cross && cmake .. -DEUI_BUILD_CROSS_EXAMPLES=ON -DEUI_TARGET_PORT=raylib -DEUI_BUILD_EXAMPLES=OFF
cmake --build .
./examples/cross/button_test/button_test
```

Expected: Window opens with a "Click Me" button and "Button Test" text. Clicking the button prints "Button clicked!" to console.

- [ ] **Step 7: Commit**

```bash
git add examples/cross/button_test/
git add examples/cross/CMakeLists.txt
git commit -m "feat: add cross-platform button_test example"
```

---

### Task 9: Cross-Platform Example — list_nav

**Files:**
- Create: `examples/cross/list_nav/list_nav.c`
- Create: `examples/cross/list_nav/CMakeLists.txt`
- Create: `examples/cross/list_nav/Kconfig`
- Modify: `examples/cross/CMakeLists.txt`

- [ ] **Step 1: Read existing list_nav.c for reference**

Read `examples/list_nav.c` to understand the list navigation UI logic.

- [ ] **Step 2: Create list_nav.c**

```c
/* examples/cross/list_nav/list_nav.c */
#include "eui/eui.h"
#include "eui/eui_port_bootstrap.h"
#include <stdio.h>

static void on_select(uint8_t idx, void *ctx) {
    (void)ctx;
    printf("[LIST] Selected item %d\n", idx);
}

void eui_example_setup(const eui_example_config_t *cfg) {
    (void)cfg;

    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();

    eui_widget_t *list = eui_list_create(0, 0, 128, 64);
    list->focus_policy = EUI_FOCUS_STRONG;
    eui_list_set_callback(list, on_select, NULL);

    for (int i = 0; i < 10; i++) {
        char buf[16];
        snprintf(buf, sizeof(buf), "Item %d", i);
        eui_list_add_item(list, buf, NULL);
    }

    eui_view_dispatcher_add(vd, 1, &list->view);
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);
}
```

- [ ] **Step 3: Create CMakeLists.txt and Kconfig**

Same pattern as button_test — copy CMakeLists.txt and Kconfig, update target name.

```bash
cp examples/cross/button_test/CMakeLists.txt examples/cross/list_nav/CMakeLists.txt
cp examples/cross/button_test/Kconfig examples/cross/list_nav/Kconfig
```

Then edit CMakeLists.txt: replace `button_test` with `list_nav` throughout.

- [ ] **Step 4: Update examples/cross/CMakeLists.txt**

Add `add_subdirectory(list_nav)`.

- [ ] **Step 5: Build and test on PC**

```bash
cd build_cross && cmake .. && cmake --build .
./examples/cross/list_nav/list_nav
```

Expected: Window opens with a scrollable list of 6 items. Arrow keys/tab navigate through items.

- [ ] **Step 6: Commit**

```bash
git add examples/cross/list_nav/
git add examples/cross/CMakeLists.txt
git commit -m "feat: add cross-platform list_nav example"
```

---

### Task 10: Final Verification

- [ ] **Step 1: Clean rebuild — all three examples on PC**

```bash
rm -rf build_cross
cmake --preset pc -B build_cross
cmake --build build_cross -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)
```

Expected: All three executables built without errors.

- [ ] **Step 2: Run all three examples**

```bash
build_cross/examples/cross/basic_label/basic_label &
build_cross/examples/cross/button_test/button_test &
build_cross/examples/cross/list_nav/list_nav &
```

Expected: Three windows open. Close them all. No crashes.

- [ ] **Step 3: Verify no regressions — legacy examples still build**

```bash
rm -rf build_legacy
cmake -B build_legacy -DEUI_BUILD_EXAMPLES=ON -DEUI_BUILD_CROSS_EXAMPLES=OFF -DEUI_BUILD_TESTS=OFF
cmake --build build_legacy -j$(sysctl -n hw.ncpu)
```

Expected: All 11 legacy raylib examples compile successfully.

- [ ] **Step 4: Verify tests still pass**

```bash
cd build_legacy && cmake .. -DEUI_BUILD_TESTS=ON -DEUI_BUILD_EXAMPLES=OFF -DEUI_BUILD_CROSS_EXAMPLES=OFF
cmake --build .
ctest --output-on-failure
```

Expected: All tests pass.

- [ ] **Step 5: Commit final verification status**

```bash
git add -A
git diff --cached --stat
git commit -m "chore: final verification — cross-platform examples build on PC"
```

---

## Execution Order

Tasks must execute sequentially because later tasks depend on files and libraries created by earlier tasks:

1. **Task 1** — Bootstrap API header (no dependencies)
2. **Task 2** — Raylib bootstrap (depends on Task 1)
3. **Task 3** — ESP-IDF bootstrap (depends on Task 1)
4. **Task 4** — nRF5 bootstrap (depends on Task 1)
5. **Task 5** — CMake infrastructure (depends on Tasks 2-4)
6. **Task 6** — Kconfig + PC config generation (depends on Task 5 for directory structure)
7. **Task 7** — basic_label example (depends on Tasks 5-6)
8. **Task 8** — button_test example (depends on Tasks 5-7)
9. **Task 9** — list_nav example (depends on Tasks 5-8)
10. **Task 10** — Final verification (depends on all)

Tasks 2, 3, 4 can run in parallel (they are independent port implementations).
Tasks 7, 8, 9 can run in parallel (they are independent examples).
