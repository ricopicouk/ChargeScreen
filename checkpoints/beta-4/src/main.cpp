#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include <NimBLEDevice.h>
#include <Preferences.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Wire.h>
#include <mbedtls/aes.h>
#include <mbedtls/sha256.h>
#include <math.h>

#include "aa_font.h"
#include "build_version.h"

static constexpr const char *SETTINGS_AP_SSID = "ChargeScreen";
static constexpr const char *SETTINGS_AP_PASSWORD = "";
static constexpr uint16_t DEFAULT_SOLAR_ARRAY_WATTS = 500;
static constexpr uint16_t MIN_SOLAR_ARRAY_WATTS = 30;
static constexpr uint16_t MAX_SOLAR_ARRAY_WATTS = 2000;
static constexpr uint16_t SOLAR_ARRAY_WATTS_STEP = 5;
static constexpr uint16_t SCREEN_TIMEOUT_NEVER = 0;
static constexpr uint16_t DEFAULT_SCREEN_TIMEOUT_SECONDS = 60;
static constexpr uint16_t MIN_SCREEN_TIMEOUT_SECONDS = 10;
static constexpr uint16_t MAX_SCREEN_TIMEOUT_SECONDS = 60;
static constexpr uint8_t BACKLIGHT_LEVEL_MAX = 0;
static constexpr uint8_t BACKLIGHT_LEVEL_MID = 1;
static constexpr uint8_t BACKLIGHT_LEVEL_LOW = 2;
static constexpr uint8_t BACKLIGHT_PWM_CHANNEL = 0;
static constexpr uint16_t BACKLIGHT_PWM_FREQUENCY = 5000;
static constexpr uint8_t BACKLIGHT_PWM_RESOLUTION = 8;

static constexpr int PIN_LCD_SCLK = 6;
static constexpr int PIN_LCD_MOSI = 7;
static constexpr int PIN_LCD_DC = 2;
static constexpr int PIN_LCD_CS = 10;
static constexpr int PIN_LCD_BL = 3;
static constexpr int PIN_TOUCH_SDA = 4;
static constexpr int PIN_TOUCH_SCL = 5;
static constexpr int PIN_TOUCH_INT = 0;
static constexpr int PIN_TOUCH_RST = 1;
static constexpr uint8_t PAGE_COUNT = 2;
static constexpr uint32_t CAPTURE_DURATION_MS = 5UL * 60UL * 1000UL;
static constexpr size_t CSV_FLUSH_EVERY_ROWS = 20;

static constexpr uint16_t VICTRON_COMPANY_ID = 0x02E1;
static constexpr uint8_t VICTRON_PRODUCT_ADVERTISEMENT = 0x10;
static constexpr uint8_t VICTRON_SOLAR_CHARGER_RECORD = 0x01;
static constexpr uint8_t VICTRON_BATTERY_MONITOR_RECORD = 0x02;
static constexpr uint8_t DISPLAY_ROTATION = 2;
static constexpr uint16_t COLOR_DARK_BLUE = 0x014A;
static constexpr uint16_t COLOR_PANEL_BLUE = 0x0129;
static constexpr uint16_t COLOR_RING_TRACK = 0x1268;
static constexpr uint16_t COLOR_RING_GREEN = 0x55E8;
static constexpr uint16_t COLOR_DIM_TEXT = 0xBDF7;
static constexpr uint16_t COLOR_BLACK_SOFT = 0x0000;
static constexpr uint16_t COLOR_SOLAR_PANEL = 0x2A20;
static constexpr uint16_t COLOR_SOLAR_TRACK = 0x5B23;
static constexpr uint16_t COLOR_SOLAR_ORANGE = 0xEC83;
static constexpr uint16_t COLOR_SOLAR_TEXT = 0xFFBD;
static constexpr uint16_t COLOR_SOLAR_DIM_TEXT = 0xD6AB;
static constexpr uint32_t VICTRON_QUIET_RESTART_MS = 6000;
static constexpr uint32_t VICTRON_SIGNAL_LOST_MS = 25000;
static constexpr uint32_t BLE_NEVER_SEEN_RESTART_MS = 15000;
static constexpr uint32_t SETTINGS_SERVER_IDLE_MS = 300000;
static constexpr bool DEMO_MODE = false;
static constexpr int16_t GAUGE_CENTER_X = 120;
static constexpr int16_t GAUGE_CENTER_Y = 120;
static constexpr int16_t GAUGE_OUTER_RADIUS = 120;
static constexpr int16_t GAUGE_INNER_RADIUS = 104;
static constexpr int16_t GAUGE_CLEAR_RADIUS = 101;
static constexpr float GAUGE_ARC_START_DEG = 128.0f;
static constexpr float GAUGE_ARC_SWEEP_DEG = 284.0f;
static constexpr uint8_t GAUGE_SEGMENTS = 8;
static constexpr float GAUGE_SEGMENT_GAP_DEG = 2.2f;
static constexpr uint8_t CST816_ADDR = 0x15;
static constexpr int16_t SWIPE_MIN_PIXELS = 45;
static constexpr float SWIPE_HORIZONTAL_BIAS = 1.25f;
static constexpr int16_t SETTINGS_BUTTON_X = 120;
static constexpr int16_t SETTINGS_BUTTON_Y = 224;
static constexpr int16_t SETTINGS_BUTTON_R = 14;
static constexpr int16_t SETTINGS_BACK_X = 84;
static constexpr int16_t SETTINGS_BACK_Y = 204;
static constexpr int16_t SETTINGS_BACK_W = 72;
static constexpr int16_t SETTINGS_BACK_H = 26;
static constexpr int16_t SETTINGS_ROW_X = 38;
static constexpr int16_t SETTINGS_LABEL_W = 118;
static constexpr int16_t SETTINGS_VALUE_X = 164;
static constexpr int16_t SETTINGS_VALUE_W = 32;
static constexpr int16_t SETTINGS_ROW_H = 28;
static constexpr int16_t SETTINGS_WIFI_Y = 66;
static constexpr int16_t SETTINGS_SOLAR_WATTS_Y = 100;
static constexpr int16_t SETTINGS_DEMO_Y = 134;
static constexpr int16_t SETTINGS_ROTATION_Y = 168;
static constexpr int16_t SETTINGS_TIMEOUT_Y = 76;
static constexpr int16_t SETTINGS_BATTERY_Y = 116;
static constexpr int16_t SETTINGS_DIMMING_Y = 156;
static constexpr int16_t SETTINGS_LABELS_Y = 98;
static constexpr int16_t SETTINGS_GRID_Y = 136;
static constexpr int16_t ROTATE_BUTTON_X = 70;
static constexpr int16_t ROTATE_BUTTON_Y = 68;
static constexpr int16_t ROTATE_BUTTON_W = 100;
static constexpr int16_t ROTATE_BUTTON_H = 96;
static constexpr uint32_t SETTINGS_INFO_HOLD_MS = 3000;

Arduino_DataBus *bus = new Arduino_ESP32SPI(
    PIN_LCD_DC,
    PIN_LCD_CS,
    PIN_LCD_SCLK,
    PIN_LCD_MOSI,
    GFX_NOT_DEFINED);

Arduino_GFX *gfx = new Arduino_GC9A01(
    bus,
    GFX_NOT_DEFINED,
    2,
    true,
    240,
    240);

Preferences secrets;
WebServer settingsServer(80);
DNSServer captiveDnsServer;
static bool firmwareUploadFailed = false;
static bool firmwareUploadFinished = false;
static bool firmwareHeaderChecked = false;
static bool firmwareHashActive = false;
static size_t firmwareUploadSize = 0;
static uint8_t firmwareDigest[32] = {};
static mbedtls_sha256_context firmwareSha;

struct BleSeen {
  String name = "-";
  String address = "-";
  int rssi = -127;
  uint32_t seenMs = 0;
};

struct BatteryStats {
  bool valid = false;
  float voltage = 0.0f;
  float current = 0.0f;
  float power = 0.0f;
  float soc = 0.0f;
  float consumedAh = 0.0f;
  uint16_t timeToGoMinutes = 0xFFFF;
  uint16_t alarmReason = 0;
  uint8_t auxMode = 0;
  float auxValue = 0.0f;
  String error = "waiting";
};

struct SolarStats {
  bool valid = false;
  uint8_t deviceState = 0xFF;
  uint8_t errorCode = 0xFF;
  float batteryVoltage = 0.0f;
  float batteryCurrent = 0.0f;
  float yieldTodayKwh = 0.0f;
  float pvPower = 0.0f;
  float loadCurrent = NAN;
  String error = "waiting";
};

BleSeen victronSeen;
BleSeen solarSeen;
BleSeen ecoSeen;
BatteryStats victronStats;
SolarStats solarStats;
static bool gaugeFaceDrawn = false;
static bool gaugeValuesDrawn = false;
static int lastSocDisplay = -1;
static String lastStatusText;
static String lastVoltageText;
static String lastCurrentText;
static String lastPowerText;
static String lastAuxText;
static String lastErrorText;
static bool solarValuesDrawn = false;
static String lastSolarPowerText;
static String lastSolarStatusText;
static String lastSolarBatteryText;
static String lastSolarCurrentText;
static String lastSolarYieldText;
static String lastSolarLoadText;
static String lastSolarErrorText;
static uint32_t lastVictronAdvertMs = 0;
static uint32_t lastVictronDecodeMs = 0;
static uint32_t lastSolarAdvertMs = 0;
static uint32_t lastSolarDecodeMs = 0;
static uint32_t lastBleWatchdogMs = 0;
static uint32_t lastBleScanStartMs = 0;
static uint8_t currentPage = 0;
static uint8_t settingsPageIndex = 0;
static bool settingsPageActive = false;
static bool settingsRotationPageActive = false;
static bool settingsInfoPageActive = false;
static bool pageTwoDrawn = false;
static bool capturePageDrawn = false;
static uint32_t lastCapturePageDrawMs = 0;
static String lastCaptureTitleText;
static String lastCaptureTimerText;
static String lastCaptureStatusText;
static String lastCapturePacketText;
static String lastCaptureStrongestText;
static String lastCaptureRssiText;
static String lastCaptureButtonText;
static bool settingsPageDrawn = false;
static bool settingsServerActive = false;
static uint32_t lastSettingsServerActivityMs = 0;
static String storedVictronKey;
static String storedSolarKey;
static String storedEcoWorthyPassword;
static bool demoModeEnabled = DEMO_MODE;
static bool ecoWorthyBatteryMode = false;
static bool showValueLabels = true;
static bool showValueGrid = true;
static uint16_t screenTimeoutSeconds = DEFAULT_SCREEN_TIMEOUT_SECONDS;
static uint8_t backlightLevel = BACKLIGHT_LEVEL_MAX;
static int screenRotationDegrees = DISPLAY_ROTATION * 90;
static uint8_t currentDisplayRotation = DISPLAY_ROTATION;
static float demoBatterySoc = 69.0f;
static float demoBatteryVoltage = 14.7f;
static float demoBatteryCurrent = 30.0f;
static float demoBatteryConsumedAh = 27.2f;
static float demoSolarPvPower = 268.0f;
static float demoSolarBatteryVoltage = 14.42f;
static float demoSolarChargeCurrent = 18.6f;
static float demoSolarYieldKwh = 1.27f;
static uint16_t solarArrayWatts = DEFAULT_SOLAR_ARRAY_WATTS;
static String settingsMessage;
static bool screenAwake = true;
static uint32_t lastTouchActivityMs = 0;
static bool captureActive = false;
static bool captureSaved = false;
static bool captureReadyScreenActive = false;
static uint32_t captureStartMs = 0;
static uint32_t captureStopMs = 0;
static uint32_t capturePacketCount = 0;
static uint32_t captureDroppedCount = 0;
static size_t captureRowsSinceFlush = 0;
static String capturePath = "/capture.csv";
static String captureFilename = "capture.csv";
static String captureStrongestName;
static String captureStrongestAddress;
static int captureStrongestRssi = -127;
static File captureFile;

struct TouchPoint {
  bool touched = false;
  int16_t x = 0;
  int16_t y = 0;
};

static void noteTouchActivity();

static String bytesToHex(const std::string &data) {
  static const char *hex = "0123456789ABCDEF";
  String out;
  out.reserve(data.size() * 2);
  for (uint8_t b : data) {
    out += hex[b >> 4];
    out += hex[b & 0x0F];
  }
  return out;
}

static String bytesToHex(const uint8_t *data, size_t len) {
  static const char *hex = "0123456789ABCDEF";
  String out;
  out.reserve(len * 2);
  for (size_t i = 0; i < len; i++) {
    uint8_t b = data[i];
    out += hex[b >> 4];
    out += hex[b & 0x0F];
  }
  return out;
}

static String vectorToHex(const std::vector<uint8_t> &data) {
  return data.empty() ? "" : bytesToHex(data.data(), data.size());
}

static String csvEscape(const String &value) {
  String out = "\"";
  for (size_t i = 0; i < value.length(); i++) {
    char c = value[i];
    if (c == '"') {
      out += "\"\"";
    } else if (c >= 32 && c <= 126) {
      out += c;
    }
  }
  out += "\"";
  return out;
}

static String jsonEscape(const String &value) {
  String out;
  out.reserve(value.length());
  for (size_t i = 0; i < value.length(); i++) {
    char c = value[i];
    switch (c) {
      case '\\':
        out += "\\\\";
        break;
      case '"':
        out += "\\\"";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        if (c >= 32 && c <= 126) {
          out += c;
        }
        break;
    }
  }
  return out;
}

static String safeFilenamePart(String value) {
  value.trim();
  if (!value.length()) {
    value = "BLE";
  }

  String out;
  out.reserve(value.length());
  for (size_t i = 0; i < value.length(); i++) {
    char c = value[i];
    if (isalnum(static_cast<unsigned char>(c))) {
      out += c;
    } else if (c == '-' || c == '_') {
      out += c;
    } else if (c == ' ' || c == ':' || c == '.') {
      out += '_';
    }
  }

  while (out.indexOf("__") >= 0) {
    out.replace("__", "_");
  }
  if (!out.length()) {
    out = "BLE";
  }
  if (out.length() > 40) {
    out = out.substring(0, 40);
  }
  return out;
}

static String advertisedName(const NimBLEAdvertisedDevice *device) {
  return device->haveName() ? String(device->getName().c_str()) : "";
}

static String manufacturerDataHex(const NimBLEAdvertisedDevice *device) {
  String out;
  for (uint8_t i = 0; i < device->getManufacturerDataCount(); i++) {
    if (i > 0) {
      out += "|";
    }
    out += bytesToHex(device->getManufacturerData(i));
  }
  return out;
}

static String serviceDataHex(const NimBLEAdvertisedDevice *device) {
  String out;
  for (uint8_t i = 0; i < device->getServiceDataCount(); i++) {
    if (i > 0) {
      out += "|";
    }
    out += device->getServiceDataUUID(i).toString().c_str();
    out += ":";
    out += bytesToHex(device->getServiceData(i));
  }
  return out;
}

static String serviceUuidList(const NimBLEAdvertisedDevice *device) {
  String out;
  for (uint8_t i = 0; i < device->getServiceUUIDCount(); i++) {
    if (i > 0) {
      out += "|";
    }
    out += device->getServiceUUID(i).toString().c_str();
  }
  return out;
}

static bool isVictronInstantReadout(const std::string &manufacturerData) {
  if (manufacturerData.size() < 4) {
    return false;
  }

  uint16_t companyId = static_cast<uint8_t>(manufacturerData[0]) |
                       (static_cast<uint8_t>(manufacturerData[1]) << 8);
  uint8_t advertisementType = static_cast<uint8_t>(manufacturerData[2]);
  return companyId == VICTRON_COMPANY_ID &&
         advertisementType == VICTRON_PRODUCT_ADVERTISEMENT;
}

static String normaliseAddress(const String &address) {
  String normalised;
  normalised.reserve(address.length());
  for (size_t i = 0; i < address.length(); i++) {
    char c = address[i];
    if (isxdigit(c)) {
      normalised += static_cast<char>(tolower(c));
    }
  }
  return normalised;
}

static bool matchesConfiguredVictronAddress(const String &address) {
  return true;
}

static bool matchesConfiguredSolarAddress(const String &address) {
  return true;
}

static bool hexToBytes(const char *hex, uint8_t *out, size_t outLen) {
  if (strlen(hex) != outLen * 2) {
    return false;
  }

  for (size_t i = 0; i < outLen; i++) {
    char pair[3] = {hex[i * 2], hex[i * 2 + 1], '\0'};
    char *end = nullptr;
    long value = strtol(pair, &end, 16);
    if (*end != '\0' || value < 0 || value > 255) {
      return false;
    }
    out[i] = static_cast<uint8_t>(value);
  }
  return true;
}

