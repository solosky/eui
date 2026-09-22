#ifndef EUI_DRV_RAYLIB_H
#define EUI_DRV_RAYLIB_H

#include "eui/eui_display_drv.h"
#include "eui/eui_input_drv.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

eui_display_drv_t* eui_drv_raylib_create_display(uint16_t width, uint16_t height,
                                                  uint8_t color_depth);
void eui_drv_raylib_destroy_display(eui_display_drv_t *hal);
void eui_drv_raylib_refresh(void);

/**
 * @brief 键位绑定条目：键盘码或鼠标键 → key_id；enc_delta != 0 折算编码器。
 */
typedef struct {
    int     raylib_key;   /**< raylib KEY_*；鼠标条目填 -1 */
    int     mouse_btn;    /**< MOUSE_BUTTON_*；键盘条目填 -1 */
    uint8_t key_id;       /**< enc_delta == 0 时发出的按键编号 */
    int8_t  enc_delta;    /**< != 0：折算为 ENCODER_CW/CCW(±enc_delta) */
} eui_raylib_keymap_entry_t;

/* 默认 keymap：编号直译现行绑定（UP=0 DOWN=1 LEFT=2 RIGHT=3 OK=4 BACK=5），
 * 方向键折算编码器（±1），鼠标左/右键为 OK/BACK。 */
eui_input_drv_t *eui_drv_raylib_create_input(void);
/**
 * @brief Create the raylib input HAL from a custom keymap.
 * @param entries  Keymap entries (the driver keeps the pointer; the caller
 *                 must keep the array alive for the driver's lifetime).
 * @param count    Number of entries.
 * @return Pointer to the input HAL, or NULL on allocation failure.
 */
eui_input_drv_t *eui_drv_raylib_create_input_keymap(const eui_raylib_keymap_entry_t *entries, int count);
void eui_drv_raylib_destroy_input(eui_input_drv_t *hal);

int eui_drv_raylib_window_should_close(void);

void eui_drv_raylib_set_scale(int scale);
int  eui_drv_raylib_get_scale(void);

void eui_drv_raylib_save_screenshot(const char *filename);

const uint8_t* eui_drv_raylib_get_rgba_buffer(uint16_t *out_width, uint16_t *out_height);

#ifdef __cplusplus
}
#endif

#endif /* EUI_DRV_RAYLIB_H */
