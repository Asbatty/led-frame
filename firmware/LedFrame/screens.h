#pragma once
// ---- The screens: clock & weather, message, countdown, Basil, art, alerts ----
#include <time.h>
#include <limits.h>

enum Screen : uint8_t { SCR_CLOCK, SCR_MESSAGE, SCR_COUNTDOWN, SCR_BASIL, SCR_ART, SCR_WEATHER };
static const uint8_t AUTO_ORDER[] = {SCR_CLOCK, SCR_WEATHER, SCR_MESSAGE, SCR_COUNTDOWN, SCR_BASIL, SCR_ART};
static const int AUTO_N = sizeof(AUTO_ORDER) / sizeof(AUTO_ORDER[0]);

uint8_t curScreen = SCR_CLOCK;
uint32_t screenStart = 0;

bool timeValid() { return time(nullptr) > 1700000000; }


// Whole days from today until the countdown date; LONG_MIN when not set.
long countdownDays() {
  if (!validDate(S.cdDate) || !timeValid()) return LONG_MIN;
  time_t now = time(nullptr);
  struct tm today;
  localtime_r(&now, &today);
  today.tm_hour = 12; today.tm_min = 0; today.tm_sec = 0;
  struct tm target = {};
  target.tm_year = S.cdDate.substring(0, 4).toInt() - 1900;
  target.tm_mon = S.cdDate.substring(5, 7).toInt() - 1;
  target.tm_mday = S.cdDate.substring(8, 10).toInt();
  target.tm_hour = 12;
  target.tm_isdst = -1;
  return lround(difftime(mktime(&target), mktime(&today)) / 86400.0);
}

// ---------- Clock & weather ----------
void drawClock() {
  String t = "--:--";
  struct tm tm;
  if (timeValid() && getLocalTime(&tm, 0)) {
    int h = tm.tm_hour % 12;
    if (h == 0) h = 12;
    char b[8];
    snprintf(b, sizeof b, "%d%c%02d", h, (frameMs % 1000) < 500 ? ':' : ' ', tm.tm_min);
    t = b;
  }
  drawText(max(1, (44 - textWidth(t, 2)) / 2), 3, t, 0xFFC478, 2);

  // Weather: icon top right, temperature under it, blinking drop when rain is coming
  String temp = "--";
  if (L.haveWeather) {
    drawWeatherIcon(L.cond, 53, 7);
    if (!isnan(L.temp)) temp = String((int)lroundf(L.temp));
  }
  temp += "\xC2\xB0";
  int tw = textWidth(temp), tx = 53 - tw / 2;
  drawText(tx, 17, temp, L.haveWeather ? 0xFFFFFF : 0x606060);
  if (L.rainSoon && (frameMs / 500) % 2 == 0) drawRows(tx + tw + 2, 17, ICON_DROP, 5, c565(0x3399FF));

  canvas.drawFastHLine(2, 22, 60, c565(0x00283C));

  // Bottom strip alternates the date and today's high/low
  bool showHiLo = L.haveWeather && !isnan(L.hi) && !isnan(L.lo) && (frameMs / 4000) % 2 == 1;
  if (showHiLo) {
    String hl = "H " + String((int)lroundf(L.hi)) + "\xC2\xB0  L " + String((int)lroundf(L.lo)) + "\xC2\xB0";
    drawTextCentered(25, hl, 0x00BEC8);
  } else if (timeValid() && getLocalTime(&tm, 0)) {
    char d[16];
    strftime(d, sizeof d, "%a %b %d", &tm);
    String ds = upper(d);
    ds.replace(" 0", " ");
    drawTextCentered(25, ds, 0x00BEC8);
  }
}

// ---------- Message board ----------
// Returns true once the message has scrolled fully past.
bool drawMessage(uint32_t elapsed) {
  drawRows(2, 1, ICON_HEART, 5, c565(0xFF3C78));
  drawText(9, 1, "MESSAGE", 0x962850);
  if (S.message.length() == 0) {
    drawTextCentered(13, "NO MESSAGE", 0x606060);
    return true;
  }
  int w = textWidth(S.message, 2);
  int x = PANEL_W - (int)(elapsed * 22 / 1000);
  drawText(x, 10, S.message, COLORS[S.color].rgb, 2);
  if (S.msgTime > 1700000000) {
    time_t mt = S.msgTime;
    struct tm tm;
    localtime_r(&mt, &tm);
    int h = tm.tm_hour % 12;
    if (h == 0) h = 12;
    char b[20];
    snprintf(b, sizeof b, "SENT %d:%02d %s", h, tm.tm_min, tm.tm_hour < 12 ? "AM" : "PM");
    drawTextCentered(25, b, 0x5A5A5A);
  }
  return x < -w;
}

