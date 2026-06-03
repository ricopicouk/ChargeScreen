# Standalone ESP32-C3 Battery Dashboard

Target board: JCZN ESP32-2424S012 with ESP32-C3-MINI-1U, 240x240 GC9A01 round LCD, CST816 touch, 4 MB flash.

Goal: a standalone BLE dashboard for a Victron 300A SmartShunt and an ECO-WORTHY 12V 100Ah LiFePO4 battery with Bluetooth. It should not need Home Assistant, Wi-Fi, MQTT, or a gateway after flashing.

## Best Firmware Route

Use Arduino/PlatformIO for the device firmware.

ESPHome/Home Assistant can help for experiments, but the final device is better as native firmware because:

- Victron Instant Readout is BLE advertising data, usually encrypted with a per-device key.
- ECO-WORTHY batteries vary by model/BMS; many appear to use JBD-like BLE or serial protocols, but that needs confirmation.
- The round display and touch UI need tighter memory control on this ESP32-C3 board, which has no PSRAM.

Victron SmartShunt, BMV, SmartSolar, and similar devices can broadcast Instant Readout data via BLE manufacturer data. The manufacturer ID is `0x02E1`, and Instant Readout records use record type `0x10`.

For the Victron shunt the dashboard should listen passively. It does not need to pair or open a Bluetooth connection for normal Instant Readout data.

## Settings Hotspot

Tap the cog at the bottom of either dashboard screen to open Settings. The Start WiFi box starts a temporary hotspot named `ChargeScreen`. Connect to it and open `http://192.168.4.1/` to paste Victron Instant Readout keys.

Keys entered there are stored in the ESP32 flash preferences area. No Victron keys are compiled into the firmware. The setup hotspot turns itself off after 5 minutes without webpage activity.

## License

ChargeScreen firmware is source-available for personal, educational, evaluation, and non-commercial use only. Commercial sale or use is not permitted without written permission from the copyright holder. See [LICENSE](LICENSE).
