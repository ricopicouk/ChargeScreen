# ESP32-C3 SuperMini BLE Capture Logger

This firmware turns an ESP32-C3 SuperMini into a small BLE advertisement recorder.

## User Flow

1. Power the device.
2. Connect to WiFi network `BLE-Capture`.
3. Open `http://192.168.4.1/`.
4. Wait for the 5 minute capture to finish, or press `Stop and save now`.
5. Download the CSV and email it for decoding.

The WiFi AP name stays fixed the whole time so phones do not get kicked off by a network rename.

## CSV Contents

The CSV records BLE advertisement packets with timing, address, name, RSSI, raw payload, manufacturer data, service data, and advertised UUIDs.

The file is named after the strongest BLE device seen. If the strongest device has no name, the filename uses its MAC address.

## Optional LED

The firmware has optional status LED support, disabled by default:

```cpp
static constexpr int STATUS_LED_PIN = -1;
```

Set that to a GPIO pin if you add an external LED with a resistor.
