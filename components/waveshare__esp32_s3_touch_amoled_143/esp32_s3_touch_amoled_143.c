/*
 * Board Support Package for the Waveshare ESP32-S3-Touch-AMOLED-1.43.
 *
 * Derived from waveshare/esp32_s3_touch_amoled_1_75 v3.0.1, which drives the same CO5300 panel.
 * Removed here: audio (no ES8311), the TCA9554 IO expander, SPIFFS and the microSD slot, none of
 * which exist on this board. Changed here: the pinout, and a CST9217 touch controller swapped
 * for an FT3168.
 */

#include <stdio.h>

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_lcd_co5300.h"

#include "bsp/esp32_s3_touch_amoled_143.h"
#include "bsp/display.h"
#include "bsp/touch.h"
#include "bsp_err_check.h"
#include "esp_lcd_touch_ft3168.h"

static const char *TAG = "ESP32-S3-Touch-AMOLED-1.43";

static i2c_master_bus_handle_t i2c_handle = NULL;
static bool i2c_initialized = false;
static lv_indev_t *disp_indev = NULL;
static esp_lcd_touch_handle_t tp = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL;
static esp_lcd_panel_io_handle_t io_handle = NULL;
static uint8_t brightness = 0;

#define LCD_CMD_BITS   (8)
#define LCD_PARAM_BITS (8)

/* CO5300 panel bring-up sequence, as shipped by Waveshare for this panel. */
static const co5300_lcd_init_cmd_t lcd_init_cmds[] = {
    {0xFE, (uint8_t[]){0x20}, 1, 0},
    {0x19, (uint8_t[]){0x10}, 1, 0},
    {0x1C, (uint8_t[]){0xA0}, 1, 0},

    {0xFE, (uint8_t[]){0x00}, 1, 0},
    {0xC4, (uint8_t[]){0x80}, 1, 0},
    {0x3A, (uint8_t[]){0x55}, 1, 0},
    {0x35, (uint8_t[]){0x00}, 1, 0},
    {0x53, (uint8_t[]){0x20}, 1, 0},
    {0x51, (uint8_t[]){0xFF}, 1, 0},
    {0x63, (uint8_t[]){0xFF}, 1, 0},
    {0x2A, (uint8_t[]){0x00, 0x06, 0x01, 0xD7}, 4, 0},
    {0x2B, (uint8_t[]){0x00, 0x00, 0x01, 0xD1}, 4, 600},
    {0x11, NULL, 0, 600},
    {0x29, NULL, 0, 0},
};

/**************************************************************************************************
 *
 * I2C
 *
 **************************************************************************************************/

