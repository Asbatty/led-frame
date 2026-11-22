#pragma once
// ---- The faces: clock, weather, message, countdown, Basil, art; alerts; rotation ----
#include <time.h>
#include <limits.h>

uint8_t curScreen = SCR_CLOCK;
uint32_t screenStart = 0;
bool previewing = false;  // drawing a gallery thumbnail: fill in sample data where real data is missing

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

String timeText(const struct tm& tm, bool colon) {
  char b[8];
  int h = tm.tm_hour;
  if (!F.clock24) { h %= 12; if (h == 0) h = 12; }
  snprintf(b, sizeof b, F.clock24 ? "%02d%c%02d" : "%d%c%02d", h, colon ? ':' : ' ', tm.tm_min);
  return b;
}

// ---------- Clock ----------
void drawClock() {
  String t = "--:--";
  struct tm tm;
  bool haveTime = timeValid() && getLocalTime(&tm, 0);
  if (haveTime) t = timeText(tm, (frameMs % 1000) < 500 || previewing);
  drawText(max(1, (44 - textWidth(t, 2)) / 2), 3, t, CLOCK_COLORS[F.clockColor].rgb, 2);

  // Weather: icon top right, temperature under it, blinking drop when rain is coming
  bool wx = L.haveWeather || previewing;
  String cond = L.haveWeather ? L.cond : String("partlycloudy");
  float temp = L.haveWeather ? L.temp : 64, hi = L.haveWeather ? L.hi : 70, lo = L.haveWeather ? L.lo : 51;
  String ts = "--";
  if (wx) {
    drawWeatherIcon(cond, 53, 7);
    if (!isnan(temp)) ts = String((int)lroundf(temp));
  }
  ts += "\xC2\xB0";
  int tw = textWidth(ts), tx = 53 - tw / 2;
  drawText(tx, 17, ts, wx ? 0xFFFFFF : 0x606060);
  if (L.rainSoon && (frameMs / 500) % 2 == 0) drawRows(tx + tw + 2, 17, ICON_DROP, 5, c565(0x3399FF));

  canvas.drawFastHLine(2, 22, 60, c565(0x00283C));

  // Bottom line: date, today's high/low, or alternating between them
  bool canHiLo = wx && !isnan(hi) && !isnan(lo);
  bool showHiLo = canHiLo && (F.clockStrip == 1 || (F.clockStrip == 2 && (frameMs / 4000) % 2 == 1));
  if (showHiLo) {
    String hl = "H " + String((int)lroundf(hi)) + "\xC2\xB0  L " + String((int)lroundf(lo)) + "\xC2\xB0";
    drawTextCentered(25, hl, 0x00BEC8);
  } else if (haveTime) {
    char d[16];
    strftime(d, sizeof d, "%a %b %d", &tm);
    String ds = upper(d);
    ds.replace(" 0", " ");
    drawTextCentered(25, ds, 0x00BEC8);
  }
}

// ---------- Message board ----------
// A message is live until it's older than the "Show message for" option.
bool messageLive() {
  if (S.message.length() == 0) return false;
  uint8_t h = MSG_HOURS_H[F.msgHours];
  if (h == 0 || !timeValid() || S.msgTime < 1700000000) return true;
  return (uint32_t)time(nullptr) < S.msgTime + h * 3600UL;
}

// Returns true once the message has scrolled fully past.
bool drawMessage(uint32_t elapsed) {
  drawRows(2, 1, ICON_HEART, 5, c565(0xFF3C78));
  drawText(9, 1, "MESSAGE", 0x962850);
  String msg = messageLive() ? S.message : String(previewing ? "WELCOME HOME" : "");
  if (msg.length() == 0) {
    drawTextCentered(13, "NO MESSAGE", 0x606060);
    return true;
  }
  int w = textWidth(msg, 2);
  int x = previewing ? 2 : PANEL_W - (int)(elapsed * MSG_SPEED_PX[F.msgSpeed] / 1000);
  drawText(x, 10, msg, COLORS[S.color].rgb, 2);
  if (S.msgTime > 1700000000) {
    time_t mt = S.msgTime;
    struct tm tm;
    localtime_r(&mt, &tm);
    char b[20];
    if (F.clock24) snprintf(b, sizeof b, "SENT %02d:%02d", tm.tm_hour, tm.tm_min);
    else snprintf(b, sizeof b, "SENT %d:%02d %s", tm.tm_hour % 12 ? tm.tm_hour % 12 : 12, tm.tm_min, tm.tm_hour < 12 ? "AM" : "PM");
    drawTextCentered(25, b, 0x5A5A5A);
  }
  return x < -w;
}

