/*
 * ESP LCD touch driver for the FocalTech FT3168.
 *
 * There is no FT3168 driver in the component registry. The part shares the FocalTech touch data
 * layout with the FT5x06/FT6x36 family, but espressif/esp_lcd_touch_ft5x06 cannot be reused as
 * is: its init writes the FT5x06 threshold/monitor registers (0x80-0x89), which the FT3168 does
 * not implement, and it never brings the controller out of its default low-power mode.
 */

#pragma once

#include "esp_lcd_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief I2C address of the FT3168 controller
 */
#define ESP_LCD_TOUCH_IO_I2C_FT3168_ADDRESS (0x38)

/**
 * @brief Touch IO configuration structure
 */
#define ESP_LCD_TOUCH_IO_I2C_FT3168_CONFIG()                \
    {                                                       \
        .dev_addr = ESP_LCD_TOUCH_IO_I2C_FT3168_ADDRESS,    \
        .scl_speed_hz = 400000,                             \
        .control_phase_bytes = 1,                           \
        .dc_bit_offset = 0,                                 \
        .lcd_cmd_bits = 8,                                  \
        .flags =                                            \
        {                                                   \
            .disable_control_phase = 1,                     \
        }                                                   \
    }

/**
 * @brief Create a new FT3168 touch driver
 *
 * @note I2C must already be initialised, and the panel must already have been reset: the
 *       FT3168's reset is tied to the panel reset line, so it does not answer before then.
 *
 * @param io LCD/Touch panel IO handle
 * @param config Touch configuration
 * @param out_touch Touch instance handle
 */
esp_err_t esp_lcd_touch_new_i2c_ft3168(const esp_lcd_panel_io_handle_t io, const esp_lcd_touch_config_t *config,
                                       esp_lcd_touch_handle_t *out_touch);

#ifdef __cplusplus
}
#endif
