# EUI — AGENTS.md

## Project Overview

EUI (Embedded UI Framework) is a lightweight embedded GUI framework in C99, targeting MCUs with < 32KB RAM. Uses raylib for desktop simulation, TLSF for deterministic memory allocation, and MotionC for FPU-free animation.

## Build System

- **Build tool:** CMake 3.22+
- **Language:** C99
- **Submodules:** `third_party/tlsf/`, `third_party/motionc/`, `third_party/raylib/`

### Key CMake variables

| Variable | Default | Desc |
|---|---|---|
| `EUI_BUILD_TESTS` | `ON` | Build unit tests |
| `EUI_BUILD_EXAMPLES` | `ON` | Build raylib desktop examples |
| `EUI_COLOR_DEPTH` | `1` | 1, 4, 8, or 16 bpp |
| `EUI_MEM_POOL_SIZE` | `8192` | TLSF pool size in bytes |
| `EUI_FONT_ENABLE_U8G2` | `ON` | u8g2-style BDF font support |
| `EUI_FONT_ENABLE_KERNING` | `ON` | Font kerning support |
| `EUI_FONT_ENABLE_MULTILINE` | `ON` | Multiline text support |

### Build Commands

```bash
# Desktop dev (raylib required: apt install libraylib-dev / brew install raylib)
cmake -B build -DEUI_BUILD_EXAMPLES=ON -DEUI_COLOR_DEPTH=1
cmake --build build -j$(nproc)

# Run tests
cmake -B build -DEUI_BUILD_TESTS=ON
cmake --build build -j$(nproc)
cd build && ctest

# Embedded (cross-compile)
cmake -B build -DCMAKE_TOOLCHAIN_FILE=arm-none-eabi.cmake -DEUI_BUILD_TESTS=OFF -DEUI_BUILD_EXAMPLES=OFF
cmake --build build
```

## Project Structure

```
eui/
├── include/eui/          # Public headers (26 files, single-include: eui.h)
│   ├── eui.h             # Main entry header, pulls in everything
│   ├── eui_types.h       # Core types: rect, color, bitmap, font
│   ├── eui_allocator.h   # External allocator interface
│   ├── eui_canvas.h      # Canvas drawing API
│   ├── eui_view.h        # View system
│   ├── eui_widget*.h     # Widget library headers
│   ├── eui_config.h.in   # Compile-time config template
│   ├── driver/           # Built-in display/input driver headers
│   └── hal/              # HAL transport & raylib headers
├── src/                  # Framework implementation (29 .c files)
│   ├── eui.c             # Framework init/tick/deinit
│   ├── eui_allocator.c   # TLSF wrapper
│   ├── eui_canvas.c      # Canvas drawing primitives
│   ├── eui_font*.c       # Font engine (bdf, vlw, u8g2 formats)
│   ├── eui_view*.c       # View + ViewDispatcher
│   ├── eui_widget*.c     # Widget library (label, button, list, menu, etc.)
│   ├── eui_anim.c        # Animation adapter (MotionC bridge)
│   ├── eui_event.c       # Event queue
│   ├── eui_input.c       # Input manager
│   ├── driver/           # Platform-agnostic display/input drivers
│   │   ├── eui_drv_ssd1306.c   # SSD1306 OLED (I2C)
│   │   ├── eui_drv_sh1106.c    # SH1106 OLED (I2C)
│   │   ├── eui_drv_st7735.c    # ST7735 TFT (SPI)
│   │   ├── eui_drv_ili9341.c   # ILI9341 TFT (SPI)
│   │   ├── eui_drv_buttons.c   # GPIO keypad
│   │   ├── eui_drv_encoder.c   # Rotary encoder
│   │   └── eui_drv_xpt2046.c   # XPT2046 touchscreen
│   └── hal/              # Platform-specific HAL implementations
├── test/                 # Unit tests (26 .c files, CTest)
├── examples/             # Raylib desktop demos (16 .c files)
├── docs/                 # Framework design, API reference, porting guide, example guidelines
├── tools/                # Font data generation scripts (Python)
├── third_party/
│   ├── tlsf/             # TLSF memory allocator (git submodule)
│   ├── motionc/          # MotionC animation engine (git submodule)
│   └── raylib/           # Desktop simulation library (git submodule)
└── build/                # Build output (gitignored)
```

## Architecture

Five-layer architecture, strict one-way dependencies:

