# Cross-Platform Examples Design

## Overview

EUI examples currently exist as separate per-platform implementations:
- `examples/*.c` — raylib desktop only
- `examples/esp-idf/` — ESP-IDF only

This design introduces a cross-platform example system where the same example code runs on all three supported platforms (raylib/PC, esp-idf, nrf5) by extracting platform-specific concerns into a port-level bootstrap layer.

## Design Decision: Port Bootstrap Layer

**Chosen approach**: Each port provides a bootstrap implementation (`eui_port_bootstrap.c`) that handles all platform-specific setup (HAL creation, display driver init, tick source, main loop). Example code calls the bootstrap API and contains only pure UI logic.

This was chosen over the alternative (example core library + per-port main files) because:
- Example files are simpler — one `.c` per example, no per-port entry files
- Bootstrap is implemented once per port, shared by all examples
- The bootstrap abstraction is natural given EUI's existing HAL architecture

### Rejected Alternative

Example-core-as-library with per-port `main_<port>.c` files. Rejected because with scheme B, example authors write less boilerplate and adding a new port requires no changes to existing examples.

## Architecture

```
examples/
  basic_label/
    basic_label.c         # Pure UI logic (shared across all ports)
    CMakeLists.txt         # Links eui + BOOTSTRAP_LIB
    Kconfig                # Hardware config (pins, driver choice)

port/
  esp-idf/
    eui_port_bootstrap.c   # esp-idf bootstrap implementation
  nrf5/
    eui_port_bootstrap.c   # nrf5 bootstrap implementation
  raylib/
    eui_port_bootstrap.c   # raylib bootstrap implementation

include/eui/
  eui_port_bootstrap.h     # Shared bootstrap API header
```

## Bootstrap API

```c
// include/eui/eui_port_bootstrap.h

typedef struct {
    int display_width;
    int display_height;
    // Filled from Kconfig defaults or menuconfig
} eui_example_config_t;

// Called by bootstrap entry point. Example implements this to build UI.
// eui_init() has already been called; the display is ready.
void eui_example_setup(const eui_example_config_t *cfg);

// Internal to bootstrap (not called by example code):
// void eui_port_bootstrap_main(const eui_example_config_t *cfg);
//   1. Allocates memory pool, creates HAL + display driver
//   2. Calls eui_init() + eui_set_tick_callback()
//   3. Calls eui_example_setup(cfg)  -- the example's UI build
//   4. Enters blocking main loop (never returns on embedded)
//   5. On PC: calls eui_deinit() + cleanup on loop exit
```

### Per-Platform Implementation Differences

| | raylib (PC) | esp-idf | nrf5 |
|---|---|---|---|
| Memory pool | Static array | Static array | Static array |
| Display driver | eui_drv_raylib | SSD1306 or ST7735 (Kconfig) | SSD1306 or ST7735 (Kconfig) |
| Input driver | eui_drv_raylib | eui_drv_buttons / encoder | eui_drv_buttons / encoder |
| Tick source | GetTime() | esp_timer_get_time() | app_timer_cnt_get() |
| Main loop | while (!WindowShouldClose) | FreeRTOS while(1) + vTaskDelay | while(1) + nrf_delay_ms |
| Bootstrap entry | `main()` calls `eui_port_bootstrap_main()` | `app_main()` calls `eui_port_bootstrap_main()` | `main()` calls `eui_port_bootstrap_main()` |

The bootstrap handles the platform entry point (`main` vs `app_main`), so the example file never defines `main`. The example only implements `eui_example_setup()`.

## Example Code Shape

```c
// examples/basic_label/basic_label.c
#include "eui/eui.h"
#include "eui/eui_port_bootstrap.h"

// Pure UI logic, identical across all platforms.
// Called by bootstrap after eui_init() is complete.
void eui_example_setup(const eui_example_config_t *cfg) {
    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();

    eui_widget_t *label = eui_label_create("Hello EUI!", 10, 5);
    eui_view_dispatcher_add(vd, 1, &label->view);
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);
}
```

## CMake Build Model

### Port Selection

A single `EUI_TARGET_PORT` cache variable controls which bootstrap library is linked:

```cmake
# Root CMakeLists.txt or examples/CMakeLists.txt
if(NOT DEFINED EUI_TARGET_PORT)
    set(EUI_TARGET_PORT "raylib" CACHE STRING "Target port: raylib, esp-idf, nrf5")
endif()

if(EUI_TARGET_PORT STREQUAL "raylib")
    set(BOOTSTRAP_LIB eui_port_raylib)
elseif(EUI_TARGET_PORT STREQUAL "esp-idf")
    set(BOOTSTRAP_LIB eui_port_esp_idf)
elseif(EUI_TARGET_PORT STREQUAL "nrf5")
    set(BOOTSTRAP_LIB eui_port_nrf5)
endif()
```

### Example CMakeLists.txt (per example, minimal)

```cmake
add_executable(basic_label basic_label.c)
target_include_directories(basic_label PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(basic_label PRIVATE eui ${BOOTSTRAP_LIB})
```

### CMake Presets

```json
{
  "configurePresets": [
    { "name": "pc",    "cacheVariables": { "EUI_TARGET_PORT": "raylib" } },
    { "name": "esp32", "cacheVariables": { "EUI_TARGET_PORT": "esp-idf" } },
    { "name": "nrf52", "cacheVariables": { "EUI_TARGET_PORT": "nrf5" } }
  ]
}
```

