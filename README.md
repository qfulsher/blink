Q Notes: 
1. Run `source "/home/quentin/.espressif/tools/activate_idf_v6.0.2.sh"` to activate the virtual environment. Then you can run `idf.py` commands.
2. Go to https://components.espressif.com/ to lookup components, after adding dependencies use `idf.py reconfigure` to pick them up


| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C5 | ESP32-C6 | ESP32-C61 | ESP32-H2 | ESP32-H21 | ESP32-H4 | ESP32-P4 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | --------- | -------- | --------- | -------- | -------- | -------- | -------- |

# openHatch / blink

This repository contains firmware for an ESP32-C6 based openHatch device. The project combines an SK6812 RGBW NeoPixel ring, a TM1637 4-digit 7-segment display, Wi-Fi control, a small embedded web UI, and SD-card storage for uploaded audio files.

## What the device does

- Drives a 16-LED SK6812 RGBW ring over RMT.
- Shows time on a TM1637 4-digit display with temporary override support for status messages.
- Starts a Wi-Fi station, syncs time with NTP, and hosts a local web interface.
- Stores uploaded `.wav` files on an SD card mounted at `/sdcard`.

## Hardware notes

- Target chip: ESP32-C6
- LED ring: SK6812 RGBW, GPIO 6
- Display: TM1637, CLK GPIO 5, DIO GPIO 4
- SD card interface: SPI2 with the pins defined in the SD subsystem
- The device is intended to be controlled over Wi-Fi through the embedded HTTP server and web UI

## Development setup

1. Activate the ESP-IDF environment in your shell.
2. Set the target to ESP32-C6.
3. Build and flash from the project root.

```bash
source "/home/quentin/.espressif/tools/activate_idf_v6.0.2.sh"
export IDF_TARGET=esp32c6
idf.py build
idf.py flash
```

## Project structure

- [main/main.c](main/main.c) is the top-level orchestration entry point.
- [main/led.c](main/led.c) and [main/display.c](main/display.c) handle the LED ring and clock display.
- [main/wifi.c](main/wifi.c) and [main/web.c](main/web.c) manage networking and the embedded UI.
- [main/sd.c](main/sd.c) handles SD card mounting and file operations.
- [main/web_assets](main/web_assets) contains the web UI assets that are embedded at build time.

## Build details

- The web UI assets are pre-gzipped during the build via [main/tools/gzip_file.py](main/tools/gzip_file.py).
- The firmware targets the ESP32-C6 and uses the ESP-IDF component manager for dependencies such as `led_strip` and `tm1637` support.

## Notes

- Wi-Fi credentials, GPIO assignments, and the mDNS hostname are currently compile-time constants.
- The SD card breakout requires a 5V supply for reliable mounting.
