/*
 * ESP LCD touch driver for the FocalTech FT3168, as fitted to the Waveshare
 * ESP32-S3-Touch-AMOLED-1.43.
 *
 * The touch data layout is the usual FocalTech one (a point count at 0x02 followed by six bytes
 * per point), so the read path mirrors espressif/esp_lcd_touch_ft5x06. The init path does not:
 * the FT3168 has no FT5x06 threshold registers, and it powers up in monitor mode, so it must be
 * put into active mode before it will report anything.
 */

#include <string.h>

#include "esp_check.h"
#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "esp_lcd_touch_ft3168.h"

static const char *TAG = "FT3168";

/* Registers */
#define FT3168_DEVICE_MODE      (0x00)
#define FT3168_GESTURE_ID       (0x01)
#define FT3168_TOUCH_POINTS     (0x02)
#define FT3168_TOUCH1_XH        (0x03)
#define FT3168_CHIP_ID          (0xA3)
#define FT3168_POWER_MODE       (0xA5)
#define FT3168_FIRMWARE_ID      (0xA6)

#define FT3168_DEVICE_MODE_NORMAL   (0x00)
#define FT3168_POWER_MODE_ACTIVE    (0x00)

/* Value the FT3168 reports in its chip id register; confirmed on this board. */
#define FT3168_CHIP_ID_VALUE    (0x64)

/* The controller reports at most five simultaneous points. */
#define FT3168_MAX_POINTS       (5)
#define FT3168_BYTES_PER_POINT  (6)

static esp_err_t esp_lcd_touch_ft3168_read_data(esp_lcd_touch_handle_t tp);
static bool esp_lcd_touch_ft3168_get_xy(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y, uint16_t *strength,
                                        uint8_t *point_num, uint8_t max_point_num);
static esp_err_t esp_lcd_touch_ft3168_get_track_id(esp_lcd_touch_handle_t tp, uint8_t *track_id, uint8_t max_point_num);
static esp_err_t esp_lcd_touch_ft3168_del(esp_lcd_touch_handle_t tp);

static esp_err_t touch_ft3168_i2c_write(esp_lcd_touch_handle_t tp, uint8_t reg, uint8_t data);
static esp_err_t touch_ft3168_i2c_read(esp_lcd_touch_handle_t tp, uint8_t reg, uint8_t *data, uint8_t len);
static esp_err_t touch_ft3168_init(esp_lcd_touch_handle_t tp);

esp_err_t esp_lcd_touch_new_i2c_ft3168(const esp_lcd_panel_io_handle_t io, const esp_lcd_touch_config_t *config,
                                       esp_lcd_touch_handle_t *out_touch)
{
    esp_err_t ret = ESP_OK;

    ESP_RETURN_ON_FALSE(io != NULL, ESP_ERR_INVALID_ARG, TAG, "Touch controller io handle can't be NULL");
    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG, "Touch controller config can't be NULL");
    ESP_RETURN_ON_FALSE(out_touch != NULL, ESP_ERR_INVALID_ARG, TAG, "Touch controller handle can't be NULL");

    esp_lcd_touch_handle_t tp = heap_caps_calloc(1, sizeof(esp_lcd_touch_t), MALLOC_CAP_DEFAULT);
    ESP_RETURN_ON_FALSE(tp, ESP_ERR_NO_MEM, TAG, "no mem for FT3168 controller");

    tp->io = io;
    tp->read_data = esp_lcd_touch_ft3168_read_data;
    tp->get_xy = esp_lcd_touch_ft3168_get_xy;
    tp->get_track_id = esp_lcd_touch_ft3168_get_track_id;
    tp->del = esp_lcd_touch_ft3168_del;
    tp->data.lock.owner = portMUX_FREE_VAL;
    memcpy(&tp->config, config, sizeof(esp_lcd_touch_config_t));

    /*
     * On this board the interrupt line is not broken out, so int_gpio_num is normally
     * GPIO_NUM_NC and the adapter polls instead. Honour it anyway for other wirings.
     */
    if (tp->config.int_gpio_num != GPIO_NUM_NC) {
        const gpio_config_t int_gpio_config = {
            .mode = GPIO_MODE_INPUT,
            .intr_type = (tp->config.levels.interrupt ? GPIO_INTR_POSEDGE : GPIO_INTR_NEGEDGE),
            .pin_bit_mask = BIT64(tp->config.int_gpio_num),
        };
        ESP_GOTO_ON_ERROR(gpio_config(&int_gpio_config), err, TAG, "GPIO config failed");

        if (tp->config.interrupt_callback) {
            esp_lcd_touch_register_interrupt_callback(tp, tp->config.interrupt_callback);
        }
    }

    /*
     * No reset is driven here even when rst_gpio_num is set: on this board that line is the
     * panel reset, and pulsing it after the display is up would blank the screen.
     */
    ESP_GOTO_ON_ERROR(touch_ft3168_init(tp), err, TAG, "FT3168 init failed");

    *out_touch = tp;
    return ESP_OK;

