#include <Arduino.h>
#include <LittleFS.h>
#include <NimBLEDevice.h>
#include <WebServer.h>
#include <WiFi.h>
#include <ctype.h>

static constexpr const char *AP_SSID = "BLE-Capture";
static constexpr const char *AP_PASSWORD = "";
static constexpr uint32_t CAPTURE_DURATION_MS = 5UL * 60UL * 1000UL;
static constexpr int STATUS_LED_PIN = -1;
static constexpr bool STATUS_LED_ACTIVE_LOW = false;
static constexpr size_t CSV_FLUSH_EVERY_ROWS = 20;

WebServer server(80);
File captureFile;

static bool captureActive = false;
static bool captureSaved = false;
static uint32_t captureStartMs = 0;
static uint32_t captureStopMs = 0;
static uint32_t packetCount = 0;
static uint32_t droppedCount = 0;
static size_t rowsSinceFlush = 0;
static String capturePath = "/capture.csv";
static String finalFilename = "ble_capture.csv";
static String strongestName;
static String strongestAddress;
static int strongestRssi = -127;

static String htmlEscape(const String &value) {
  String out;
  out.reserve(value.length());
  for (size_t i = 0; i < value.length(); i++) {
    char c = value[i];
    switch (c) {
      case '&':
        out += F("&amp;");
        break;
      case '<':
        out += F("&lt;");
        break;
      case '>':
        out += F("&gt;");
        break;
      case '"':
        out += F("&quot;");
        break;
      default:
        out += c;
        break;
    }
  }
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

static String bytesToHex(const std::string &data) {
  return bytesToHex(reinterpret_cast<const uint8_t *>(data.data()), data.size());
}

static String vectorToHex(const std::vector<uint8_t> &data) {
  return data.empty() ? "" : bytesToHex(data.data(), data.size());
}

static String safeNamePart(String value) {
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
  out.trim();
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

static void writeLed(bool on) {
  if (STATUS_LED_PIN < 0) {
    return;
  }
  digitalWrite(STATUS_LED_PIN, STATUS_LED_ACTIVE_LOW ? !on : on);
}

static void updateLed() {
  if (STATUS_LED_PIN < 0) {
    return;
  }

  if (captureActive) {
    writeLed((millis() / 800) % 2 == 0);
  } else if (captureSaved) {
    writeLed((millis() / 100) % 2 == 0);
  } else {
    writeLed(false);
  }
}

static void closeCaptureFile() {
  if (captureFile) {
    captureFile.flush();
    captureFile.close();
  }
}

static void updateFinalFilename() {
  String base = strongestName.length() ? strongestName : strongestAddress;
  base = safeNamePart(base);
  finalFilename = base + "_ble_capture.csv";
  capturePath = "/" + finalFilename;
}

static void stopCapture(const char *reason) {
  if (!captureActive) {
    return;
  }

  captureActive = false;
  captureSaved = true;
  captureStopMs = millis();
  closeCaptureFile();
  updateFinalFilename();

  if (capturePath != "/capture.csv") {
    LittleFS.remove(capturePath);
    LittleFS.rename("/capture.csv", capturePath);
  }

  Serial.printf("Capture stopped: %s, packets=%lu, file=%s\n",
                reason,
                static_cast<unsigned long>(packetCount),
                capturePath.c_str());
}

static void startCapture() {
  closeCaptureFile();
  LittleFS.remove("/capture.csv");

  capturePath = "/capture.csv";
  finalFilename = "capture.csv";
  packetCount = 0;
  droppedCount = 0;
  rowsSinceFlush = 0;
  strongestName = "";
  strongestAddress = "";
  strongestRssi = -127;
  captureStartMs = millis();
  captureStopMs = 0;
  captureSaved = false;
  captureActive = true;

  captureFile = LittleFS.open("/capture.csv", FILE_WRITE);
  if (!captureFile) {
    captureActive = false;
    Serial.println("Failed to open capture file");
    return;
  }

  captureFile.println(F("millis,elapsed_s,address,address_type,name,rssi,adv_type,connectable,scannable,tx_power,manufacturer_hex,service_data_hex,service_uuids,raw_payload_hex"));
  captureFile.flush();
  Serial.println("Capture started");
}

class CaptureCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice *device) override {
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

    if (rssi > strongestRssi) {
      strongestRssi = rssi;
      strongestAddress = address;
      strongestName = name;
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

    size_t written = captureFile.println(line);
    if (written == 0) {
      droppedCount++;
      return;
    }

    packetCount++;
    rowsSinceFlush++;
    if (rowsSinceFlush >= CSV_FLUSH_EVERY_ROWS) {
      captureFile.flush();
      rowsSinceFlush = 0;
    }
  }
};

static uint32_t remainingSeconds() {
  if (!captureActive) {
    return 0;
  }
  uint32_t elapsed = millis() - captureStartMs;
  if (elapsed >= CAPTURE_DURATION_MS) {
    return 0;
  }
  return (CAPTURE_DURATION_MS - elapsed + 999) / 1000;
}

static String statusJson() {
  String json = "{";
  json += "\"active\":";
  json += captureActive ? "true" : "false";
  json += ",\"saved\":";
  json += captureSaved ? "true" : "false";
  json += ",\"remaining_s\":";
  json += String(remainingSeconds());
  json += ",\"packets\":";
  json += String(packetCount);
  json += ",\"dropped\":";
  json += String(droppedCount);
  json += ",\"strongest_name\":\"";
  json += jsonEscape(strongestName);
  json += "\",\"strongest_address\":\"";
  json += jsonEscape(strongestAddress);
  json += "\",\"strongest_rssi\":";
  json += String(strongestRssi);
  json += ",\"filename\":\"";
  json += jsonEscape(finalFilename);
  json += "\"}";
  return json;
}

static void handleRoot() {
  String page;
  page.reserve(5200);
  page += F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
  page += F("<title>BLE Capture</title><style>");
  page += F("body{font-family:system-ui,-apple-system,Segoe UI,sans-serif;margin:0;background:#101418;color:#eef;padding:20px}");
  page += F("main{max-width:560px;margin:0 auto}.panel{border:1px solid #34404c;background:#18202a;padding:18px;margin:14px 0}");
  page += F("h1{font-size:24px;margin:0 0 8px}.big{font-size:44px;font-weight:750;margin:8px 0}");
  page += F(".row{display:flex;justify-content:space-between;border-top:1px solid #2b3540;padding:12px 0;gap:20px}");
  page += F(".row span:last-child{text-align:right;word-break:break-all}.muted{color:#aeb8c4}");
  page += F("button,a.button{display:block;box-sizing:border-box;width:100%;margin-top:12px;padding:14px;border:0;background:#30b86f;color:#04140b;text-align:center;text-decoration:none;font-weight:750;font-size:16px}");
  page += F("button.secondary{background:#26313d;color:#eef;border:1px solid #526273}button.danger{background:#a94842;color:#fff}");
  page += F("button:disabled,a.disabled{opacity:.45;pointer-events:none}</style></head><body><main>");
  page += F("<h1>BLE Capture</h1><p class='muted'>Connect here after powering the device. The network name stays fixed.</p>");
  page += F("<section class='panel'><div class='muted' id='state'>Loading</div><div class='big' id='timer'>--:--</div>");
  page += F("<div class='row'><span>Packets</span><span id='packets'>0</span></div>");
  page += F("<div class='row'><span>Strongest</span><span id='strongest'>-</span></div>");
  page += F("<div class='row'><span>RSSI</span><span id='rssi'>-</span></div>");
  page += F("<div class='row'><span>File</span><span id='file'>-</span></div></section>");
  page += F("<form action='/stop' method='post'><button class='danger' id='stop'>Stop and save now</button></form>");
  page += F("<a class='button' id='download' href='/download'>Download CSV</a>");
  page += F("<form action='/start' method='post'><button class='secondary'>Start new capture</button></form>");
  page += F("<script>");
  page += F("function mmss(s){let m=Math.floor(s/60),r=s%60;return String(m).padStart(2,'0')+':'+String(r).padStart(2,'0')}");
  page += F("async function tick(){let r=await fetch('/status');let j=await r.json();");
  page += F("state.textContent=j.active?'Recording':(j.saved?'Ready to download':'Idle');");
  page += F("timer.textContent=mmss(j.remaining_s);packets.textContent=j.packets;");
  page += F("strongest.textContent=(j.strongest_name||'')+(j.strongest_address?' '+j.strongest_address:'')||'-';");
  page += F("rssi.textContent=j.strongest_rssi>-127?j.strongest_rssi+' dBm':'-';file.textContent=j.filename;");
  page += F("stop.disabled=!j.active;download.className=j.saved?'button':'button disabled';}");
  page += F("setInterval(tick,1000);tick();</script></main></body></html>");
  server.send(200, "text/html", page);
}

static void handleStatus() {
  server.send(200, "application/json", statusJson());
}

static void handleStop() {
  stopCapture("web");
  server.sendHeader("Location", "/", true);
  server.send(303, "text/plain", "");
}

static void handleStart() {
  startCapture();
  server.sendHeader("Location", "/", true);
  server.send(303, "text/plain", "");
}

static void handleDownload() {
  if (!captureSaved || !LittleFS.exists(capturePath)) {
    server.send(404, "text/plain", "No saved capture yet");
    return;
  }

  File file = LittleFS.open(capturePath, FILE_READ);
  if (!file) {
    server.send(500, "text/plain", "Could not open capture");
    return;
  }

  server.sendHeader("Content-Disposition", "attachment; filename=\"" + finalFilename + "\"");
  server.streamFile(file, "text/csv");
  file.close();
}

static void startWebServer() {
  WiFi.mode(WIFI_AP);
  bool ok = strlen(AP_PASSWORD) >= 8
                ? WiFi.softAP(AP_SSID, AP_PASSWORD)
                : WiFi.softAP(AP_SSID);
  Serial.printf("WiFi AP %s, IP %s\n", ok ? "started" : "failed", WiFi.softAPIP().toString().c_str());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/stop", HTTP_POST, handleStop);
  server.on("/start", HTTP_POST, handleStart);
  server.on("/download", HTTP_GET, handleDownload);
  server.begin();
}

static void startBleScan() {
  NimBLEDevice::init("ble-capture");
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);

  NimBLEScan *scan = NimBLEDevice::getScan();
  scan->setScanCallbacks(new CaptureCallbacks(), true);
  scan->setDuplicateFilter(false);
  scan->setActiveScan(true);
  scan->setMaxResults(0);
  scan->setInterval(80);
  scan->setWindow(60);
  scan->start(0, false, false);
}

void setup() {
  Serial.begin(115200);
  delay(300);

  if (STATUS_LED_PIN >= 0) {
    pinMode(STATUS_LED_PIN, OUTPUT);
    writeLed(false);
  }

  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS failed");
  }

  startWebServer();
  startCapture();
  startBleScan();
}

void loop() {
  server.handleClient();
  updateLed();

  if (captureActive && millis() - captureStartMs >= CAPTURE_DURATION_MS) {
    stopCapture("timer");
  }

  NimBLEScan *scan = NimBLEDevice::getScan();
  if (!scan->isScanning()) {
    scan->start(0, true, true);
  }

  delay(5);
}
