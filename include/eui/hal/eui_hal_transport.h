#ifndef EUI_HAL_TRANSPORT_H
#define EUI_HAL_TRANSPORT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void (*write_cmd)(uint8_t cmd, void *user_data);
    void (*write_data)(const uint8_t *buf, uint32_t len, void *user_data);
    void (*delay_ms)(uint32_t ms, void *user_data);
    void *user_data;
} eui_hal_i2c_t;

typedef struct {
    void (*write_cmd)(uint8_t cmd, void *user_data);
    void (*write_data)(const uint8_t *buf, uint32_t len, void *user_data);
    void (*read_data)(uint8_t *buf, uint32_t len, void *user_data);
    void (*set_dc)(bool data_mode, void *user_data);
    void (*set_cs)(bool active, void *user_data);
    void (*set_rst)(bool active, void *user_data);
    void (*delay_ms)(uint32_t ms, void *user_data);
    void *user_data;
} eui_hal_spi_t;

#ifdef __cplusplus
}
#endif

#endif /* EUI_HAL_TRANSPORT_H */
