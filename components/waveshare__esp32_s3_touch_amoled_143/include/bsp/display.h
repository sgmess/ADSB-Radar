#pragma once
#include "esp_lcd_types.h"
#include "esp_err.h"

/* LCD color formats */
#define ESP_LCD_COLOR_FORMAT_RGB565    (1)
#define ESP_LCD_COLOR_FORMAT_RGB888    (2)

/* LCD display color format */
#define BSP_LCD_COLOR_FORMAT        (ESP_LCD_COLOR_FORMAT_RGB565)
/* LCD display color bytes endianess */
#define BSP_LCD_BIGENDIAN           (0)
/* LCD display color bits */
#define BSP_LCD_BITS_PER_PIXEL      (16)
/* LCD display color space */
#define BSP_LCD_COLOR_SPACE         (LCD_RGB_ELEMENT_ORDER_RGB)

/* LCD display definition. Single fixed panel, unlike the dual-profile ESP32-P4 board. */
#define BSP_LCD_H_RES              (466)
#define BSP_LCD_V_RES              (466)

/*
 * The CO5300 has a 480-wide RAM behind a 466-wide panel, so the visible window starts six
 * columns in. Dialled in on hardware by nudging a full-panel rim-ring test card one pixel at a
 * time: 6 centres the image, 7 and 8 visibly overshoot. Rows need no offset.
 */
#define BSP_LCD_COL_OFFSET         (6)
#define BSP_LCD_ROW_OFFSET         (0)

/* Vendor QSPI rate. 80 MHz is untested on this board. */
#define BSP_LCD_QSPI_HZ            (40 * 1000 * 1000)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief BSP display configuration structure
 */
typedef struct {
    int max_transfer_sz;    /*!< Maximum transfer size, in bytes. */
} bsp_display_config_t;

/**
 * @brief Create new display panel
 *
 * For maximum flexibility, this function performs only reset and initialization of the display.
 *
 * @param[in]  config    display configuration
 * @param[out] ret_panel esp_lcd panel handle
 * @param[out] ret_io    esp_lcd IO handle
 */
esp_err_t bsp_display_new(const bsp_display_config_t *config, esp_lcd_panel_handle_t *ret_panel, esp_lcd_panel_io_handle_t *ret_io);

/**
 * @brief Initialize display's brightness
 *
 * The AMOLED has no backlight rail; brightness is the panel's own 0x51 register.
 */
esp_err_t bsp_display_brightness_init(void);

/**
 * @brief Set display's brightness
 *
 * @param[in] brightness_percent Brightness in [%]
 */
esp_err_t bsp_display_brightness_set(int brightness_percent);

/**
 * @brief Get display's brightness in [%]
 */
int bsp_display_brightness_get(void);

/**
 * @brief Turn the display on at full brightness
 */
esp_err_t bsp_display_backlight_on(void);

/**
 * @brief Turn the display off
 */
esp_err_t bsp_display_backlight_off(void);

#ifdef __cplusplus
}
#endif
