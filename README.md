# Standalone ESP32-C3 Battery Dashboard

Standalone BLE battery dashboard firmware for the JCZN ESP32-2424S012 round touchscreen ESP32-C3 board.

Designed for use with a Victron SmartShunt using Victron BLE Instant Readout data.

This project is intended to be fully standalone after flashing and does not require:

- Home Assistant
- MQTT
- Internet access
- Permanent Wi-Fi connection
- External gateway hardware

---

## Hardware

Target board:

- JCZN ESP32-2424S012
- ESP32-C3-MINI-1U
- 240x240 GC9A01 round LCD
- CST816 touch controller
- 4 MB flash

These devices can be found on Ali Express for around £15 GBP. 
"ESP32-C3 1.28 Inch Round Display GC9A01 IPS LCD Touch"

## Firmware Features

Current features include:

- Standalone Victron BLE monitoring
- Victron Instant Readout decryption
- Passive BLE listening
- Round touchscreen interface
- On-device configuration
- Temporary Wi-Fi setup hotspot
- Flash-based settings storage
- No cloud dependency

---

## Victron BLE Support

Victron SmartShunt and related Victron devices broadcast Instant Readout data over BLE manufacturer advertisements.

The dashboard listens passively and does not require a permanent BLE connection for normal monitoring.

Supported data includes:

- Voltage
- Current
- Power
- State of charge
- Signal status
- Connection state

To decrypt Instant Readout data, the firmware requires the Victron Instant Readout encryption key from VictronConnect.

---

## Device Settings

Settings are accessed using the three-dot vertical menu button on the dashboard.

From there, a temporary Wi-Fi hotspot can be enabled for setup and configuration.

Default hotspot:

- SSID: `ChargeScreen`
- Address: `http://192.168.4.1/`

The hotspot is intended only for local configuration and automatically disables itself after a period of inactivity.

Settings entered through the web interface are stored directly in ESP32 flash memory.

---

## Development Goals

Planned and in-progress work:

1. Improve UI polish and animations
2. Add additional dashboard pages
3. Improve BLE reliability and reconnect handling
4. Optimise memory usage on ESP32-C3
5. Add additional Victron device compatibility
6. Improve touchscreen interactions
7. Add optional battery alarm/status indicators

---

## Platform

This project uses:

- PlatformIO
- Arduino framework
- ESP32-C3
- Native firmware approach

The firmware is intentionally designed to avoid requiring Home Assistant or ESPHome once flashed.

---

## Licence

Copyright (c) 2026 RicoPicoUK

This project may be:

- used
- modified
- shared
- forked

for personal and non-commercial purposes only.

You may not:

- sell this software
- sell hardware containing this software
- include this software in commercial products
- redistribute modified versions commercially

Commercial use requires written permission from the author.

---

## Credits

Special thanks to Ed Hicks from VW Transporter Owners Club for the original concepts, ideas, and inspiration behind the project.