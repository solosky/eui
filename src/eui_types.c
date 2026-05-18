#include "eui/eui_types.h"

bool eui_rect_intersect(const eui_rect_t *a, const eui_rect_t *b, eui_rect_t *out)
{
    int16_t left   = a->x > b->x ? a->x : b->x;
    int16_t right  = a->x + (int16_t)a->w < b->x + (int16_t)b->w ? a->x + (int16_t)a->w : b->x + (int16_t)b->w;
    int16_t top    = a->y > b->y ? a->y : b->y;
    int16_t bottom = a->y + (int16_t)a->h < b->y + (int16_t)b->h ? a->y + (int16_t)a->h : b->y + (int16_t)b->h;

    if (left >= right || top >= bottom) {
        return false;
    }

    out->x = left;
    out->y = top;
    out->w = (uint16_t)(right - left);
    out->h = (uint16_t)(bottom - top);

    return true;
}

bool eui_rect_contains(const eui_rect_t *rect, int16_t x, int16_t y)
{
    return (x >= rect->x && x < rect->x + (int16_t)rect->w &&
            y >= rect->y && y < rect->y + (int16_t)rect->h);
}

void eui_rect_union(const eui_rect_t *a, const eui_rect_t *b, eui_rect_t *out)
{
    int16_t left   = a->x < b->x ? a->x : b->x;
    int16_t top    = a->y < b->y ? a->y : b->y;
    int16_t right  = a->x + (int16_t)a->w > b->x + (int16_t)b->w ? a->x + (int16_t)a->w : b->x + (int16_t)b->w;
    int16_t bottom = a->y + (int16_t)a->h > b->y + (int16_t)b->h ? a->y + (int16_t)a->h : b->y + (int16_t)b->h;

    out->x = left;
    out->y = top;
    out->w = (uint16_t)(right - left);
    out->h = (uint16_t)(bottom - top);
}

eui_color_t eui_color_from_rgb(uint8_t r, uint8_t g, uint8_t b)
{
#if EUI_COLOR_DEPTH == 1
    uint16_t avg = ((uint16_t)r + (uint16_t)g + (uint16_t)b) / 3u;
    return (avg >= 128) ? EUI_COLOR_WHITE : EUI_COLOR_BLACK;
#elif EUI_COLOR_DEPTH == 4
    uint8_t gray = (uint8_t)(((uint16_t)r * 30u + (uint16_t)g * 59u + (uint16_t)b * 11u) / 100u);
    return (eui_color_t)(gray * 15u / 255u);
#elif EUI_COLOR_DEPTH == 8
    return (eui_color_t)(((uint16_t)r * 30u + (uint16_t)g * 59u + (uint16_t)b * 11u) / 100u);
#elif EUI_COLOR_DEPTH == 16
    return (eui_color_t)(((uint16_t)(r >> 3) << 11) | ((uint16_t)(g >> 2) << 5) | (uint16_t)(b >> 3));
#else
    #error "EUI_COLOR_DEPTH must be 1, 4, 8, or 16"
#endif
}

eui_color_t eui_color_from_gray(uint8_t gray)
{
#if EUI_COLOR_DEPTH == 1
    return (gray >= 128) ? EUI_COLOR_WHITE : EUI_COLOR_BLACK;
#elif EUI_COLOR_DEPTH == 4
    return (eui_color_t)(gray * 15u / 255u);
#elif EUI_COLOR_DEPTH == 8
    return (eui_color_t)gray;
#elif EUI_COLOR_DEPTH == 16
    return (eui_color_t)(((uint16_t)(gray >> 3) << 11) | ((uint16_t)(gray >> 2) << 5) | (uint16_t)(gray >> 3));
#else
    #error "EUI_COLOR_DEPTH must be 1, 4, 8, or 16"
#endif
}