static bool isValidKeyHex(const String &key) {
  if (key.length() != 32) {
    return false;
  }
  for (size_t i = 0; i < key.length(); i++) {
    if (!isxdigit(key[i])) {
      return false;
    }
  }
  return true;
}

static bool isValidEcoWorthyPassword(const String &password) {
  if (password.length() < 1 || password.length() > 16) {
    return false;
  }
  for (size_t i = 0; i < password.length(); i++) {
    uint8_t c = static_cast<uint8_t>(password[i]);
    if (c < 32 || c > 126) {
      return false;
    }
  }
  return true;
}

static String normaliseKeyHex(String key) {
  key.trim();
  key.replace(" ", "");
  key.replace(":", "");
  key.replace("-", "");
  key.toUpperCase();
  return key;
}

static String normalisePassword(String password) {
  password.trim();
  return password;
}

static const char *activeVictronKey() {
  return storedVictronKey.c_str();
}

static const char *activeSolarKey() {
  if (storedSolarKey.length()) {
    return storedSolarKey.c_str();
  }
  return "";
}

static const char *activeEcoWorthyPassword() {
  return storedEcoWorthyPassword.length() ? storedEcoWorthyPassword.c_str() : "123123";
}

static String batteryModeText() {
  return ecoWorthyBatteryMode ? "EcoW" : "Victron";
}

static uint16_t sanitiseSolarArrayWatts(int value) {
  if (value < MIN_SOLAR_ARRAY_WATTS) {
    return MIN_SOLAR_ARRAY_WATTS;
  }
  if (value > MAX_SOLAR_ARRAY_WATTS) {
    return MAX_SOLAR_ARRAY_WATTS;
  }
  return static_cast<uint16_t>(value);
}

static uint16_t sanitiseScreenTimeoutSeconds(int value) {
  if (value == SCREEN_TIMEOUT_NEVER) {
    return SCREEN_TIMEOUT_NEVER;
  }
  if (value <= 10) {
    return MIN_SCREEN_TIMEOUT_SECONDS;
  }
  if (value <= 30) {
    return 30;
  }
  if (value <= MAX_SCREEN_TIMEOUT_SECONDS) {
    return MAX_SCREEN_TIMEOUT_SECONDS;
  }
  return SCREEN_TIMEOUT_NEVER;
}

static String screenTimeoutText() {
  return screenTimeoutSeconds == SCREEN_TIMEOUT_NEVER
             ? "0"
             : String(screenTimeoutSeconds) + "s";
}

static uint8_t sanitiseBacklightLevel(uint8_t level) {
  if (level > BACKLIGHT_LEVEL_LOW) {
    return BACKLIGHT_LEVEL_MAX;
  }
  return level;
}

static uint8_t backlightDutyForLevel(uint8_t level) {
  switch (sanitiseBacklightLevel(level)) {
    case BACKLIGHT_LEVEL_LOW:
      return 45;
    case BACKLIGHT_LEVEL_MID:
      return 128;
    default:
      return 255;
  }
}

static String backlightLevelText() {
  switch (sanitiseBacklightLevel(backlightLevel)) {
    case BACKLIGHT_LEVEL_LOW:
      return "Low";
    case BACKLIGHT_LEVEL_MID:
      return "Mid";
    default:
      return "Max";
  }
}

static void loadStoredSecrets() {
  secrets.begin("victron", false);
  storedVictronKey = secrets.getString("shunt_key", "");
  storedSolarKey = secrets.getString("solar_key", "");
  storedEcoWorthyPassword = secrets.getString("eco_key", "");
  demoModeEnabled = secrets.getBool("demo_mode", DEMO_MODE);
  ecoWorthyBatteryMode = secrets.getBool("battery_eco", false);
  showValueLabels = secrets.getBool("value_labels", true);
  showValueGrid = secrets.getBool("value_grid", true);
  solarArrayWatts = secrets.getUShort("solar_watts", DEFAULT_SOLAR_ARRAY_WATTS);
  uint16_t sanitisedSolarArrayWatts = sanitiseSolarArrayWatts(solarArrayWatts);
  if (sanitisedSolarArrayWatts != solarArrayWatts) {
    solarArrayWatts = sanitisedSolarArrayWatts;
    secrets.putUShort("solar_watts", solarArrayWatts);
  }
  demoBatterySoc = secrets.getFloat("demo_soc", 69.0f);
  demoBatteryVoltage = secrets.getFloat("demo_volt", 14.7f);
  demoBatteryCurrent = secrets.getFloat("demo_curr", 30.0f);
  demoBatteryConsumedAh = secrets.getFloat("demo_ah", 27.2f);
  demoSolarPvPower = secrets.getFloat("demo_pv", 268.0f);
  demoSolarBatteryVoltage = secrets.getFloat("demo_svolt", 14.42f);
  demoSolarChargeCurrent = secrets.getFloat("demo_scurr", 18.6f);
  demoSolarYieldKwh = secrets.getFloat("demo_yield", 1.27f);
  screenTimeoutSeconds = secrets.getUShort("screen_timeout", DEFAULT_SCREEN_TIMEOUT_SECONDS);
  uint16_t sanitisedTimeoutSeconds = sanitiseScreenTimeoutSeconds(screenTimeoutSeconds);
  if (sanitisedTimeoutSeconds != screenTimeoutSeconds) {
    screenTimeoutSeconds = sanitisedTimeoutSeconds;
    secrets.putUShort("screen_timeout", screenTimeoutSeconds);
  }
  uint8_t storedBacklightLevel = secrets.getUChar("backlight", BACKLIGHT_LEVEL_MAX);
  backlightLevel = sanitiseBacklightLevel(storedBacklightLevel);
  if (backlightLevel != storedBacklightLevel) {
    secrets.putUChar("backlight", backlightLevel);
  }
  screenRotationDegrees = secrets.getInt("screen_rotation", DISPLAY_ROTATION * 90);
  if (screenRotationDegrees < 0 || screenRotationDegrees > 365) {
    screenRotationDegrees = DISPLAY_ROTATION * 90;
  }
  if (screenRotationDegrees >= 360) {
    screenRotationDegrees %= 360;
    secrets.putInt("screen_rotation", screenRotationDegrees);
  }
  currentDisplayRotation = static_cast<uint8_t>(((screenRotationDegrees + 45) / 90) % 4);
}

static int32_t signExtend(uint32_t value, uint8_t width) {
  uint32_t signBit = 1UL << (width - 1);
  uint32_t mask = (1UL << width) - 1;
  value &= mask;
  return static_cast<int32_t>((value ^ signBit) - signBit);
}

static uint32_t readBits(const uint8_t *data, size_t dataLen, uint16_t startBit, uint8_t bitCount) {
  uint32_t value = 0;
  for (uint8_t i = 0; i < bitCount; i++) {
    uint16_t bit = startBit + i;
    if ((bit / 8) >= dataLen) {
      break;
    }
    if (data[bit / 8] & (1 << (bit % 8))) {
      value |= (1UL << i);
    }
  }
  return value;
}

static bool readLeU16(const uint8_t *data, size_t dataLen, size_t offset, uint16_t &value) {
  if (offset + 1 >= dataLen) {
    return false;
  }
  value = static_cast<uint16_t>(data[offset]) |
          (static_cast<uint16_t>(data[offset + 1]) << 8);
  return true;
}

static void logDecryptedBytes(const char *label, const uint8_t *plain, size_t plainLen) {
  String hex;
  hex.reserve(plainLen * 2);
  for (size_t i = 0; i < plainLen; i++) {
    if (i > 0) {
      hex += ' ';
    }
    if (plain[i] < 0x10) {
      hex += '0';
    }
    hex += String(plain[i], HEX);
  }
  hex.toUpperCase();
  Serial.printf("%s decrypted bytes: %s\n", label, hex.c_str());
}

static bool aesCtrDecrypt(const uint8_t *key,
                          uint16_t nonce,
                          const uint8_t *encrypted,
                          size_t encryptedLen,
                          uint8_t *plain) {
  uint8_t counter[16] = {};
  uint8_t stream[16] = {};
  counter[0] = nonce & 0xFF;
  counter[1] = nonce >> 8;

  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  if (mbedtls_aes_setkey_enc(&aes, key, 128) != 0) {
    mbedtls_aes_free(&aes);
    return false;
  }

  for (size_t offset = 0; offset < encryptedLen; offset += sizeof(stream)) {
    if (mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, counter, stream) != 0) {
      mbedtls_aes_free(&aes);
      return false;
    }

    size_t blockLen = min(sizeof(stream), encryptedLen - offset);
    for (size_t i = 0; i < blockLen; i++) {
      plain[offset + i] = encrypted[offset + i] ^ stream[i];
    }

    for (int i = 0; i < 16; i++) {
      if (++counter[i] != 0) {
        break;
      }
    }
  }

  mbedtls_aes_free(&aes);
  return true;
}

static bool decryptVictronPayload(const uint8_t *payload,
                                  size_t payloadLen,
                                  const char *keyHex,
                                  uint8_t *plain,
                                  size_t &plainLen,
                                  String &error) {
  if (keyHex == nullptr || keyHex[0] == '\0') {
    error = "add key via WiFi";
    return false;
  }

  uint8_t key[16] = {};
  if (!hexToBytes(keyHex, key, sizeof(key))) {
    error = "bad key";
    return false;
  }

  if (payload[7] != key[0]) {
    error = "key mismatch";
    return false;
  }

  uint16_t nonce = static_cast<uint16_t>(payload[5]) |
                   (static_cast<uint16_t>(payload[6]) << 8);
  const uint8_t *encrypted = payload + 8;
  size_t encryptedLen = payloadLen - 8;
  plainLen = min(encryptedLen, plainLen);

  if (!aesCtrDecrypt(key, nonce, encrypted, plainLen, plain)) {
    error = "decrypt fail";
    return false;
  }

  error = "";
  return true;
}

static bool decodeVictronBatteryMonitor(const std::string &manufacturerData) {
  victronStats.error = "";

  if (manufacturerData.size() < 11) {
    victronStats.valid = false;
    victronStats.error = "short advert";
    return false;
  }

  const uint8_t *data = reinterpret_cast<const uint8_t *>(manufacturerData.data());
  const uint8_t *payload = data + 2;
  size_t payloadLen = manufacturerData.size() - 2;

  if (payloadLen < 9 || payload[0] != VICTRON_PRODUCT_ADVERTISEMENT) {
    victronStats.valid = false;
    victronStats.error = "not instant";
    return false;
  }

  if (payload[4] != VICTRON_BATTERY_MONITOR_RECORD) {
    victronStats.valid = false;
    victronStats.error = "not shunt record";
    return false;
  }

  uint8_t plain[24] = {};
  size_t encryptedLen = sizeof(plain);

  if (!decryptVictronPayload(payload,
                             payloadLen,
                             activeVictronKey(),
                             plain,
                             encryptedLen,
                             victronStats.error)) {
    victronStats.valid = false;
    return false;
  }

  if (encryptedLen < 14) {
    victronStats.valid = false;
    victronStats.error = "short data";
    return false;
  }

  int32_t currentRaw = signExtend(readBits(plain, encryptedLen, 66, 22), 22);
  uint32_t consumedRaw = readBits(plain, encryptedLen, 88, 20);

  victronStats.timeToGoMinutes = readBits(plain, encryptedLen, 0, 16);
  victronStats.voltage = static_cast<int16_t>(readBits(plain, encryptedLen, 16, 16)) * 0.01f;
  victronStats.alarmReason = readBits(plain, encryptedLen, 32, 16);
  victronStats.auxValue = static_cast<int16_t>(readBits(plain, encryptedLen, 48, 16)) * 0.01f;
  victronStats.auxMode = readBits(plain, encryptedLen, 64, 2);
  victronStats.current = currentRaw * 0.001f;
  victronStats.power = victronStats.voltage * victronStats.current;
  victronStats.consumedAh = consumedRaw == 0xFFFFF ? NAN : -static_cast<float>(consumedRaw) * 0.1f;
  victronStats.soc = readBits(plain, encryptedLen, 108, 10) * 0.1f;
  victronStats.valid = true;
  victronStats.error = "";
  lastVictronDecodeMs = millis();

  Serial.printf("Victron decoded V=%.2fV I=%.3fA P=%.0fW consumed=%.1fAh soc=%.1f%% auxMode=%u aux=%.2f alarm=0x%04X\n",
                victronStats.voltage,
                victronStats.current,
                victronStats.power,
                victronStats.consumedAh,
                victronStats.soc,
                victronStats.auxMode,
                victronStats.auxValue,
                victronStats.alarmReason);
  return true;
}

static bool decodeVictronSolarCharger(const std::string &manufacturerData) {
  solarStats.error = "";

  if (manufacturerData.size() < 11) {
    solarStats.valid = false;
    solarStats.error = "short advert";
    return false;
  }

  const uint8_t *data = reinterpret_cast<const uint8_t *>(manufacturerData.data());
  const uint8_t *payload = data + 2;
  size_t payloadLen = manufacturerData.size() - 2;

  if (payloadLen < 9 || payload[0] != VICTRON_PRODUCT_ADVERTISEMENT) {
    solarStats.valid = false;
    solarStats.error = "not instant";
    return false;
  }

  if (payload[4] != VICTRON_SOLAR_CHARGER_RECORD) {
    solarStats.valid = false;
    solarStats.error = "not solar record";
    return false;
  }

  uint8_t plain[16] = {};
  size_t encryptedLen = sizeof(plain);

  if (!decryptVictronPayload(payload,
                             payloadLen,
                             activeSolarKey(),
                             plain,
                             encryptedLen,
                             solarStats.error)) {
    solarStats.valid = false;
    return false;
  }

  if (encryptedLen < 12) {
    solarStats.valid = false;
    solarStats.error = "short data";
    return false;
  }

  uint16_t batteryVoltageRaw = 0;
  uint16_t batteryCurrentRaw = 0;
  uint16_t yieldRaw = 0;
  uint16_t pvPowerRaw = 0;
  if (!readLeU16(plain, encryptedLen, 2, batteryVoltageRaw) ||
      !readLeU16(plain, encryptedLen, 4, batteryCurrentRaw) ||
      !readLeU16(plain, encryptedLen, 6, yieldRaw) ||
      !readLeU16(plain, encryptedLen, 8, pvPowerRaw)) {
    solarStats.valid = false;
    solarStats.error = "short data";
    return false;
  }
  uint16_t loadCurrentRaw = readBits(plain, encryptedLen, 80, 9);

  solarStats.deviceState = plain[0];
  solarStats.errorCode = plain[1];
  solarStats.batteryVoltage = batteryVoltageRaw == 0x7FFF
                                  ? NAN
                                  : static_cast<int16_t>(batteryVoltageRaw) * 0.01f;
  solarStats.batteryCurrent = batteryCurrentRaw == 0x7FFF
                                  ? NAN
                                  : static_cast<int16_t>(batteryCurrentRaw) * 0.1f;
  solarStats.yieldTodayKwh = yieldRaw == 0xFFFF ? NAN : yieldRaw * 0.01f;
  solarStats.pvPower = pvPowerRaw == 0xFFFF ? NAN : static_cast<float>(pvPowerRaw);
  solarStats.loadCurrent = loadCurrentRaw == 0x1FF ? NAN : loadCurrentRaw * 0.1f;
  solarStats.valid = true;
  solarStats.error = "";
  lastSolarDecodeMs = millis();

  logDecryptedBytes("Victron solar", plain, encryptedLen);
  Serial.printf("Victron solar raw state=%u err=%u V=0x%04X I=0x%04X yield=0x%04X PV=0x%04X load=0x%03X\n",
                solarStats.deviceState,
                solarStats.errorCode,
                batteryVoltageRaw,
                batteryCurrentRaw,
                yieldRaw,
                pvPowerRaw,
                loadCurrentRaw);
  Serial.printf("Victron solar decoded state=%u err=%u V=%.2fV I=%.1fA PV=%.0fW yield=%.2fkWh load=%.1fA\n",
                solarStats.deviceState,
                solarStats.errorCode,
                solarStats.batteryVoltage,
                solarStats.batteryCurrent,
                solarStats.pvPower,
                solarStats.yieldTodayKwh,
                solarStats.loadCurrent);
  return true;
}

static bool nameLooksEcoWorthyOrBms(const String &name) {
  String lower = name;
  lower.toLowerCase();
  return lower.indexOf("eco") >= 0 ||
         lower.indexOf("jbd") >= 0 ||
         lower.indexOf("xiaoxiang") >= 0 ||
         lower.indexOf("bms") >= 0;
}

static void drawTextCentered(const String &text, int16_t centerX, int16_t y, uint8_t textSize, uint16_t color) {
  gfx->setFont();
  int16_t textWidth = text.length() * 6 * textSize;
  gfx->setTextColor(color);
  gfx->setTextSize(textSize);
  gfx->setCursor(centerX - textWidth / 2, y);
  gfx->print(text);
}

