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
  d["mqtt"] = L.mqttUp;
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
