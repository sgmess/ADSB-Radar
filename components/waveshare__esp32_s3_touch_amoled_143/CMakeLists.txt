idf_component_register(
    SRCS "esp32_s3_touch_amoled_143.c"
         "src/esp_lcd_touch_ft3168.c"
    INCLUDE_DIRS "include" "include/bsp"
    PRIV_INCLUDE_DIRS "priv_include"
    REQUIRES esp_driver_gpio esp_driver_i2c esp_driver_spi esp_lcd
    PRIV_REQUIRES esp_timer esp_psram
)
