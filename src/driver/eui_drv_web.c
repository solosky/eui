#include "eui/driver/eui_drv_web.h"
#include "eui/eui_allocator.h"
#include <emscripten.h>
#include <string.h>

#define WEB_EVENT_Q_SIZE 32

static eui_event_t g_events[WEB_EVENT_Q_SIZE];
static int        g_head = 0;
static int        g_tail = 0;

void eui_web_push_key_event(int type, int key) {
    int next = (g_head + 1) % WEB_EVENT_Q_SIZE;
    if (next == g_tail) return;
    g_events[g_head].type      = (eui_event_type_t)type;
    g_events[g_head].data.key  = (eui_key_t)key;
    g_events[g_head].timestamp = 0;
    g_head = next;
}

static int web_pop(eui_event_t *out) {
    if (g_head == g_tail) return 0;
    *out = g_events[g_tail];
    g_tail = (g_tail + 1) % WEB_EVENT_Q_SIZE;
    return 1;
}

/* ── Display driver ──────────────────────────────────────────────── */

typedef struct {
    eui_display_drv_t base;
    uint16_t          width;
    uint16_t          height;
    uint8_t           color_depth;
} web_display_t;

static void js_render_1bpp(const uint8_t *buf, int w, int h) {
    EM_ASM({
        var src = $0;
        var sw = $1;
        var sh = $2;
        var el = document.getElementById('eui-canvas');
        if (!el) return;
        var ctx = el.getContext('2d');
        var img = ctx.createImageData(sw, sh);
        var d = img.data;
        var rb = (sw + 7) >> 3;
        for (var y = 0; y < sh; y++) {
            for (var x = 0; x < sw; x++) {
                var bit = (HEAPU8[src + y * rb + (x >> 3)] >> (x & 7)) & 1;
                var i = (y * sw + x) * 4;
                d[i] = d[i+1] = d[i+2] = bit ? 255 : 0;
                d[i+3] = 255;
            }
        }
        ctx.putImageData(img, 0, 0);
    }, buf, w, h);
}

static void js_render_2bpp(const uint8_t *buf, int w, int h) {
    EM_ASM({
        var src = $0;
        var sw = $1;
        var sh = $2;
        var el = document.getElementById('eui-canvas');
        if (!el) return;
        var ctx = el.getContext('2d');
        var img = ctx.createImageData(sw, sh);
        var d = img.data;
        var rb = sw >> 2;
        for (var y = 0; y < sh; y++) {
            for (var x = 0; x < sw; x++) {
                var shift = 6 - 2 * (x & 3);
                var pixel = (HEAPU8[src + y * rb + (x >> 2)] >> shift) & 3;
                var i = (y * sw + x) * 4;
                var v = pixel * 85;
                d[i] = d[i+1] = d[i+2] = v;
                d[i+3] = 255;
            }
        }
        ctx.putImageData(img, 0, 0);
    }, buf, w, h);
}

static void js_render_4bpp(const uint8_t *buf, int w, int h) {
    EM_ASM({
        var src = $0;
        var sw = $1;
        var sh = $2;
        var el = document.getElementById('eui-canvas');
        if (!el) return;
        var ctx = el.getContext('2d');
        var img = ctx.createImageData(sw, sh);
        var d = img.data;
        var rb = sw >> 1;
        for (var y = 0; y < sh; y++) {
            for (var x = 0; x < sw; x++) {
                var shift = (x & 1) ? 0 : 4;
                var pixel = (HEAPU8[src + y * rb + (x >> 1)] >> shift) & 15;
                var i = (y * sw + x) * 4;
                var v = pixel * 17;
                d[i] = d[i+1] = d[i+2] = v;
                d[i+3] = 255;
            }
        }
        ctx.putImageData(img, 0, 0);
    }, buf, w, h);
}

static void js_render_16bpp(const uint8_t *buf, int w, int h) {
    EM_ASM({
        var src = $0;
        var sw = $1;
        var sh = $2;
        var el = document.getElementById('eui-canvas');
        if (!el) return;
        var ctx = el.getContext('2d');
        var img = ctx.createImageData(sw, sh);
        var d = img.data;
        var s16 = src >> 1;
        for (var y = 0; y < sh; y++) {
            for (var x = 0; x < sw; x++) {
                var c = HEAPU16[s16 + y * sw + x];
                var r = ((c >> 11) & 0x1F) << 3;
                var g = ((c >> 5)  & 0x3F) << 2;
                var b = (c & 0x1F) << 3;
                var i = (y * sw + x) * 4;
                d[i] = r | (r >> 5);
                d[i+1] = g | (g >> 6);
                d[i+2] = b | (b >> 5);
                d[i+3] = 255;
            }
        }
        ctx.putImageData(img, 0, 0);
    }, buf, w, h);
}