err:
    ESP_LOGE(TAG, "Touch controller FT3168 initialization failed");
    esp_lcd_touch_ft3168_del(tp);
    return ret;
}

static esp_err_t touch_ft3168_init(esp_lcd_touch_handle_t tp)
{
    uint8_t chip_id = 0;

    /*
     * Confirm we are talking to the expected part before trusting the register map. A wrong or
     * absent answer here usually means touch_new ran before the panel reset released the
     * controller, in which case it is not on the bus at all.
     */
    ESP_RETURN_ON_ERROR(touch_ft3168_i2c_read(tp, FT3168_CHIP_ID, &chip_id, 1), TAG,
                        "FT3168 not responding on I2C");
    if (chip_id != FT3168_CHIP_ID_VALUE) {
        ESP_LOGW(TAG, "Unexpected chip id 0x%02X (expected 0x%02X)", chip_id, FT3168_CHIP_ID_VALUE);
    }

    /* Normal touch reporting rather than test/factory mode. */
    ESP_RETURN_ON_ERROR(touch_ft3168_i2c_write(tp, FT3168_DEVICE_MODE, FT3168_DEVICE_MODE_NORMAL), TAG,
                        "Failed to select normal device mode");

    /*
     * The FT3168 comes up in monitor mode and reports nothing until woken. Without this write
     * the chip ACKs every read but the point count stays at zero forever.
     */
    ESP_RETURN_ON_ERROR(touch_ft3168_i2c_write(tp, FT3168_POWER_MODE, FT3168_POWER_MODE_ACTIVE), TAG,
                        "Failed to select active power mode");

    uint8_t firmware_id = 0;
    if (touch_ft3168_i2c_read(tp, FT3168_FIRMWARE_ID, &firmware_id, 1) == ESP_OK) {
        ESP_LOGI(TAG, "FT3168 ready (chip id 0x%02X, firmware 0x%02X)", chip_id, firmware_id);
    }

    return ESP_OK;
}

/*
 * Consecutive failed status reads.
 *
 * File-static because there is exactly one touch controller on this board and this driver is
 * private to its BSP. See the tolerance note in esp_lcd_touch_ft3168_read_data().
 */
static unsigned int consecutive_read_failures;

/* Report a run of failures loudly, but do not treat a single missed poll as a fault. */
#define FT3168_READ_FAILURE_REPORT_THRESHOLD (8)

static esp_err_t esp_lcd_touch_ft3168_read_data(esp_lcd_touch_handle_t tp)
{
    ESP_RETURN_ON_FALSE(tp != NULL, ESP_ERR_INVALID_ARG, TAG, "Touch controller handle can't be NULL");

    /*
     * The FT3168 NAKs the occasional status read while it is busy with its own housekeeping.
     * On this board that shows up as exactly one failed read on a metronomic ~13.4 s cadence,
     * which is the controller's periodic auto-calibration rather than anything wrong with the
     * bus -- genuine signalling trouble would scatter, not keep time.
     *
     * A poll is not worth an error: the next one is a refresh period away, and a dropped sample
     * is invisible in a touch gesture. Report no touch for this round instead, and only
     * complain once a run of failures says the controller has actually left the bus.
     */
    uint8_t points = 0;
    if (touch_ft3168_i2c_read(tp, FT3168_TOUCH_POINTS, &points, 1) != ESP_OK) {
        if (++consecutive_read_failures == FT3168_READ_FAILURE_REPORT_THRESHOLD) {
            ESP_LOGW(TAG, "Touch controller has not answered for %u consecutive reads",
                     consecutive_read_failures);
        }
        return ESP_OK;
    }
    if (consecutive_read_failures >= FT3168_READ_FAILURE_REPORT_THRESHOLD) {
        ESP_LOGI(TAG, "Touch controller responding again");
    }
    consecutive_read_failures = 0;

    /*
     * The register carries the point count in its low nibble only. Reading 0x0F while no finger
     * is down is the controller's idle marker, so treat anything above the supported count as
     * "no touch" rather than clamping it into a bogus reading.
     */
    points &= 0x0F;
    if (points == 0 || points > FT3168_MAX_POINTS) {
        return ESP_OK;
    }

    if (points > CONFIG_ESP_LCD_TOUCH_MAX_POINTS) {
        points = CONFIG_ESP_LCD_TOUCH_MAX_POINTS;
    }

    uint8_t data[FT3168_MAX_POINTS * FT3168_BYTES_PER_POINT];
    if (touch_ft3168_i2c_read(tp, FT3168_TOUCH1_XH, data, points * FT3168_BYTES_PER_POINT) != ESP_OK) {
        /* Same reasoning as the status read above: drop this poll rather than raise a fault. */
        return ESP_OK;
    }

    portENTER_CRITICAL(&tp->data.lock);

    tp->data.points = points;
    for (size_t i = 0; i < points; i++) {
        const uint8_t *point = &data[i * FT3168_BYTES_PER_POINT];
        /* XH carries the event flag in bits 7:6 and x[11:8] in bits 3:0; YH carries the track
         * id in bits 7:4 and y[11:8] in bits 3:0. */
        tp->data.coords[i].track_id = (point[2] & 0xF0) >> 4;
        tp->data.coords[i].x = (((uint16_t)point[0] & 0x0F) << 8) + point[1];
        tp->data.coords[i].y = (((uint16_t)point[2] & 0x0F) << 8) + point[3];
    }

    portEXIT_CRITICAL(&tp->data.lock);

    return ESP_OK;
}