// ---------- Countdown ----------
void drawCountdown() {
  int px0 = (int)((frameMs / 70) % 84) - 10;
  for (int x = px0 - 12; x < px0; x += 3) canvas.drawPixel(x, 2, c565(0x3C3C50));
  drawRows(px0, 1, ICON_PLANE, 3, c565(0xFFFFFF));

  long days = countdownDays();
  if (days == LONG_MIN) {
    drawTextCentered(12, "SET A", 0x808080);
    drawTextCentered(19, "COUNTDOWN", 0x808080);
    return;
  }
  String name = upper(S.cdName.length() ? S.cdName : String("COUNTDOWN"));
  if (name.length() > 15) name = name.substring(0, 15);
  drawTextCentered(7, name, 0xFFFFFF);

  if (days == 0) {
    drawTextCentered(15, "TODAY!", COLOR_RAINBOW, 2);
  } else if (days < 0) {
    drawTextCentered(17, "PASSED", 0x808080);
  } else {
    String d = String(days), unit = days == 1 ? "DAY" : "DAYS";
    int g = textWidth(d, 2) + 3 + textWidth(unit), x0 = (PANEL_W - g) / 2;
    drawText(x0, 14, d, 0x00C8FF, 2);
    drawText(x0 + textWidth(d, 2) + 3, 19, unit, 0xC8C8C8);
    // Bar fills up over the last 60 days
    int fill = 56 - (int)(56 * min(days, 60L) / 60);
    for (int x = 0; x < 56; x++) canvas.drawPixel(4 + x, 28, x < fill ? c565(0x00C88C) : c565(0x1E1E1E));
  }
}

// ---------- Basil's food (only shown when under 10 days) ----------
void drawBasil() {
  uint32_t col = L.basilDays < 5 ? 0xFF5030 : 0xFFAA00;
  drawRows(2, 1, ICON_CAT, 6, c565(col));
  drawText(11, 2, "BASIL", 0xC8C8C8);
  String d = String(L.basilDays), unit = L.basilDays == 1 ? "DAY" : "DAYS";
  int g = textWidth(d, 2) + 3 + textWidth(unit), x0 = (PANEL_W - g) / 2;
  drawText(x0, 11, d, col, 2);
  drawText(x0 + textWidth(d, 2) + 3, 16, unit, 0xC8C8C8);
  drawTextCentered(25, "OF FOOD LEFT", 0x808080);
}

// ---------- Ambient art: slow colour waves with a small clock ----------
int8_t SIN8[256];
uint16_t PALETTE[256];

// Very faint colour channels flicker on HUB75 panels at low brightness, so each
// channel is either fully off or at least ART_MIN.
#define ART_MIN 48

void initArt() {
  for (int i = 0; i < 256; i++) {
    SIN8[i] = (int8_t)lroundf(sinf(i * 2 * PI / 256) * 127);
    uint32_t c = hsvRGB(i * 360 / 256 + 180, 255, 150);
    uint8_t ch[3] = {(uint8_t)(c >> 16), (uint8_t)(c >> 8), (uint8_t)c};
    for (uint8_t& v : ch) v = v < ART_MIN / 2 ? 0 : max(v, (uint8_t)ART_MIN);
    PALETTE[i] = c565(((uint32_t)ch[0] << 16) | ((uint32_t)ch[1] << 8) | ch[2]);
  }
}

bool artFrozen = false;      // debug: /api/tune?freeze=1 holds the art still
uint32_t artFrozenAt = 0;

void drawArt() {
  uint32_t t = (artFrozen ? artFrozenAt : frameMs) / 40;
  for (int y = 0; y < PANEL_H; y++)
    for (int x = 0; x < PANEL_W; x++) {
      int v = SIN8[(x * 8 + t) & 255] + SIN8[(y * 11 + t * 4 / 3) & 255] + SIN8[((x + y) * 5 + t * 2 / 3) & 255];
      canvas.drawPixel(x, y, PALETTE[(v / 3 + t / 3) & 255]);
    }
  struct tm tm;
  if (timeValid() && getLocalTime(&tm, 0)) {
    int h = tm.tm_hour % 12;
    if (h == 0) h = 12;
    char b[8];
    snprintf(b, sizeof b, "%d:%02d", h, tm.tm_min);
    int w = textWidth(b);
    canvas.fillRect(PANEL_W - w - 4, 23, w + 4, 9, 0);
    drawText(PANEL_W - w - 2, 25, b, 0xDCDCDC);
  }
}

// ---------- Alerts (from Home Assistant notify or the phone page) ----------
void showAlert(const String& text, const String& icon, uint32_t color, uint32_t seconds) {
  L.alertText = upper(text);
  L.alertIcon = icon;
  L.alertColor = color;
  L.alertMs = constrain(seconds, 3UL, 300UL) * 1000;
  L.alertStart = millis();
  L.alert = true;
}

