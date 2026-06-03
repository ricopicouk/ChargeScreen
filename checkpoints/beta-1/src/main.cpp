#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <NimBLEDevice.h>
#include <mbedtls/aes.h>
#include <math.h>

#include "aa_font.h"
#include "config.h"

static constexpr int PIN_LCD_SCLK = 6;
static constexpr int PIN_LCD_MOSI = 7;
static constexpr int PIN_LCD_DC = 2;
static constexpr int PIN_LCD_CS = 10;
static constexpr int PIN_LCD_BL = 3;

static constexpr uint16_t VICTRON_COMPANY_ID = 0x02E1;
static constexpr uint8_t VICTRON_PRODUCT_ADVERTISEMENT = 0x10;
static constexpr uint8_t VICTRON_BATTERY_MONITOR_RECORD = 0x02;
static constexpr uint8_t DISPLAY_ROTATION = 2;
static constexpr uint16_t COLOR_DARK_BLUE = 0x014A;
static constexpr uint16_t COLOR_PANEL_BLUE = 0x0129;
static constexpr uint16_t COLOR_RING_TRACK = 0x1268;
static constexpr uint16_t COLOR_RING_GREEN = 0x55E8;
static constexpr uint16_t COLOR_DIM_TEXT = 0xBDF7;
static constexpr uint16_t COLOR_BLACK_SOFT = 0x0000;
static constexpr uint32_t VICTRON_QUIET_RESTART_MS = 6000;
static constexpr uint32_t VICTRON_SIGNAL_LOST_MS = 25000;
static constexpr uint32_t BLE_NEVER_SEEN_RESTART_MS = 15000;
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

