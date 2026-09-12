#pragma once

#include "sdkconfig.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "bsp/config.h"
#include "bsp/display.h"

#include "lvgl.h"
#include "esp_lv_adapter.h"

/**************************************************************************************************
 *  BSP Capabilities
 *
 *  Unlike the 1.75" sibling board this one carries no AXP2101 PMIC, no ES8311 codec and no
 *  microSD slot, so audio and storage are absent rather than merely unused. See the pinout note
 *  below for why leaving audio enabled here would be actively harmful.
 **************************************************************************************************/

#define BSP_CAPS_DISPLAY        1
#define BSP_CAPS_TOUCH          1
#define BSP_CAPS_BUTTONS        0
#define BSP_CAPS_AUDIO          0
#define BSP_CAPS_AUDIO_SPEAKER  0
#define BSP_CAPS_AUDIO_MIC      0
#define BSP_CAPS_SDCARD         0
#define BSP_CAPS_IMU            0

/**************************************************************************************************
 * Waveshare ESP32-S3-Touch-AMOLED-1.43 pinout
 *
 * Pins come from the Arduino-ESP32 core's board variant
 * (variants/waveshare_esp32_s3_touch_amoled_143/pins_arduino.h, shared with the 1.64) and were
 * each confirmed on hardware: an I2C scan on 47/48 answers 0x51 + 0x6B + 0x38, and the panel
 * drives correctly on CS=9 / SCK=10 / D0-D3=11-14 / RST=21.
 *
 * Do NOT "correct" these against the Waveshare wiki pin table: that table is misaligned and
 * lists an I2C SCL of GPIO49, which does not exist on an ESP32-S3.
 *
 * Note that GPIO 9 and 10 are this board's QSPI CS and SCLK. On the 1.75 the same two pins are
 * the I2S BCLK/DIN, which is why the audio support from that BSP is removed here rather than
 * left in place unused.
 **************************************************************************************************/

/* I2C - shared by touch (FT3168), IMU (QMI8658) and RTC (PCF85063) */
#define BSP_I2C_SCL           (GPIO_NUM_48)
#define BSP_I2C_SDA           (GPIO_NUM_47)

/* Display - CO5300 AMOLED on QSPI */
#define BSP_LCD_CS            (GPIO_NUM_9)
#define BSP_LCD_PCLK          (GPIO_NUM_10)
#define BSP_LCD_DATA0         (GPIO_NUM_11)
#define BSP_LCD_DATA1         (GPIO_NUM_12)
#define BSP_LCD_DATA2         (GPIO_NUM_13)
#define BSP_LCD_DATA3         (GPIO_NUM_14)
#define BSP_LCD_RST           (GPIO_NUM_21)

/* The AMOLED has no backlight; brightness is a panel register. Kept for API compatibility. */
#define BSP_LCD_BACKLIGHT     (GPIO_NUM_NC)

/*
 * The FT3168 has no reset line of its own: it is tied to the panel reset (GPIO21) and does not
 * answer on I2C at all until the display has been initialised. bsp_display_start_with_config()
 * therefore brings the panel up before the touch controller -- do not reorder those two.
 */
#define BSP_LCD_TOUCH_RST     (GPIO_NUM_NC)
#define BSP_LCD_TOUCH_INT     (GPIO_NUM_NC)

/* Interrupt lines that are broken out but unused by this BSP. */
#define BSP_IMU_INT           (GPIO_NUM_8)
#define BSP_RTC_INT           (GPIO_NUM_15)

/*
 * There is a divided battery voltage on GPIO4, but the divider ratio is not documented, so this
 * BSP exposes the pin and reports no battery rather than inventing a percentage.
 */
#define BSP_BAT_ADC           (GPIO_NUM_4)

#define BSP_BOOT_BUTTON       (GPIO_NUM_0)

/* I2C addresses of the devices on the shared bus. */
#define BSP_I2C_ADDR_TOUCH    (0x38)
#define BSP_I2C_ADDR_IMU      (0x6B)
#define BSP_I2C_ADDR_RTC      (0x51)

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
 *
 * I2C interface
 *
 **************************************************************************************************/
#define BSP_I2C_NUM     CONFIG_BSP_I2C_NUM

/**
 * @brief Init I2C driver
 */
esp_err_t bsp_i2c_init(void);

/**
 * @brief Deinit I2C driver and free its resources
 */
esp_err_t bsp_i2c_deinit(void);

/**
 * @brief Get I2C driver handle
 */
i2c_master_bus_handle_t bsp_i2c_get_handle(void);

/**************************************************************************************************
 *
 * LCD interface
 *
 * LVGL is used as graphics library. LVGL is NOT thread safe, therefore the user must take the
 * LVGL mutex by calling bsp_display_lock() before calling any LVGL API (lv_...) and then give
 * the mutex back with bsp_display_unlock().
 *
 **************************************************************************************************/
#define BSP_LCD_SPI_NUM            (SPI2_HOST)

#if (BSP_CONFIG_NO_GRAPHIC_LIB == 0)

/**
 * @brief BSP display configuration structure
 */
typedef struct {
    esp_lv_adapter_config_t          lv_adapter_cfg;
    esp_lv_adapter_rotation_t        rotation;
    esp_lv_adapter_tear_avoid_mode_t tear_avoid_mode;
    struct {
        unsigned int swap_xy;  /*!< Swap X and Y after read coordinates */
        unsigned int mirror_x; /*!< Mirror X after read coordinates */
        unsigned int mirror_y; /*!< Mirror Y after read coordinates */
    } touch_flags;
} bsp_display_cfg_t;

/**
 * @brief Initialize display with default settings
 *
 * This function initializes QSPI, the display controller and starts the LVGL handling task.
 *
 * @return Pointer to LVGL display or NULL when error occurred
 */
lv_display_t *bsp_display_start(void);

/**
 * @brief Initialize display
 *
 * @param cfg display configuration
 * @return Pointer to LVGL display or NULL when error occurred
 */
lv_display_t *bsp_display_start_with_config(bsp_display_cfg_t *cfg);

/**
 * @brief Get pointer to input device (touch)
 */
lv_indev_t *bsp_display_get_input_dev(void);

/**
 * @brief Take LVGL mutex
 *
 * @param timeout_ms Timeout in [ms]. -1 will block indefinitely.
 */
esp_err_t bsp_display_lock(uint32_t timeout_ms);

/**
 * @brief Give LVGL mutex
 */
void bsp_display_unlock(void);

#endif // BSP_CONFIG_NO_GRAPHIC_LIB == 0

#ifdef __cplusplus
}
#endif
