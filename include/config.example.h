#pragma once

// Copy this file to include/config.h and fill in your device-specific values.
// Do not share config.h publicly.

// This is NOT the normal Bluetooth pairing PIN printed on many Victron devices.
// It is the Instant Readout encryption key shown inside VictronConnect.
#define VICTRON_INSTANT_READOUT_KEY_HEX ""

// Optional: put the BLE MAC address here once the scanner finds the shunt.
// Leave empty to accept the first Victron SmartShunt-like advertisement.
#define VICTRON_BLE_ADDRESS ""

// Optional: a second Victron device for screen 2, usually a SmartSolar MPPT.
// If left empty, the firmware falls back to VICTRON_INSTANT_READOUT_KEY_HEX.
#define VICTRON_SOLAR_INSTANT_READOUT_KEY_HEX ""

// Optional: put the BLE MAC address here once the scanner finds the solar controller.
// Leave empty to accept the first Victron Solar Charger advertisement.
#define VICTRON_SOLAR_BLE_ADDRESS ""

// Used only to scale the solar power ring on screen 2.
#define VICTRON_SOLAR_MAX_POWER_W 500.0f

// Temporary setup hotspot shown from the settings page.
#define SETTINGS_AP_SSID "ChargeScreen"
#define SETTINGS_AP_PASSWORD ""

// Optional: put the ECO-WORTHY/JBD BLE address here once the scanner finds it.
#define ECOWORTHY_BLE_ADDRESS ""