BleSeen victronSeen;
BleSeen ecoSeen;
BatteryStats victronStats;
static bool gaugeFaceDrawn = false;
static bool gaugeValuesDrawn = false;
static int lastSocDisplay = -1;
static String lastStatusText;
static String lastVoltageText;
static String lastCurrentText;
static String lastPowerText;
static String lastAuxText;
static String lastErrorText;
static uint32_t lastVictronAdvertMs = 0;
static uint32_t lastVictronDecodeMs = 0;
static uint32_t lastBleWatchdogMs = 0;
static uint32_t lastBleScanStartMs = 0;

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
  String wanted = VICTRON_BLE_ADDRESS;
  if (wanted.length() == 0) {
    return true;
  }
  return normaliseAddress(address) == normaliseAddress(wanted);
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

  uint8_t key[16] = {};
  if (!hexToBytes(VICTRON_INSTANT_READOUT_KEY_HEX, key, sizeof(key))) {
    victronStats.valid = false;
    victronStats.error = "bad key";
    return false;
  }

  if (payload[7] != key[0]) {
    victronStats.valid = false;
    victronStats.error = "key mismatch";
    return false;
  }

  uint16_t nonce = static_cast<uint16_t>(payload[5]) |
                   (static_cast<uint16_t>(payload[6]) << 8);
  const uint8_t *encrypted = payload + 8;
  size_t encryptedLen = payloadLen - 8;
  uint8_t plain[24] = {};
  encryptedLen = min(encryptedLen, sizeof(plain));

  if (!aesCtrDecrypt(key, nonce, encrypted, encryptedLen, plain)) {
    victronStats.valid = false;
    victronStats.error = "decrypt fail";
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

static void drawGaugeArc(float startDeg, float sweepDeg, uint16_t color) {
  if (sweepDeg <= 0.0f) {
    return;
  }

  float endDeg = startDeg + sweepDeg;
  while (startDeg >= 360.0f) {
    startDeg -= 360.0f;
    endDeg -= 360.0f;
  }

  if (endDeg <= 360.0f) {
    gfx->fillArc(GAUGE_CENTER_X, GAUGE_CENTER_Y, GAUGE_OUTER_RADIUS, GAUGE_INNER_RADIUS, startDeg, endDeg, color);
  } else {
    gfx->fillArc(GAUGE_CENTER_X, GAUGE_CENTER_Y, GAUGE_OUTER_RADIUS, GAUGE_INNER_RADIUS, startDeg, 360.0f, color);
    gfx->fillArc(GAUGE_CENTER_X, GAUGE_CENTER_Y, GAUGE_OUTER_RADIUS, GAUGE_INNER_RADIUS, 0.0f, endDeg - 360.0f, color);
  }
}

static void drawSegmentedGaugeArc(float fillSweepDeg, uint16_t trackColor, uint16_t fillColor) {
  float segmentSweep = GAUGE_ARC_SWEEP_DEG / GAUGE_SEGMENTS;
  float fillEndDeg = GAUGE_ARC_START_DEG + constrain(fillSweepDeg, 0.0f, GAUGE_ARC_SWEEP_DEG);

  for (uint8_t i = 0; i < GAUGE_SEGMENTS; i++) {
    float segmentStart = GAUGE_ARC_START_DEG + i * segmentSweep + GAUGE_SEGMENT_GAP_DEG * 0.5f;
    float segmentEnd = GAUGE_ARC_START_DEG + (i + 1) * segmentSweep - GAUGE_SEGMENT_GAP_DEG * 0.5f;
    float visibleSweep = max(0.0f, segmentEnd - segmentStart);

    drawGaugeArc(segmentStart, visibleSweep, trackColor);

    float filledEnd = min(segmentEnd, fillEndDeg);
    if (filledEnd > segmentStart) {
      drawGaugeArc(segmentStart, filledEnd - segmentStart, 0x0A05);
      if (filledEnd - segmentStart > 1.4f) {
        drawGaugeArc(segmentStart + 0.7f, filledEnd - segmentStart - 1.4f, fillColor);
      }
    }
  }
}

static const char *victronStatusText() {
  if (!victronStats.valid) {
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

static BatteryStats displayStats() {
  if (!DEMO_MODE) {
    return victronStats;
  }

  BatteryStats stats;
  stats.valid = true;
  stats.voltage = 14.68f;
  stats.current = 30.0f;
  stats.power = 123.0f;
  stats.soc = 100.0f;
  stats.consumedAh = 69.0f;
  stats.error = "";
  return stats;
}

static const char *displayStatusText(const BatteryStats &stats) {
  if (DEMO_MODE) {
    return "Demo";
  }

  if (!stats.valid) {
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

static void restartBleScan(const char *reason) {
  NimBLEScan *scan = NimBLEDevice::getScan();
  Serial.printf("Restarting BLE scan: %s\n", reason);
  scan->start(0, true, true);
  lastBleScanStartMs = millis();
}

class ScanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice *device) override {
    String name = device->haveName() ? device->getName().c_str() : "";
    String address = device->getAddress().toString().c_str();
    int rssi = device->getRSSI();

    if (device->haveManufacturerData()) {
      std::string manufacturerData = device->getManufacturerData();
      if (isVictronInstantReadout(manufacturerData) &&
          matchesConfiguredVictronAddress(address)) {
        victronSeen.name = name.length() ? name : "Victron";
        victronSeen.address = address;
        victronSeen.rssi = rssi;
        victronSeen.seenMs = millis();
        lastVictronAdvertMs = victronSeen.seenMs;
        bool decoded = decodeVictronBatteryMonitor(manufacturerData);

        Serial.printf("Victron %s RSSI %d %s data %s\n",
                      address.c_str(),
                      rssi,
                      decoded ? "decoded" : victronStats.error.c_str(),
                      bytesToHex(manufacturerData).c_str());
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

static void drawGaugeFace() {
  gfx->fillScreen(COLOR_BLACK_SOFT);
  gfx->fillCircle(120, 120, 120, COLOR_PANEL_BLUE);
  gfx->drawCircle(120, 120, 119, 0x18C3);

  drawSegmentedGaugeArc(0.0f, COLOR_RING_TRACK, COLOR_RING_GREEN);
  gfx->fillCircle(120, 120, GAUGE_CLEAR_RADIUS, COLOR_PANEL_BLUE);
  gaugeFaceDrawn = true;
  gaugeValuesDrawn = false;
}

static String auxDisplayText() {
  BatteryStats stats = displayStats();
  float tempC = DEMO_MODE ? NAN : victronTemperatureC();
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
    drawAaCentered(AA_FONT_LARGE, String(socDisplay) + "%", 120, 43, WHITE);
    drawAaCentered(AA_FONT_SMALL, statusText, 120, 95, COLOR_DIM_TEXT);
    drawAaCentered(AA_FONT_SMALL, voltageText, 78, 124, WHITE);
    drawAaCentered(AA_FONT_SMALL, currentText, 164, 124, WHITE);
    drawAaCentered(AA_FONT_SMALL, powerText, 78, 153, WHITE);
    drawAaCentered(AA_FONT_SMALL, auxText, 164, 153, WHITE);
  } else {
    drawAaCentered(AA_FONT_LARGE, "--%", 120, 50, WHITE);
    drawAaCentered(AA_FONT_SMALL, statusText, 120, 106, COLOR_DIM_TEXT);
    drawDeviceLine(135, "Victron", victronSeen);
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
  gaugeValuesDrawn = true;
}

static void updateBleWatchdog() {
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
             now - lastBleScanStartMs > BLE_NEVER_SEEN_RESTART_MS) {
    restartBleScan("no Victron yet");
  }

  if (victronStats.valid && lastVictronDecodeMs > 0 && now - lastVictronDecodeMs > VICTRON_SIGNAL_LOST_MS) {
    victronStats.valid = false;
    victronStats.error = "signal lost";
    drawGaugeValues(true);
  }

  if (DEMO_MODE) {
    return;
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(PIN_LCD_BL, OUTPUT);
  digitalWrite(PIN_LCD_BL, HIGH);

  if (!gfx->begin(80000000)) {
    Serial.println("Display init failed");
  }

  gfx->setRotation(DISPLAY_ROTATION);
  drawGaugeFace();
  drawGaugeValues();

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
  updateBleWatchdog();

  if (millis() - lastDraw > 1000) {
    lastDraw = millis();
    drawGaugeValues();
  }

  delay(20);
}