static int disp_init(void *ud) {
    web_display_t *d = (web_display_t *)ud;
    EM_ASM({
        var el = document.getElementById('eui-canvas');
        if (el) { el.width = $0; el.height = $1; }
    }, (int)d->width, (int)d->height);
    return 0;
}

static int disp_deinit(void *ud) { (void)ud; return 0; }

static void disp_draw_pixel(int16_t x, int16_t y, eui_color_t c, void *ud) {
    (void)x; (void)y; (void)c; (void)ud;
}

static void disp_write_buffer(const uint8_t *buf, const eui_rect_t *rect,
                              void *ud) {
    web_display_t *d = (web_display_t *)ud;
    (void)rect;
    if (d->color_depth == 1)
        js_render_1bpp(buf, d->width, d->height);
    else if (d->color_depth == 2)
        js_render_2bpp(buf, d->width, d->height);
    else if (d->color_depth == 4)
        js_render_4bpp(buf, d->width, d->height);
    else
        js_render_16bpp(buf, d->width, d->height);
}

static void disp_set_contrast(uint8_t l, void *ud)  { (void)l; (void)ud; }
static void disp_set_power(bool on, void *ud)       { (void)on; (void)ud; }
static void disp_set_invert(bool inv, void *ud)     { (void)inv; (void)ud; }

static void disp_fill_rect(int16_t x, int16_t y, uint16_t w, uint16_t h,
                           eui_color_t c, void *ud) {
    (void)x; (void)y; (void)w; (void)h; (void)c; (void)ud;
}

eui_display_drv_t *eui_drv_web_create_display(uint16_t width, uint16_t height,
                                               uint8_t color_depth) {
    web_display_t *d = (web_display_t *)eui_malloc(sizeof(web_display_t));
    if (!d) return NULL;
    memset(d, 0, sizeof(*d));
    d->width       = width;
    d->height      = height;
    d->color_depth = color_depth;
    d->base.caps.width       = width;
    d->base.caps.height      = height;
    d->base.caps.color_depth = color_depth;
    d->base.caps.buffer_mode = EUI_BUFFER_FULL;
    d->base.caps.has_gram    = true;
    d->base.caps.hw_scroll   = false;
    d->base.init         = disp_init;
    d->base.deinit       = disp_deinit;
    d->base.draw_pixel   = disp_draw_pixel;
    d->base.write_buffer = disp_write_buffer;
    d->base.set_contrast = disp_set_contrast;
    d->base.set_power    = disp_set_power;
    d->base.set_invert   = disp_set_invert;
    d->base.fill_rect    = disp_fill_rect;
    d->base.user_data    = d;
    return &d->base;
}

void eui_drv_web_destroy_display(eui_display_drv_t *drv) {
    if (drv) eui_free(drv->user_data);
}

/* ── Input driver ────────────────────────────────────────────────── */

typedef struct { eui_input_drv_t base; } web_input_t;

static int  inp_init(void *ud)  { (void)ud; return 0; }
static int  inp_deinit(void *ud) { (void)ud; return 0; }

static int inp_poll(eui_event_t *evt, void *ud) {
    (void)ud;
    return web_pop(evt);
}

static void inp_set_cb(void (*cb)(const eui_event_t *), void *ud) {
    (void)cb; (void)ud;
}

eui_input_drv_t *eui_drv_web_create_input(void) {
    web_input_t *inp = (web_input_t *)eui_malloc(sizeof(web_input_t));
    if (!inp) return NULL;
    memset(inp, 0, sizeof(*inp));
    inp->base.init         = inp_init;
    inp->base.deinit       = inp_deinit;
    inp->base.poll         = inp_poll;
    inp->base.set_callback = inp_set_cb;
    inp->base.user_data    = inp;
    return &inp->base;
}

void eui_drv_web_destroy_input(eui_input_drv_t *drv) {
    if (drv) eui_free(drv->user_data);
}