Usage: `cmake --preset pc -B build_pc && cmake --build build_pc`

### Bootstrap Libraries

| Port | Library Target | Dependencies |
|------|---------------|-------------|
| raylib | `eui_port_raylib` | `eui` + `eui_drv_raylib` + `raylib` |
| esp-idf | `eui_port_esp_idf` | `eui` + ESP-IDF components (i2c, spi, gpio) |
| nrf5 | `eui_port_nrf5` | `eui` + nRF5 SDK components (twi, spi, gpiote) |

## Kconfig Hardware Configuration

Kconfig manages only hardware parameters (pins, driver choice, bus config). Framework parameters (color depth, memory pool size, widget limits) remain as root CMakeLists.txt cache variables.

### Example Kconfig

```kconfig
menu "Display Configuration"
    choice "Display Driver"
        default EUI_EXAMPLE_DISPLAY_SSD1306
        config EUI_EXAMPLE_DISPLAY_SSD1306
            bool "SSD1306 (I2C OLED)"
        config EUI_EXAMPLE_DISPLAY_ST7735
            bool "ST7735 (SPI TFT)"
    endchoice

    config EUI_EXAMPLE_DISPLAY_WIDTH
        int "Display Width"
        default 128
    config EUI_EXAMPLE_DISPLAY_HEIGHT
        int "Display Height"
        default 64
endmenu

menu "I2C (for SSD1306)"
    depends on EUI_EXAMPLE_DISPLAY_SSD1306
    config EUI_EXAMPLE_I2C_PORT
        int "I2C Port"         default 0
    config EUI_EXAMPLE_I2C_SDA
        int "SDA Pin"          default 21
    config EUI_EXAMPLE_I2C_SCL
        int "SCL Pin"          default 22
    config EUI_EXAMPLE_I2C_FREQ
        int "I2C Freq (Hz)"    default 400000
    config EUI_EXAMPLE_I2C_ADDR
        hex "I2C Address"      default 0x3C
endmenu

menu "SPI (for ST7735)"
    depends on EUI_EXAMPLE_DISPLAY_ST7735
    config EUI_EXAMPLE_SPI_HOST
        int "SPI Host"         default 1
    config EUI_EXAMPLE_SPI_MOSI
        int "MOSI Pin"         default 23
    config EUI_EXAMPLE_SPI_SCLK
        int "SCLK Pin"         default 18
    config EUI_EXAMPLE_SPI_CS
        int "CS Pin"           default 5
    config EUI_EXAMPLE_SPI_DC
        int "DC Pin"           default 22
    config EUI_EXAMPLE_SPI_RST
        int "RST Pin"          default 21
endmenu
```

### Per-Platform Kconfig Handling

| Platform | Mechanism |
|----------|-----------|
| esp-idf | Native Kconfig support via `idf.py menuconfig` |
| nrf5 | nRF Connect SDK (Zephyr-based) supports Kconfig; bare nRF5 SDK uses defaults from header |
| raylib/PC | CMake parses Kconfig `default` values and generates `eui_example_config_auto.h` at configure time. No kconfig tool required on PC. Users can override via `-D` flags. |

## Migration Plan

### Phase 1: Bootstrap Implementation
Implement `eui_port_bootstrap.c` in all three port directories.

### Phase 2: Migrate Examples (in order)
1. **`basic_label`** — simplest, no input, validates full cross-platform pipeline
2. **`button_test`** — introduces input HAL, validates input across platforms
3. **`list_nav`** — complex UI + input, validates end-to-end

### Phase 3: Coexistence
Existing platform-specific examples (`examples/*.c`, `examples/esp-idf/`) remain in place. Two CMake options control which examples are built:
- `EUI_BUILD_EXAMPLES` — legacy raylib-only examples
- `EUI_BUILD_CROSS_EXAMPLES` — new cross-platform examples

Once the new system is proven, legacy examples can be removed.

## Testing & Verification

| Verification | Method |
|-------------|--------|
| PC compile + run | `cmake --preset pc && cmake --build . && ./examples/basic_label/basic_label` |
| ESP-IDF compile | `idf.py set-target esp32 && idf.py build` from example directory |
| nRF5 compile | ARM GCC build, verify link succeeds |
| Unit tests | Core UI logic is testable on PC without bootstrap (pass mock eui_config_t) |
| CI integration | Add cross-platform example builds to GitHub Actions matrix |

## Scope Boundaries

**In scope:**
- Bootstrap API and three implementations (raylib, esp-idf, nrf5)
- CMake presets and `EUI_TARGET_PORT` switching
- Kconfig for hardware config with PC fallback via CMake header generation
- Migration of `basic_label`, `button_test`, `list_nav` examples

**Out of scope:**
- Migrating all 11+ existing examples (future work)
- Platform-specific input config via Kconfig (basic buttons only initially)
- nRF5 bare-metal SDK Kconfig integration (use header defaults)
- Bootstrap support for non-standard setups (multiple displays, custom drivers) — users can copy the bootstrap and modify

## References

- Existing HAL: `include/eui/hal/eui_hal_types.h`
- Existing port implementations: `port/esp-idf/`, `port/nrf5/`
- Existing examples: `examples/basic_label.c`, `examples/esp-idf/ssd1306/main/main.c`
- Framework config: root `CMakeLists.txt` (EUI_COLOR_DEPTH, EUI_MEM_POOL_SIZE, etc.)