static uint16_t blend565(uint16_t fg, uint16_t bg, uint8_t alpha) {
  if (alpha == 255) {
    return fg;
  }
  if (alpha == 0) {
    return bg;
  }

  uint8_t fgR = ((fg >> 11) & 0x1F) << 3;
  uint8_t fgG = ((fg >> 5) & 0x3F) << 2;
  uint8_t fgB = (fg & 0x1F) << 3;
  uint8_t bgR = ((bg >> 11) & 0x1F) << 3;
  uint8_t bgG = ((bg >> 5) & 0x3F) << 2;
  uint8_t bgB = (bg & 0x1F) << 3;

  uint8_t outR = (fgR * alpha + bgR * (255 - alpha)) / 255;
  uint8_t outG = (fgG * alpha + bgG * (255 - alpha)) / 255;
  uint8_t outB = (fgB * alpha + bgB * (255 - alpha)) / 255;
  return ((outR & 0xF8) << 8) | ((outG & 0xFC) << 3) | (outB >> 3);
}

static bool readAaGlyph(const AaFont &font, char c, AaGlyph &glyph) {
  for (uint8_t i = 0; i < font.glyphCount; i++) {
    memcpy_P(&glyph, &font.glyphs[i], sizeof(AaGlyph));
    if (glyph.c == c) {
      return true;
    }
  }
  return false;
}

static int16_t aaTextWidth(const AaFont &font, const String &text) {
  int16_t width = 0;
  AaGlyph glyph;
  for (size_t i = 0; i < text.length(); i++) {
    if (readAaGlyph(font, text[i], glyph)) {
      width += glyph.xAdvance;
    }
  }
  return width;
}

static void drawAaText(const AaFont &font, const String &text, int16_t x, int16_t y, uint16_t color) {
  int16_t cursorX = x;
  AaGlyph glyph;
  for (size_t i = 0; i < text.length(); i++) {
    if (!readAaGlyph(font, text[i], glyph)) {
      continue;
    }

    for (uint8_t py = 0; py < glyph.h; py++) {
      for (uint8_t px = 0; px < glyph.w; px++) {
        uint8_t alpha = pgm_read_byte(font.bitmap + glyph.offset + py * glyph.w + px);
        if (alpha == 0) {
          continue;
        }
        gfx->drawPixel(cursorX + glyph.xOffset + px,
                       y + glyph.yOffset + py,
                       blend565(color, COLOR_PANEL_BLUE, alpha));
      }
    }
    cursorX += glyph.xAdvance;
  }
}

static void drawAaCentered(const AaFont &font, const String &text, int16_t centerX, int16_t topY, uint16_t color) {
  int16_t width = aaTextWidth(font, text);
  drawAaText(font, text, centerX - width / 2, topY, color);
}

static bool touchReadBytes(uint8_t reg, uint8_t *buffer, size_t len) {
  Wire.beginTransmission(CST816_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  size_t read = Wire.requestFrom(static_cast<int>(CST816_ADDR), static_cast<int>(len));
  if (read != len) {
    return false;
  }

  for (size_t i = 0; i < len; i++) {
    buffer[i] = Wire.read();
  }
  return true;
}

static bool readTouchPoint(TouchPoint &point) {
  uint8_t data[6] = {};
  if (!touchReadBytes(0x01, data, sizeof(data))) {
    point.touched = false;
    return false;
  }

  uint8_t points = data[1] & 0x0F;
  if (points == 0) {
    point.touched = false;
    return true;
  }

  point.touched = true;
  point.x = ((data[2] & 0x0F) << 8) | data[3];
  point.y = ((data[4] & 0x0F) << 8) | data[5];
  int16_t rawX = point.x;
  int16_t rawY = point.y;
  switch (currentDisplayRotation) {
    case 1:
      point.x = rawY;
      point.y = 239 - rawX;
      break;
    case 2:
      point.x = 239 - rawX;
      point.y = 239 - rawY;
      break;
    case 3:
      point.x = 239 - rawY;
      point.y = rawX;
      break;
    default:
      point.x = rawX;
      point.y = rawY;
      break;
  }
  return true;
}

static void drawGaugeArcAt(int16_t centerX, float startDeg, float sweepDeg, uint16_t color) {
  if (sweepDeg <= 0.0f) {
    return;
  }

  float endDeg = startDeg + sweepDeg;
  while (startDeg >= 360.0f) {
    startDeg -= 360.0f;
    endDeg -= 360.0f;
  }

  if (endDeg <= 360.0f) {
    gfx->fillArc(centerX, GAUGE_CENTER_Y, GAUGE_OUTER_RADIUS, GAUGE_INNER_RADIUS, startDeg, endDeg, color);
  } else {
    gfx->fillArc(centerX, GAUGE_CENTER_Y, GAUGE_OUTER_RADIUS, GAUGE_INNER_RADIUS, startDeg, 360.0f, color);
    gfx->fillArc(centerX, GAUGE_CENTER_Y, GAUGE_OUTER_RADIUS, GAUGE_INNER_RADIUS, 0.0f, endDeg - 360.0f, color);
  }
}

static void drawGaugeArc(float startDeg, float sweepDeg, uint16_t color) {
  drawGaugeArcAt(GAUGE_CENTER_X, startDeg, sweepDeg, color);
}

static void invalidateScreens() {
  gaugeFaceDrawn = false;
  gaugeValuesDrawn = false;
  pageTwoDrawn = false;
  solarValuesDrawn = false;
  capturePageDrawn = false;
  settingsPageDrawn = false;
}

static void drawSegmentedGaugeArcAt(int16_t centerX,
                                    float fillSweepDeg,
                                    uint16_t trackColor,
                                    uint16_t fillColor,
                                    uint16_t backgroundColor = COLOR_PANEL_BLUE) {
  float segmentSweep = GAUGE_ARC_SWEEP_DEG / GAUGE_SEGMENTS;
  float fillEndDeg = GAUGE_ARC_START_DEG + constrain(fillSweepDeg, 0.0f, GAUGE_ARC_SWEEP_DEG);

  drawGaugeArcAt(centerX, GAUGE_ARC_START_DEG, GAUGE_ARC_SWEEP_DEG, backgroundColor);

  for (uint8_t i = 0; i < GAUGE_SEGMENTS; i++) {
    float segmentStart = GAUGE_ARC_START_DEG + i * segmentSweep + GAUGE_SEGMENT_GAP_DEG * 0.5f;
    float segmentEnd = GAUGE_ARC_START_DEG + (i + 1) * segmentSweep - GAUGE_SEGMENT_GAP_DEG * 0.5f;
    float visibleSweep = max(0.0f, segmentEnd - segmentStart);

    drawGaugeArcAt(centerX, segmentStart, visibleSweep, trackColor);

    float filledEnd = min(segmentEnd, fillEndDeg);
    if (filledEnd > segmentStart) {
      drawGaugeArcAt(centerX, segmentStart, filledEnd - segmentStart, fillColor);
    }
  }
}

static void drawSegmentedGaugeArc(float fillSweepDeg,
                                  uint16_t trackColor,
                                  uint16_t fillColor,
                                  uint16_t backgroundColor = COLOR_PANEL_BLUE) {
  drawSegmentedGaugeArcAt(GAUGE_CENTER_X, fillSweepDeg, trackColor, fillColor, backgroundColor);
}

static const char *victronStatusText() {
  if (!victronStats.valid) {
    if (!storedVictronKey.length()) {
      return "Add key via WiFi";
    }
    if (lastVictronAdvertMs > 0 && millis() - lastVictronAdvertMs < 10000) {
      return "Decode error";
    }
    return "Searching";
  }

  if (millis() - lastVictronDecodeMs > 10000) {
    return "Signal lost";
  }

  if (fabs(victronStats.current) < 0.05f) {
    return "Idle";
  }

  return victronStats.current > 0.0f ? "Charging" : "Discharging";
}

static float victronTemperatureC() {
  return victronStats.auxMode == 2 ? victronStats.auxValue - 273.15f : NAN;
}

static String formatSignedOneDecimal(float value, const char *unit) {
  char buffer[18];
  if (fabs(value) < 0.05f) {
    value = 0.0f;
  }
  snprintf(buffer, sizeof(buffer), "%.1f %s", value, unit);
  return String(buffer);
}

static String formatOneDecimalOrDash(float value, const char *unit) {
  if (isnan(value)) {
    return String("-- ") + unit;
  }
  char buffer[18];
  snprintf(buffer, sizeof(buffer), "%.1f %s", value, unit);
  return String(buffer);
}

static String formatTwoDecimalOrDash(float value, const char *unit) {
  if (isnan(value)) {
    return String("-- ") + unit;
  }
  char buffer[18];
  snprintf(buffer, sizeof(buffer), "%.2f %s", value, unit);
  return String(buffer);
}

static String formatWattsOrDash(float value) {
  if (isnan(value)) {
    return "-- W";
  }
  char buffer[18];
  snprintf(buffer, sizeof(buffer), "%.0f W", value);
  return String(buffer);
}

static void drawTinyLabel(const String &label, int16_t centerX, int16_t y, uint16_t color) {
  gfx->setFont();
  gfx->setTextSize(1);
  gfx->setTextColor(color);
  int16_t textWidth = label.length() * 6;
  gfx->setCursor(centerX - textWidth / 2, y);
  gfx->print(label);
}

static const char *solarStateText(uint8_t state) {
  switch (state) {
    case 0:
      return "Off";
    case 1:
      return "Low power";
    case 2:
      return "Fault";
    case 3:
      return "Bulk";
    case 4:
      return "Absorption";
    case 5:
      return "Float";
    case 6:
      return "Storage";
    case 7:
      return "Equalize";
    case 245:
      return "Starting";
    case 246:
      return "Absorb again";
    case 247:
      return "Recondition";
    case 248:
      return "BatterySafe";
    case 252:
      return "External";
    default:
      return "Charging";
  }
}

static SolarStats displaySolarStats() {
  if (!demoModeEnabled) {
    return solarStats;
  }

  SolarStats stats;
  stats.valid = true;
  stats.deviceState = 3;
  stats.errorCode = 0;
  stats.batteryVoltage = demoSolarBatteryVoltage;
  stats.batteryCurrent = demoSolarChargeCurrent;
  stats.yieldTodayKwh = demoSolarYieldKwh;
  stats.pvPower = demoSolarPvPower;
  stats.loadCurrent = 2.0f;
  stats.error = "";
  return stats;
}

static String displaySolarStatusText(const SolarStats &stats) {
  if (demoModeEnabled) {
    return "Demo solar";
  }

  if (!stats.valid) {
    if (!storedSolarKey.length()) {
      return "Add key via WiFi";
    }
    if (lastSolarAdvertMs > 0 && millis() - lastSolarAdvertMs < 10000) {
      return "Decode error";
    }
    return "Searching";
  }

  if (millis() - lastSolarDecodeMs > 10000) {
    return "Signal lost";
  }

  if (stats.errorCode != 0 && stats.errorCode != 0xFF) {
    return String("Error ") + String(stats.errorCode);
  }

  return solarStateText(stats.deviceState);
}

static BatteryStats displayStats() {
  if (!demoModeEnabled) {
    return victronStats;
  }

  BatteryStats stats;
  stats.valid = true;
  stats.voltage = demoBatteryVoltage;
  stats.current = demoBatteryCurrent;
  stats.power = demoBatteryVoltage * demoBatteryCurrent;
  stats.soc = demoBatterySoc;
  stats.consumedAh = demoBatteryConsumedAh;
  stats.error = "";
  return stats;
}

static const char *displayStatusText(const BatteryStats &stats) {
  if (demoModeEnabled) {
    return "Demo";
  }

  if (!stats.valid) {
    if (ecoWorthyBatteryMode) {
      return "Searching Eco-Worthy";
    }
    if (!storedVictronKey.length()) {
      return "Add key via WiFi";
    }
    if (lastVictronAdvertMs > 0 && millis() - lastVictronAdvertMs < 10000) {
      return "Decode error";
    }
    return "Searching";
  }

  if (millis() - lastVictronDecodeMs > 10000) {
    return "Signal lost";
  }

  if (fabs(stats.current) < 0.05f) {
    return "Idle";
  }

  return stats.current > 0.0f ? "Charging" : "Discharging";
}

static void closeCaptureFile() {
  if (captureFile) {
    captureFile.flush();
    captureFile.close();
  }
}

static void updateCaptureFilename() {
  String base = captureStrongestName.length() ? captureStrongestName : captureStrongestAddress;
  captureFilename = safeFilenamePart(base) + "_ble_capture.csv";
  capturePath = "/" + captureFilename;
}

static uint32_t captureRemainingSeconds() {
  if (!captureActive) {
    return 0;
  }

  uint32_t elapsed = millis() - captureStartMs;
  if (elapsed >= CAPTURE_DURATION_MS) {
    return 0;
  }
  return (CAPTURE_DURATION_MS - elapsed + 999) / 1000;
}

static String captureDeviceLabel() {
  if (captureStrongestName.length()) {
    return captureStrongestName.substring(0, 18);
  }
  if (captureStrongestAddress.length()) {
    return captureStrongestAddress;
  }
  return "-";
}

static void stopCapture(const char *reason) {
  if (!captureActive) {
    return;
  }

  captureActive = false;
  captureSaved = true;
  captureReadyScreenActive = true;
  captureStopMs = millis();
  closeCaptureFile();
  updateCaptureFilename();

  if (capturePath != "/capture.csv") {
    LittleFS.remove(capturePath);
    LittleFS.rename("/capture.csv", capturePath);
  }

  noteTouchActivity();
  if (settingsServerActive) {
    lastSettingsServerActivityMs = millis();
  }
  capturePageDrawn = false;
  Serial.printf("BLE capture stopped: %s packets=%lu file=%s\n",
                reason,
                static_cast<unsigned long>(capturePacketCount),
                capturePath.c_str());
}

static void cancelCapture(const char *reason) {
  if (!captureActive) {
    return;
  }

  captureActive = false;
  captureSaved = false;
  captureReadyScreenActive = false;
  captureStopMs = millis();
  closeCaptureFile();
  LittleFS.remove("/capture.csv");
  capturePageDrawn = false;
  Serial.printf("BLE capture cancelled: %s packets=%lu\n",
                reason,
                static_cast<unsigned long>(capturePacketCount));
}

static void startCapture() {
  closeCaptureFile();
  LittleFS.remove("/capture.csv");

  capturePath = "/capture.csv";
  captureFilename = "capture.csv";
  capturePacketCount = 0;
  captureDroppedCount = 0;
  captureRowsSinceFlush = 0;
  captureStrongestName = "";
  captureStrongestAddress = "";
  captureStrongestRssi = -127;
  captureStopMs = 0;
  captureStartMs = millis();
  captureSaved = false;
  captureReadyScreenActive = false;
  captureActive = true;

  captureFile = LittleFS.open("/capture.csv", FILE_WRITE);
  if (!captureFile) {
    captureActive = false;
    settingsMessage = "Capture file failed";
    Serial.println("BLE capture file open failed");
    return;
  }

  captureFile.println(F("millis,elapsed_s,address,address_type,name,rssi,adv_type,connectable,scannable,tx_power,manufacturer_hex,service_data_hex,service_uuids,raw_payload_hex"));
  captureFile.flush();
  noteTouchActivity();
  capturePageDrawn = false;
  Serial.println("BLE capture started");
}

static String captureStatusJson() {
  String json = "{";
  json += "\"active\":";
  json += captureActive ? "true" : "false";
  json += ",\"saved\":";
  json += captureSaved ? "true" : "false";
  json += ",\"remaining_s\":";
  json += String(captureRemainingSeconds());
  json += ",\"packets\":";
  json += String(capturePacketCount);
  json += ",\"dropped\":";
  json += String(captureDroppedCount);
  json += ",\"strongest\":\"";
  json += jsonEscape(captureDeviceLabel());
  json += "\",\"rssi\":";
  json += String(captureStrongestRssi);
  json += ",\"filename\":\"";
  json += jsonEscape(captureFilename);
  json += "\"}";
  return json;
}

static void captureAdvert(const NimBLEAdvertisedDevice *device) {
  if (!captureActive || !captureFile) {
    return;
  }

  uint32_t now = millis();
  if (now - captureStartMs >= CAPTURE_DURATION_MS) {
    stopCapture("timer");
    return;
  }

  String address = device->getAddress().toString().c_str();
  String name = advertisedName(device);
  int rssi = device->getRSSI();

  if (rssi > captureStrongestRssi) {
    captureStrongestRssi = rssi;
    captureStrongestAddress = address;
    captureStrongestName = name;
  }

  String line;
  line.reserve(260);
  line += String(now);
  line += ",";
  line += String((now - captureStartMs) / 1000.0f, 3);
  line += ",";
  line += csvEscape(address);
  line += ",";
  line += String(device->getAddressType());
  line += ",";
  line += csvEscape(name);
  line += ",";
  line += String(rssi);
  line += ",";
  line += String(device->getAdvType());
  line += ",";
  line += String(device->isConnectable() ? 1 : 0);
  line += ",";
  line += String(device->isScannable() ? 1 : 0);
  line += ",";
  line += device->haveTXPower() ? String(device->getTXPower()) : "";
  line += ",";
  line += csvEscape(manufacturerDataHex(device));
  line += ",";
  line += csvEscape(serviceDataHex(device));
  line += ",";
  line += csvEscape(serviceUuidList(device));
  line += ",";
  line += csvEscape(vectorToHex(device->getPayload()));

  if (captureFile.println(line) == 0) {
    captureDroppedCount++;
    return;
  }

  capturePacketCount++;
  captureRowsSinceFlush++;
  if (captureRowsSinceFlush >= CSV_FLUSH_EVERY_ROWS) {
    captureFile.flush();
    captureRowsSinceFlush = 0;
  }
}

static void restartBleScan(const char *reason) {
  NimBLEScan *scan = NimBLEDevice::getScan();
  Serial.printf("Restarting BLE scan: %s\n", reason);
  scan->start(0, true, true);
  lastBleScanStartMs = millis();
}

class ScanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice *device) override {
    if (captureActive) {
      captureAdvert(device);
      return;
    }