1. **HAL Layer** — Display (`eui_display_hal_t`), Input (`eui_input_hal_t`), Timer HALs. Only platform-specific code.
2. **Foundation Layer** — TLSF allocator (`eui_allocator`), Animation adapter (`eui_anim`).
3. **Graphics Engine** — Canvas (immediate-mode drawing), FontEngine (BDF + VLW + u8g2 formats).
4. **View Management** — View (single handler: draw/input/enter/exit events), ViewDispatcher (switch_to + overlay stack), SceneManager.
5. **Widget Library** — Pre-built controls (Label, Button, List, Menu, Progress, Slider, ScrollContainer, Dialog).

### Widget Architecture

- C struct inheritance: `eui_widget_t` must be the **first member** of any custom widget struct.
- Virtual function table: `eui_widget_vtable_t` with `draw`, `input`, `enter`, `exit`, `layout`, `destroy`.
- Widgets are Views — framework uses `container_of` to bridge View events to widget vtable methods.
- Widget tree: parent/children with Z-order drawing (index 0 first), reverse-order input dispatch.

### Buffer Modes

| Mode | Memory | When to use |
|---|---|---|
| `EUI_BUFFER_FULL` | W×H×bpp/8 | Displays with GRAM |
| `EUI_BUFFER_PAGE` | W×8×bpp/8 (~1KB) | Low RAM MCU + large display |
| `EUI_BUFFER_DIRECT` | 0 | Extreme memory constraints |

## Code Conventions

- **Language:** C99, no compiler extensions.
- **Naming:** `eui_` prefix for all public symbols. `eui_module_action_object()` pattern.
- **Struct inheritance:** Base struct as first member, `container_of` for upcasting.
- **Error handling:** Return `int` (0 = success, -1 = error) for init/creation functions. `bool` for event handlers (`true` = handled).
- **Memory:** Use `eui_malloc()` / `eui_free()` only. Never use stdlib `malloc`/`free`. Static initialization macros (`EUI_LIST()`, `EUI_MENU()`) for zero-runtime-allocation in tight RAM environments.
- **Header includes:** Single `#include "eui.h"` is the main entry. Config is auto-generated from `eui_config.h.in`.
- **Comments:** Minimal. Chinese comments in documentation files are acceptable; code comments should be constrained.
- **Tests:** Simple `assert()`-based unit tests (no test framework). Each `.c` file is a standalone executable linked to `libeui.a`. Run with `ctest`.

## Key Patterns

### Creating a custom widget
```c
typedef struct { eui_widget_t widget; /* custom fields */ } my_widget_t;
static void my_draw(eui_widget_t *w, eui_canvas_t *c) { /* ... */ }
static bool my_input(eui_widget_t *w, const eui_event_t *e) { /* ... */ }
static const eui_widget_vtable_t my_vt = { .draw = my_draw, .input = my_input };
// Create: eui_malloc + eui_widget_init(&w->widget, &my_vt, x, y, w, h);
```

### Display HAL (minimum)
```c
eui_display_hal_t hal = {
    .caps = { .width=128, .height=64, .color_depth=1, .buffer_mode=EUI_BUFFER_PAGE },
    .init = my_init, .write_buffer = my_flush,
};
```

### Input HAL (minimum)
```c
eui_input_hal_t hal = { .poll = my_poll };
// my_poll fills eui_event_t, returns 1 if event ready, 0 otherwise
```

## Dependencies

- **TLSF** (submodule): Two-Level Segregated Fit allocator. O(1) malloc/free.
- **MotionC** (submodule): Q16.16 fixed-point animation engine, 30 easing functions, spring physics.
- **Raylib** (submodule, optional): Desktop simulation only. Not used on target hardware.
- **U8g2** (optional, external): Reference comparison test only. Path set via `U8G2_DIR` CMake variable.

## Cross-Platform Examples

See `docs/example_guidelines.md` for the standard example pattern (source
skeleton, CMakeLists.txt, requirements.cmake) and accepted variations.

### Key rules
- Include `eui/eui_port_bootstrap.h`, define `eui_example_setup()`.
- No `main()`, no platform `#ifdef`, no driver creation.
- CMakeLists.txt links `${BOOTSTRAP_LIB}` — the build system selects the right port.

## Test Commands

```bash
# Build and run all tests
cmake -B build && cmake --build build -j$(nproc) && cd build && ctest --output-on-failure

# Run a single test
cd build && ctest -R allocator

# Build tests only (without examples)
cmake -B build -DEUI_BUILD_EXAMPLES=OFF -DEUI_BUILD_TESTS=ON
cmake --build build -j$(nproc)
```
