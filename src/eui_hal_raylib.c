#include "eui/eui_hal_raylib.h"
#include "eui/eui_allocator.h"
#include <raylib.h>
#include <string.h>

/* ---- Display HAL ---- */

typedef struct {
    eui_display_hal_t base;
    RenderTexture2D   fb;
    uint16_t          width;
    uint16_t          height;
    uint8_t           color_depth;
    int               scale;
} raylib_display_t;

static raylib_display_t *g_active_display = NULL;

static int disp_init(void *ud) {
    raylib_display_t *d = (raylib_display_t*)ud;
    int sw = d->width * d->scale;
    int sh = d->height * d->scale;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(sw, sh, "EUI - raylib");
    d->fb = LoadRenderTexture(d->width, d->height);
    g_active_display = d;
    SetTargetFPS(60);
    return 0;
}

static int disp_deinit(void *ud) {
    raylib_display_t *d = (raylib_display_t*)ud;
    g_active_display = NULL;
    UnloadRenderTexture(d->fb);
    CloseWindow();
    return 0;
}

static void disp_draw_pixel(int16_t x, int16_t y, eui_color_t color, void *ud) {
    (void)x; (void)y; (void)color; (void)ud;
}

static void disp_write_buffer(const uint8_t *buffer, const eui_rect_t *rect, void *ud) {
    raylib_display_t *d = (raylib_display_t*)ud;
    UpdateTexture(d->fb.texture, buffer);
    (void)rect;
}

static void disp_set_contrast(uint8_t lvl, void *ud) { (void)lvl; (void)ud; }
static void disp_set_power(bool on, void *ud) { (void)on; (void)ud; }
static void disp_set_invert(bool inv, void *ud) { (void)inv; (void)ud; }

static void disp_fill_rect(int16_t x, int16_t y, uint16_t w, uint16_t h,
                           eui_color_t color, void *ud) {
    (void)x; (void)y; (void)w; (void)h; (void)color; (void)ud;
}

eui_display_hal_t* eui_hal_raylib_create_display(uint16_t width, uint16_t height,
                                                  uint8_t color_depth) {
    raylib_display_t *d = eui_malloc(sizeof(raylib_display_t));
    if (!d) return NULL;
    memset(d, 0, sizeof(*d));
    d->width = width;
    d->height = height;
    d->color_depth = color_depth;
    d->scale = 4;
    d->base.caps.width = width;
    d->base.caps.height = height;
    d->base.caps.color_depth = color_depth;
    d->base.caps.buffer_mode = EUI_BUFFER_FULL;
    d->base.caps.has_gram = true;
    d->base.caps.hw_scroll = false;
    d->base.init = disp_init;
    d->base.deinit = disp_deinit;
    d->base.draw_pixel = disp_draw_pixel;
    d->base.write_buffer = disp_write_buffer;
    d->base.set_contrast = disp_set_contrast;
    d->base.set_power = disp_set_power;
    d->base.set_invert = disp_set_invert;
    d->base.fill_rect = disp_fill_rect;
    d->base.user_data = d;
    return &d->base;
}

void eui_hal_raylib_destroy_display(eui_display_hal_t *hal) {
    if (hal) eui_free(hal->user_data);
}

void eui_hal_raylib_refresh(void) {
    if (!g_active_display) return;
    raylib_display_t *d = g_active_display;

    BeginDrawing();
    ClearBackground(BLACK);

    Rectangle src = {
        0, 0,
        (float)d->fb.texture.width,
        -(float)d->fb.texture.height
    };
    Rectangle dst = {
        0, 0,
        (float)d->width * d->scale,
        (float)d->height * d->scale
    };
    DrawTexturePro(d->fb.texture, src, dst, (Vector2){0, 0}, 0.0f, WHITE);

    EndDrawing();
}

int eui_hal_raylib_window_should_close(void) {
    return WindowShouldClose();
}

/* ---- Input HAL ---- */

typedef struct {
    eui_input_hal_t base;
} raylib_input_t;

static int input_init(void *ud) {
    (void)ud;
    return 0;
}

static int input_deinit(void *ud) {
    (void)ud;
    return 0;
}

static int input_poll(eui_event_t *evt, void *ud) {
    (void)ud;

    static int prev_states[EUI_KEY_COUNT] = {0};
    int keys[EUI_KEY_COUNT] = {
        KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_ENTER, KEY_ESCAPE
    };

    for (int k = 0; k < EUI_KEY_COUNT; k++) {
        int pressed = IsKeyDown(keys[k]);
        if (pressed && !prev_states[k]) {
            evt->type = EUI_EVT_KEY_PRESS;
            evt->data.key = (eui_key_t)k;
            evt->timestamp = (uint32_t)(GetTime() * 1000);
            prev_states[k] = 1;
            return 1;
        }
        if (!pressed && prev_states[k]) {
            evt->type = EUI_EVT_KEY_RELEASE;
            evt->data.key = (eui_key_t)k;
            evt->timestamp = (uint32_t)(GetTime() * 1000);
            prev_states[k] = 0;
            return 1;
        }
    }

    int wheel = GetMouseWheelMove();
    if (wheel != 0) {
        evt->type = wheel > 0 ? EUI_EVT_ENCODER_CW : EUI_EVT_ENCODER_CCW;
        evt->data.enc_delta = wheel;
        evt->timestamp = (uint32_t)(GetTime() * 1000);
        return 1;
    }

    return 0;
}

static void input_set_callback(void (*cb)(const eui_event_t *evt), void *user_data) {
    (void)cb;
    (void)user_data;
}

eui_input_hal_t* eui_hal_raylib_create_input(void) {
    raylib_input_t *inp = eui_malloc(sizeof(raylib_input_t));
    if (!inp) return NULL;
    memset(inp, 0, sizeof(*inp));
    inp->base.init = input_init;
    inp->base.deinit = input_deinit;
    inp->base.poll = input_poll;
    inp->base.set_callback = input_set_callback;
    inp->base.user_data = inp;
    return &inp->base;
}

void eui_hal_raylib_destroy_input(eui_input_hal_t *hal) {
    if (hal) eui_free(hal->user_data);
}