    String name = device->haveName() ? device->getName().c_str() : "";
    String address = device->getAddress().toString().c_str();
    int rssi = device->getRSSI();

    if (device->haveManufacturerData()) {
      std::string manufacturerData = device->getManufacturerData();
      if (isVictronInstantReadout(manufacturerData)) {
        const uint8_t *data = reinterpret_cast<const uint8_t *>(manufacturerData.data());
        uint8_t recordType = manufacturerData.size() > 6 ? data[6] : 0xFF;

        if (recordType == VICTRON_BATTERY_MONITOR_RECORD &&
            matchesConfiguredVictronAddress(address)) {
          victronSeen.name = name.length() ? name : "Victron";
          victronSeen.address = address;
          victronSeen.rssi = rssi;
          victronSeen.seenMs = millis();
          lastVictronAdvertMs = victronSeen.seenMs;
          bool decoded = decodeVictronBatteryMonitor(manufacturerData);

          Serial.printf("Victron shunt %s RSSI %d %s data %s\n",
                        address.c_str(),
                        rssi,
                        decoded ? "decoded" : victronStats.error.c_str(),
                        bytesToHex(manufacturerData).c_str());
        } else if (recordType == VICTRON_SOLAR_CHARGER_RECORD &&
                   matchesConfiguredSolarAddress(address)) {
          solarSeen.name = name.length() ? name : "SmartSolar";
          solarSeen.address = address;
          solarSeen.rssi = rssi;
          solarSeen.seenMs = millis();
          lastSolarAdvertMs = solarSeen.seenMs;
          bool decoded = decodeVictronSolarCharger(manufacturerData);

          Serial.printf("Victron solar %s RSSI %d %s data %s\n",
                        address.c_str(),
                        rssi,
                        decoded ? "decoded" : solarStats.error.c_str(),
                        bytesToHex(manufacturerData).c_str());
        }
      }
    }

    if (nameLooksEcoWorthyOrBms(name)) {
      ecoSeen.name = name;
      ecoSeen.address = address;
      ecoSeen.rssi = rssi;
      ecoSeen.seenMs = millis();
      Serial.printf("BMS candidate %s %s RSSI %d\n",
                    name.c_str(),
                    address.c_str(),
                    rssi);
      Serial.printf("Eco-Worthy/JBD password source: %s\n",
                    storedEcoWorthyPassword.length() ? "saved" : "default");
    }
  }
};

static void drawDeviceLine(int y, const char *label, const BleSeen &seen) {
  uint32_t ageSec = seen.seenMs == 0 ? 999 : (millis() - seen.seenMs) / 1000;

  gfx->setTextSize(1);
  gfx->setTextColor(0xAD55);
  gfx->setCursor(34, y);
  gfx->print(label);

  gfx->setTextColor(WHITE);
  gfx->setCursor(34, y + 14);
  gfx->print(seen.name.substring(0, 20));

  gfx->setTextColor(0xC618);
  gfx->setCursor(34, y + 28);
  if (seen.seenMs == 0) {
    gfx->print("scanning...");
  } else {
    gfx->printf("%ddBm  %lus", seen.rssi, static_cast<unsigned long>(ageSec));
  }
}

static void drawGaugeValues(bool force);
static String auxDisplayText();
static void drawCurrentPage(bool force = false);
static void setScreenRotationDegrees(int degrees);

static void writeBacklightDuty(uint8_t duty) {
  ledcWrite(BACKLIGHT_PWM_CHANNEL, duty);
}

static void applyBacklightLevel() {
  writeBacklightDuty(screenAwake ? backlightDutyForLevel(backlightLevel) : 0);
}

static void initBacklight() {
  ledcSetup(BACKLIGHT_PWM_CHANNEL, BACKLIGHT_PWM_FREQUENCY, BACKLIGHT_PWM_RESOLUTION);
  ledcAttachPin(PIN_LCD_BL, BACKLIGHT_PWM_CHANNEL);
  applyBacklightLevel();
}

static void setScreenAwake(bool awake) {
  if (screenAwake == awake) {
    return;
  }

  screenAwake = awake;
  applyBacklightLevel();
  if (awake) {
    invalidateScreens();
    drawCurrentPage(true);
  }
}

static void noteTouchActivity() {
  lastTouchActivityMs = millis();
  setScreenAwake(true);
}

static void updateScreenTimeout() {
  if (!screenAwake || captureActive || screenTimeoutSeconds == SCREEN_TIMEOUT_NEVER) {
    return;
  }

  uint32_t timeoutMs = static_cast<uint32_t>(screenTimeoutSeconds) * 1000UL;
  if (millis() - lastTouchActivityMs >= timeoutMs) {
    setScreenAwake(false);
  }
}

static void drawSettingsButtonAt(int16_t centerX, int16_t centerY, uint16_t color) {
  gfx->fillCircle(centerX, centerY - 7, 1, color);
  gfx->fillCircle(centerX, centerY, 1, color);
  gfx->fillCircle(centerX, centerY + 7, 1, color);
}

static void drawSettingsButton(uint16_t backgroundColor = COLOR_PANEL_BLUE,
                               uint16_t iconColor = WHITE) {
  gfx->fillCircle(SETTINGS_BUTTON_X, SETTINGS_BUTTON_Y, SETTINGS_BUTTON_R + 1, backgroundColor);
  drawSettingsButtonAt(SETTINGS_BUTTON_X, SETTINGS_BUTTON_Y, iconColor);
}

static void drawBackButton() {
  gfx->drawRect(SETTINGS_BACK_X, SETTINGS_BACK_Y, SETTINGS_BACK_W, SETTINGS_BACK_H, 0x6B4D);
  drawAaCentered(AA_FONT_SMALL, "Back", 120, SETTINGS_BACK_Y - 5, COLOR_DIM_TEXT);
}

static bool pointInCircle(int16_t x, int16_t y, int16_t centerX, int16_t centerY, int16_t radius) {
  int16_t dx = x - centerX;
  int16_t dy = y - centerY;
  return dx * dx + dy * dy <= radius * radius;
}

static bool pointInRect(int16_t x, int16_t y, int16_t left, int16_t top, int16_t width, int16_t height) {
  return x >= left && x < left + width && y >= top && y < top + height;
}

static String maskedKeyText(const String &key) {
  if (!key.length()) {
    return "not set";
  }
  return String("saved ...") + key.substring(28);
}

static String maskedPasswordText(const String &password) {
  return password.length() ? "saved" : "default 123123";
}

static String settingsStatusMessage(const String &code) {
  if (code == "saved") {
    return "Saved. Waiting for fresh BLE data.";
  }
  if (code == "cleared") {
    return "Saved keys cleared.";
  }
  if (code == "unchanged") {
    return "No changes.";
  }
  if (code == "invalid") {
    return "Check key/password/settings format.";
  }
  if (code == "ota-ok") {
    return "Firmware uploaded. Device is restarting.";
  }
  if (code == "ota-failed") {
    return "Firmware update failed. Try the correct firmware.bin.";
  }
  if (code == "capture") {
    return "BLE capture started. The round screen now shows progress.";
  }
  if (code == "capture-saved") {
    return "BLE capture saved. Download the CSV below.";
  }
  if (code == "capture-cancelled") {
    return "BLE capture cancelled. No CSV was saved.";
  }
  return "";
}

static void redirectToSettingsPage(const char *statusCode) {
  String location = String("/?status=") + statusCode;
  settingsServer.sendHeader("Location", location, true);
  settingsServer.send(303, "text/plain", "");
}

static bool looksLikeEsp32FirmwareChunk(const uint8_t *data, size_t len) {
  if (len < 24) {
    return false;
  }

  uint8_t magic = data[0];
  uint8_t segmentCount = data[1];
  uint8_t flashMode = data[2];
  uint8_t chipIdLow = data[12];
  uint8_t chipIdHigh = data[13];
  uint16_t chipId = static_cast<uint16_t>(chipIdLow) |
                    (static_cast<uint16_t>(chipIdHigh) << 8);

  return magic == 0xE9 &&
         segmentCount > 0 &&
         segmentCount <= 16 &&
         flashMode <= 3 &&
         (chipId == 0 || chipId == 5);
}

static String settingsPageHtml(const String &message = "") {
  String html;
  html.reserve(11000);
  html += F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<meta http-equiv='Cache-Control' content='no-store'>");
  html += F("<title>ChargeScreen Setup</title><style>");
  html += F("body{font-family:system-ui,-apple-system,Segoe UI,sans-serif;margin:0;background:#07151c;color:#eef;padding:22px}");
  html += F("main{max-width:560px;margin:auto}h1{text-align:center;margin:8px 0 4px}h2{margin-top:28px;border-top:1px solid #274653;padding-top:18px}label{display:block;margin:18px 0 6px;color:#b8cad2}");
  html += F("input,select{box-sizing:border-box;width:100%;padding:13px;border:1px solid #42616d;background:#102832;color:#fff;font-size:16px}");
  html += F("input[type=checkbox]{width:auto;margin-right:8px}.check{display:flex;align-items:center;margin-top:14px;color:#eef}.grid{display:grid;grid-template-columns:1fr 1fr;gap:0 12px}");
  html += F("button{margin-top:18px;width:100%;padding:14px;border:0;background:#2fb36d;color:#04140b;font-weight:700;font-size:16px}");
  html += F("button.secondary{background:#183743;color:#eef;border:1px solid #42616d}");
  html += F("button.danger{background:#56302f;color:#fff;border:1px solid #a85c58}");
  html += F("button:disabled{opacity:.65}.msg{padding:12px;background:#123345;margin:12px 0}");
  html += F(".hint,.version{color:#b8cad2;font-size:14px;line-height:1.45}.version{margin-top:22px;text-align:center}.split{display:grid;gap:10px;margin-top:8px}</style></head><body><main>");
  html += F("<h1>ChargeScreen</h1>");
  html += F("<p class='version'><a href='mailto:contact@chargescreen.co.uk' style='color:#8fe6ff'>contact@chargescreen.co.uk</a></p>");
  html += F("<p class='version'>");
  html += FIRMWARE_VERSION;
  html += F("</p>");
  if (message.length()) {
    html += F("<div class='msg' id='status'>");
    html += message;
    html += F("</div>");
  } else {
    html += F("<div class='msg' id='status' hidden></div>");
  }
  html += F("<h2>Using ChargeScreen</h2>");
  html += F("<p class='hint'>Power ChargeScreen using USB-C or the rear connector. If using the rear connector, plug it in carefully with the <strong>red wire closest to the USB-C port</strong>. Follow the connector position, not the cable colours: voltage and ground use the opposite colours to the usual convention.</p>");
  html += F("<p class='hint'>Open Settings from the cog on the round display, start WiFi, join the ChargeScreen hotspot, and use this page to enter keys and configure the display.</p>");
  html += F("<form method='post' action='/save' data-working='Saving...'>");
  html += F("<h2>Victron</h2>");
  html += F("<p class='hint'>For a Victron Shunt or Victron MPPT solar controller, open VictronConnect, connect to the device, open Settings, tap the three dots, then open Product info. Enable Instant Readout if needed, show the Instant Readout encryption data, and paste the 32 character encryption key below. A MAC address is not required. Saved keys stay on this device.</p>");
  html += F("<label for='shunt_key'>Battery monitor key</label><input id='shunt_key' name='shunt_key' autocomplete='off' placeholder='");
  html += maskedKeyText(storedVictronKey);
  html += F("'>");
  html += F("<label for='solar_key'>Solar controller key</label><input id='solar_key' name='solar_key' autocomplete='off' placeholder='");
  html += maskedKeyText(storedSolarKey);
  html += F("'>");
  html += F("<h2>Eco-Worthy</h2>");
  html += F("<p class='hint'>Eco-Worthy/JBD support is not working yet. These settings are here for testing future compatibility. Batteries commonly use password 123123.</p>");
  html += F("<label for='eco_key'>Eco-Worthy/JBD battery password</label><input id='eco_key' name='eco_key' autocomplete='off' placeholder='");
  html += maskedPasswordText(storedEcoWorthyPassword);
  html += F("'>");
  html += F("<h2>Sources</h2>");
  html += F("<label for='solar_source'>Solar</label><select id='solar_source' name='solar_source'><option value='victron' selected>Victron</option><option value='other' disabled>Other - not implemented</option></select>");
  html += F("<label for='battery_mode'>Battery source</label><select id='battery_mode' name='battery_mode'><option value='victron'");
  html += !ecoWorthyBatteryMode ? F(" selected") : F("");
  html += F(">Victron</option><option value='eco'");
  html += ecoWorthyBatteryMode ? F(" selected") : F("");
  html += F(">Eco-Worthy</option></select>");
  html += F("<h2>Settings</h2>");
  html += F("<label for='solar_watts'>Solar panel watts</label><input id='solar_watts' name='solar_watts' type='number' min='");
  html += MIN_SOLAR_ARRAY_WATTS;
  html += F("' max='");
  html += MAX_SOLAR_ARRAY_WATTS;
  html += F("' step='");
  html += SOLAR_ARRAY_WATTS_STEP;
  html += F("' value='");
  html += solarArrayWatts;
  html += F("'>");
  html += F("<label for='screen_timeout'>Screen off delay</label><select id='screen_timeout' name='screen_timeout'>");
  html += F("<option value='10'");
  html += screenTimeoutSeconds == 10 ? F(" selected") : F("");
  html += F(">10 seconds</option><option value='30'");
  html += screenTimeoutSeconds == 30 ? F(" selected") : F("");
  html += F(">30 seconds</option><option value='60'");
  html += screenTimeoutSeconds == 60 ? F(" selected") : F("");
  html += F(">60 seconds</option><option value='0'");
  html += screenTimeoutSeconds == SCREEN_TIMEOUT_NEVER ? F(" selected") : F("");
  html += F(">0</option></select>");
  html += F("<label for='screen_rotation'>Screen rotation</label><select id='screen_rotation' name='screen_rotation'>");
  const int rotationOptions[] = {0, 90, 180, 270};
  for (int degrees : rotationOptions) {
    html += F("<option value='");
    html += degrees;
    html += F("'");
    html += screenRotationDegrees == degrees ? F(" selected") : F("");
    html += F(">");
    html += degrees;
    html += F("</option>");
  }
  html += F("</select>");
  html += F("<label for='dimming'>Dimming</label><select id='dimming' name='dimming'><option value='0'");
  html += backlightLevel == BACKLIGHT_LEVEL_MAX ? F(" selected") : F("");
  html += F(">Max</option><option value='1'");
  html += backlightLevel == BACKLIGHT_LEVEL_MID ? F(" selected") : F("");
  html += F(">Mid</option><option value='2'");
  html += backlightLevel == BACKLIGHT_LEVEL_LOW ? F(" selected") : F("");
  html += F(">Low</option></select>");
  html += F("<label class='check'><input type='checkbox' name='labels' value='1'");
  html += showValueLabels ? F(" checked") : F("");
  html += F(">Show labels</label>");
  html += F("<label class='check'><input type='checkbox' name='value_grid' value='1'");
  html += showValueGrid ? F(" checked") : F("");
  html += F(">Show value grid</label>");
  html += F("<label class='check'><input type='checkbox' name='demo_mode' value='1'");
  html += demoModeEnabled ? F(" checked") : F("");
  html += F(">Demo mode</label>");
  html += F("<h2>Demo Values</h2><div class='grid'>");
  html += F("<label for='demo_soc'>Battery %</label><label for='demo_voltage'>Battery V</label>");
  html += F("<input id='demo_soc' name='demo_soc' type='number' step='1' value='");
  html += String(demoBatterySoc, 0);
  html += F("'><input id='demo_voltage' name='demo_voltage' type='number' step='0.1' value='");
  html += String(demoBatteryVoltage, 1);
  html += F("'><label for='demo_current'>Battery A</label><label for='demo_ah'>Used Ah</label><input id='demo_current' name='demo_current' type='number' step='0.1' value='");
  html += String(demoBatteryCurrent, 1);
  html += F("'><input id='demo_ah' name='demo_ah' type='number' step='0.1' value='");
  html += String(demoBatteryConsumedAh, 1);
  html += F("'><label for='demo_pv'>Solar W</label><label for='demo_solar_v'>Solar battery V</label><input id='demo_pv' name='demo_pv' type='number' step='1' value='");
  html += String(demoSolarPvPower, 0);
  html += F("'><input id='demo_solar_v' name='demo_solar_v' type='number' step='0.1' value='");
  html += String(demoSolarBatteryVoltage, 1);
  html += F("'><label for='demo_solar_a'>Solar charge A</label><label for='demo_yield'>Solar yield kWh</label><input id='demo_solar_a' name='demo_solar_a' type='number' step='0.1' value='");
  html += String(demoSolarChargeCurrent, 1);
  html += F("'><input id='demo_yield' name='demo_yield' type='number' step='0.01' value='");
  html += String(demoSolarYieldKwh, 2);
  html += F("'></div>");
  html += F("<button type='submit'>Save Settings</button></form>");
  html += F("<form method='post' action='/clear-keys' data-working='Clearing keys...'>");
  html += F("<button class='danger' type='submit'>Clear Saved Keys</button></form>");
  html += F("<h2>Premade ChargeScreen</h2>");
  html += F("<p class='hint'>Premade units with firmware already installed are available through the ChargeScreen project thread on the T6 Forum.</p>");
  html += F("<a href='https://www.t6forum.com/threads/standalone-esp32-ble-battery-display-for-victron-systems.66515/' style='display:block;box-sizing:border-box;width:100%;margin-top:18px;padding:14px;background:#2fb36d;color:#04140b;text-align:center;text-decoration:none;font-weight:700'>View T6 Forum thread</a>");
  html += F("<h2>BLE Capture</h2>");
  html += F("<p class='hint'>This records nearby Bluetooth Low Energy advertisements for 5 minutes so the data can be inspected later. During capture, the round screen changes to a capture display and normal dashboard decoding is paused. The CSV records timing, address, name, signal strength, manufacturer data, service data, advertised UUIDs, and raw advertisement payloads.</p>");
  html += F("<p class='hint'>Privacy note: this captures all BLE advertisements nearby, including packets from things like watches, phones, TVs, and sensors. Most of that data is encrypted or only useful to the device it belongs to, but the packets will still appear in the CSV. It is safe to share for debugging.</p>");
  html += F("<p class='hint'>Your battery data may also be encrypted. If the battery app shows Bluetooth encryption codes, include them with the email containing the capture. Those codes only help decode the battery packets; they do not give access to your WiFi, phone, or other devices. Realistically, the most they could reveal is your battery status if someone was physically nearby, for example on the same campsite.</p>");
  html += F("<p class='hint'>Send the completed CSV and any battery encryption codes to <a href='mailto:blecapture@chargescreen.co.uk' style='color:#8fe6ff'>blecapture@chargescreen.co.uk</a>. Captures are used to improve compatibility with more devices in future firmware.</p>");
  html += F("<p class='hint'>Use this with the device physically close to the battery or accessory you want to identify. The saved CSV is named after the strongest BLE signal heard during the capture.</p>");
  html += F("<div class='msg'><strong id='cap_state'>Capture status loading...</strong><br><span id='cap_detail' class='hint'></span></div>");
  if (captureActive) {
    html += F("<div class='msg'>Capture running. Watch the round screen or refresh this page for status.</div>");
    html += F("<form method='post' action='/capture-stop' data-working='Saving capture...'><button type='submit'>Stop and Save Now</button></form>");
    html += F("<form method='post' action='/capture-cancel' data-working='Cancelling capture...'><button class='secondary' type='submit'>Cancel Capture</button></form>");
  } else {
    html += F("<form method='post' action='/capture-start' data-working='Starting capture...'><button type='submit'>Start 5 Minute BLE Capture</button></form>");
  }
  if (captureSaved && LittleFS.exists(capturePath)) {
    html += F("<p class='hint'>BLE capture ready: ");
    html += captureFilename;
    html += F("</p><p class='hint'>After downloading, email the CSV file together with any Bluetooth encryption keys or codes shown in your battery app to <a href='mailto:blecapture@chargescreen.co.uk' style='color:#8fe6ff'>blecapture@chargescreen.co.uk</a>. This capture will be used to aid future development and improve compatibility with more devices. The keys are only used to decode battery packets in the capture.</p>");
    html += F("<a href='/download-capture' style='display:block;box-sizing:border-box;width:100%;margin-top:18px;padding:14px;background:#2fb36d;color:#04140b;text-align:center;text-decoration:none;font-weight:700'>Download BLE CSV</a>");
  } else {
    html += F("<p class='hint'>No BLE capture CSV is ready yet.</p>");
  }
  html += F("<form method='post' action='/firmware' enctype='multipart/form-data' data-working='Uploading firmware...'>");
  html += F("<label for='firmware'>Firmware update</label><input id='firmware' name='firmware' type='file' accept='.bin,application/octet-stream'>");
  html += F("<button class='secondary' type='submit'>Upload Firmware</button></form>");
  html += F("<button class='secondary' type='button' onclick='location.reload()'>Refresh Status</button>");
  html += F("<p class='hint'>Leave a key field blank to keep its current value. Firmware updates must be a valid ESP32-C3 firmware.bin built for this project. The hotspot turns off after 5 minutes with no page activity.</p>");
  html += F("<script>for(const f of document.forms){f.addEventListener('submit',()=>{const s=document.getElementById('status');s.hidden=false;s.textContent=f.dataset.working||'Working...';for(const b of document.querySelectorAll('button'))b.disabled=true;});}");
  html += F("function mmss(s){let m=Math.floor(s/60),r=s%60;return String(m).padStart(2,'0')+':'+String(r).padStart(2,'0')}");
  html += F("async function cap(){try{let r=await fetch('/capture-status');let j=await r.json();cap_state.textContent=j.active?'Recording '+mmss(j.remaining_s):(j.saved?'Capture ready':'No capture running');cap_detail.textContent='Packets '+j.packets+', strongest '+(j.strongest||'-')+', RSSI '+(j.rssi>-127?j.rssi+' dBm':'-');}catch(e){}}setInterval(cap,1000);cap();</script>");
  html += F("</main></body></html>");
  return html;
}