// ---------- Countdown ----------
void drawCountdown() {
  int px0 = previewing ? 40 : (int)((frameMs / 70) % 84) - 10;
  for (int x = px0 - 12; x < px0; x += 3) canvas.drawPixel(x, 2, c565(0x3C3C50));
  drawRows(px0, 1, ICON_PLANE, 3, c565(0xFFFFFF));

  long days = countdownDays();
  String name = S.cdName;
  if (days == LONG_MIN && previewing) { days = 12; name = "BEACH TRIP"; }
  if (days == LONG_MIN) {
    drawTextCentered(12, "SET A", 0x808080);
    drawTextCentered(19, "COUNTDOWN", 0x808080);
    return;
  }
  name = upper(name.length() ? name : String("COUNTDOWN"));
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
    int fill = 56 - (int)(56 * min(days, 60L) / 60);  // bar fills up over the last 60 days
    for (int x = 0; x < 56; x++) canvas.drawPixel(4 + x, 28, x < fill ? c565(0x00C88C) : c565(0x1E1E1E));
  }
}

// ---------- Basil's portrait: grey-and-white cat, cute-sprite style, from a photo ----------
// Big wide head, big eyes with a highlight, small upright body, tail curling right.
// G grey, D dark-grey edge, W white, w soft shadow on white, P pink (ears, nose),
// E amber iris, H eye highlight, K pupil (off = black)
static const char* const BASIL_ART[] = {
  "..DD..............DD..",
  ".DGGD............DGGD.",
  ".DGPGD..........DGPGD.",
  ".DGPPGDDDDDDDDDDGPPGD.",
  "DGGPPGGGGGGGGGGGGPPGGD",
  "DGGGGGGGGGWWGGGGGGGGGD",
  "DGGEEEEGGGWWGGGEEEEGGD",
  "DGGEHKEGGWWWWGGEHKEGGD",
  "DGGEKKEGGWWWWGGEKKEGGD",
  "DGGGEEGGWWWWWWGGEEGGGD",
  "DGGGGGGWWWPPWWWGGGGGGD",
  ".DGGGGWWWWwwWWWWGGGGD.",
  "..DGGGGWWWWWWWWGGGGD..",
  "...DDGGGwwwwwwGGGDD...",
  ".....DGWWWWWWWWGD.....",
  "....DGGWWWWWWWWGGD....",
  "....DGGWWWWWWWWGGD....",
  "...DGGGWWWWWWWWGGGD...",
  "...DGGGWWwWWwWWGGGD...",
  "..DGGGGWWwWWwWWGGGGDD.",
  "..DGGGGWWwWWwWWGGGGGGD",
  "..DDGGGWWWDDWWWGGGGGGD",
  "....DDDWWW..WWWDDDDDD.",
};
static const int BASIL_ROWS = sizeof(BASIL_ART) / sizeof(BASIL_ART[0]);

void drawBasilPortrait(int x, int y) {
  for (int j = 0; j < BASIL_ROWS; j++)
    for (int i = 0; BASIL_ART[j][i]; i++) {
      uint32_t c;
      switch (BASIL_ART[j][i]) {
        case 'G': c = 0x6E7280; break;
        case 'D': c = 0x444752; break;
        case 'W': c = 0xE6E6E6; break;
        case 'w': c = 0xA8A8B2; break;
        case 'P': c = 0xFF96AA; break;
        case 'E': c = 0xE0A020; break;
        case 'H': c = 0xFFFFFF; break;
        default: continue;  // '.' and pupils stay off
      }
      canvas.drawPixel(x + i, y + j, c565(c));
    }
  // She blinks every few seconds: eyes become closed lines
  if (!previewing && (frameMs % 4200) < 160) {
    canvas.fillRect(x + 3, y + 6, 4, 4, c565(0x6E7280));
    canvas.fillRect(x + 15, y + 6, 4, 4, c565(0x6E7280));
    canvas.drawFastHLine(x + 3, y + 8, 4, c565(0x30323A));
    canvas.drawFastHLine(x + 15, y + 8, 4, c565(0x30323A));
  }
}

// ---------- Basil's food (rotation shows it only under 10 days) ----------
void drawBasil() {
  int days = L.basilDays >= 0 ? L.basilDays : (previewing ? 7 : -1);
  drawBasilPortrait(0, 5);
  const int cx = 44;  // text column centre, right of the portrait
  drawText(cx - textWidth("BASIL") / 2, 3, "BASIL", 0xFFB4C0);
  if (days < 0) {
    drawText(cx - textWidth("NO DATA") / 2, 15, "NO DATA", 0x606060);
    return;
  }
  uint32_t col = days < 5 ? 0xFF5030 : 0xFFAA00;
  String d = String(days), unit = days == 1 ? "DAY" : "DAYS";
  int g = textWidth(d, 2) + 3 + textWidth(unit), x0 = cx - g / 2;
  drawText(x0, 11, d, col, 2);
  drawText(x0 + textWidth(d, 2) + 3, 16, unit, 0xC8C8C8);
  drawText(cx - textWidth("FOOD LEFT") / 2, 25, "FOOD LEFT", 0x808080);
}

// ---------- Ambient art: slow colour waves with a small clock ----------
int8_t SIN8[256];
uint16_t PALETTE[256];
uint8_t paletteBuilt = 255;

// Very faint colour channels flicker on HUB75 panels at low brightness, so each
// channel is either fully off or at least ART_MIN.
#define ART_MIN 48

void initArt() {
  for (int i = 0; i < 256; i++) SIN8[i] = (int8_t)lroundf(sinf(i * 2 * PI / 256) * 127);
}

