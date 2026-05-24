# Web Port Design

## Overview

Add a web-based port to EUI using Emscripten + HTML5 Canvas, enabling EUI examples to run directly in a browser. The port serves two primary purposes: (1) live showcase of EUI examples on documentation sites, and (2) fast development feedback loop without requiring a native desktop setup.

**Scope**: Cross-platform examples only (`examples/cross/basic_label`, `button_test`, `list_nav`). Legacy `examples/*.c` are out of scope.

## Design Decision: Native HTML5 Canvas Driver

**Chosen approach**: Write a new lightweight `eui_drv_web` driver that renders EUI's framebuffer directly to an HTML5 `<canvas>` element and captures keyboard/touch events from the DOM. No raylib dependency in the web build.

This was chosen over the Emscripten + Raylib alternative because:

- **Binary size**: ~50KB WASM vs 500KB+ with raylib. Critical for embedded documentation sites.
- **Web-native feel**: Canvas-based rendering integrates seamlessly into documentation pages; raylib web builds look like a desktop app trapped in a browser tab.
- **Dependency hygiene**: Avoids pulling in raylib for a port that doesn't need OpenGL, audio, or 3D math.
- **Implementation effort**: EUI's HAL interface is minimal — `write_buffer` + `poll` — making a from-scratch driver about 400 lines of C + 100 lines of JS glue.

### Rejected Alternative

Emscripten + Raylib: compile existing `eui_drv_raylib` to WASM via raylib's built-in emscripten backend. Rejected because the large binary and desktop-app-in-browser presentation conflict with showcase and dev-convenience goals. Also rejected because the native approach is straightforward enough that the shortcut isn't worth the long-term baggage.

## Architecture

### File Layout

```
port/web/
├── index.html                 ← Example navigation main page
├── shell.html                 ← Shared HTML template for individual examples
├── shell.js                   ← Canvas setup, virtual D-pad, keyboard capture, main loop
├── CMakeLists.txt             ← Web bootstrap library build
├── eui_port_bootstrap.c       ← Web port bootstrap (init HAL, main loop, export tick)
└── eui_port_bootstrap.h       ← eui_example_setup() signature

src/driver/eui_drv_web.c       ← Web Canvas display + input HAL driver
include/eui/driver/eui_drv_web.h ← Driver public API header
```

### Data Flow

```
eui_tick() → canvas commit()
    → display->write_buffer(fb)
        → EM_ASM: fb pixels → RGBA Uint8ClampedArray
        → CanvasRenderingContext2D.putImageData()

eui_input_update()
    → input->poll()
        → reads from shared ring buffer in WASM memory
        ← JS keydown/keyup + virtual D-pad touch events enqueue here
    → event dispatched to eui_view_dispatcher → active view
```

### Bootstrap Pattern

Follows the existing cross-platform bootstrap pattern established by `port/raylib/eui_port_bootstrap.c`:

1. `eui_port_bootstrap_init()` — calls example's `eui_example_setup()`, creates canvas display + keyboard input HALs, initializes EUI
2. JS side manages the `requestAnimationFrame` loop, calling into WASM's `eui_port_tick()` each frame
3. `eui_port_tick()` calls `eui_tick()` then triggers canvas refresh

## Display HAL (`eui_drv_web.c`)

### `eui_drv_web_create_display(width, height, color_depth)`

Creates and returns an `eui_display_drv_t*` allocated via `eui_malloc()`.

**`init(void *user_data)`**:
- Retrieves the `<canvas>` element by ID (passed via `user_data` as a C string)
- Creates a `CanvasRenderingContext2D` via `EM_ASM`
- Stores the context reference for later use in `write_buffer`

**`write_buffer(const uint8_t *buffer, const eui_rect_t *rect, void *user_data)`**:
- Main rendering path. Called by `eui_canvas_commit()` with the full framebuffer.
- Converts EUI pixel format to RGBA:
  - **1bpp**: Each bit → black RGBA (0,0,0,255) for bit=1, white RGBA (255,255,255,255) for bit=0. Consistent with monochrome OLED convention.
  - **16bpp (RGB565)**: Each 16-bit word → 8-bit R/G/B/A components.
- Uses `EM_ASM` to inline the conversion loop in JS, avoiding extra allocations on the C heap.
- Calls `ctx.putImageData(imageData, 0, 0)` to render.

**`deinit(void *user_data)`**:
- Releases JS-side references (context, image data).

**`draw_pixel`, `set_contrast`, `set_power`, `set_invert`, `fill_rect`**: All no-ops (same as raylib driver).

### Buffer Mode

Fixed to `EUI_BUFFER_FULL`. Browser Canvas has no memory constraints and no physical GRAM — full framebuffer is the natural model.

### Frame Rate

Canvas refresh is synchronous with EUI's tick loop. JS `requestAnimationFrame` drives the loop at ~60fps. No separate render thread or double buffering — EUI's single-threaded model maps cleanly here.

## Input HAL (`eui_drv_web.c`)

### `eui_drv_web_create_input()`

Creates and returns an `eui_input_drv_t*` allocated via `eui_malloc()`.

**Keyboard mapping** (desktop):

| Physical Key | EUI Event |
|---|---|
| Arrow Up | `EUI_KEY_UP` press/release |
| Arrow Down | `EUI_KEY_DOWN` press/release |
| Arrow Left | `EUI_KEY_LEFT` press/release |
| Arrow Right | `EUI_KEY_RIGHT` press/release |
| Enter | `EUI_KEY_OK` press/release |
| Backspace | `EUI_KEY_BACK` press/release |

