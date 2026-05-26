# Cross-Platform Example Guidelines

All cross-platform examples live in `examples/cross/<name>/` and share a
consistent structure that works across raylib (desktop) and Emscripten (web)
via the port bootstrap system.

## File structure

```
examples/cross/<name>/
├── CMakeLists.txt        # Build target definition
├── requirements.cmake    # (optional) Display/color constraints
└── <name>.c              # Single source file
```

## Source file skeleton

Every example must include `eui/eui_port_bootstrap.h` and define
`eui_example_setup()`:

```c
/* examples/cross/<name>/<name>.c */
#include "eui/eui.h"
#include "eui/eui_port_bootstrap.h"

void eui_example_setup(const eui_example_config_t *cfg) {
    (void)cfg;
    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();

    // Build your UI here — views, widgets, etc.
    eui_view_dispatcher_add(vd, 1, &widget->view);
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);
}
```

**Rules:**
- No `main()` function — the port bootstrap provides it.
- No platform-specific `#ifdef` blocks — the bootstrap handles raylib vs web.
- No `eui_allocator_init_tlsf()` or memory pool — the bootstrap owns them.
- No direct driver creation (`eui_drv_raylib_create_display`, etc.).

## CMakeLists.txt

```cmake
# examples/cross/<name>/CMakeLists.txt
add_executable(<name> <name>.c)
target_include_directories(<name> PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(<name> PRIVATE eui ${BOOTSTRAP_LIB})
```

`${BOOTSTRAP_LIB}` is set by the build system to `eui_port_raylib`,
`eui_port_web`, or `eui_port_esp_idf` depending on `EUI_TARGET_PORT`.

## requirements.cmake (optional)

Declare minimum display and color-depth requirements. Examples that need a
specific display size or bpp are skipped automatically when the active config
profile does not meet them.

```cmake
# examples/cross/<name>/requirements.cmake
set(EXAMPLE_REQUIRES_COLOR_DEPTH_MIN 2)
set(EXAMPLE_REQUIRES_WIDTH_MIN       400)
set(EXAMPLE_REQUIRES_HEIGHT_MIN      300)
```

Supported variables:

| Variable | Description |
|---|---|
| `EXAMPLE_REQUIRES_COLOR_DEPTH_MIN` | Minimum color depth (1, 2, 4, 8, 16) |
| `EXAMPLE_REQUIRES_WIDTH_MIN` | Minimum display width in pixels |
| `EXAMPLE_REQUIRES_HEIGHT_MIN` | Minimum display height in pixels |

## Variations

### Simple — widget-only

Minimal example using pre-built widgets:

```c
void eui_example_setup(const eui_example_config_t *cfg) {
    (void)cfg;
    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();
    eui_widget_t *label = eui_label_create("Hello EUI!", 10, 5);
    eui_view_dispatcher_add(vd, 1, &label->view);
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);
}
```

### Custom draw — raw canvas

Use when you need full control over rendering:

```c
static void my_draw(eui_widget_t *w, eui_canvas_t *c) {
    eui_canvas_set_color(c, EUI_COLOR_WHITE);
    eui_canvas_fill_rect(c, w->area.x, w->area.y, w->area.w, w->area.h);
}

static bool my_input(eui_widget_t *w, const eui_event_t *e) {
    (void)w; (void)e;
    return false;
}

static eui_widget_vtable_t my_vt = { .draw = my_draw, .input = my_input };

void eui_example_setup(const eui_example_config_t *cfg) {
    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();
    eui_widget_t *widget = eui_malloc(sizeof(eui_widget_t));
    eui_widget_init(widget, &my_vt, 0, 0, cfg->display_width, cfg->display_height);
    eui_view_dispatcher_add(vd, 1, &widget->view);
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);
}
```

### Custom view handlers

For complex navigation (multiple views, overlays):

```c
static bool my_view_handler(eui_view_event_t *evt, void *context) {
    eui_view_t *view = (eui_view_t*)context;
    switch (evt->type) {
    case EUI_VIEW_EVT_DRAW: { /* ... */ return true; }
    case EUI_VIEW_EVT_INPUT: { /* ... */ return true; }
    default: return false;
    }
}

void eui_example_setup(const eui_example_config_t *cfg) {
    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();
    eui_view_t *view = eui_malloc(sizeof(eui_view_t));
    eui_view_init(view, my_view_handler, view);
    view->area = (eui_rect_t){ 0, 0, cfg->display_width, cfg->display_height };
    eui_view_dispatcher_add(vd, 1, view);
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);
}
```

### Animation — framework animation API

Use `eui_anim_start()` for built-in tweens:

```c
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

### Custom tick — MotionC spring animation

Override the tick callback for per-frame physics (e.g. spring animations).
Must still return the current timestamp in ms.

```c
#include <math.h>
#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#define GET_TICK_MS() ((uint32_t)emscripten_get_now())
#elif defined(__has_include)
#if __has_include(<raylib.h>)
#include <raylib.h>
#define GET_TICK_MS() ((uint32_t)(GetTime() * 1000.0f))
#else
#define GET_TICK_MS() 0
#endif
#else
#define GET_TICK_MS() 0
#endif

static uint32_t g_last_tick = 0;

static uint32_t my_tick(void) {
    uint32_t now = GET_TICK_MS();
    if (g_last_tick == 0) { g_last_tick = now; return now; }
    uint32_t dt = now - g_last_tick;
    g_last_tick = now;
    if (dt > 100) dt = 100;
    // Update spring animation, mark dirty if active
    return now;
}

void eui_example_setup(const eui_example_config_t *cfg) {
    // ... setup views ...
    eui_set_tick_callback(my_tick);
    eui_anim_init();
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);
}
```

The platform-specific `GET_TICK_MS()` macros are the **only** acceptable
`#ifdef` in an example source file.

## Anti-patterns

| What not to do | Why |
|---|---|
| Define `main()` | Bootstrap provides it; duplicate will cause link error. |
| `#include <raylib.h>` directly | Use `eui/eui_port_bootstrap.h` instead; raylib is a bootstrap detail. |
| `#include "eui/driver/eui_drv_raylib.h"` | Driver creation belongs in the bootstrap, not the example. |
| `eui_allocator_init_tlsf()` | Bootstrap allocates the memory pool. |
| Hardcoded display size with `#define W 128`, `#define H 64` | Either use `cfg->display_width/height` or keep it as a fixed constant with a comment; the profile system ensures the display matches. |
| Calling `eui_init()` | Bootstrap handles framework initialization. |
| `#if defined(__EMSCRIPTEN__)` for anything other than `GET_TICK_MS()` | All platform abstraction goes through the bootstrap. |
