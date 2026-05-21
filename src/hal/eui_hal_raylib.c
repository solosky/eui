#include "eui/hal/eui_hal_raylib.h"
#include "eui/eui_allocator.h"
#include <raylib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ---- Display HAL ---- */

typedef struct {
    eui_display_hal_t base;
    RenderTexture2D   fb;
    uint16_t          width;
    uint16_t          height;
    uint8_t           color_depth;
    int               scale;
    uint8_t          *rgba_buffer;
} raylib_display_t;

static raylib_display_t *g_active_display = NULL;

static int disp_init(void *ud) {
    raylib_display_t *d = (raylib_display_t*)ud;
    int sw = d->width * d->scale;
    int sh = d->height * d->scale;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(sw, sh, "EUI - raylib");
    d->fb = LoadRenderTexture(d->width, d->height);
    SetTextureWrap(d->fb.texture, TEXTURE_WRAP_CLAMP);
    d->rgba_buffer = (uint8_t*)malloc((size_t)d->width * d->height * 4);
    g_active_display = d;
    SetTargetFPS(60);
    return 0;
}

static int disp_deinit(void *ud) {
    raylib_display_t *d = (raylib_display_t*)ud;
    g_active_display = NULL;
    free(d->rgba_buffer);
    d->rgba_buffer = NULL;
    UnloadRenderTexture(d->fb);
    CloseWindow();
    return 0;
}

static void disp_draw_pixel(int16_t x, int16_t y, eui_color_t color, void *ud) {
    (void)x; (void)y; (void)color; (void)ud;
}

static void disp_write_buffer(const uint8_t *buffer, const eui_rect_t *rect, void *ud) {
    raylib_display_t *d = (raylib_display_t*)ud;
    (void)rect;

    if (!d->rgba_buffer) return;

    if (d->color_depth == 1) {
        int bytes_per_row = (int)d->width / 8;
        for (int y = 0; y < (int)d->height; y++) {
            int flipped_y = (int)d->height - 1 - y;
            for (int x = 0; x < (int)d->width; x++) {
                int byte_idx = y * bytes_per_row + (x / 8);
                int bit_pos = x % 8;
                uint8_t pixel = (buffer[byte_idx] >> bit_pos) & 1;
                uint8_t *dst = d->rgba_buffer + (size_t)(flipped_y * d->width + x) * 4;
                if (pixel) {
                    dst[0] = 255; dst[1] = 255; dst[2] = 255; dst[3] = 255;
                } else {
                    dst[0] = 0;   dst[1] = 0;   dst[2] = 0;   dst[3] = 255;
                }
            }
        }
    } else if (d->color_depth == 16) {
        const uint16_t *src16 = (const uint16_t*)buffer;
        for (int y = 0; y < (int)d->height; y++) {
            int flipped_y = (int)d->height - 1 - y;
            for (int x = 0; x < (int)d->width; x++) {
                uint16_t c = src16[y * d->width + x];
                uint8_t r = (uint8_t)(((c >> 11) & 0x1F) << 3);
                uint8_t g = (uint8_t)(((c >> 5)  & 0x3F) << 2);
                uint8_t b = (uint8_t)((c & 0x1F) << 3);
                uint8_t *dst = d->rgba_buffer + (size_t)(flipped_y * d->width + x) * 4;
                dst[0] = r | (r >> 5);
                dst[1] = g | (g >> 6);
                dst[2] = b | (b >> 5);
                dst[3] = 255;
            }
        }
    } else {
        for (int y = 0; y < (int)d->height; y++) {
            int flipped_y = (int)d->height - 1 - y;
            for (int x = 0; x < (int)d->width; x++) {
                uint8_t *dst = d->rgba_buffer + (size_t)(flipped_y * d->width + x) * 4;
                dst[0] = 0; dst[1] = 0; dst[2] = 0; dst[3] = 255;
            }
        }
    }

    UpdateTexture(d->fb.texture, d->rgba_buffer);
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

void eui_hal_raylib_save_screenshot(const char *filename) {
    if (!g_active_display) return;
    raylib_display_t *d = g_active_display;
    if (!d->rgba_buffer) return;

    FILE *f = fopen(filename, "wb");
    if (!f) return;

    /* PPM P6 binary format: header + 24-bit RGB data */
    fprintf(f, "P6\n%d %d\n255\n", (int)d->width, (int)d->height);

    /* RGBA -> RGB, skipping alpha */
    for (int y = 0; y < (int)d->height; y++) {
        for (int x = 0; x < (int)d->width; x++) {
            uint8_t *p = d->rgba_buffer + (size_t)(y * d->width + x) * 4;
            fwrite(p, 1, 3, f);
        }
    }

    fclose(f);
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
