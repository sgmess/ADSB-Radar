# ADSB Radar

ADSB Radar is ESP-IDF firmware for the Waveshare
ESP32-S3-Touch-AMOLED-1.43 round touch display, a 466x466 CO5300 AMOLED. It is a
standalone aircraft radar display: the ESP32 joins WiFi, fetches live ADS-B
traffic, plots aircraft around a configurable centre point, and draws a
radar-style screen with range rings, heading information, altitude colours,
airport markers, country outlines, weather icons, and optional runway geometry.

Once the device has been configured it does not need a Raspberry Pi or a
browser left open. WiFi details, radar location, data source, display styling,
notifications, ranges, API keys, and hardware-control settings are saved on the
ESP32. The radar is operated from the touch screen, the optional physical
controls, or the built-in browser admin page.

AI assistance has been used during development of this project, mainly for code
generation, refactoring, documentation, and debugging. The firmware has still
been built and tested on the target hardware as the project has evolved.

> **This branch is the ESP32-S3-Touch-AMOLED-1.43 port**, built and running on
> that board. The original firmware
> targets the Waveshare ESP32-P4-WIFI6-Touch-LCD-XC with a 720x720 or 800x800
> round LCD; that version lives on `main`. The two boards differ in SoC, display
> bus, touch controller, and WiFi path, so they are separate builds rather than
> one image. See [Differences From The ESP32-P4 Build](#differences-from-the-esp32-p4-build).

## Screenshots

| Live airport view | Wide-area traffic view |
| --- | --- |
| ![ADSB Radar centred on London Heathrow with nearby aircraft, airport labels, range rings, and the live sweep line](assets/Radar%20Images/egll.png) | ![ADSB Radar at 250 miles showing dense regional aircraft traffic, country outlines, altitude colours, and radar labels](assets/Radar%20Images/250miles.png) |
| Centred on London Heathrow, showing close-range aircraft, airport context, range rings, heading arrows, and the sweep line. | A 250-mile regional view with country outlines, altitude-coloured aircraft, dense traffic, and range labels. |

| Busy terminal area | Aircraft detail popup |
| --- | --- |
| ![ADSB Radar centred around Los Angeles with clustered aircraft, altitude colour bands, airport markers, and coastline outline](assets/Radar%20Images/lax.png) | ![ADSB Radar aircraft detail popup showing callsign, PlaneSpotters thumbnail, aircraft type, altitude, speed, heading, range, and vertical rate](assets/Radar%20Images/lax-photo.png) |
| Los Angeles terminal traffic with coastline overlay, airport markers, altitude colouring, and aircraft labels. | Touching an aircraft opens a detail popup with aircraft metadata and a PlaneSpotters thumbnail when one is available. |

| Weather overlay | 2026 King's flypast |
| --- | --- |
| ![ADSB Radar showing airport weather icons next to airport labels](assets/Radar%20Images/weather.png) | ![ADSB Radar showing aircraft from the 2026 King's flypast heading towards London](assets/Radar%20Images/king1.png) |
| Airport weather can be drawn beside airport labels using Open-Meteo data. | A wide-area view of the 2026 King's flypast traffic heading towards London. |

| Country boundary overlay | Military aircraft example |
| --- | --- |
| ![ADSB Radar over south-east England with country boundary outlines, airport labels, altitude-coloured aircraft, and range rings](assets/Radar%20Images/eufi.png) | ![ADSB Radar showing a highlighted C-17 aircraft example](assets/Radar%20Images/c17.png) |
| Country outlines can be drawn subtly behind the radar data, giving extra context without hiding aircraft. | Notification and aircraft-colour settings can be used to make aircraft of interest stand out. |

## Browser UI

The browser admin is part of the firmware and is served directly by the ESP32.
These screenshots are from `assets/UI Images`.

| Dashboard | Location |
| --- | --- |
| ![Dashboard page showing memory, GPS, caches, data source and aircraft status](assets/UI%20Images/Dashboard.png) | ![Location page with centre-source cards and OpenStreetMap location picker](assets/UI%20Images/Location.png) |
| The dashboard brings together device state, aircraft status, screenshots, and settings import/export. | The location page supports manual position, GPS, airport search, and named-place search with a map picker. |

| Data Sources | Display |
| --- | --- |
| ![Data Sources page with aircraft source cards and service keys](assets/UI%20Images/Data%20Sources.png) | ![Display settings page showing grouped visual controls](assets/UI%20Images/Display.png) |
| Aircraft feeds, local receiver URLs, and service keys live in one place. | Display settings are grouped by purpose, including sweep, maps, aircraft, labels, colours, line widths, and visibility. |

| Notifications | Ranges |
| --- | --- |
| ![Notifications page with single-line aircraft type rules](assets/UI%20Images/Notifications.png) | ![Ranges page with compact range preset rows](assets/UI%20Images/Ranges.png) |
| Notification rules can colour, label, bold, or focus aircraft matching an exact aircraft type. | Up to ten range presets can be configured with refresh intervals, label limits, and optional history trails. |

| Hardware | WiFi |
| --- | --- |
| ![Hardware page with display type, SSD1306 module settings and a placeholder ST7789T3 tab](assets/UI%20Images/Hardware.png) | ![WiFi page showing current connection and scanned networks](assets/UI%20Images/Wifi.png) |
| Hardware covers the LCD profile, SSD1306 status display, physical buttons, rotary encoder pins, and action mapping. | WiFi setup can scan networks, save credentials, and report the current connection state. |

| Colour groups | Radar configuration |
| --- | --- |
| ![Display colour settings grouped into tabs](assets/UI%20Images/Colours.png) | ![Radar configuration controls in the browser UI](assets/UI%20Images/Radar%20Config.png) |
| Colour, width, visibility, and label controls are grouped so busy pages stay manageable. | The radar configuration controls expose the low-level display options without needing a rebuild. |

The project is built as one firmware image for the supported round LCD variants:

- 1.43-inch 466x466 round AMOLED (CO5300, QSPI).
- ESP32-S3 target with native WiFi.
- Waveshare ESP32-S3-Touch-AMOLED-1.43 BSP, included in this repository.
- LVGL 9 user interface

This board has one panel, so there is no display profile to choose. The radar
layout is compiled for 466x466.

Waveshare hardware documentation:

- [ESP32-S3-Touch-AMOLED-1.43 wiki](https://www.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.43)

The wiki's pin table is misaligned and lists an I2C SCL of GPIO 49, which does
not exist on an ESP32-S3. The pinout used by this firmware is documented in
`components/waveshare__esp32_s3_touch_amoled_143/include/bsp/esp32_s3_touch_amoled_143.h`
and was confirmed on hardware.

## Features

- Full-screen radar drawn for the 1.43-inch 466x466 round AMOLED.
- Live aircraft data from Airplanes.live, ADSB.lol, ADSB.fi, or a local
  `aircraft.json` feed such as readsb or dump1090.
- Touch-screen range control with up to ten configurable ranges, refresh
  intervals, label limits, and optional per-range aircraft history trails.
- Aircraft symbols coloured by altitude, with optional emergency squawk
  highlighting.
- Callsign and flight-level/speed labels, including configurable aircraft label
  font and size.
- ATC-style aircraft target symbols with configurable heading indicators:
  none, line, or arrow.
- Optional climb/descent triangles beside the flight level.
- Aircraft filtering from the radar DATA menu: all aircraft, military aircraft,
  or interesting aircraft.
- DATA menu toggles for aircraft headings, airports, country boundaries,
  runways, and aircraft on the ground.
- Aircraft detail popup from touch selection, with optional PlaneSpotters photo
  thumbnails and optional route information where available.
- Notification rules for aircraft type matches, including per-rule colour,
  enabled state, bold labels, optional "dim other aircraft" focus mode, and
  radar banner text.
- Airport markers generated from `data/airports.csv`, with airport search for
  setting the radar centre.
- Optional airport weather icons from Open-Meteo, including configurable icon
  size, colour, and refresh interval.
- Optional country boundary overlay generated from `data/world.geojson`.
- Optional runway overlay when the radar is centred on an airport, using
  AirportDB with cached, rate-limited API calls.
- Captive portal for first-time WiFi setup and a radar WiFi menu for IP address,
  changing WiFi, clearing NVS, and rebooting the device.
- Browser-based Bootstrap admin page with Location, Data Sources, Display,
  Hardware, Notifications, Ranges, WiFi, and Dashboard sections.
- Browser admin supports light and dark themes, stored per browser.
- Display controls for colours, line widths, visibility, label fonts and sizes,
  tick lengths, radial spacing, sweep step, sweep draw interval, and sweep trail
  behaviour.
- Dashboard pages for memory, heap low-water marks, caches, WiFi, GPS, data
  source, the full aircraft list, current LVGL screenshot download, and settings
  import/export.
- Aircraft table in the browser uses DataTables search and grouping, and marks
  aircraft currently shown with labels on the radar.
- Optional SSD1306 128x64 I2C display for local status such as WiFi/IP address
  and live background activity such as aircraft fetches, weather refreshes,
  photo downloads, route lookups, and runway fetches.
- Optional physical confirm/back buttons and rotary encoder for range changes
  and a small device-side menu. GPIO pins and the SSD1306 I2C address are
  configurable from the browser.
- Hardware page includes Display Type, SSD1306 Module, and a placeholder tab for
  future ST7789T3 module options.

## Required Hardware

Required:

- Waveshare ESP32-S3-Touch-AMOLED-1.43 (ESP32-S3, 16 MB flash, 8 MB PSRAM).
- USB-C cable for flashing, power, and serial monitor.
- A WiFi network with internet access.
- A development machine running Linux, macOS, or Windows.

Optional:

- SSD1306 128x64 I2C display at address `0x3c` for status output, on the shared
  I2C bus (SDA GPIO 47, SCL GPIO 48).
- Physical controls: a confirm button, a back button, and a rotary encoder.
  These have no default pins on this board and are off until you assign them in
  `Hardware -> SSD1306 Module` in the browser admin. Do not use GPIO 47 or 48
  (the I2C bus), 9-14 or 21 (the panel), or 0 (the boot button).
- AirportDB API token if you want runway drawing. AirportDB requires
  registration at [airportdb.io](https://airportdb.io) and the free API limit is
  5000 calls per month.
- OpenWeather API key if you want to search for the radar centre by place name.
- A local ADS-B receiver if you prefer to use a local `aircraft.json` feed
  instead of an internet aircraft API.

## Software Required

Install:

- ESP-IDF. This project has been built with ESP-IDF `v5.5.2`.
- Python 3.
- Git.
- CMake and Ninja. These are normally installed by the ESP-IDF tools installer.
- VS Code with the Espressif IDF extension, optional but useful.

The ESP-IDF extension is enough if it is configured to use a working ESP-IDF
installation and can run `idf.py` tasks. The command line examples below are the
same operations the extension performs.

## Project Layout

```text
.
|-- CMakeLists.txt
|-- data/
|   |-- airports.csv
|   `-- world.geojson
|-- assets/
|   |-- Radar Images/
|   `-- UI Images/
|-- main/
|   |-- app/
|   |-- aircraft/
|   |-- airports/
|   |-- core/
|   |-- gps/
|   |-- net/
|   |-- settings/
|   |-- ui/
|   `-- wifi/
|-- tools/
|   |-- generate_airports.py
|   `-- generate_boundaries.py
|-- components/
|-- main/idf_component.yml
|-- dependencies.lock
|-- partitions.csv
|-- sdkconfig
`-- sdkconfig.defaults
```

`data/airports.csv` and `data/world.geojson` are build inputs. The generated C++
files are written into the build directory and should not be committed.

`assets/Radar Images/` and `assets/UI Images/` are documentation images used by
this README. They are not compiled into the firmware.

`managed_components/` is downloaded by ESP-IDF Component Manager and is ignored
by Git.

## Clone And Open

```bash
git clone <your-repo-url> radar
cd radar
```

If you are using VS Code, open this folder and select the ESP-IDF extension
environment for ESP-IDF `v5.5.2` or another compatible ESP-IDF 5.x install.

## Configure ESP-IDF

From a shell:

```bash
. ~/esp/esp-idf-v5.5.2/export.sh
```

Adjust the path to match your ESP-IDF installation. If ESP-IDF is installed
somewhere else, use that full path instead.

Set the target:

```bash
idf.py set-target esp32s3
```

The project already includes `sdkconfig` and `sdkconfig.defaults`. If you need to
regenerate local config from defaults:

```bash
idf.py fullclean
idf.py set-target esp32s3
idf.py reconfigure
```

## Build

```bash
. ~/esp/esp-idf-v5.5.2/export.sh
idf.py build
```

During the build:

- ESP-IDF downloads managed dependencies into `managed_components/`.
- `tools/generate_airports.py` converts `data/airports.csv` into generated C++.
- `tools/generate_boundaries.py` converts `data/world.geojson` into generated C++.
- The final firmware is written to `build/radar_console.bin`.

The build output should end with something like:

```text
Project build complete.
```

## Flash

Connect the board to the development machine using the USB programming port.

Find the serial port:

```bash
ls /dev/ttyACM* /dev/ttyUSB*
```

On this board the USB serial device may appear as `QinHeng Electronics USB
Single Serial`, usually mapped to something like `/dev/ttyACM0` or `/dev/ttyUSB0`.

Flash and monitor:

```bash
. ~/esp/esp-idf-v5.5.2/export.sh
idf.py -p /dev/ttyACM0 flash monitor
```

Replace `/dev/ttyACM0` with the port on your machine.

If flashing cannot connect, put the ESP32-S3 into download mode:

1. Hold `BOOT`.
2. Press and release `RESET`.
3. Release `BOOT` after the flash command starts connecting.

Exit the serial monitor with `Ctrl+]`.

## First Boot And WiFi Setup

On first boot, or when no saved WiFi credentials exist, the device starts a
captive portal.

1. On a phone or computer, connect to the WiFi network shown on the radar screen.
   It is normally named `RadarSetup-XXXX`.
2. Open `http://192.168.4.1`.
3. Select your WiFi network and enter its password.
4. Save. The ESP32 stores the credentials and reconnects automatically.

After the device has joined your WiFi network, the radar screen shows the current
status on the WiFi menu. Use the WiFi button on the radar to view the IP address,
restart captive portal mode, or reboot the ESP32.

## Browser Admin

When the radar is connected to WiFi, open the device IP address in a browser.
The admin page is where most of the setup work happens:

- Dashboard shows the health of the ESP32: memory, heap low-water marks, cache
  use, WiFi, GPS, data source state, aircraft fetch state, a searchable aircraft
  table, a downloadable LVGL screenshot, and settings import/export.
- Location sets the radar centre. You can type coordinates, use a USB GPS fix,
  search the airport database, or search for a place name and pick the position
  from the map.
- Data Sources chooses the aircraft feed. The app can use Airplanes.live,
  ADSB.lol, ADSB.fi, or a local readsb/dump1090-style `aircraft.json` URL. This
  page also stores the AirportDB and OpenWeather keys.
- Display controls the look of the radar: layers, country outlines, runways,
  airport weather, heading indicators, ground aircraft, sweep timing, fading
  trail, colours, line widths, label fonts, tick spacing, and altitude bands.
- Hardware has a Display Type tab for the LCD profile, an SSD1306 Module tab for
  the status OLED, physical button GPIOs, rotary encoder GPIOs, and action
  mapping. A blank ST7789T3 tab is present ready for the next hardware-control
  implementation.
- Notifications configures up to ten exact aircraft-type rules. A match can
  colour the aircraft, keep its label visible, use bold text, show radar banner
  text, and optionally dim every other aircraft.
- Ranges configures up to ten selectable range presets. Each preset has its own
  distance, aircraft refresh interval, label limit, and optional ten-point
  history trail.
- WiFi shows the current connection, scans nearby networks with a normal signal
  strength indicator, and stores WiFi credentials.

Settings are stored in NVS on the ESP32 and are applied to the radar after they
are saved.

## Data Sources

The built-in internet aircraft feeds use the configured radar centre and range.
Ranges are entered in statute miles on the device, then converted where the
upstream API expects nautical miles.

Airplanes.live:

```text
https://api.airplanes.live/v2/point/{lat}/{lon}/{range_nm}
```

ADSB.lol:

```text
https://api.adsb.lol/v2/point/{lat}/{lon}/{range_nm}
```

ADSB.fi:

```text
https://opendata.adsb.fi/api/v3/lat/{lat}/lon/{lon}/dist/{range_nm}
```

ADSB.fi open data is documented at <https://github.com/adsbfi/opendata>.
Their public endpoint is for personal, non-commercial use and is rate limited,
so keep the range refresh intervals sensible.

Local receiver mode expects a readsb/dump1090-style JSON document, for example:

```text
http://192.168.1.28:8080/data/aircraft.json
```

Aircraft photos:

```text
https://api.planespotters.net/pub/photos/hex/{icao_hex}
```

The app adds browser-like headers and serialises HTTPS operations so aircraft
data and photo downloads do not overlap at the TLS layer.

Aircraft routes:

```text
https://api.adsbdb.com/v0/callsign/{flight}
```

Route lookups are optional and can be disabled in the Display settings. The
popup can show either a short route or a longer route description.

Airport runways:

```text
https://airportdb.io/api/v1/airport/{ICAO}?apiToken={apiToken}
```

Runway drawing is optional. It is only enabled when the radar is centred on an
airport and an AirportDB API token has been saved in settings.
[AirportDB](https://airportdb.io) requires registration and the free API limit is
5000 calls per month. The firmware caches and rate-limits AirportDB requests to
avoid wasting that quota.

Airport weather:

```text
https://api.open-meteo.com/v1/forecast
```

The app batches visible airport coordinates into one Open-Meteo request where
possible. Weather refresh defaults to 15 minutes and is configurable in the
Display settings. This does not require an API key.

Location search:

```text
http://api.openweathermap.org/geo/1.0/direct
```

OpenWeather geocoding is used only for named-place search on the Location page
and requires an OpenWeather API key.

## GPS

**Not available on this board.** The ESP32-P4 build supports a USB CDC NMEA GPS
receiver on the board's USB-A host port. The ESP32-S3-Touch-AMOLED-1.43 exposes
only the S3's native USB-C, which is the programming and console port and has no
switchable VBUS, so a bus-powered GPS dongle can neither be powered nor
enumerated.

GPS is therefore compiled out here rather than left to fail: the browser admin
hides the GPS centre source, and a settings file imported from a board that does
have GPS falls back to the manual centre. Set the radar centre manually, by
airport, or by place search.

## Device Controls

The round display has touch controls around the radar edge:

- Range button: tap the left side to step down, the right side to step up, or
  the middle to open the range list.
- DATA button: choose the aircraft filter and toggle map/aircraft layers.
- WiFi button: show the IP address, start WiFi setup, clear NVS, or reboot.

If the optional physical controls are fitted, the rotary encoder can change the
range or drive the small device menu. The confirm, back, and encoder push
buttons can be mapped to common actions in the Hardware page. This board wires
none of these itself and reserves most of its pins, so they start unassigned and
disabled; enter your own GPIO numbers on the Hardware page to enable them.

The optional SSD1306 display shows compact status information: WiFi state, IP
address, current background activity, aircraft data status, range, aircraft
count, and GPS state. It is deliberately simple, but useful when the round
display is showing the radar and you still want to know what the ESP32 is doing.

## Differences From The ESP32-P4 Build

This branch targets a different SoC, not just a different panel, so the changes
go deeper than a resolution swap.

| | ESP32-P4 build (`main`) | This build |
|---|---|---|
| SoC | ESP32-P4, dual RISC-V 400 MHz, 2D PPA | ESP32-S3, dual Xtensa 240 MHz, software rendering |
| Panel | 720x720 or 800x800 round LCD over MIPI-DSI | 466x466 CO5300 AMOLED over QSPI |
| Panel choice | Two profiles selected at runtime from NVS | One panel, compiled in |
| Touch | GT911 | FT3168 |
| WiFi | `esp_wifi_remote` + `esp_hosted` via an ESP32-C6 | Native `esp_wifi` |
| PSRAM | 32 MB hex | 8 MB octal |
| GPS | USB CDC NMEA on the USB-A host port | Not available; compiled out |
| Audio / PMIC | ES8311 codec, AXP2101 | Neither is fitted |

Things worth knowing if you are working on this branch:

- **Touch comes up after the display, always.** The FT3168 has no reset line of
  its own; it is tied to the panel reset on GPIO 21 and does not answer on I2C
  until the panel has been initialised. `bsp_display_start_with_config()` brings
  the panel up before the touch controller for that reason.
- **The FT3168 driver is part of this repository.** There is no FT3168 driver in
  the ESP component registry. It shares the FocalTech touch data layout with the
  FT5x06 family, but `espressif/esp_lcd_touch_ft5x06` cannot be reused: its init
  writes FT5x06 threshold registers the FT3168 does not implement, and it never
  brings the controller out of its default monitor mode, so the chip ACKs every
  read while reporting zero touch points forever.
- **Audio is removed rather than disabled.** The 1.75-inch sibling board's I2S
  BCLK and DIN are GPIO 9 and 10, which on this board are the panel's QSPI CS
  and SCLK. Leaving audio enabled here would drive the panel's own bus lines.
- **Layout constants are fixed, not scaled.** 466 is 65% of 720 linearly, but
  hit targets have a fingertip floor and text has a legibility floor, so the
  chrome dimensions in `main/core/RadarTypes.hpp` were re-chosen for this panel
  rather than multiplied through. Menus are centred because an off-centre box on
  a round panel loses its corners to the bezel.
- **Internal RAM is the scarce resource**, not PSRAM, and it is shared between
  the LVGL draw buffers and WiFi. Three settings were tuned against the board
  rather than guessed, and each comment in `sdkconfig.defaults` says what it
  costs: `LV_ATTRIBUTE_FAST_MEM_USE_IRAM`, `SPIRAM_FETCH_INSTRUCTIONS` and
  `SPIRAM_RODATA` are off, and WiFi's IRAM optimisations are disabled with its
  static RX buffers reduced. At stock WiFi settings `esp_wifi_init()` fails with
  `ESP_ERR_NO_MEM` and panics the board into a boot loop. The firmware logs free
  internal heap at startup for exactly this reason; it currently reports about
  220 KB free with a 116 KB largest block.
- **The LVGL draw buffers must be in internal RAM, not PSRAM.** PSRAM buffers
  fail two different ways: without `psram_dma_direct` on the panel IO, every
  flush tries to allocate an internal bounce buffer and fails once WiFi is up
  (`setup_dma_priv_buffer: Failed to allocate priv TX buffer`); with it, the DMA
  reads PSRAM directly but cannot keep the SPI FIFO fed while WiFi is also using
  PSRAM (`DMA TX underflow detected`). They are small on purpose --
  `BSP_DISPLAY_LVGL_BUF_HEIGHT` trades directly against WiFi.
- **Internal RAM runs out long before PSRAM does, and contiguity binds before
  capacity.** Three P4 assumptions had to go: task stacks in PSRAM (see below),
  `SPIRAM_MALLOC_ALWAYSINTERNAL=1024` (which forced the radar's hundreds of
  ~300-byte LVGL objects into internal RAM, leaving ~15 KB free with PSRAM 85%
  empty), and mbedTLS allocating internally. The last one mattered most for
  stability: TLS handshakes fragment internal heap, and after a burst of them
  the largest free block fell from 29 KB to 14 KB and never recovered, so the
  aircraft fetch's own headroom check latched off and the radar stopped updating
  until reboot -- with 34 KB still free. `MBEDTLS_EXTERNAL_MEM_ALLOC` plus
  thresholds recalibrated in `RadarHttpClient.cpp` fixed it; the largest block
  now holds steady around 30 KB.
- **Task stacks must be in internal RAM** (`BOARD_TASK_STACK_CAPS`). Flash and
  PSRAM share one cache on the S3, so a flash operation stops a PSRAM-stacked
  task dead; IDF asserts on it. The P4 build puts every task stack in PSRAM,
  which is safe there because flash and PSRAM have separate controllers.
- **The FT3168 NAKs one status read about every 13.4 seconds**, on a cadence
  steady to within 40 ms, which is its own periodic self-calibration and not a
  bus problem. The driver drops that poll rather than reporting a fault, and
  only warns after a run of consecutive failures.
- **Physical controls have no default pins.** The ESP32-P4 defaults (GPIO 30/46/
  47/52/48) are wrong here: 52 does not exist on an ESP32-S3, and 47/48 are the
  I2C bus shared by touch, the IMU and the RTC.

## Useful Commands

Clean build artifacts:

```bash
idf.py fullclean
```

Reconfigure after changing Kconfig or component files:

```bash
idf.py reconfigure
```

Open menuconfig:

```bash
idf.py menuconfig
```

Build, flash, and monitor in one command:

```bash
idf.py -p /dev/ttyACM0 build flash monitor
```

## Git Notes

Commit these files:

- Source under `main/`
- Data under `data/`
- Generators under `tools/`
- `components/bsp_extra/`
- `components/waveshare__esp32_s3_touch_amoled_143/`
- `main/idf_component.yml`
- `dependencies.lock`
- `sdkconfig`
- `sdkconfig.defaults`
- `partitions.csv`

Do not commit:

- `build/`
- `managed_components/`
- editor swap files
- local SDK config backups

The `.gitignore` file is set up for this.

## Troubleshooting

No serial port:

- Check the USB cable supports data.
- Try another USB port.
- On Linux, check `dmesg` after plugging in the board.
- Add your user to the serial group if required, for example `dialout` on Debian
  or Raspberry Pi OS.

Build fails after moving files or changing data:

```bash
idf.py fullclean
idf.py build
```

WiFi setup portal does not appear:

- Reboot the ESP32.
- Use the radar WiFi menu to start captive portal mode.
- Connect directly to the displayed `RadarSetup-XXXX` network and open
  `http://192.168.4.1`.

Aircraft data is stale:

- Check that the ESP32 has joined WiFi.
- Check the configured radar centre and range.
- Confirm the WiFi network has internet access and allows HTTPS.
- If you use a local receiver source, confirm the configured
  `aircraft.json` URL is reachable from the ESP32 network.
- Check the Dashboard aircraft table to see whether data is being received but
  filtered or outside the active range.

Runways are not displayed:

- Confirm the radar centre source is an airport.
- Confirm an AirportDB API token is saved in settings. Register at
  [airportdb.io](https://airportdb.io) if you do not have one.
- Confirm runway drawing is enabled in settings.

Sweep animation is slow:

- Disable optional aircraft history trails for the active range.
- Reduce the number of visible labels for busy ranges.
- Increase the sweep draw interval or reduce the sweep trail count in Display
  settings.
- Check the Dashboard memory panel for low internal/DMA heap.
