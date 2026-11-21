// LED wall frame: 64x32 RGB matrix on an ESP32, controlled from Home Assistant
// (MQTT discovery) and from a phone page at http://matrix.local.
//
// Board: ESP32 Dev Module, Partition scheme: Minimal SPIFFS (room for wireless updates).
// Libraries: ESP32 HUB75 LED MATRIX PANEL DMA Display, Adafruit GFX, PubSubClient,
//            ArduinoJson 7, WiFiManager (tzapu).

#include <WiFi.h>
#include <WiFiManager.h>
#include "config.h"
#include "secrets.h"
#include "state.h"
#include "gfx.h"
#include "font.h"
String upper(String s) { s.toUpperCase(); return s; }
#include "scenes.h"
#include "screens.h"
#include "web.h"
#include "mqtt.h"

uint32_t lastFrame = 0, dirtyAt = 0;
bool blanked = false;

void applyBrightness() {
  display->setBrightness8(S.on ? map(S.brightness, 0, 255, 0, BRIGHTNESS_CAP) : 0);
}

void settingsChanged() {
  dirtyAt = millis();  // saved to flash a moment later, so quick changes don't wear it
  applyBrightness();
  publishState();
}

void applyPatch(JsonObjectConst p) {
  bool changed = false;
  if (p["on"].is<bool>()) { S.on = p["on"]; changed = true; }
  if (p["brightness"].is<int>()) { S.brightness = constrain(p["brightness"].as<int>(), 1, 255); changed = true; }
  if (p["mode"].is<const char*>()) {
    int m = modeIndexByName(p["mode"].as<String>());
    if (m >= 0) { S.mode = m; showScreen(curScreen); changed = true; }
  }
  if (p["color"].is<const char*>()) {
    int c = colorIndexByName(p["color"].as<String>());
    if (c >= 0) { S.color = c; changed = true; }
  }
  if (p["message"].is<const char*>()) {
    String m = p["message"].as<String>();
    m.trim();
    if (m.length() > 100) m = m.substring(0, 100);
    S.message = m;
    S.msgTime = time(nullptr);
    if (m.length() && (S.mode == MODE_AUTO || S.mode == MODE_MESSAGE)) showScreen(SCR_MESSAGE);  // show it right away
    changed = true;
  }
  if (p["cd_name"].is<const char*>()) {
    String n = p["cd_name"].as<String>();
    n.trim();
    S.cdName = n.substring(0, 15);
    changed = true;
  }
  if (p["cd_date"].is<const char*>()) {
    String d = p["cd_date"].as<String>();
    d.trim();
    if (d.length() == 0 || validDate(d)) { S.cdDate = d; changed = true; }
  }

  // ---- faces ----
  if (p["show"].is<const char*>()) {  // pin one face, like picking a watch face
    int s = faceByKey(p["show"].as<String>());
    if (s >= 0) { S.mode = FACE_MODE[s]; showScreen(s); changed = true; }
  }
  if (p["rotate"] == true) { S.mode = MODE_AUTO; showScreen(curScreen); changed = true; }
  if (p["faces"].is<JsonObjectConst>()) {  // {"clock":{"enabled":true,"dur":20}, ...}
    for (JsonPairConst kv : p["faces"].as<JsonObjectConst>()) {
      int s = faceByKey(kv.key().c_str());
      if (s < 0) continue;
      JsonObjectConst f = kv.value();
      if (f["enabled"].is<bool>()) F.cfg[s].enabled = f["enabled"].as<bool>();
      if (f["dur"].is<int>()) F.cfg[s].dur = constrain(f["dur"].as<int>(), 3, 600);
      changed = true;
    }
  }
  if (p["order"].is<JsonArrayConst>()) {  // ["clock","weather",...]; missing faces keep their relative order
    uint8_t next[SCR_COUNT], n = 0;
    bool used[SCR_COUNT] = {};
    for (JsonVariantConst v : p["order"].as<JsonArrayConst>()) {
      int s = faceByKey(v.as<String>());
      if (s >= 0 && !used[s] && n < SCR_COUNT) { next[n++] = s; used[s] = true; }
    }
    for (int i = 0; i < SCR_COUNT; i++) if (!used[F.order[i]]) next[n++] = F.order[i];
    memcpy(F.order, next, SCR_COUNT);
    changed = true;
  }
  if (p["opts"].is<JsonObjectConst>()) {
    JsonObjectConst o = p["opts"];
    auto pick = [](JsonVariantConst v, const char* const* names, int n, uint8_t& out) {
      if (!v.is<const char*>()) return false;
      for (int i = 0; i < n; i++) if (v.as<String>().equalsIgnoreCase(names[i])) { out = i; return true; }
      return false;
    };
    const char* clockColorNames[5];
    for (int i = 0; i < 5; i++) clockColorNames[i] = CLOCK_COLORS[i].name;
    if (o["clock24"].is<bool>()) F.clock24 = o["clock24"].as<bool>();
    if (o["wxTemp"].is<bool>()) F.wxTemp = o["wxTemp"].as<bool>();
    if (o["artClock"].is<bool>()) F.artClock = o["artClock"].as<bool>();
    pick(o["clockColor"], clockColorNames, 5, F.clockColor);
    pick(o["clockStrip"], CLOCK_STRIPS, 3, F.clockStrip);
    pick(o["msgSpeed"], MSG_SPEEDS, 3, F.msgSpeed);
    pick(o["msgHours"], MSG_HOURS, 4, F.msgHours);
    pick(o["artPalette"], ART_PALETTES, 4, F.artPalette);
    changed = true;
  }
  if (changed) settingsChanged();
}