esp_err_t bsp_i2c_init(void)
{
    if (i2c_initialized) {
        return ESP_OK;
    }

    const i2c_master_bus_config_t i2c_config = {
        .i2c_port = BSP_I2C_NUM,
        .sda_io_num = BSP_I2C_SDA,
        .scl_io_num = BSP_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    BSP_ERROR_CHECK_RETURN_ERR(i2c_new_master_bus(&i2c_config, &i2c_handle));

    i2c_initialized = true;

    return ESP_OK;
}

esp_err_t bsp_i2c_deinit(void)
{
    BSP_ERROR_CHECK_RETURN_ERR(i2c_del_master_bus(i2c_handle));
    i2c_handle = NULL;
    i2c_initialized = false;
    return ESP_OK;
}

i2c_master_bus_handle_t bsp_i2c_get_handle(void)
{
    bsp_i2c_init();
    return i2c_handle;
}

/**************************************************************************************************
 *
 * Brightness
 *
 * The AMOLED has no backlight rail, so "backlight" here is the panel's own write-display-
 * brightness register (0x51). The command word is assembled the way the CO5300 QSPI framing
 * wants it: 0x02 in the top byte selects the command write, with the register in the middle.
 *
 **************************************************************************************************/

esp_err_t bsp_display_brightness_set(int brightness_percent)
{
    if (panel_handle == NULL) {
        ESP_LOGE(TAG, "Panel handle is not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (brightness_percent < 0 || brightness_percent > 100) {
        ESP_LOGE(TAG, "Invalid brightness percentage. Should be between 0 and 100.");
        return ESP_ERR_INVALID_ARG;
    }

    brightness = (uint8_t)(brightness_percent * 255 / 100);

    uint32_t lcd_cmd = 0x51;
    lcd_cmd &= 0xff;
    lcd_cmd <<= 8;
    lcd_cmd |= 0x02 << 24;

    uint8_t param = brightness;
    return esp_lcd_panel_io_tx_param(io_handle, lcd_cmd, &param, 1);
}

esp_err_t bsp_display_brightness_init(void)
{
    return bsp_display_brightness_set(100);
}

int bsp_display_brightness_get(void)
{
    if (panel_handle == NULL) {
        ESP_LOGE(TAG, "Panel handle is not initialized");
        return -1;
    }

    return brightness * 100 / 255;
}

esp_err_t bsp_display_backlight_off(void)
{
    return bsp_display_brightness_set(0);
}

esp_err_t bsp_display_backlight_on(void)
{
    return bsp_display_brightness_set(100);
}

/**************************************************************************************************
 *
 * Display
 *
 **************************************************************************************************/

/*
 * The CO5300 addresses its RAM in pairs of pixels, so every flush window must start on an even
 * coordinate and end on an odd one. LVGL is free to invalidate any area it likes, so round it
 * out here rather than trusting the caller.
 *
 * Note this rounds x1 down as well, which the 1.75" BSP this was derived from does not -- it
 * reads x1 and then never uses it. Rounding all four edges only ever enlarges the flush area,
 * which is always valid, and it makes the column window obey the same alignment as the row one.
 */
static void rounder_event_cb(lv_event_t *e)
{
    lv_area_t *area = (lv_area_t *)lv_event_get_param(e);

    area->x1 = (area->x1 >> 1) << 1;
    area->y1 = (area->y1 >> 1) << 1;
    area->x2 = ((area->x2 >> 1) << 1) + 1;
    area->y2 = ((area->y2 >> 1) << 1) + 1;
}

esp_err_t bsp_display_new(const bsp_display_config_t *config, esp_lcd_panel_handle_t *ret_panel, esp_lcd_panel_io_handle_t *ret_io)
{
    assert(config != NULL && config->max_transfer_sz > 0);

    ESP_LOGI(TAG, "Initialize QSPI bus");
    const spi_bus_config_t buscfg = CO5300_PANEL_BUS_QSPI_CONFIG(BSP_LCD_PCLK,
                                                                 BSP_LCD_DATA0,
                                                                 BSP_LCD_DATA1,
                                                                 BSP_LCD_DATA2,
                                                                 BSP_LCD_DATA3,
                                                                 config->max_transfer_sz);
    ESP_RETURN_ON_ERROR(spi_bus_initialize(BSP_LCD_SPI_NUM, &buscfg, SPI_DMA_CH_AUTO), TAG, "SPI bus init failed");

    esp_lcd_panel_io_spi_config_t io_config = CO5300_PANEL_IO_QSPI_CONFIG(BSP_LCD_CS, NULL, NULL);
    io_config.pclk_hz = BSP_LCD_QSPI_HZ;
    io_config.trans_queue_depth = CONFIG_BSP_LCD_TRANS_QUEUE_DEPTH;
    /*
     * Only takes effect if a colour buffer is ever in PSRAM, which it is not below -- but
     * without it esp_lcd never sets SPI_TRANS_DMA_USE_PSRAM, so spi_master would treat such a
     * flush as non-DMA-capable and allocate an internal bounce buffer the size of the whole
     * chunk, per transaction. That succeeds until WiFi claims its internal RAM and then fails
     * forever, which reads as a dead panel rather than an allocation problem:
     *
     *   spi_master: setup_dma_priv_buffer: Failed to allocate priv TX buffer
     *   co5300_spi: panel_co5300_draw_bitmap: send color data failed
     *
     * Kept so that moving the draw buffers back to PSRAM cannot silently reintroduce that.
     */
    io_config.flags.psram_dma_direct = 1;

    co5300_vendor_config_t vendor_config = {
        .init_cmds = lcd_init_cmds,
        .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0]),
        .flags = {
            .use_qspi_interface = 1,
        },
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)BSP_LCD_SPI_NUM, &io_config, &io_handle),
                        TAG, "Panel IO init failed");

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = BSP_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = BSP_LCD_BITS_PER_PIXEL,
        .vendor_config = &vendor_config,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_co5300(io_handle, &panel_config, &panel_handle), TAG, "Panel init failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_set_gap(panel_handle, BSP_LCD_COL_OFFSET, BSP_LCD_ROW_OFFSET),
                        TAG, "Panel gap failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(panel_handle), TAG, "Panel reset failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(panel_handle), TAG, "Panel init failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(panel_handle, true), TAG, "Panel on failed");

    if (ret_panel) {
        *ret_panel = panel_handle;
    }
    if (ret_io) {
        *ret_io = io_handle;
    }

    return ESP_OK;
}

