#ifndef EUI_PORT_BOOTSTRAP_H
#define EUI_PORT_BOOTSTRAP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t display_width;
    uint16_t display_height;
} eui_example_config_t;

/**
 * Called by the port bootstrap after eui_init() and display init are complete.
 * Each cross-platform example implements this function to build its UI.
 */
void eui_example_setup(const eui_example_config_t *cfg);

#ifdef __cplusplus
}
#endif

#endif /* EUI_PORT_BOOTSTRAP_H */