**Implementation**:
- JS side registers `keydown`/`keyup` event listeners on `document`
- Filters to the 6 relevant keys, writes `eui_event_t` structs into a **shared ring buffer** in WASM linear memory
- This avoids per-event JS→WASM boundary crossing; C side reads directly from its own memory
- Ring buffer size: 32 events (generous for keyboard input)

**C side `poll(eui_event_t *event, void *user_data)`**:
- Dequeues from the ring buffer into `*event`
- Returns 0 (has event) or 1 (no events)
- EUI's `eui_input_update()` handles debounce and long-press repeat — no change needed in the driver

### Mobile Virtual D-Pad (`shell.js`)

Rendered below the canvas as a set of HTML buttons:
```
        [↑]
   [←]  [OK]  [→]
        [↓]
             [BACK]
```

- Touch events on these buttons are mapped to `EUI_KEY_*` events and enqueued into the same ring buffer
- Buttons use `touchstart`/`touchend` to avoid 300ms click delay
- Canvas itself is touch-passive (no zoom/scroll interference)
- Layout is CSS Flexbox, works on screens ≥320px wide

## Build System Integration

### Trigger

```bash
emcmake cmake -B build \
  -DEUI_BUILD_WEB=ON \
  -DEUI_BUILD_CROSS_EXAMPLES=ON \
  -DEUI_TARGET_PORT=web \
  -DEUI_COLOR_DEPTH=1
cmake --build build -j$(nproc)
```

### CMake Changes

**Top-level `CMakeLists.txt`**:
- New option: `option(EUI_BUILD_WEB "Build web (Emscripten) port" OFF)`
- When `EUI_BUILD_WEB=ON`:
  - Sets `EMSCRIPTEN` compile definitions where needed
  - Skips raylib submodule build (not needed for web)
  - Detects Emscripten toolchain (check `EMSCRIPTEN` env var or `emcc` on PATH)

**`src/CMakeLists.txt`**:
- When `EUI_BUILD_WEB`: adds `driver/eui_drv_web.c` to the `eui` static library
- Driver is compiled into `libeui.a` rather than a separate library to allow Emscripten link-time optimization across the full call graph

**`port/web/CMakeLists.txt`** (new):
```
eui_port_bootstrap.c  →  eui_port_bootstrap.o
  + libeui.a
  → basic_label.wasm + basic_label.js
  → button_test.wasm + button_test.js
  → list_nav.wasm + list_nav.js
```

- Links each cross example's `eui_example_setup()` against the web bootstrap
- Emscripten flags: `-s WASM=1`, `-s EXPORTED_FUNCTIONS='["_eui_port_init","_eui_port_tick","_malloc","_free"]'`, `-s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]'`
- `add_custom_command` copies `shell.html` → `basic_label.html` etc., injects the correct `.js` script reference

### Output Structure

```
build/web/
├── index.html              ← Navigation page linking to all examples
├── shell.html              ← Shared template (not served directly)
├── shell.js                ← Shared JS runtime
├── basic_label.html        ← basic_label example page
├── basic_label.js          ← Emscripten JS glue
├── basic_label.wasm        ← Compiled example
├── button_test.html
├── button_test.js
├── button_test.wasm
├── list_nav.html
├── list_nav.js
├── list_nav.wasm
```

### Serving for Development

```bash
cd build/web && python3 -m http.server 8000
# Open http://localhost:8000 in browser
```

No build step needed for iteration — just recompile WASM and refresh browser.

## Example Pages

### Navigation Page (`index.html`)

Simple listing page with links to each example:
- `basic_label.html` — Basic Label
- `button_test.html` — Button Test
- `list_nav.html` — List Navigation

Structured as a minimal HTML page with a `<ul>` linking to each example.

### Example Page (`shell.html` template)

Each example page contains:
- **Canvas element** (id=`eui-canvas`, sized to `width * scale x height * scale` pixels)
- **Virtual D-pad** (id=`eui-dpad`, hidden on non-touch devices via CSS media query)
- **Example title** as `<h1>`
- **Status bar** showing FPS counter (optional, toggled via URL param `?fps=1`)

Layout: Canvas centered, D-pad below, title above. Responsive CSS, mobile-first.

## Testing Strategy

No separate test suite for the web port. Verification approach:

1. **Build test**: `emcmake cmake -B build -DEUI_BUILD_WEB=ON ...` must succeed
2. **Manual smoke test**: Open each of the 3 example pages in Chrome/Firefox/Safari, verify:
   - Canvas renders the EUI widget (label text, buttons, list items)
   - Arrow keys navigate focus between widgets
   - Enter/Backspace triggers expected behavior (button click, list selection)
   - Mobile D-pad works equivalently to keyboard
3. **Existing C tests**: `ctest` suite continues to pass; web port adds no regressions to desktop builds

No automated browser testing in initial implementation.

## Non-Goals (Explicitly Out of Scope)

- 4bpp or 8bpp color depth support (1bpp and 16bpp only)
- Legacy `examples/*.c` migration (only `examples/cross/` examples)
- Production web app deployment (no offline support, no PWA, no CDN optimization)
- Touch events as `EUI_EVT_TOUCH_DOWN/UP/MOVE` (only D-pad mapping)
- Multi-touch or gesture support
- WebGL rendering backend (Canvas2D only)
- Automated CI with headless browser testing
- Sound/audio output
- EUI buffer modes other than FULL (PAGE and DIRECT are not needed in browser)