// Payload: plain text, or JSON {"text":..,"icon":"rain|bell|cat|heart","color":"#3399ff","duration":20}
void alertFromPayload(const String& p) {
  String text = p, icon = "bell";
  uint32_t color = 0xFFAA00, secs = 20;
  if (p.startsWith("{")) {
    JsonDocument d;
    if (deserializeJson(d, p)) return;
    text = d["text"].is<const char*>() ? d["text"].as<String>() : String(d["message"] | "");
    icon = d["icon"] | "bell";
    String c = d["color"] | "";
    if (c.startsWith("#") && c.length() == 7) color = strtoul(c.c_str() + 1, nullptr, 16);
    secs = d["duration"] | 20;
  }
  text.trim();
  if (text.length()) showAlert(text, icon, color, secs);
}

void drawAlert() {
  uint32_t el = frameMs - L.alertStart;
  int w = textWidth(L.alertText, 2);
  int x = PANEL_W - (int)((el * 22 / 1000) % (uint32_t)(w + 50));
  drawText(x, 11, L.alertText, L.alertColor, 2);
  canvas.fillRect(0, 0, 16, PANEL_H, 0);
  if (L.alertIcon == "rain") drawWeatherIcon("rainy", 8, 14);
  else if (L.alertIcon == "cat") drawRows(4, 13, ICON_CAT, 6, c565(L.alertColor));
  else if (L.alertIcon == "heart") drawRows(5, 13, ICON_HEART, 5, c565(0xFF3C78));
  else drawRows(4, 12, ICON_BELL, 7, c565(L.alertColor));
  if ((el / 400) % 2 == 0) canvas.drawRect(0, 0, PANEL_W, PANEL_H, c565(dimRGB(L.alertColor, 180)));
}

// ---------- Choosing what to show ----------
bool screenAvailable(uint8_t s) {
  switch (s) {
    case SCR_MESSAGE: return S.message.length() > 0;
    case SCR_COUNTDOWN: return countdownDays() != LONG_MIN;
    case SCR_BASIL: return L.basilDays >= 0 && L.basilDays < 10;
    case SCR_WEATHER: return L.haveWeather || wxDemo;
    default: return true;
  }
}

uint32_t screenDuration(uint8_t s) {
  switch (s) {
    case SCR_CLOCK: return 20000;
    case SCR_ART: return 15000;
    case SCR_WEATHER: return 15000;
    case SCR_MESSAGE: return 45000;  // or until one full scroll
    default: return 8000;
  }
}

void showScreen(uint8_t s) { curScreen = s; screenStart = frameMs; }

void renderFrame() {
  canvas.fillScreen(0);
  if (L.alert) {
    if (frameMs - L.alertStart < L.alertMs) { drawAlert(); return; }
    L.alert = false;
  }
  if (wxDemo) { drawWeather(); return; }  // demo holds the weather screen

  if (S.mode != MODE_AUTO) {
    uint8_t want = S.mode == MODE_CLOCK ? SCR_CLOCK : S.mode == MODE_MESSAGE ? SCR_MESSAGE
                 : S.mode == MODE_COUNTDOWN ? SCR_COUNTDOWN : S.mode == MODE_WEATHER ? SCR_WEATHER : SCR_ART;
    if (want != curScreen) showScreen(want);
  }

  uint32_t el = frameMs - screenStart;
  bool done = false;
  switch (curScreen) {
    case SCR_CLOCK: drawClock(); break;
    case SCR_MESSAGE:
      done = drawMessage(el);
      if (done && S.mode == MODE_MESSAGE) { screenStart = frameMs; done = false; }  // loop forever
      break;
    case SCR_COUNTDOWN: drawCountdown(); break;
    case SCR_BASIL: drawBasil(); break;
    case SCR_WEATHER:
      if (L.haveWeather || wxDemo) drawWeather();
      else drawTextCentered(13, "NO WEATHER YET", 0x606060);
      break;
    default: drawArt(); break;
  }

  if (S.mode == MODE_AUTO && (done || el > screenDuration(curScreen) || !screenAvailable(curScreen))) {
    int idx = 0;
    for (int i = 0; i < AUTO_N; i++) if (AUTO_ORDER[i] == curScreen) idx = i;
    for (int k = 1; k <= AUTO_N; k++) {
      uint8_t s = AUTO_ORDER[(idx + k) % AUTO_N];
      if (screenAvailable(s)) { showScreen(s); break; }
    }
  }
}

// Simple full-screen status text (boot, Wi-Fi setup, updates)
void showStatus(const String& a, const String& b = "", const String& c = "", uint32_t color = 0x00C8FF) {
  canvas.fillScreen(0);
  drawTextCentered(4, a, color);
  drawTextCentered(13, b, 0xFFFFFF);
  drawTextCentered(22, c, 0xFFAA00);
  pushFrame();
}
