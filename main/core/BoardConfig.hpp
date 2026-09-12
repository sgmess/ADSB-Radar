#pragma once

#include "esp_heap_caps.h"

/*
 * Board capabilities and panel geometry for the Waveshare ESP32-S3-Touch-AMOLED-1.43.
 *
 * This build targets one board and one panel. The ESP32-P4 variant of this firmware supported
 * two round LCD profiles selected at runtime from NVS; here the panel is a fixed 466x466 CO5300
 * AMOLED, so the layout constants below are compile-time values rather than derived at boot.
 *
 * The 466x466 panel is 65% of the linear size of the 720x720 profile the original layout was
 * drawn for, so every chrome dimension has been re-chosen for this panel rather than scaled
 * arithmetically: hit targets have a floor set by fingertips, not by pixel ratios, and text has
 * a floor set by legibility. Numbers here that look like odd choices are usually one of those
 * two floors winning over a proportional value.
 */

/*
 * No USB-A host port and no switchable VBUS on this board, so a USB CDC GPS dongle cannot be
 * powered or enumerated. GpsReceiver keeps its API but compiles to a stub that reports the
 * feature as unavailable, and the browser admin hides the GPS centre source.
 */
#define BOARD_HAS_USB_GPS 0

/*
 * No AXP2101 PMIC and no ES8311 codec. Battery level is a divided analog reading on GPIO4, but
 * the divider ratio is not documented, so nothing reports a percentage.
 */
#define BOARD_HAS_PMIC    0
#define BOARD_HAS_AUDIO   0

/* Single fixed panel; there is no display-type choice to persist or offer. */
#define BOARD_HAS_DISPLAY_TYPE_CHOICE 0

#define BOARD_SCREEN_W 466
#define BOARD_SCREEN_H 466

/*
 * Heap capabilities for FreeRTOS task stacks.
 *
 * These MUST stay in internal RAM on the ESP32-S3. Flash and PSRAM share one cache here, so any
 * flash operation -- an NVS read counts -- disables the cache that PSRAM is reached through, and
 * a task whose stack lives in PSRAM cannot execute while that is true. IDF catches it:
 *
 *   assert failed: spi_flash_disable_interrupts_caches_and_other_cpu cache_utils.c:127
 *   (esp_task_stack_is_sane_cache_disabled())
 *
 * The ESP32-P4 build this was ported from puts every task stack in PSRAM, which is safe there
 * because flash and PSRAM sit on separate MSPI controllers. Do not copy that back.
 *
 * The failure is easy to miss: it needs a task that both has a PSRAM stack and reaches flash, so
 * on a fresh board it stays hidden until NVS actually holds something worth reading -- the fetch
 * task only trips it once WiFi credentials have been saved.
 */
#define BOARD_TASK_STACK_CAPS (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)
