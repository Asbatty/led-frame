#pragma once
// ---- Saved settings (flash) and live data pushed in from Home Assistant ----
#include <Preferences.h>
#include <ArduinoJson.h>

// New modes go at the end so saved settings keep their meaning.
enum Mode : uint8_t { MODE_AUTO = 0, MODE_CLOCK, MODE_MESSAGE, MODE_COUNTDOWN, MODE_ART, MODE_WEATHER, MODE_BASIL, MODE_COUNT };
static const char* const MODE_NAMES[MODE_COUNT] = {"Auto", "Clock", "Message", "Countdown", "Art", "Weather", "Basil"};

// ---- Faces (screens), like watch faces: rotation, order, per-face options ----
enum Screen : uint8_t { SCR_CLOCK, SCR_MESSAGE, SCR_COUNTDOWN, SCR_BASIL, SCR_ART, SCR_WEATHER, SCR_COUNT };
static const char* const FACE_KEYS[SCR_COUNT] = {"clock", "message", "countdown", "basil", "art", "weather"};
static const char* const FACE_NAMES[SCR_COUNT] = {"Clock", "Message", "Countdown", "Basil", "Art", "Weather"};
static const uint8_t FACE_MODE[SCR_COUNT] = {MODE_CLOCK, MODE_MESSAGE, MODE_COUNTDOWN, MODE_BASIL, MODE_ART, MODE_WEATHER};

// Option choices (index stored in Faces)
struct NamedRGB { const char* name; uint32_t rgb; };
static const NamedRGB CLOCK_COLORS[] = {{"Amber", 0xFFC478}, {"White", 0xE6E6E6}, {"Cyan", 0x00C8FF}, {"Green", 0x30FF60}, {"Pink", 0xFF3C8C}};
static const char* const CLOCK_STRIPS[] = {"Date", "High / low", "Alternate"};
static const char* const MSG_SPEEDS[] = {"Slow", "Normal", "Fast"};
static const uint8_t MSG_SPEED_PX[] = {12, 22, 34};
static const char* const MSG_HOURS[] = {"1 hour", "2 hours", "4 hours", "Until changed"};
static const uint8_t MSG_HOURS_H[] = {1, 2, 4, 0};  // 0 = never expires
static const char* const ART_PALETTES[] = {"Ocean", "Sunset", "Forest", "Rainbow"};

struct FaceCfg { uint8_t enabled; uint16_t dur; };  // in rotation, seconds on screen
struct Faces {
  uint8_t version = 3;
  FaceCfg cfg[SCR_COUNT] = {{1, 20}, {1, 45}, {1, 8}, {1, 8}, {1, 15}, {1, 15}};
  uint8_t order[SCR_COUNT] = {SCR_CLOCK, SCR_WEATHER, SCR_MESSAGE, SCR_COUNTDOWN, SCR_BASIL, SCR_ART};
  uint8_t clock24 = 0, clockColor = 0, clockStrip = 2;
  uint8_t wxTemp = 1;
  uint8_t msgSpeed = 1;
  uint8_t msgHours = 1;  // index into MSG_HOURS (2 hours)
  uint8_t artPalette = 0, artClock = 1;
};
Faces F;

int faceByKey(const String& k) {
  for (int i = 0; i < SCR_COUNT; i++) if (k.equalsIgnoreCase(FACE_KEYS[i])) return i;
  return -1;
}

int screenForMode(uint8_t m) {
  for (int i = 0; i < SCR_COUNT; i++) if (FACE_MODE[i] == m) return i;
  return -1;
}

static const uint32_t COLOR_RAINBOW = 0x1000000;
struct NamedColor { const char* name; uint32_t rgb; };
static const NamedColor COLORS[] = {
  {"Rainbow", COLOR_RAINBOW}, {"White", 0xFFFFFF}, {"Pink", 0xFF3C8C}, {"Cyan", 0x00C8FF},
  {"Amber", 0xFFAA00},        {"Green", 0x30FF60}, {"Red", 0xFF2020},
};
static const int COLOR_COUNT = sizeof(COLORS) / sizeof(COLORS[0]);

struct Settings {
  bool on = true;
  uint8_t brightness = 60;   // Home Assistant scale 1-255 (capped on the panel)
  uint8_t mode = MODE_AUTO;
  String message = "HELLO FROM THE FRAME";
  uint8_t color = 0;         // index into COLORS
  String cdName;             // countdown title, up to 15 chars
  String cdDate;             // countdown date, YYYY-MM-DD
  uint32_t msgTime = 0;      // when the message was set (epoch seconds)
};

struct LiveData {
  bool haveWeather = false;
  String cond;               // Home Assistant weather state, e.g. "rainy"
  float temp = NAN, hi = NAN, lo = NAN;
  bool rainSoon = false;     // rain expected in the next few hours
  int rainIn = -1;           // minutes until rain, -1 = unknown
  int basilDays = -1;        // days of cat food left, -1 = unknown

  bool alert = false;        // temporary alert overlay
  String alertText, alertIcon;
  uint32_t alertColor = 0xFFAA00;
  uint32_t alertStart = 0, alertMs = 0;

  bool mqttUp = false;
};

Settings S;
LiveData L;
Preferences prefs;

void loadSettings() {
  prefs.begin("frame", true);
  S.on = prefs.getBool("on", true);
  S.brightness = prefs.getUChar("bri", 60);
  S.mode = prefs.getUChar("mode", MODE_AUTO);
  S.message = prefs.getString("msg", S.message);
  S.color = prefs.getUChar("color", 0);
  S.cdName = prefs.getString("cdName", "");
  S.cdDate = prefs.getString("cdDate", "");
  S.msgTime = prefs.getULong("msgTime", 0);
  Faces saved;
  if (prefs.getBytesLength("faces") == sizeof(Faces)) {
    prefs.getBytes("faces", &saved, sizeof(Faces));
    if (saved.version == Faces().version) F = saved;
  }
  prefs.end();
  if (S.mode >= MODE_COUNT) S.mode = MODE_AUTO;
  if (S.color >= COLOR_COUNT) S.color = 0;
  if (S.brightness == 0) S.brightness = 60;
}

void saveSettings() {
  prefs.begin("frame", false);
  prefs.putBool("on", S.on);
  prefs.putUChar("bri", S.brightness);
  prefs.putUChar("mode", S.mode);
  prefs.putString("msg", S.message);
  prefs.putUChar("color", S.color);
  prefs.putString("cdName", S.cdName);
  prefs.putString("cdDate", S.cdDate);
  prefs.putULong("msgTime", S.msgTime);
  prefs.putBytes("faces", &F, sizeof(Faces));
  prefs.end();
}

int modeIndexByName(const String& n) {
  for (int i = 0; i < MODE_COUNT; i++) if (n.equalsIgnoreCase(MODE_NAMES[i])) return i;
  return -1;
}

int colorIndexByName(const String& n) {
  for (int i = 0; i < COLOR_COUNT; i++) if (n.equalsIgnoreCase(COLORS[i].name)) return i;
  return -1;
}

bool validDate(const String& d) {
  if (d.length() != 10 || d[4] != '-' || d[7] != '-') return false;
  for (int i : {0, 1, 2, 3, 5, 6, 8, 9}) if (!isDigit(d[i])) return false;
  int m = d.substring(5, 7).toInt(), day = d.substring(8, 10).toInt();
  return m >= 1 && m <= 12 && day >= 1 && day <= 31;
}

// Defined in LedFrame.ino
void applyPatch(JsonObjectConst p);
void settingsChanged();
void buildStateJson(JsonDocument& d);