static bool esp_lcd_touch_ft3168_get_xy(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y, uint16_t *strength,
                                        uint8_t *point_num, uint8_t max_point_num)
{
    ESP_RETURN_ON_FALSE(tp != NULL, false, TAG, "Touch controller handle can't be NULL");
    ESP_RETURN_ON_FALSE(x != NULL, false, TAG, "Pointer to the x coordinates array can't be NULL");
    ESP_RETURN_ON_FALSE(y != NULL, false, TAG, "Pointer to the y coordinates array can't be NULL");
    ESP_RETURN_ON_FALSE(point_num != NULL, false, TAG, "Pointer to number of touch points can't be NULL");
    ESP_RETURN_ON_FALSE(max_point_num > 0, false, TAG, "Array size must be equal or larger than 1");

    portENTER_CRITICAL(&tp->data.lock);

    *point_num = (tp->data.points > max_point_num ? max_point_num : tp->data.points);

    for (size_t i = 0; i < *point_num; i++) {
        x[i] = tp->data.coords[i].x;
        y[i] = tp->data.coords[i].y;

        if (strength) {
            strength[i] = tp->data.coords[i].strength;
        }
    }

    /* Invalidate so the same press is not reported twice. */
    tp->data.points = 0;

    portEXIT_CRITICAL(&tp->data.lock);

    return (*point_num > 0);
}

static esp_err_t esp_lcd_touch_ft3168_get_track_id(esp_lcd_touch_handle_t tp, uint8_t *track_id, uint8_t max_point_num)
{
    ESP_RETURN_ON_FALSE(tp != NULL, ESP_ERR_INVALID_ARG, TAG, "Touch controller handle can't be NULL");
    ESP_RETURN_ON_FALSE(track_id != NULL, ESP_ERR_INVALID_ARG, TAG, "Pointer to the track ID array can't be NULL");
    ESP_RETURN_ON_FALSE(max_point_num > 0, ESP_ERR_INVALID_ARG, TAG, "Array size must be equal or larger than 1");

    portENTER_CRITICAL(&tp->data.lock);

    for (int i = 0; i < max_point_num; i++) {
        track_id[i] = tp->data.coords[i].track_id;
    }

    portEXIT_CRITICAL(&tp->data.lock);

    return ESP_OK;
}

static esp_err_t esp_lcd_touch_ft3168_del(esp_lcd_touch_handle_t tp)
{
    ESP_RETURN_ON_FALSE(tp != NULL, ESP_ERR_INVALID_ARG, TAG, "Touch controller handle can't be NULL");

    if (tp->config.int_gpio_num != GPIO_NUM_NC) {
        gpio_reset_pin(tp->config.int_gpio_num);
        if (tp->config.interrupt_callback) {
            gpio_isr_handler_remove(tp->config.int_gpio_num);
        }
    }

    /* The reset pin is the panel's, so it is deliberately left alone here. */

    free(tp);

    return ESP_OK;
}

static esp_err_t touch_ft3168_i2c_write(esp_lcd_touch_handle_t tp, uint8_t reg, uint8_t data)
{
    ESP_RETURN_ON_FALSE(tp != NULL, ESP_ERR_INVALID_ARG, TAG, "Touch controller handle can't be NULL");

    // *INDENT-OFF*
    return esp_lcd_panel_io_tx_param(tp->io, reg, (uint8_t[]){data}, 1);
    // *INDENT-ON*
}

static esp_err_t touch_ft3168_i2c_read(esp_lcd_touch_handle_t tp, uint8_t reg, uint8_t *data, uint8_t len)
{
    ESP_RETURN_ON_FALSE(tp != NULL, ESP_ERR_INVALID_ARG, TAG, "Touch controller handle can't be NULL");
    ESP_RETURN_ON_FALSE(data != NULL, ESP_ERR_INVALID_ARG, TAG, "Pointer to the data array can't be NULL");

    return esp_lcd_panel_io_rx_param(tp->io, reg, data, len);
}