void buildPalette(uint8_t p) {
  for (int i = 0; i < 256; i++) {
    int tri = i < 128 ? i : 255 - i;  // 0..127..0 so non-rainbow palettes loop smoothly
    int h;
    switch (p) {
      case 1: h = 300 + tri * 110 / 127; break;  // Sunset: magenta -> orange
      case 2: h = 70 + tri * 100 / 127; break;   // Forest: lime -> teal
      case 3: h = i * 360 / 256; break;          // Rainbow
      default: h = 170 + tri * 100 / 127; break; // Ocean: cyan -> violet
    }
    uint32_t c = hsvRGB(h, 255, 150);
    uint8_t ch[3] = {(uint8_t)(c >> 16), (uint8_t)(c >> 8), (uint8_t)c};
    for (uint8_t& v : ch) v = v < ART_MIN / 2 ? 0 : max(v, (uint8_t)ART_MIN);
    PALETTE[i] = c565(((uint32_t)ch[0] << 16) | ((uint32_t)ch[1] << 8) | ch[2]);
  }
  paletteBuilt = p;
}

bool artFrozen = false;      // debug: /api/tune?freeze=1 holds the art still
uint32_t artFrozenAt = 0;

void drawArt() {
  if (paletteBuilt != F.artPalette) buildPalette(F.artPalette);
  uint32_t t = (artFrozen ? artFrozenAt : frameMs) / 40;
  for (int y = 0; y < PANEL_H; y++)
    for (int x = 0; x < PANEL_W; x++) {
      int v = SIN8[(x * 8 + t) & 255] + SIN8[(y * 11 + t * 4 / 3) & 255] + SIN8[((x + y) * 5 + t * 2 / 3) & 255];
      canvas.drawPixel(x, y, PALETTE[(v / 3 + t / 3) & 255]);
    }
  struct tm tm;
  if (F.artClock && timeValid() && getLocalTime(&tm, 0)) {
    String b = timeText(tm, true);
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

// ---------- Drawing one face ----------
// Returns true when a one-pass face (message) has finished.
bool drawFace(uint8_t s, uint32_t el) {
  switch (s) {
    case SCR_CLOCK: drawClock(); return false;
    case SCR_MESSAGE: return drawMessage(el);
    case SCR_COUNTDOWN: drawCountdown(); return false;
    case SCR_BASIL: drawBasil(); return false;
    case SCR_WEATHER:
      if (L.haveWeather || wxDemo) drawWeather();
      else if (previewing) drawWeatherScene("sunny", F.wxTemp ? "72\xC2\xB0" : "");
      else drawTextCentered(13, "NO WEATHER YET", 0x606060);
      return false;
    default: drawArt(); return false;
  }
}

// ---------- Rotation ----------
// A face has something to show (rotation skips faces that don't).
bool screenAvailable(uint8_t s) {
  switch (s) {
    case SCR_MESSAGE: return messageLive();
    case SCR_COUNTDOWN: return countdownDays() != LONG_MIN;
    case SCR_BASIL: return L.basilDays >= 0 && L.basilDays < 10;
    case SCR_WEATHER: return L.haveWeather || wxDemo;
    default: return true;
  }
}

bool inRotation(uint8_t s) { return F.cfg[s].enabled && screenAvailable(s); }

void showScreen(uint8_t s) { curScreen = s; screenStart = frameMs; }

void nextInRotation() {
  int idx = 0;
  for (int i = 0; i < SCR_COUNT; i++) if (F.order[i] == curScreen) idx = i;
  for (int k = 1; k <= SCR_COUNT; k++) {
    uint8_t s = F.order[(idx + k) % SCR_COUNT];
    if (inRotation(s)) { showScreen(s); return; }
  }
  if (!inRotation(curScreen)) showScreen(SCR_CLOCK);  // nothing enabled: fall back to the clock
}

void renderFrame() {
  canvas.fillScreen(0);
  if (L.alert) {
    if (frameMs - L.alertStart < L.alertMs) { drawAlert(); return; }
    L.alert = false;
  }
  if (wxDemo) { drawWeather(); return; }  // demo holds the weather screen

  if (S.mode != MODE_AUTO) {
    int want = screenForMode(S.mode);
    if (want >= 0 && want != curScreen) showScreen(want);
  }

  uint32_t el = frameMs - screenStart;
  bool done = drawFace(curScreen, el);
  if (done && S.mode != MODE_AUTO) { screenStart = frameMs; done = false; }  // pinned message loops

  if (S.mode == MODE_AUTO && (done || el > F.cfg[curScreen].dur * 1000UL || !inRotation(curScreen)))
    nextInRotation();
}

// Simple full-screen status text (boot, Wi-Fi setup, updates)
void showStatus(const String& a, const String& b = "", const String& c = "", uint32_t color = 0x00C8FF) {
  canvas.fillScreen(0);
  drawTextCentered(4, a, color);
  drawTextCentered(13, b, 0xFFFFFF);
  drawTextCentered(22, c, 0xFFAA00);
  pushFrame();
}
