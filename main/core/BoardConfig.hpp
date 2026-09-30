#pragma once

#include "esp_heap_caps.h"

/*
 * Board capabilities and panel geometry for the Waveshare ESP32-S3-Touch-LCD-2.8C.
 *
 * This build targets one board and one panel: a fixed 480x480 ST7701 round IPS, so the layout
 * constants below are compile-time values rather than derived at boot.
 *
 * The chrome dimensions in RadarTypes.hpp were chosen for the 466x466 ESP32-S3-Touch-AMOLED-1.43
 * and are kept here unchanged. This panel is only 3% larger, and those numbers are mostly set by
 * fingertip and legibility floors rather than by the panel size; everything that should track
 * the panel (radar size, control arc radius) is already derived from SCREEN_W.
 */

/*
 * No USB host port on this board -- the USB-C socket goes to a CH343P USB-UART bridge, not the
 * S3's native USB -- so a USB CDC GPS dongle cannot be enumerated. GpsReceiver keeps its API but compiles to a stub that reports the
 * feature as unavailable, and the browser admin hides the GPS centre source.
 */
#define BOARD_HAS_USB_GPS 0

/*
 * No AXP2101 PMIC and no audio codec: there is a buzzer on the IO expander, which the BSP holds
 * off, and a divided battery voltage on GPIO4 that this firmware does not read.
 */
#define BOARD_HAS_PMIC    0
#define BOARD_HAS_AUDIO   0

/* Single fixed panel; there is no display-type choice to persist or offer. */
#define BOARD_HAS_DISPLAY_TYPE_CHOICE 0

#define BOARD_SCREEN_W 480
#define BOARD_SCREEN_H 480

/*
 * Heap capabilities for the application's FreeRTOS task stacks: PSRAM on this board.
 *
 * Internal RAM is what decides whether this board boots. With the RGB bounce buffers taken out
 * of it, internal stacks leave esp_wifi_init() about 42 KB (18 KB largest block), which is not
 * enough for its static buffers, and the board panics into a boot loop with ESP_ERR_NO_MEM.
 * Moving the fetch, weather, photo, route, DNS and GPS stacks to PSRAM gives that back.
 *
 * The 1.43 build had to keep them internal, and the reason is worth knowing because it returns
 * if CONFIG_SPIRAM_XIP_FROM_PSRAM is ever turned off. Flash and PSRAM share one cache on the
 * S3, and a flash write -- an NVS save counts -- normally disables it, so a task with a PSRAM
 * stack cannot run while that is true. IDF asserts on it:
 *
 *   assert failed: spi_flash_disable_interrupts_caches_and_other_cpu cache_utils.c:127
 *   (esp_task_stack_is_sane_cache_disabled())
 *
 * With code and rodata running from PSRAM, IDF never disables the cache for flash operations
 * (SPI_FLASH_CACHE_NO_DISABLE in spi_flash_os_func_app.c takes a mutex instead), so that path
 * is unreachable. The RGB panel needs XIP regardless; do not turn it off without also turning
 * this back to MALLOC_CAP_INTERNAL and finding the internal RAM elsewhere.
 */
#define BOARD_TASK_STACK_CAPS (MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