void buildStateJson(JsonDocument& d) {
  d["on"] = S.on;
  d["brightness"] = S.brightness;
  d["mode"] = MODE_NAMES[S.mode];
  d["message"] = S.message;
  d["color"] = COLORS[S.color].name;
  d["cd_name"] = S.cdName;
  d["cd_date"] = S.cdDate;
  if (L.haveWeather) {
    d["weather"]["cond"] = L.cond;
    d["weather"]["temp"] = L.temp;
    d["weather"]["rain_soon"] = L.rainSoon;
  }
  d["basil_days"] = L.basilDays;
  d["showing"] = FACE_KEYS[curScreen];
  JsonArray faces = d["faces"].to<JsonArray>();
  for (int i = 0; i < SCR_COUNT; i++) {
    uint8_t s = F.order[i];
    JsonObject f = faces.add<JsonObject>();
    f["key"] = FACE_KEYS[s];
    f["name"] = FACE_NAMES[s];
    f["enabled"] = (bool)F.cfg[s].enabled;
    f["dur"] = F.cfg[s].dur;
    f["available"] = screenAvailable(s);
  }
  JsonObject o = d["opts"].to<JsonObject>();
  o["clock24"] = (bool)F.clock24;
  o["clockColor"] = CLOCK_COLORS[F.clockColor].name;
  o["clockStrip"] = CLOCK_STRIPS[F.clockStrip];
  o["wxTemp"] = (bool)F.wxTemp;
  o["msgSpeed"] = MSG_SPEEDS[F.msgSpeed];
  o["msgHours"] = MSG_HOURS[F.msgHours];
  d["msg_live"] = messageLive();
  d["msg_time"] = S.msgTime;
  o["artPalette"] = ART_PALETTES[F.artPalette];
  o["artClock"] = (bool)F.artClock;
  d["mqtt"] = L.mqttUp;
  d["uptime"] = millis() / 1000;
  static const char* const RESETS[] = {"unknown", "power-on", "external", "software", "crash", "interrupt watchdog",
                                       "task watchdog", "other watchdog", "deep sleep", "brownout", "sdio"};
  int rr = (int)esp_reset_reason();
  d["reset"] = rr >= 0 && rr <= 10 ? RESETS[rr] : "unknown";
  d["heap"] = ESP.getFreeHeap();
  d["heap_min"] = ESP.getMinFreeHeap();
  d["ip"] = WiFi.localIP().toString();
  d["rssi"] = WiFi.RSSI();
  d["fw"] = FW_VERSION;
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.printf("\nLED Frame %s\n", FW_VERSION);
  loadSettings();
  initArt();

  HUB75_I2S_CFG::i2s_pins pins = {PIN_R1, PIN_G1, PIN_B1, PIN_R2, PIN_G2, PIN_B2,
                                  PIN_A, PIN_B, PIN_C, PIN_D, PIN_E, PIN_LAT, PIN_OE, PIN_CLK};
  HUB75_I2S_CFG cfg(PANEL_W, PANEL_H, 1, pins);
  // Panel tuning, changeable over Wi-Fi with /api/tune?depth=6&refresh=90 (restarts)
  prefs.begin("panel", true);
  uint8_t depth = prefs.getUChar("depth", PANEL_COLOR_DEPTH);
  uint16_t refresh = prefs.getUShort("refresh", PANEL_REFRESH);
  bool clkphase = prefs.getBool("clkphase", PANEL_CLKPHASE);
  panelFlip = prefs.getBool("flip", PANEL_FLIP_180);
  prefs.end();
  cfg.setPixelColorDepthBits(depth);
  cfg.min_refresh_rate = refresh;
  cfg.clkphase = clkphase;
  Serial.printf("Panel: %u-bit colour, min refresh %u Hz, clkphase %d\n", depth, refresh, clkphase);
  display = new MatrixPanel_I2S_DMA(cfg);
  if (!display->begin()) {
    Serial.println("Matrix init failed");
    while (true) delay(1000);
  }
  display->setBrightness8(map(60, 0, 255, 0, BRIGHTNESS_CAP));
  showStatus("LED FRAME", "CONNECTING", "TO WI-FI");

  // Wi-Fi: uses saved network, or opens the setup hotspot (phone joins it and picks Wi-Fi)
  WiFi.setHostname(HOSTNAME);
  WiFiManager wm;
  wm.setConfigPortalTimeout(300);
  wm.setConnectTimeout(20);
  wm.setAPCallback([](WiFiManager*) { showStatus("WI-FI SETUP", "JOIN NETWORK", SETUP_AP); });
  if (!wm.autoConnect(SETUP_AP)) {
    showStatus("NO WI-FI", "RESTARTING", "", 0xFF2020);
    delay(3000);
    ESP.restart();
  }
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  Serial.println("Wi-Fi: " + WiFi.localIP().toString());

  configTzTime(TZ_POSIX, "pool.ntp.org", "time.google.com");
  setupWeb();
  mqttSetup();

  showStatus("WI-FI OK", WiFi.localIP().toString(), "MATRIX.LOCAL", 0x30FF60);
  delay(2500);
  applyBrightness();
  showScreen(S.mode == MODE_MESSAGE ? SCR_MESSAGE : SCR_CLOCK);
}

void loop() {
  server.handleClient();
  ArduinoOTA.handle();
  mqttLoop();

  uint32_t now = millis();
  if (now - lastFrame >= FRAME_MS) {
    lastFrame = now;
    frameMs = now;
    if (S.on) {
      renderFrame();
      pushFrame();
      blanked = false;
    } else if (!blanked) {
      canvas.fillScreen(0);
      pushFrame();
      blanked = true;
    }
  }

  if (dirtyAt && now - dirtyAt > 2000) {
    dirtyAt = 0;
    saveSettings();
  }
}