static void noteSettingsServerActivity() {
  lastSettingsServerActivityMs = millis();
}

static void handleSettingsRoot() {
  noteSettingsServerActivity();
  settingsServer.sendHeader("Cache-Control", "no-store");
  settingsServer.send(200, "text/html", settingsPageHtml(settingsStatusMessage(settingsServer.arg("status"))));
}

static void handleSettingsSave() {
  noteSettingsServerActivity();
  bool saved = false;
  bool invalid = false;
  String shuntKey = normaliseKeyHex(settingsServer.arg("shunt_key"));
  String solarKey = normaliseKeyHex(settingsServer.arg("solar_key"));
  String ecoKey = normalisePassword(settingsServer.arg("eco_key"));
  String solarWattsText = settingsServer.arg("solar_watts");
  String screenTimeoutText = settingsServer.arg("screen_timeout");
  String batteryModeText = settingsServer.arg("battery_mode");
  String rotationText = settingsServer.arg("screen_rotation");
  String dimmingText = settingsServer.arg("dimming");
  solarWattsText.trim();
  screenTimeoutText.trim();
  batteryModeText.trim();
  rotationText.trim();
  dimmingText.trim();

  if (shuntKey.length()) {
    if (isValidKeyHex(shuntKey)) {
      storedVictronKey = shuntKey;
      secrets.putString("shunt_key", storedVictronKey);
      saved = true;
    } else {
      invalid = true;
    }
  }

  if (solarKey.length()) {
    if (isValidKeyHex(solarKey)) {
      storedSolarKey = solarKey;
      secrets.putString("solar_key", storedSolarKey);
      saved = true;
    } else {
      invalid = true;
    }
  }

  if (ecoKey.length()) {
    if (isValidEcoWorthyPassword(ecoKey)) {
      storedEcoWorthyPassword = ecoKey;
      secrets.putString("eco_key", storedEcoWorthyPassword);
      saved = true;
    } else {
      invalid = true;
    }
  }

  if (solarWattsText.length()) {
    int requestedWatts = solarWattsText.toInt();
    if (requestedWatts >= MIN_SOLAR_ARRAY_WATTS && requestedWatts <= MAX_SOLAR_ARRAY_WATTS) {
      uint16_t nextWatts = sanitiseSolarArrayWatts(requestedWatts);
      if (nextWatts != solarArrayWatts) {
        solarArrayWatts = nextWatts;
        secrets.putUShort("solar_watts", solarArrayWatts);
        pageTwoDrawn = false;
        solarValuesDrawn = false;
      }
      saved = true;
    } else {
      invalid = true;
    }
  }

  if (screenTimeoutText.length()) {
    int requestedSeconds = screenTimeoutText.toInt();
    if (requestedSeconds == SCREEN_TIMEOUT_NEVER ||
        requestedSeconds == 10 ||
        requestedSeconds == 30 ||
        requestedSeconds == 60) {
      uint16_t nextSeconds = sanitiseScreenTimeoutSeconds(requestedSeconds);
      if (nextSeconds != screenTimeoutSeconds) {
        screenTimeoutSeconds = nextSeconds;
        secrets.putUShort("screen_timeout", screenTimeoutSeconds);
      }
      saved = true;
    } else {
      invalid = true;
    }
  }

  bool nextEcoMode = batteryModeText == "eco";
  if (nextEcoMode != ecoWorthyBatteryMode) {
    ecoWorthyBatteryMode = nextEcoMode;
    secrets.putBool("battery_eco", ecoWorthyBatteryMode);
    victronStats.valid = false;
  }
  saved = true;

  bool nextLabels = settingsServer.hasArg("labels");
  if (nextLabels != showValueLabels) {
    showValueLabels = nextLabels;
    secrets.putBool("value_labels", showValueLabels);
  }
  saved = true;

  bool nextGrid = settingsServer.hasArg("value_grid");
  if (nextGrid != showValueGrid) {
    showValueGrid = nextGrid;
    secrets.putBool("value_grid", showValueGrid);
  }
  saved = true;

  bool nextDemoMode = settingsServer.hasArg("demo_mode");
  if (nextDemoMode != demoModeEnabled) {
    demoModeEnabled = nextDemoMode;
    secrets.putBool("demo_mode", demoModeEnabled);
    victronStats.valid = false;
    solarStats.valid = false;
  }
  saved = true;

  if (rotationText.length()) {
    int requestedRotation = rotationText.toInt();
    if (requestedRotation == 0 || requestedRotation == 90 ||
        requestedRotation == 180 || requestedRotation == 270) {
      setScreenRotationDegrees(requestedRotation);
      saved = true;
    } else {
      invalid = true;
    }
  }

  if (dimmingText.length()) {
    int requestedDimming = dimmingText.toInt();
    if (requestedDimming >= BACKLIGHT_LEVEL_MAX && requestedDimming <= BACKLIGHT_LEVEL_LOW) {
      backlightLevel = sanitiseBacklightLevel(static_cast<uint8_t>(requestedDimming));
      secrets.putUChar("backlight", backlightLevel);
      applyBacklightLevel();
      saved = true;
    } else {
      invalid = true;
    }
  }

  demoBatterySoc = constrain(settingsServer.arg("demo_soc").toFloat(), 0.0f, 100.0f);
  demoBatteryVoltage = constrain(settingsServer.arg("demo_voltage").toFloat(), 0.0f, 80.0f);
  demoBatteryCurrent = constrain(settingsServer.arg("demo_current").toFloat(), -300.0f, 300.0f);
  demoBatteryConsumedAh = constrain(settingsServer.arg("demo_ah").toFloat(), -999.0f, 999.0f);
  demoSolarPvPower = constrain(settingsServer.arg("demo_pv").toFloat(), 0.0f, 5000.0f);
  demoSolarBatteryVoltage = constrain(settingsServer.arg("demo_solar_v").toFloat(), 0.0f, 80.0f);
  demoSolarChargeCurrent = constrain(settingsServer.arg("demo_solar_a").toFloat(), 0.0f, 300.0f);
  demoSolarYieldKwh = constrain(settingsServer.arg("demo_yield").toFloat(), 0.0f, 99.0f);
  secrets.putFloat("demo_soc", demoBatterySoc);
  secrets.putFloat("demo_volt", demoBatteryVoltage);
  secrets.putFloat("demo_curr", demoBatteryCurrent);
  secrets.putFloat("demo_ah", demoBatteryConsumedAh);
  secrets.putFloat("demo_pv", demoSolarPvPower);
  secrets.putFloat("demo_svolt", demoSolarBatteryVoltage);
  secrets.putFloat("demo_scurr", demoSolarChargeCurrent);
  secrets.putFloat("demo_yield", demoSolarYieldKwh);
  saved = true;

  if (invalid) {
    settingsMessage = "Check settings";
    redirectToSettingsPage("invalid");
  } else if (saved) {
    if (!demoModeEnabled) {
      victronStats.valid = false;
      solarStats.valid = false;
    }
    settingsMessage = "Saved settings";
    redirectToSettingsPage("saved");
  } else {
    settingsMessage = "No changes";
    redirectToSettingsPage("unchanged");
  }

  drawCurrentPage(true);
}

static void handleSettingsClearKeys() {
  noteSettingsServerActivity();
  storedVictronKey = "";
  storedSolarKey = "";
  storedEcoWorthyPassword = "";
  secrets.remove("shunt_key");
  secrets.remove("solar_key");
  secrets.remove("eco_key");
  if (!demoModeEnabled) {
    victronStats.valid = false;
    solarStats.valid = false;
  }
  settingsMessage = "Keys cleared";
  redirectToSettingsPage("cleared");
  drawCurrentPage(true);
}

static void drawFirmwareUpdateWarning() {
  screenAwake = true;
  applyBacklightLevel();
  gfx->fillScreen(0xF800);
  gfx->fillCircle(120, 120, 118, 0xC000);
  gfx->drawCircle(120, 120, 119, WHITE);
  drawAaCentered(AA_FONT_SMALL, "Firmware update", 120, 36, WHITE);
  drawAaCentered(AA_FONT_LARGE, "WAIT", 120, 64, WHITE);
  drawAaCentered(AA_FONT_SMALL, "Do not remove", 120, 122, WHITE);
  drawAaCentered(AA_FONT_SMALL, "power", 120, 146, WHITE);
  drawAaCentered(AA_FONT_SMALL, "Takes 20 sec", 120, 178, WHITE);
}

