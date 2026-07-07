# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

An ESP-IDF (v6.0.1) firmware project for the **ESP32-C6**. Despite the `blink`
name and leftover example boilerplate, it is an "openHatch" device: an SK6812
RGBW NeoPixel ring + a TM1637 4-digit 7-segment clock display, controlled over
Wi-Fi via an embedded HTTP server and a web UI, with an SD card for storing
uploaded `.wav` files.

## Environment setup (Windows)

Before any `idf.py` command, in a fresh PowerShell:

1. `subst E: C:\Users\QuentinFulsher\.espressif` — maps the toolchain to `E:`
   to dodge the Windows 260-char path limit during compilation. **Required.**
2. `E:\v6.0.1\esp-idf\export.ps1` — sets up the IDF CLI environment for the shell.

Target is `esp32c6` (see `.vscode/settings.json` `IDF_TARGET`). To switch:
`idf.py set-target <chip>`.

## Common commands

- Build: `idf.py build`
- Flash + serial monitor: `idf.py -p <PORT> flash monitor` (exit monitor: `Ctrl-]`)
- Config menu: `idf.py menuconfig`
- After adding/changing a dependency in `main/idf_component.yml`: `idf.py reconfigure`
- Look up components at https://components.espressif.com/

There are no unit tests for the application logic. `pytest_blink.py` is an
Espressif example harness that only checks the built binary size.

## Architecture

`app_main` (`main/main.c`) is the entire orchestration: it inits NVS, sets the
timezone (`PST8PDT`), then calls each subsystem's init in order and returns.
After init, work happens on background tasks and HTTP handler callbacks — there
is no main loop.

Each subsystem is a `.c`/`.h` pair in `main/` with a small init-and-go API:

- **led** (`led.c`) — SK6812 RGBW ring (16 LEDs, GPIO 6) via the `led_strip`
  RMT driver. State is a single `led_color_t` guarded by a mutex; the whole ring
  is one color. `led_set(bool)` is convenience on/off (warm white).
- **display** (`display.c`) — TM1637 7-segment (CLK GPIO 5, DIO GPIO 4). Key
  concept: a **temporary override**. `display_show_value_temp()` pins a value for
  N ms; while active, "background" renders from `display_show_time()` silently
  no-op. This is how an LED on/off flashes "1"/"0" on the display (`led.c` calls
  `display_show_value_temp` on transitions) without the clock immediately
  overwriting it.
- **clock** (`clock.c`) — spawns a FreeRTOS task that renders 12-hour local time
  to the display once per second via `display_show_time()`.
- **wifi** (`wifi.c`) — station mode; `wifi_init_sta()` blocks until connected or
  retries exhausted. `init_sntp()` syncs time from `pool.ntp.org`.
- **sd** (`sd.c`) — mounts a FAT filesystem over SPI (host SPI2; MISO 21, MOSI 20,
  SCK 19, CS 18) at `/sdcard`. **Fail-soft**: if no card mounts, init logs and
  returns, and the helper calls fail gracefully. All helpers take paths *relative*
  to the mount point and build absolute paths internally. Use these wrappers
  (`sd_fopen`, `sd_list_dir`, etc.) rather than touching `/sdcard` directly.
- **web** (`web.c`) — starts mDNS (`openHatch.local`) and an `esp_http_server`.
  Serves the embedded UI and a JSON API: `GET/POST /api/led`,
  `GET/POST /api/led/color`, `GET /api/files`, `POST/DELETE /api/files/<name>`.
  Uploads are streamed in 4 KB chunks to `sd_fopen`; only `.wav`, filenames
  validated against `/` and `..`. Wildcard routing (`/api/files/*`) requires
  `httpd_uri_match_wildcard`.

### Web assets are embedded at build time

`main/web_assets/{index.html,style.css,app.js}` are **pre-gzipped** by
`main/tools/gzip_file.py` via a CMake custom command (see `main/CMakeLists.txt`),
then baked into firmware with `EMBED_FILES`. `web.c` references them through
`_binary_<name>_gz_start/_end` symbols and serves them with
`Content-Encoding: gzip`. Editing a web asset and rebuilding re-gzips
automatically; there is no separate filesystem image for the UI.

## Gotchas

- **Hardcoded config.** GPIO pins, the Wi-Fi SSID/password (`wifi.c`), and the
  mDNS hostname (`web.c`) are compile-time constants, not Kconfig/NVS. The
  `Example Configuration` Kconfig menu and `CONFIG_BLINK_*` defaults are
  **vestigial** from the original blink example and do not drive the LED ring.
- **SD card power.** The SD breakout needs a **5V** supply (it has an onboard
  regulator + level shifters); feeding it 3.3V causes an ACMD41 timeout on mount.
- `CMakeLists.txt` sets `MINIMAL_BUILD ON` — only `main` and its declared
  `PRIV_REQUIRES` components are built. If a new feature needs an IDF component,
  add it to `PRIV_REQUIRES` in `main/CMakeLists.txt`.