esp_err_t bsp_touch_new(const bsp_display_cfg_t *cfg, esp_lcd_touch_handle_t *ret_touch)
{
    assert(cfg != NULL);

    BSP_ERROR_CHECK_RETURN_ERR(bsp_i2c_init());

    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = BSP_LCD_H_RES,
        .y_max = BSP_LCD_V_RES,
        /*
         * Both are GPIO_NUM_NC on this board: the FT3168's reset is tied to the panel reset and
         * its interrupt is not broken out, so the adapter polls it.
         */
        .rst_gpio_num = BSP_LCD_TOUCH_RST,
        .int_gpio_num = BSP_LCD_TOUCH_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = cfg->touch_flags.swap_xy,
            .mirror_x = cfg->touch_flags.mirror_x,
            .mirror_y = cfg->touch_flags.mirror_y,
        },
    };

    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_FT3168_CONFIG();
    tp_io_config.scl_speed_hz = CONFIG_BSP_I2C_CLK_SPEED_HZ;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(i2c_handle, &tp_io_config, &tp_io_handle), TAG, "Touch IO init failed");

    /*
     * Silence the panel IO layer's per-transaction error log. The FT3168 NAKs one status read
     * roughly every 13 seconds while it self-calibrates, and esp_lcd logs that at ERROR before
     * the driver ever sees it, so a healthy board otherwise prints a fault every 13 seconds
     * forever. The FT3168 driver tracks consecutive failures itself and reports a run of them,
     * which is the signal that actually means something.
     *
     * Only touch uses this tag on this board -- the panel is on SPI ("lcd_panel.io.spi") -- so
     * nothing else is muted by this.
     */
    esp_log_level_set("lcd_panel.io.i2c", ESP_LOG_NONE);

    return esp_lcd_touch_new_i2c_ft3168(tp_io_handle, &tp_cfg, ret_touch);
}

static lv_display_t *bsp_display_lcd_init(const bsp_display_cfg_t *cfg)
{
    assert(cfg != NULL);

    const bsp_display_config_t disp_config = {
        .max_transfer_sz = BSP_LCD_H_RES * BSP_LCD_V_RES * BSP_LCD_BITS_PER_PIXEL / 8,
    };

    BSP_ERROR_CHECK_RETURN_NULL(bsp_display_new(&disp_config, &panel_handle, &io_handle));

    ESP_LOGD(TAG, "Add LCD screen");
    esp_lv_adapter_display_config_t disp_cfg = {
        .panel = panel_handle,
        .panel_io = io_handle,
        .profile = {
            .interface = ESP_LV_ADAPTER_PANEL_IF_OTHER,
            .rotation = cfg->rotation,
            .hor_res = BSP_LCD_H_RES,
            .ver_res = BSP_LCD_V_RES,
            .buffer_height = CONFIG_BSP_DISPLAY_LVGL_BUF_HEIGHT,
            /*
             * Draw buffers in internal RAM, not PSRAM. The DMA can read PSRAM directly (see
             * psram_dma_direct above), but not fast enough here: WiFi and LWIP buffers are also
             * in PSRAM on this build, and under that contention the SPI FIFO runs dry mid-frame
             * and the driver reports "DMA TX underflow detected". Internal RAM has no such
             * limit. The cost is CONFIG_BSP_DISPLAY_LVGL_BUF_HEIGHT * hor_res * 2 bytes twice,
             * which is why that default is modest.
             */
            .use_psram = false,
            .enable_ppa_accel = false,
            .require_double_buffer = true,
        },
        .tear_avoid_mode = cfg->tear_avoid_mode,
    };

    lv_display_t *disp = esp_lv_adapter_register_display(&disp_cfg);
    if (!disp) {
        return NULL;
    }

    lv_display_add_event_cb(disp, rounder_event_cb, LV_EVENT_INVALIDATE_AREA, NULL);

    return disp;
}

static lv_indev_t *bsp_display_indev_init(const bsp_display_cfg_t *cfg, lv_display_t *disp)
{
    assert(cfg != NULL);

    BSP_ERROR_CHECK_RETURN_NULL(bsp_touch_new(cfg, &tp));
    assert(tp);

    const esp_lv_adapter_touch_config_t touch_cfg = ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG(disp, tp);

    return esp_lv_adapter_register_touch(&touch_cfg);
}

lv_display_t *bsp_display_start(void)
{
    bsp_display_cfg_t cfg = {
        .lv_adapter_cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG(),
        .rotation = ESP_LV_ADAPTER_ROTATE_0,
        .tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_NONE,
        /* Raw FT3168 coordinates already match the panel orientation on this board. */
        .touch_flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };
    return bsp_display_start_with_config(&cfg);
}

lv_display_t *bsp_display_start_with_config(bsp_display_cfg_t *cfg)
{
    lv_display_t *disp;

    assert(cfg != NULL);
    BSP_ERROR_CHECK_RETURN_NULL(esp_lv_adapter_init(&cfg->lv_adapter_cfg));

    /*
     * Display first, touch second, always. The FT3168's reset is tied to the panel reset line,
     * so it does not appear on I2C until bsp_display_lcd_init() has brought the panel up.
     */
    BSP_NULL_CHECK(disp = bsp_display_lcd_init(cfg), NULL);
    BSP_NULL_CHECK(disp_indev = bsp_display_indev_init(cfg, disp), NULL);

    BSP_ERROR_CHECK_RETURN_NULL(bsp_display_brightness_init());

    ESP_ERROR_CHECK(esp_lv_adapter_start());

    return disp;
}

lv_indev_t *bsp_display_get_input_dev(void)
{
    return disp_indev;
}

esp_err_t bsp_display_lock(uint32_t timeout_ms)
{
    return esp_lv_adapter_lock(timeout_ms);
}

void bsp_display_unlock(void)
{
    esp_lv_adapter_unlock();
}