static void handleFirmwareUpload() {
  HTTPUpload &upload = settingsServer.upload();

  if (upload.status == UPLOAD_FILE_START) {
    noteSettingsServerActivity();
    firmwareUploadFailed = false;
    firmwareUploadFinished = false;
    firmwareHeaderChecked = false;
    firmwareUploadSize = 0;
    settingsMessage = "OTA upload";
    drawFirmwareUpdateWarning();
    if (captureActive) {
      cancelCapture("ota");
    }
    NimBLEDevice::getScan()->stop();
    mbedtls_sha256_init(&firmwareSha);
    firmwareHashActive = mbedtls_sha256_starts_ret(&firmwareSha, 0) == 0;
    if (!firmwareHashActive) {
      firmwareUploadFailed = true;
      Serial.println("OTA SHA256 init failed");
    }
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      firmwareUploadFailed = true;
      Serial.printf("OTA begin failed: %s\n", Update.errorString());
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    noteSettingsServerActivity();
    if (!firmwareHeaderChecked) {
      firmwareHeaderChecked = true;
      if (!looksLikeEsp32FirmwareChunk(upload.buf, upload.currentSize)) {
        firmwareUploadFailed = true;
        Serial.println("OTA rejected: file does not look like ESP32 firmware");
      }
    }

    if (!firmwareUploadFailed && firmwareHashActive &&
        mbedtls_sha256_update_ret(&firmwareSha, upload.buf, upload.currentSize) != 0) {
      firmwareUploadFailed = true;
      Serial.println("OTA SHA256 update failed");
    }
    if (!firmwareUploadFailed &&
        Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      firmwareUploadFailed = true;
      Serial.printf("OTA write failed: %s\n", Update.errorString());
    }
    firmwareUploadSize += upload.currentSize;
  } else if (upload.status == UPLOAD_FILE_END) {
    noteSettingsServerActivity();
    if (firmwareHashActive) {
      if (mbedtls_sha256_finish_ret(&firmwareSha, firmwareDigest) != 0) {
        firmwareUploadFailed = true;
        Serial.println("OTA SHA256 finish failed");
      }
      mbedtls_sha256_free(&firmwareSha);
      firmwareHashActive = false;
    }
    if (firmwareUploadSize == 0) {
      firmwareUploadFailed = true;
      Serial.println("OTA rejected: empty upload");
    }

    firmwareUploadFinished = !firmwareUploadFailed && firmwareHeaderChecked;
    if (firmwareUploadFailed || !firmwareUploadFinished) {
      firmwareUploadFailed = true;
      Update.abort();
      settingsMessage = "OTA failed";
      Serial.printf("OTA receive failed: %s\n", Update.errorString());
    } else {
      settingsMessage = "Verify OTA";
      Serial.printf("OTA received: %u bytes\n", upload.totalSize);
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    firmwareUploadFailed = true;
    firmwareUploadFinished = false;
    if (firmwareHashActive) {
      mbedtls_sha256_free(&firmwareSha);
      firmwareHashActive = false;
    }
    Update.abort();
    settingsMessage = "OTA aborted";
  }
}

static void handleFirmwareUpdateDone() {
  noteSettingsServerActivity();
  bool ok = !firmwareUploadFailed &&
            firmwareUploadFinished &&
            Update.end(true) &&
            !Update.hasError();
  if (!ok) {
    Update.abort();
    settingsMessage = "OTA failed";
    Serial.printf("OTA finalise failed: %s\n", Update.errorString());
  } else {
    settingsMessage = "OTA complete";
  }
  if (ok) {
    drawFirmwareUpdateWarning();
  }
  settingsServer.sendHeader("Connection", "close");
  settingsServer.send(200,
                      "text/html",
                      ok ? F("<!doctype html><meta name='viewport' content='width=device-width,initial-scale=1'><body style='font-family:system-ui;background:#07151c;color:#eef;padding:22px;max-width:560px;margin:auto'><h1>Firmware updated</h1><p>ChargeScreen is restarting. Wait for the round display to return, open Settings on the device, start WiFi again, and reconnect your phone to the <strong>ChargeScreen</strong> hotspot.</p><p>Once reconnected, use the button below to reopen the setup page.</p><a href='http://192.168.4.1/' style='display:block;box-sizing:border-box;width:100%;margin-top:22px;padding:14px;background:#2fb36d;color:#04140b;text-align:center;text-decoration:none;font-weight:700'>Reload ChargeScreen setup</a></body>")
                         : F("<!doctype html><meta name='viewport' content='width=device-width,initial-scale=1'><body style='font-family:system-ui;background:#07151c;color:#eef;padding:22px'><h1>Firmware update failed</h1><p>Upload a valid ESP32-C3 firmware.bin built for this project.</p></body>"));
  delay(500);
  if (ok) {
    ESP.restart();
  } else {
    drawCurrentPage(true);
  }
}

static void handleCaptureDownload() {
  noteSettingsServerActivity();
  if (!captureSaved || !LittleFS.exists(capturePath)) {
    settingsServer.send(404, "text/plain", "No saved BLE capture yet");
    return;
  }

  File file = LittleFS.open(capturePath, FILE_READ);
  if (!file) {
    settingsServer.send(500, "text/plain", "Could not open BLE capture");
    return;
  }

  settingsServer.sendHeader("Content-Disposition", "attachment; filename=\"" + captureFilename + "\"");
  settingsServer.streamFile(file, "text/csv");
  file.close();
}

static void handleCaptureStatus() {
  settingsServer.sendHeader("Cache-Control", "no-store");
  settingsServer.send(200, "application/json", captureStatusJson());
}

static void handleCaptureStart() {
  noteSettingsServerActivity();
  if (!captureActive) {
    startCapture();
  }
  settingsServer.sendHeader("Location", "/?status=capture", true);
  settingsServer.send(303, "text/plain", "");
  drawCurrentPage(true);
}

static void handleCaptureStop() {
  noteSettingsServerActivity();
  stopCapture("web");
  settingsServer.sendHeader("Location", "/?status=capture-saved", true);
  settingsServer.send(303, "text/plain", "");
  drawCurrentPage(true);
}

static void handleCaptureCancel() {
  noteSettingsServerActivity();
  cancelCapture("web");
  settingsServer.sendHeader("Location", "/?status=capture-cancelled", true);
  settingsServer.send(303, "text/plain", "");
  drawCurrentPage(true);
}

static void startSettingsServer() {
  if (settingsServerActive) {
    noteSettingsServerActivity();
    settingsMessage = "WiFi already on";
    drawCurrentPage(true);
    return;
  }

  WiFi.mode(WIFI_AP);
  bool started = strlen(SETTINGS_AP_PASSWORD) >= 8
                     ? WiFi.softAP(SETTINGS_AP_SSID, SETTINGS_AP_PASSWORD)
                     : WiFi.softAP(SETTINGS_AP_SSID);

  if (!started) {
    settingsMessage = "WiFi failed";
    drawCurrentPage(true);
    return;
  }

  settingsServer.on("/", HTTP_GET, handleSettingsRoot);
  settingsServer.on("/save", HTTP_POST, handleSettingsSave);
  settingsServer.on("/clear-keys", HTTP_POST, handleSettingsClearKeys);
  settingsServer.on("/firmware", HTTP_POST, handleFirmwareUpdateDone, handleFirmwareUpload);
  settingsServer.on("/download-capture", HTTP_GET, handleCaptureDownload);
  settingsServer.on("/capture-status", HTTP_GET, handleCaptureStatus);
  settingsServer.on("/capture-start", HTTP_POST, handleCaptureStart);
  settingsServer.on("/capture-stop", HTTP_POST, handleCaptureStop);
  settingsServer.on("/capture-cancel", HTTP_POST, handleCaptureCancel);
  settingsServer.onNotFound(handleSettingsRoot);
  captiveDnsServer.start(53, "*", WiFi.softAPIP());
  settingsServer.begin();
  settingsServerActive = true;
  noteSettingsServerActivity();
  settingsMessage = String("Open ") + WiFi.softAPIP().toString();
  drawCurrentPage(true);
}

static void stopSettingsServer() {
  if (!settingsServerActive) {
    return;
  }
  settingsServer.stop();
  captiveDnsServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  settingsServerActive = false;
  settingsMessage = "WiFi off";
  invalidateScreens();
  drawCurrentPage(true);
}

static void updateSettingsServer() {
  if (!settingsServerActive) {
    return;
  }

  captiveDnsServer.processNextRequest();
  settingsServer.handleClient();
  if (captureActive) {
    lastSettingsServerActivityMs = millis();
    return;
  }
  if (millis() - lastSettingsServerActivityMs > SETTINGS_SERVER_IDLE_MS) {
    stopSettingsServer();
  }
}

static void drawSettingsPage(bool force = false) {
  if (!force && settingsPageDrawn) {
    return;
  }

  gfx->fillScreen(COLOR_BLACK_SOFT);
  gfx->fillCircle(120, 120, 120, COLOR_PANEL_BLUE);
  gfx->drawCircle(120, 120, 119, 0x18C3);
  gfx->fillCircle(104, 20, settingsPageIndex == 0 ? 3 : 2, COLOR_DIM_TEXT);
  gfx->fillCircle(120, 20, settingsPageIndex == 1 ? 3 : 2, COLOR_DIM_TEXT);
  gfx->fillCircle(136, 20, settingsPageIndex == 2 ? 3 : 2, COLOR_DIM_TEXT);
  drawAaCentered(AA_FONT_SMALL, "Settings", 120, 32, WHITE);

  auto drawRow = [](int16_t y, const String &label, const String &value, bool active) {
    uint16_t fill = active ? 0x24A7 : 0x01EB;
    gfx->drawRect(SETTINGS_ROW_X, y, SETTINGS_LABEL_W, SETTINGS_ROW_H, 0x6B4D);
    gfx->fillRect(SETTINGS_ROW_X + 1, y + 1, SETTINGS_LABEL_W - 2, SETTINGS_ROW_H - 2, fill);
    drawAaCentered(AA_FONT_SMALL, label, SETTINGS_ROW_X + SETTINGS_LABEL_W / 2, y - 1, WHITE);

    gfx->drawRect(SETTINGS_VALUE_X, y, SETTINGS_VALUE_W, SETTINGS_ROW_H, 0x6B4D);
    gfx->fillRect(SETTINGS_VALUE_X + 1, y + 1, SETTINGS_VALUE_W - 2, SETTINGS_ROW_H - 2, active ? 0x4540 : 0x01EB);
    drawAaCentered(AA_FONT_SMALL, value, SETTINGS_VALUE_X + SETTINGS_VALUE_W / 2, y - 1, WHITE);
  };

  auto drawStepperRow = [](int16_t y, const String &label, const String &value) {
    gfx->drawRect(SETTINGS_ROW_X, y, 92, SETTINGS_ROW_H, 0x6B4D);
    gfx->fillRect(SETTINGS_ROW_X + 1, y + 1, 90, SETTINGS_ROW_H - 2, 0x01EB);
    drawAaCentered(AA_FONT_SMALL, label, SETTINGS_ROW_X + 46, y - 1, WHITE);

    gfx->drawRect(138, y, 24, SETTINGS_ROW_H, 0x6B4D);
    gfx->fillRect(139, y + 1, 22, SETTINGS_ROW_H - 2, 0x01EB);
    drawAaCentered(AA_FONT_SMALL, "-", 150, y - 1, WHITE);

    gfx->drawRect(164, y, 36, SETTINGS_ROW_H, 0x6B4D);
    gfx->fillRect(165, y + 1, 34, SETTINGS_ROW_H - 2, 0x01EB);
    drawAaCentered(AA_FONT_SMALL, value, 182, y - 1, WHITE);

    gfx->drawRect(202, y, 24, SETTINGS_ROW_H, 0x6B4D);
    gfx->fillRect(203, y + 1, 22, SETTINGS_ROW_H - 2, 0x01EB);
    drawAaCentered(AA_FONT_SMALL, "+", 214, y - 1, WHITE);
  };

  if (settingsPageIndex == 0) {
    drawRow(SETTINGS_WIFI_Y, "WiFi", settingsServerActive ? "On" : "-", settingsServerActive);
    drawStepperRow(SETTINGS_SOLAR_WATTS_Y, "Solar W", String(solarArrayWatts));
    drawRow(SETTINGS_DEMO_Y, "Demo mode", demoModeEnabled ? "On" : "-", demoModeEnabled);
    drawRow(SETTINGS_ROTATION_Y, "Rotation", String(screenRotationDegrees), false);
  } else if (settingsPageIndex == 1) {
    drawRow(SETTINGS_TIMEOUT_Y, "Timeout", screenTimeoutText(), false);

    gfx->drawRect(SETTINGS_ROW_X, SETTINGS_BATTERY_Y, 96, SETTINGS_ROW_H, 0x6B4D);
    gfx->fillRect(SETTINGS_ROW_X + 1, SETTINGS_BATTERY_Y + 1, 94, SETTINGS_ROW_H - 2, 0x01EB);
    drawAaCentered(AA_FONT_SMALL, "Battery", SETTINGS_ROW_X + 48, SETTINGS_BATTERY_Y - 1, WHITE);

    gfx->drawRect(140, SETTINGS_BATTERY_Y, 62, SETTINGS_ROW_H, 0x6B4D);
    gfx->fillRect(141, SETTINGS_BATTERY_Y + 1, 60, SETTINGS_ROW_H - 2, 0x01EB);
    drawAaCentered(AA_FONT_SMALL, batteryModeText(), 171, SETTINGS_BATTERY_Y - 1, WHITE);

    drawRow(SETTINGS_DIMMING_Y, "Dimming", backlightLevelText(), false);
  } else {
    drawRow(SETTINGS_LABELS_Y, "Labels", showValueLabels ? "On" : "-", showValueLabels);
    drawRow(SETTINGS_GRID_Y, "Grid", showValueGrid ? "On" : "-", showValueGrid);
  }

  drawBackButton();
  settingsPageDrawn = true;
}

static uint8_t displayRotationForDegrees(int degrees) {
  while (degrees < 0) {
    degrees += 360;
  }
  degrees %= 360;
  return static_cast<uint8_t>(((degrees + 45) / 90) % 4);
}

static void setScreenRotationDegrees(int degrees) {
  while (degrees < 0) {
    degrees += 360;
  }
  if (degrees >= 360) {
    degrees %= 360;
  }
  screenRotationDegrees = degrees;
  secrets.putInt("screen_rotation", screenRotationDegrees);

  uint8_t nextRotation = displayRotationForDegrees(screenRotationDegrees);
  if (nextRotation != currentDisplayRotation) {
    currentDisplayRotation = nextRotation;
    gfx->setRotation(currentDisplayRotation);
    invalidateScreens();
  }

  settingsPageDrawn = false;
  drawCurrentPage(true);
}

static void drawRotationSettingsPage(bool force = false) {
  if (!force && settingsPageDrawn) {
    return;
  }

  gfx->fillScreen(COLOR_BLACK_SOFT);
  gfx->fillCircle(120, 120, 120, COLOR_PANEL_BLUE);
  gfx->drawCircle(120, 120, 119, 0x18C3);

  gfx->drawRect(42, 42, 156, 156, COLOR_DIM_TEXT);
  gfx->drawFastHLine(42, 198, 156, WHITE);
  gfx->drawFastHLine(42, 197, 156, WHITE);
  gfx->drawFastHLine(42, 196, 156, WHITE);

  gfx->drawRect(82, 104, 76, 32, 0x6B4D);
  gfx->fillRect(83, 105, 74, 30, 0x01EB);
  drawAaCentered(AA_FONT_SMALL, "Rotate", 120, 110, WHITE);

  drawBackButton();
  settingsPageDrawn = true;
}

static String savedStatus(bool saved) {
  return saved ? "saved" : "-";
}

static void drawSettingsInfoPage(bool force = false) {
  if (!force && settingsPageDrawn) {
    return;
  }

  gfx->fillScreen(COLOR_BLACK_SOFT);
  gfx->fillCircle(120, 120, 120, COLOR_PANEL_BLUE);
  gfx->drawCircle(120, 120, 119, 0x18C3);
  drawAaCentered(AA_FONT_SMALL, "Info", 120, 30, WHITE);
  drawAaCentered(AA_FONT_SMALL, FIRMWARE_VERSION, 120, 56, WHITE);

  auto drawInfoLine = [](int16_t y, const String &label, const String &value) {
    gfx->setTextSize(1);
    gfx->setTextColor(COLOR_DIM_TEXT);
    gfx->setCursor(42, y);
    gfx->print(label);
    gfx->setTextColor(WHITE);
    gfx->setCursor(122, y);
    gfx->print(value.substring(0, 14));
  };

  drawInfoLine(84, "WiFi", settingsServerActive ? "On" : "Off");
  drawInfoLine(104, "Demo", demoModeEnabled ? "On" : "Off");
  drawInfoLine(124, "Rotation", String(screenRotationDegrees));
  drawInfoLine(144, "Battery", batteryModeText());
  drawInfoLine(164, "Label/Grid", String(showValueLabels ? "On/" : "Off/") + (showValueGrid ? "On" : "Off"));
  drawInfoLine(184, "Keys", storedVictronKey.length() || storedSolarKey.length() ? "saved" : "-");

  drawBackButton();
  settingsPageDrawn = true;
}

static void drawGaugeFace() {
  gfx->fillScreen(COLOR_BLACK_SOFT);
  gfx->fillCircle(120, 120, 120, COLOR_PANEL_BLUE);
  gfx->drawCircle(120, 120, 119, 0x18C3);

  drawSegmentedGaugeArc(0.0f, COLOR_RING_TRACK, COLOR_RING_GREEN);
  gfx->fillCircle(120, 120, GAUGE_CLEAR_RADIUS, COLOR_PANEL_BLUE);
  gaugeFaceDrawn = true;
  gaugeValuesDrawn = false;
}

static void drawBatteryValueGridAt(int16_t centerX) {
  if (!showValueGrid) {
    return;
  }

  uint16_t gridColor = 0x6B4D;
  gfx->drawFastVLine(centerX, 106, 72, gridColor);
  gfx->drawFastHLine(centerX - 67, 144, 134, gridColor);
}

static void drawPageTwoAtOffset(int16_t offsetX) {
  int16_t centerX = 120 + offsetX;
  SolarStats stats = displaySolarStats();
  String statusText = displaySolarStatusText(stats);
  String powerText = stats.valid ? formatWattsOrDash(stats.pvPower) : "-- W";
  float maxPower = max(1.0f, static_cast<float>(solarArrayWatts));
  float ringPower = stats.valid && !isnan(stats.pvPower)
                        ? constrain(stats.pvPower, 0.0f, maxPower)
                        : 0.0f;

  gfx->fillCircle(centerX, 120, 120, COLOR_SOLAR_PANEL);
  gfx->drawCircle(centerX, 120, 119, COLOR_SOLAR_PANEL);
  drawSegmentedGaugeArcAt(centerX,
                          GAUGE_ARC_SWEEP_DEG * ringPower / maxPower,
                          COLOR_SOLAR_TRACK,
                          COLOR_SOLAR_ORANGE,
                          COLOR_SOLAR_PANEL);
  gfx->fillRect(centerX - 24, 199, 48, 25, COLOR_SOLAR_PANEL);
  gfx->fillCircle(centerX, 120, GAUGE_CLEAR_RADIUS, COLOR_SOLAR_PANEL);

  if (stats.valid) {
    if (showValueLabels) {
      drawTinyLabel("PV POWER", centerX, 42, COLOR_SOLAR_DIM_TEXT);
    }
    drawAaCentered(AA_FONT_LARGE, powerText, centerX, 43, COLOR_SOLAR_TEXT);

    drawAaCentered(AA_FONT_SMALL, formatOneDecimalOrDash(stats.batteryVoltage, "V"), centerX - 42, 112, COLOR_SOLAR_TEXT);
    drawAaCentered(AA_FONT_SMALL, formatOneDecimalOrDash(stats.batteryCurrent, "A"), centerX + 44, 112, COLOR_SOLAR_TEXT);
    if (showValueLabels) {
      drawTinyLabel("BATTERY", centerX - 42, 139, COLOR_SOLAR_DIM_TEXT);
      drawTinyLabel("CHARGE", centerX + 44, 139, COLOR_SOLAR_DIM_TEXT);
    }

    drawAaCentered(AA_FONT_SMALL, formatTwoDecimalOrDash(stats.yieldTodayKwh, "kWh"), centerX, 148, COLOR_SOLAR_TEXT);
    if (showValueLabels) {
      drawTinyLabel("TODAY", centerX, 175, COLOR_SOLAR_DIM_TEXT);
    }
    drawAaCentered(AA_FONT_SMALL, statusText, centerX, 184, COLOR_SOLAR_DIM_TEXT);
  } else {
    drawAaCentered(AA_FONT_LARGE, "-- W", centerX, 50, COLOR_SOLAR_TEXT);
    drawAaCentered(AA_FONT_SMALL, statusText, centerX, 106, COLOR_SOLAR_DIM_TEXT);
  }
}

static void drawDashboardAtOffset(int16_t offsetX) {
  int16_t centerX = 120 + offsetX;
  BatteryStats stats = displayStats();
  int socDisplay = stats.valid ? static_cast<int>(round(stats.soc)) : -1;
  String statusText = displayStatusText(stats);

  gfx->fillCircle(centerX, 120, 120, COLOR_PANEL_BLUE);
  gfx->drawCircle(centerX, 120, 119, 0x18C3);
  drawSegmentedGaugeArcAt(centerX, 0.0f, COLOR_RING_TRACK, COLOR_RING_GREEN);
  if (stats.valid) {
    float soc = constrain(stats.soc, 0.0f, 100.0f);
    drawSegmentedGaugeArcAt(centerX, GAUGE_ARC_SWEEP_DEG * soc / 100.0f, COLOR_RING_TRACK, COLOR_RING_GREEN);
  }
  gfx->fillCircle(centerX, 120, GAUGE_CLEAR_RADIUS, COLOR_PANEL_BLUE);

  if (stats.valid) {
    drawBatteryValueGridAt(centerX);
    if (showValueLabels) {
      drawTinyLabel("STATE OF CHARGE", centerX, 42, COLOR_DIM_TEXT);
    }
    drawAaCentered(AA_FONT_LARGE, String(socDisplay) + "%", centerX, 47, WHITE);
    drawAaCentered(AA_FONT_SMALL, formatSignedOneDecimal(stats.voltage, "V"), centerX - 42, 104, WHITE);
    drawAaCentered(AA_FONT_SMALL, formatSignedOneDecimal(stats.current, "A"), centerX + 44, 104, WHITE);
    if (showValueLabels) {
      drawTinyLabel("BATTERY", centerX - 42, 130, COLOR_DIM_TEXT);
      drawTinyLabel("CURRENT", centerX + 44, 130, COLOR_DIM_TEXT);
    }
    drawAaCentered(AA_FONT_SMALL, formatSignedOneDecimal(stats.power, "W"), centerX - 42, 147, WHITE);
    drawAaCentered(AA_FONT_SMALL, auxDisplayText(), centerX + 44, 147, WHITE);
    if (showValueLabels) {
      drawTinyLabel("POWER", centerX - 42, 173, COLOR_DIM_TEXT);
      drawTinyLabel("USAGE", centerX + 44, 173, COLOR_DIM_TEXT);
    }
    drawAaCentered(AA_FONT_SMALL, statusText, centerX, 185, COLOR_DIM_TEXT);
  } else {
    drawAaCentered(AA_FONT_LARGE, "--%", centerX, 50, WHITE);
    drawAaCentered(AA_FONT_SMALL, statusText, centerX, 106, COLOR_DIM_TEXT);
  }
}

static void drawPageTwo(bool force = false) {
  SolarStats stats = displaySolarStats();
  String powerText = stats.valid ? formatWattsOrDash(stats.pvPower) : "";
  String statusText = displaySolarStatusText(stats);
  String batteryText = stats.valid ? formatOneDecimalOrDash(stats.batteryVoltage, "V") : "";
  String currentText = stats.valid ? formatOneDecimalOrDash(stats.batteryCurrent, "A") : "";
  String yieldText = stats.valid ? formatTwoDecimalOrDash(stats.yieldTodayKwh, "kWh") : "";
  String loadText = stats.valid ? statusText : "";
  String errorText = stats.valid ? "" : (!storedSolarKey.length()
                                             ? "add key via WiFi"
                                             : (solarStats.error.length() ? solarStats.error : "no solar BLE"));

  bool changed = force || !pageTwoDrawn || !solarValuesDrawn ||
                 powerText != lastSolarPowerText ||
                 statusText != lastSolarStatusText ||
                 batteryText != lastSolarBatteryText ||
                 currentText != lastSolarCurrentText ||
                 yieldText != lastSolarYieldText ||
                 loadText != lastSolarLoadText ||
                 errorText != lastSolarErrorText;

  if (!changed) {
    return;
  }

  gfx->fillScreen(COLOR_BLACK_SOFT);
  drawPageTwoAtOffset(0);
  if (!stats.valid) {
    drawDeviceLine(135, "Solar", solarSeen);
    if (errorText.length()) {
      gfx->setTextSize(1);
      gfx->setTextColor(0xF800);
      gfx->setCursor(74, 178);
      gfx->print(errorText.substring(0, 20));
    }
  }

  lastSolarPowerText = powerText;
  lastSolarStatusText = statusText;
  lastSolarBatteryText = batteryText;
  lastSolarCurrentText = currentText;
  lastSolarYieldText = yieldText;
  lastSolarLoadText = loadText;
  lastSolarErrorText = errorText;
  drawSettingsButton(COLOR_SOLAR_PANEL, COLOR_SOLAR_TEXT);
  pageTwoDrawn = true;
  solarValuesDrawn = true;
}

static void drawCaptureSecondRing(uint32_t remainingSeconds) {
  uint8_t litSegments = 0;
  if (captureActive && remainingSeconds > 0) {
    litSegments = remainingSeconds % 60;
    if (litSegments == 0) {
      litSegments = 60;
    }
  }

  for (uint8_t i = 0; i < 60; i++) {
    float angle = (-90.0f + i * 6.0f) * 0.0174532925f;
    int16_t x1 = 120 + static_cast<int16_t>(cosf(angle) * 109.0f);
    int16_t y1 = 120 + static_cast<int16_t>(sinf(angle) * 109.0f);
    int16_t x2 = 120 + static_cast<int16_t>(cosf(angle) * 116.0f);
    int16_t y2 = 120 + static_cast<int16_t>(sinf(angle) * 116.0f);
    uint16_t color = i < litSegments ? 0xFFE0 : 0x39E7;
    gfx->drawLine(x1, y1, x2, y2, color);
  }
}

static void drawCapturePage(bool force = false) {
  uint32_t now = millis();
  if (!force && capturePageDrawn && now - lastCapturePageDrawMs < 1000) {
    return;
  }
  lastCapturePageDrawMs = now;

  uint32_t remaining = captureActive ? captureRemainingSeconds() : 0;
  String timerText;
  String statusText;
  if (captureActive) {
    char timerBuffer[8];
    snprintf(timerBuffer, sizeof(timerBuffer), "%lu-%02lu",
             static_cast<unsigned long>(remaining / 60),
             static_cast<unsigned long>(remaining % 60));
    timerText = timerBuffer;
    statusText = "Recording";
  } else if (captureSaved) {
    timerText = "";
    statusText = settingsServerActive ? "Download via WiFi" : "Start WiFi to download";
  } else {
    timerText = "Idle";
    statusText = "Use WiFi page";
  }

  gfx->fillScreen(COLOR_BLACK_SOFT);
  gfx->fillCircle(120, 120, 108, 0x2104);
  drawCaptureSecondRing(remaining);

  drawAaCentered(AA_FONT_SMALL, "BLE Capture", 120, 22, WHITE);
  if (timerText.length()) {
    drawAaCentered(AA_FONT_LARGE, timerText, 120, 40, WHITE);
  }
  drawAaCentered(AA_FONT_SMALL, statusText, 120, captureSaved ? 66 : 88, captureActive || captureSaved ? 0xFFE0 : COLOR_DIM_TEXT);

  gfx->setTextSize(1);
  gfx->setTextColor(COLOR_DIM_TEXT);
  gfx->setCursor(42, 118);
  gfx->print("Packets");
  gfx->setCursor(42, 138);
  gfx->print("Strongest");
  gfx->setCursor(42, 158);
  gfx->print("RSSI");

  gfx->setTextColor(WHITE);
  gfx->setCursor(126, 118);
  gfx->print(String(capturePacketCount).substring(0, 8));
  gfx->setCursor(112, 138);
  gfx->print(captureDeviceLabel().substring(0, 16));
  gfx->setCursor(126, 158);
  gfx->print(captureStrongestRssi > -127 ? String(captureStrongestRssi) + "dBm" : "-");

  if (captureActive) {
    gfx->drawRect(78, 178, 84, 28, 0x6B4D);
    gfx->fillRect(79, 179, 82, 26, 0x7820);
    drawAaCentered(AA_FONT_SMALL, "Stop", 120, 176, WHITE);
  } else if (captureSaved) {
    gfx->drawRect(82, 178, 76, 28, 0x6B4D);
    gfx->fillRect(83, 179, 74, 26, 0x24A7);
    drawAaCentered(AA_FONT_SMALL, "Done", 120, 178, WHITE);
  }

  lastCaptureTimerText = timerText;
  lastCaptureStatusText = statusText;
  lastCapturePacketText = String(capturePacketCount);
  lastCaptureStrongestText = captureDeviceLabel();
  lastCaptureRssiText = captureStrongestRssi > -127 ? String(captureStrongestRssi) + "dBm" : "-";
  lastCaptureButtonText = captureActive ? "Stop" : (captureSaved ? "Done" : "");
  capturePageDrawn = true;
}

static void drawCurrentPage(bool force) {
  if (!screenAwake) {
    return;
  }

  if (captureActive || captureReadyScreenActive) {
    drawCapturePage(force);
    return;
  }

  if (settingsInfoPageActive) {
    drawSettingsInfoPage(force);
    return;
  }

  if (settingsRotationPageActive) {
    drawRotationSettingsPage(force);
    return;
  }

  if (settingsPageActive) {
    drawSettingsPage(force);
    return;
  }

  if (currentPage == 0) {
    if (force) {
      drawGaugeFace();
    }
    drawGaugeValues(force);
  } else {
    drawPageTwo(force);
  }
}

static void switchPage(uint8_t page, int8_t direction = 1) {
  if (settingsPageActive || settingsRotationPageActive || settingsInfoPageActive) {
    return;
  }

  page %= PAGE_COUNT;
  if (page == currentPage) {
    return;
  }

  currentPage = page;
  pageTwoDrawn = false;
  solarValuesDrawn = false;
  gaugeFaceDrawn = false;
  capturePageDrawn = false;
  drawCurrentPage(true);
}

static void togglePageFromSwipe(int8_t direction) {
  uint8_t nextPage = direction >= 0
                         ? (currentPage + 1) % PAGE_COUNT
                         : (currentPage + PAGE_COUNT - 1) % PAGE_COUNT;
  switchPage(nextPage, direction);
}

static void openSettingsPage() {
  settingsPageIndex = 0;
  settingsPageActive = true;
  settingsRotationPageActive = false;
  settingsInfoPageActive = false;
  settingsPageDrawn = false;
  drawCurrentPage(true);
}

static void closeSettingsPage() {
  settingsPageActive = false;
  settingsRotationPageActive = false;
  settingsInfoPageActive = false;
  settingsPageDrawn = false;
  gaugeFaceDrawn = false;
  pageTwoDrawn = false;
  solarValuesDrawn = false;
  capturePageDrawn = false;
  drawCurrentPage(true);
}

static void advanceScreenTimeoutSeconds() {
  uint16_t nextSeconds = 10;
  if (screenTimeoutSeconds == 10) {
    nextSeconds = 30;
  } else if (screenTimeoutSeconds == 30) {
    nextSeconds = 60;
  } else if (screenTimeoutSeconds == 60) {
    nextSeconds = SCREEN_TIMEOUT_NEVER;
  }

  screenTimeoutSeconds = sanitiseScreenTimeoutSeconds(nextSeconds);
  secrets.putUShort("screen_timeout", screenTimeoutSeconds);
  settingsMessage = String("Timeout ") + screenTimeoutText();
  settingsPageDrawn = false;
  noteTouchActivity();
  drawCurrentPage(true);
}

static void adjustSolarArrayWatts(int deltaWatts) {
  uint16_t nextWatts = sanitiseSolarArrayWatts(static_cast<int>(solarArrayWatts) + deltaWatts);
  if (nextWatts == solarArrayWatts) {
    return;
  }

  solarArrayWatts = nextWatts;
  secrets.putUShort("solar_watts", solarArrayWatts);
  pageTwoDrawn = false;
  solarValuesDrawn = false;
  settingsMessage = String("Solar ") + String(solarArrayWatts) + "W";
  settingsPageDrawn = false;
  noteTouchActivity();
  drawCurrentPage(true);
}

static void cycleBacklightLevel() {
  backlightLevel = sanitiseBacklightLevel(backlightLevel + 1);
  secrets.putUChar("backlight", backlightLevel);
  settingsMessage = String("Dimming ") + backlightLevelText();
  settingsPageDrawn = false;
  noteTouchActivity();
  applyBacklightLevel();
  drawCurrentPage(true);
}

static void handleTap(int16_t x, int16_t y) {
  if (captureActive) {
    if (pointInRect(x, y, 78, 178, 84, 28)) {
      stopCapture("touch");
      drawCurrentPage(true);
      return;
    }
    return;
  }

  if (captureReadyScreenActive) {
    if (pointInRect(x, y, 82, 178, 76, 28)) {
      captureReadyScreenActive = false;
      capturePageDrawn = false;
      drawCurrentPage(true);
    }
    return;
  }

  if (settingsInfoPageActive) {
    if (pointInRect(x, y, SETTINGS_BACK_X, SETTINGS_BACK_Y, SETTINGS_BACK_W, SETTINGS_BACK_H)) {
      settingsInfoPageActive = false;
      settingsPageActive = true;
      settingsPageDrawn = false;
      drawCurrentPage(true);
    }
    return;
  }

  if (settingsRotationPageActive) {
    if (pointInRect(x, y, ROTATE_BUTTON_X, ROTATE_BUTTON_Y, ROTATE_BUTTON_W, ROTATE_BUTTON_H)) {
      setScreenRotationDegrees(screenRotationDegrees + 90);
      return;
    }

    if (pointInRect(x, y, SETTINGS_BACK_X, SETTINGS_BACK_Y, SETTINGS_BACK_W, SETTINGS_BACK_H)) {
      settingsRotationPageActive = false;
      settingsPageActive = true;
      settingsPageDrawn = false;
      drawCurrentPage(true);
      return;
    }
    return;
  }

  if (settingsPageActive) {
    if (settingsPageIndex == 1) {
      if (pointInRect(x, y, SETTINGS_ROW_X, SETTINGS_TIMEOUT_Y, SETTINGS_VALUE_X + SETTINGS_VALUE_W - SETTINGS_ROW_X, SETTINGS_ROW_H)) {
        advanceScreenTimeoutSeconds();
        return;
      }

      if (pointInRect(x, y, SETTINGS_ROW_X, SETTINGS_BATTERY_Y, 164, SETTINGS_ROW_H)) {
        ecoWorthyBatteryMode = !ecoWorthyBatteryMode;
        secrets.putBool("battery_eco", ecoWorthyBatteryMode);
        victronStats.valid = false;
        settingsMessage = ecoWorthyBatteryMode ? "Battery EcoW" : "Battery Victron";
        settingsPageDrawn = false;
        drawCurrentPage(true);
        return;
      }

      if (pointInRect(x, y, SETTINGS_ROW_X, SETTINGS_DIMMING_Y, SETTINGS_VALUE_X + SETTINGS_VALUE_W - SETTINGS_ROW_X, SETTINGS_ROW_H)) {
        cycleBacklightLevel();
        return;
      }

      if (pointInRect(x, y, SETTINGS_BACK_X, SETTINGS_BACK_Y, SETTINGS_BACK_W, SETTINGS_BACK_H)) {
        closeSettingsPage();
        return;
      }
      return;
    }

    if (settingsPageIndex == 2) {
      if (pointInRect(x, y, SETTINGS_ROW_X, SETTINGS_LABELS_Y, SETTINGS_VALUE_X + SETTINGS_VALUE_W - SETTINGS_ROW_X, SETTINGS_ROW_H)) {
        showValueLabels = !showValueLabels;
        secrets.putBool("value_labels", showValueLabels);
        settingsMessage = showValueLabels ? "Labels on" : "Labels off";
        invalidateScreens();
        settingsPageActive = true;
        settingsPageIndex = 2;
        settingsPageDrawn = false;
        drawCurrentPage(true);
        return;
      }

      if (pointInRect(x, y, SETTINGS_ROW_X, SETTINGS_GRID_Y, SETTINGS_VALUE_X + SETTINGS_VALUE_W - SETTINGS_ROW_X, SETTINGS_ROW_H)) {
        showValueGrid = !showValueGrid;
        secrets.putBool("value_grid", showValueGrid);
        settingsMessage = showValueGrid ? "Grid on" : "Grid off";
        invalidateScreens();
        settingsPageActive = true;
        settingsPageIndex = 2;
        settingsPageDrawn = false;
        drawCurrentPage(true);
        return;
      }

      if (pointInRect(x, y, SETTINGS_BACK_X, SETTINGS_BACK_Y, SETTINGS_BACK_W, SETTINGS_BACK_H)) {
        closeSettingsPage();
        return;
      }
      return;
    }

    if (pointInRect(x, y, SETTINGS_ROW_X, SETTINGS_WIFI_Y, SETTINGS_VALUE_X + SETTINGS_VALUE_W - SETTINGS_ROW_X, SETTINGS_ROW_H)) {
      if (settingsServerActive) {
        stopSettingsServer();
      } else {
        startSettingsServer();
      }
      return;
    }

    if (pointInRect(x, y, 138, SETTINGS_SOLAR_WATTS_Y, 24, SETTINGS_ROW_H)) {
      adjustSolarArrayWatts(-SOLAR_ARRAY_WATTS_STEP);
      return;
    }

    if (pointInRect(x, y, 202, SETTINGS_SOLAR_WATTS_Y, 24, SETTINGS_ROW_H)) {
      adjustSolarArrayWatts(SOLAR_ARRAY_WATTS_STEP);
      return;
    }

    if (pointInRect(x, y, SETTINGS_ROW_X, SETTINGS_DEMO_Y, SETTINGS_VALUE_X + SETTINGS_VALUE_W - SETTINGS_ROW_X, SETTINGS_ROW_H)) {
      demoModeEnabled = !demoModeEnabled;
      secrets.putBool("demo_mode", demoModeEnabled);
      victronStats.valid = false;
      solarStats.valid = false;
      settingsMessage = demoModeEnabled ? "Demo mode on" : "Demo mode off";
      settingsPageDrawn = false;
      drawCurrentPage(true);
      return;
    }

    if (pointInRect(x, y, SETTINGS_ROW_X, SETTINGS_ROTATION_Y, SETTINGS_VALUE_X + SETTINGS_VALUE_W - SETTINGS_ROW_X, SETTINGS_ROW_H)) {
      settingsRotationPageActive = true;
      settingsPageActive = false;
      settingsInfoPageActive = false;
      settingsPageDrawn = false;
      drawCurrentPage(true);
      return;
    }

    if (pointInRect(x, y, SETTINGS_BACK_X, SETTINGS_BACK_Y, SETTINGS_BACK_W, SETTINGS_BACK_H)) {
      closeSettingsPage();
      return;
    }
    return;
  }

  if (pointInCircle(x, y, SETTINGS_BUTTON_X, SETTINGS_BUTTON_Y, SETTINGS_BUTTON_R + 8)) {
    openSettingsPage();
  }
}

static bool isDemoModeRowPoint(int16_t x, int16_t y) {
  if (settingsPageIndex != 0) {
    return false;
  }
  return pointInRect(x,
                     y,
                     SETTINGS_ROW_X,
                     SETTINGS_DEMO_Y,
                     SETTINGS_VALUE_X + SETTINGS_VALUE_W - SETTINGS_ROW_X,
                     SETTINGS_ROW_H);
}

static void openSettingsInfoPage() {
  settingsInfoPageActive = true;
  settingsPageActive = false;
  settingsRotationPageActive = false;
  settingsPageDrawn = false;
  drawCurrentPage(true);
}

static String auxDisplayText() {
  BatteryStats stats = displayStats();
  float tempC = demoModeEnabled ? NAN : victronTemperatureC();
  if (!isnan(tempC)) {
    return formatSignedOneDecimal(tempC, "C");
  }
  if (isnan(stats.consumedAh)) {
    return "-- Ah";
  }
  return formatSignedOneDecimal(stats.consumedAh, "Ah");
}

static String displayErrorText() {
  BatteryStats stats = displayStats();
  if (stats.valid) {
    return "";
  }

  if (ecoWorthyBatteryMode) {
    return "looking for battery";
  }

  if (!storedVictronKey.length()) {
    return "add key via WiFi";
  }

  if (lastVictronAdvertMs > 0 && millis() - lastVictronAdvertMs < 10000) {
    return victronStats.error.length() ? victronStats.error : "decode failed";
  }

  return "no Victron BLE";
}

static void drawGaugeValues(bool force = false) {
  if (!gaugeFaceDrawn) {
    drawGaugeFace();
  }

  BatteryStats stats = displayStats();
  int socDisplay = stats.valid ? static_cast<int>(round(stats.soc)) : -1;
  String statusText = displayStatusText(stats);
  String voltageText = stats.valid ? formatSignedOneDecimal(stats.voltage, "V") : "";
  String currentText = stats.valid ? formatSignedOneDecimal(stats.current, "A") : "";
  String powerText = stats.valid ? formatSignedOneDecimal(stats.power, "W") : "";
  String auxText = stats.valid ? auxDisplayText() : "";
  String errorText = displayErrorText();

  bool changed = force || !gaugeValuesDrawn ||
                 socDisplay != lastSocDisplay ||
                 statusText != lastStatusText ||
                 voltageText != lastVoltageText ||
                 currentText != lastCurrentText ||
                 powerText != lastPowerText ||
                 auxText != lastAuxText ||
                 errorText != lastErrorText;
  if (!changed) {
    return;
  }

  if (force || socDisplay != lastSocDisplay || !gaugeValuesDrawn) {
    drawSegmentedGaugeArc(0.0f, COLOR_RING_TRACK, COLOR_RING_GREEN);
    if (stats.valid) {
      float soc = constrain(stats.soc, 0.0f, 100.0f);
      drawSegmentedGaugeArc(GAUGE_ARC_SWEEP_DEG * soc / 100.0f, COLOR_RING_TRACK, COLOR_RING_GREEN);
    }
  }

  gfx->fillCircle(120, 120, GAUGE_CLEAR_RADIUS, COLOR_PANEL_BLUE);

  if (stats.valid) {
    drawBatteryValueGridAt(120);
    if (showValueLabels) {
      drawTinyLabel("STATE OF CHARGE", 120, 42, COLOR_DIM_TEXT);
    }
    drawAaCentered(AA_FONT_LARGE, String(socDisplay) + "%", 120, 47, WHITE);
    drawAaCentered(AA_FONT_SMALL, voltageText, 78, 104, WHITE);
    drawAaCentered(AA_FONT_SMALL, currentText, 164, 104, WHITE);
    if (showValueLabels) {
      drawTinyLabel("BATTERY", 78, 130, COLOR_DIM_TEXT);
      drawTinyLabel("CURRENT", 164, 130, COLOR_DIM_TEXT);
    }
    drawAaCentered(AA_FONT_SMALL, powerText, 78, 147, WHITE);
    drawAaCentered(AA_FONT_SMALL, auxText, 164, 147, WHITE);
    if (showValueLabels) {
      drawTinyLabel("POWER", 78, 173, COLOR_DIM_TEXT);
      drawTinyLabel("USAGE", 164, 173, COLOR_DIM_TEXT);
    }
    drawAaCentered(AA_FONT_SMALL, statusText, 120, 185, COLOR_DIM_TEXT);
  } else {
    drawAaCentered(AA_FONT_LARGE, "--%", 120, 50, WHITE);
    drawAaCentered(AA_FONT_SMALL, statusText, 120, 106, COLOR_DIM_TEXT);
    drawDeviceLine(135, ecoWorthyBatteryMode ? "Eco-Worthy" : "Victron", ecoWorthyBatteryMode ? ecoSeen : victronSeen);
    if (errorText.length()) {
      gfx->setTextSize(1);
      gfx->setTextColor(0xF800);
      gfx->setCursor(74, 178);
      gfx->print(errorText.substring(0, 20));
    }
  }

  lastSocDisplay = socDisplay;
  lastStatusText = statusText;
  lastVoltageText = voltageText;
  lastCurrentText = currentText;
  lastPowerText = powerText;
  lastAuxText = auxText;
  lastErrorText = errorText;
  drawSettingsButton(COLOR_PANEL_BLUE, WHITE);
  gaugeValuesDrawn = true;
}

static void updateBleWatchdog() {
  if (captureActive) {
    return;
  }

  uint32_t now = millis();
  if (now - lastBleWatchdogMs < 5000) {
    return;
  }
  lastBleWatchdogMs = now;

  NimBLEScan *scan = NimBLEDevice::getScan();
  if (!scan->isScanning()) {
    Serial.println("BLE scan was stopped; restarting");
    restartBleScan("stopped");
  } else if (lastVictronAdvertMs > 0 &&
             now - lastVictronAdvertMs > VICTRON_QUIET_RESTART_MS &&
             now - lastBleScanStartMs > VICTRON_QUIET_RESTART_MS) {
    restartBleScan("Victron quiet");
  } else if (lastVictronAdvertMs == 0 &&
             lastSolarAdvertMs == 0 &&
             now - lastBleScanStartMs > BLE_NEVER_SEEN_RESTART_MS) {
    restartBleScan("no Victron yet");
  }

  if (victronStats.valid && lastVictronDecodeMs > 0 && now - lastVictronDecodeMs > VICTRON_SIGNAL_LOST_MS) {
    victronStats.valid = false;
    victronStats.error = "signal lost";
    drawCurrentPage(true);
  }

  if (solarStats.valid && lastSolarDecodeMs > 0 && now - lastSolarDecodeMs > VICTRON_SIGNAL_LOST_MS) {
    solarStats.valid = false;
    solarStats.error = "signal lost";
    drawCurrentPage(true);
  }

  if (demoModeEnabled) {
    return;
  }
}

static void initTouch() {
  pinMode(PIN_TOUCH_INT, INPUT_PULLUP);
  pinMode(PIN_TOUCH_RST, OUTPUT);
  digitalWrite(PIN_TOUCH_RST, LOW);
  delay(5);
  digitalWrite(PIN_TOUCH_RST, HIGH);
  delay(50);

  Wire.begin(PIN_TOUCH_SDA, PIN_TOUCH_SCL);
  Wire.setClock(400000);

  uint8_t chipId = 0;
  if (touchReadBytes(0xA7, &chipId, 1)) {
    Serial.printf("Touch controller ready, chip id 0x%02X\n", chipId);
  } else {
    Serial.println("Touch controller not seen yet");
  }
}

static void updateTouchSwipe() {
  static bool wasTouched = false;
  static bool consumeUntilRelease = false;
  static bool wakeCandidateActive = false;
  static uint8_t wakeCandidateCount = 0;
  static int16_t wakeStartX = 0;
  static int16_t wakeStartY = 0;
  static uint32_t wakeStartMs = 0;
  static int16_t startX = 0;
  static int16_t startY = 0;
  static int16_t lastX = 0;
  static int16_t lastY = 0;
  static uint32_t startMs = 0;

  TouchPoint point;
  if (!readTouchPoint(point)) {
    return;
  }

  if (point.touched) {
    if (!screenAwake) {
      if (!wakeCandidateActive) {
        wakeCandidateActive = true;
        wakeCandidateCount = 1;
        wakeStartX = point.x;
        wakeStartY = point.y;
        wakeStartMs = millis();
        return;
      }

      if (abs(point.x - wakeStartX) > 18 || abs(point.y - wakeStartY) > 18) {
        wakeCandidateCount = 1;
        wakeStartX = point.x;
        wakeStartY = point.y;
        wakeStartMs = millis();
        return;
      }

      wakeCandidateCount++;
      if (wakeCandidateCount >= 3 && millis() - wakeStartMs >= 40) {
        wakeCandidateActive = false;
        consumeUntilRelease = true;
        wasTouched = false;
        noteTouchActivity();
      }
      return;
    }

    wakeCandidateActive = false;
    noteTouchActivity();
    if (consumeUntilRelease) {
      return;
    }

    if (!wasTouched) {
      startX = point.x;
      startY = point.y;
      startMs = millis();
    }
    lastX = point.x;
    lastY = point.y;
    wasTouched = true;
    return;
  }

  wakeCandidateActive = false;
  wakeCandidateCount = 0;

  if (consumeUntilRelease) {
    consumeUntilRelease = false;
    wasTouched = false;
    return;
  }

  if (!wasTouched) {
    return;
  }

  wasTouched = false;
  int16_t dx = lastX - startX;
  int16_t dy = lastY - startY;
  int16_t absDx = abs(dx);
  int16_t absDy = abs(dy);
  uint32_t durationMs = millis() - startMs;

  if (settingsPageActive &&
      durationMs >= SETTINGS_INFO_HOLD_MS &&
      absDx < 32 &&
      absDy < 32 &&
      isDemoModeRowPoint((startX + lastX) / 2, (startY + lastY) / 2)) {
    openSettingsInfoPage();
    return;
  }

  if (settingsPageActive &&
      durationMs < 1000 &&
      absDx >= SWIPE_MIN_PIXELS &&
      absDx > static_cast<int16_t>(absDy * SWIPE_HORIZONTAL_BIAS)) {
    if (dx < 0) {
      settingsPageIndex = (settingsPageIndex + 1) % 3;
    } else {
      settingsPageIndex = (settingsPageIndex + 2) % 3;
    }
    settingsPageDrawn = false;
    drawCurrentPage(true);
    return;
  }

  if (durationMs < 800 && absDx < 32 && absDy < 32) {
    handleTap((startX + lastX) / 2, (startY + lastY) / 2);
    return;
  }

  if (durationMs < 1000 &&
      absDx >= SWIPE_MIN_PIXELS &&
      absDx > static_cast<int16_t>(absDy * SWIPE_HORIZONTAL_BIAS)) {
    togglePageFromSwipe(dx < 0 ? -1 : 1);
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  loadStoredSecrets();

  initBacklight();
  noteTouchActivity();

  if (!gfx->begin(80000000)) {
    Serial.println("Display init failed");
  }

  gfx->setRotation(currentDisplayRotation);
  initTouch();
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS init failed");
  }
  drawCurrentPage(true);

  NimBLEDevice::init("round-battery-dashboard");
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);

  NimBLEScan *scan = NimBLEDevice::getScan();
  scan->setScanCallbacks(new ScanCallbacks(), true);
  scan->setDuplicateFilter(0);
  scan->setMaxResults(0);
  scan->setActiveScan(true);
  scan->setInterval(97);
  scan->setWindow(67);
  scan->start(0, false, false);
  lastBleScanStartMs = millis();
  Serial.println("BLE scan started");
}

void loop() {
  static uint32_t lastDraw = 0;
  updateSettingsServer();
  updateBleWatchdog();
  updateTouchSwipe();
  updateScreenTimeout();

  if (captureActive && millis() - captureStartMs >= CAPTURE_DURATION_MS) {
    stopCapture("timer");
  }

  uint32_t drawIntervalMs = 1000;
  if (screenAwake && millis() - lastDraw > drawIntervalMs) {
    lastDraw = millis();
    drawCurrentPage();
  }

  delay(20);
}
