# Standalone ESP32-C3 Battery Dashboard

Target board: JCZN ESP32-2424S012 with ESP32-C3-MINI-1U, 240x240 GC9A01 round LCD, CST816 touch, 4 MB flash.

Goal: a standalone BLE dashboard for a Victron 300A SmartShunt and an ECO-WORTHY 12V 100Ah LiFePO4 battery with Bluetooth. It should not need Home Assistant, Wi-Fi, MQTT, or a gateway after flashing.

## Best Firmware Route

Use Arduino/PlatformIO for the device firmware.

ESPHome/Home Assistant can help for experiments, but the final device is better as native firmware because:

- Victron Instant Readout is BLE advertising data, usually encrypted with a per-device key.
- ECO-WORTHY batteries vary by model/BMS; many appear to use JBD-like BLE or serial protocols, but that needs confirmation.
- The round display and touch UI need tighter memory control on this ESP32-C3 board, which has no PSRAM.

## Hardware Pins

Display:

- LCD SCLK: GPIO 6
- LCD MOSI: GPIO 7
- LCD DC: GPIO 2
- LCD CS: GPIO 10
- LCD Backlight: GPIO 3
- LCD reset: not listed, assumed not connected

Touch:

- I2C SDA: GPIO 4
- I2C SCL: GPIO 5
- INT: GPIO 0
- RST: GPIO 1
- I2C address: 0x15

Other:

- Button: GPIO 9

## Victron Data Path

Victron SmartShunt, BMV, SmartSolar, and similar devices can broadcast Instant Readout data via BLE manufacturer data. The manufacturer ID is `0x02E1`, and Instant Readout records use record type `0x10`.

For the Victron shunt the dashboard should listen passively. It does not need to pair or open a Bluetooth connection for normal Instant Readout data.

To decode live values we need:

- Bluetooth MAC/address or a stable way to identify the device name.
- The Instant Readout encryption key from VictronConnect.
- Product type: 300A SmartShunt.

The printed Bluetooth pairing PIN/key on the side of the device is not normally the value needed here. The firmware needs the Instant Readout encryption key, which VictronConnect can show in the product info/settings area when Instant Readout is enabled.

## Settings Hotspot

Tap the cog at the bottom of either dashboard screen to open Settings. The Start WiFi box starts a temporary hotspot named `ChargeScreen`. Connect to it and open `http://192.168.4.1/` to paste Victron Instant Readout keys.

Keys entered there are stored in the ESP32 flash preferences area. No Victron keys are compiled into the firmware. The setup hotspot turns itself off after 5 minutes without webpage activity.

## BLE Capture Mode

Beta 4 adds a BLE Capture option to the WiFi settings page. Open Settings, start WiFi, connect to `ChargeScreen`, and visit `http://192.168.4.1/`. The web page explains what the capture will do and has a Start 5 Minute BLE Capture button.

During capture, normal dashboard decoding is paused and the round screen switches to a capture display with countdown, packet count, strongest device, RSSI, Stop & Save, and Cancel. The display updates the changing fields without constantly repainting the whole screen.

The CSV is stored in LittleFS and named after the strongest BLE device name or address. To download a saved capture, return to the WiFi page and use Download BLE CSV.

## Screen Power

Beta 4 includes configurable screen dimming and screen-off timeout. On settings page 2, tap Timeout to cycle the screen-off delay and tap Dimming to cycle Max, Mid, and Low brightness. The WiFi settings page also has a precise Screen off delay field.

When the screen is off, a deliberate touch wakes it. That wake touch is consumed until release so it does not also press a control underneath.

## ECO-WORTHY Data Path

ECO-WORTHY is the discovery item. The current target is the ECO-WORTHY 12V 100Ah LiFePO4 battery with Bluetooth. Some ECO-WORTHY batteries expose Bluetooth through a JBD-compatible BMS, while some larger batteries also expose RS485/CAN/RS232. For BLE we need:

- Exact battery model: 12V 100Ah LiFePO4 with Bluetooth.
- The phone app name used to view it.
- BLE advertised name and MAC address.
- Whether only one phone/device can connect at a time.

If the battery is JBD-compatible, we can add an active BLE client that connects, asks for pack/cell data, then disconnects. This is still BLE, but unlike the Victron shunt it may require a short actual BLE connection because many BMS units do not broadcast full battery statistics in advertisements.

## Build Phases

1. Bring up display and backlight.
2. Scan BLE and show nearby Victron/ECO-WORTHY candidates on screen.
3. Add Victron Instant Readout decryption.
4. Add ECO-WORTHY/JBD polling after confirming protocol.
5. Build the final dashboard screens:
   - Main: voltage, current, watts, SOC
   - Detail: cell voltages, temperature, alarms
   - Radio/status: last seen, signal strength, error state
6. Tune refresh rate and memory use.

## First Questions To Answer

- Can you get the Victron Instant Readout encryption key from VictronConnect?
- Which mobile app currently reads the ECO-WORTHY battery?
- What BLE name does the ECO-WORTHY battery advertise?
