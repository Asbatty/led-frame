#pragma once
// ---- Weather screen: a stick figure acting out the current weather ----
// Full-width animated scenes, one per Home Assistant weather state.
// Ground is y = 30/31; a standing figure is 15 px tall.

static const uint32_t SKIN = 0xFFC896, FIG = 0xF0F0F0;

static inline void ln(int x0, int y0, int x1, int y1, uint32_t c = FIG) { canvas.drawLine(x0, y0, x1, y1, c565(c)); }
static inline void dot(int x, int y, uint32_t c) { canvas.drawPixel(x, y, c565(c)); }
static inline void headAt(int x, int y) { canvas.fillCircle(x, y, 2, c565(SKIN)); }

// Standing figure, feet on y = g. Arm end points are given relative to the shoulder (x, g-8).
void stander(int x, int g, int ax1, int ay1, int ax2, int ay2, uint32_t body = FIG) {
  ln(x, g - 9, x, g - 5, body);                       // body
  ln(x, g - 4, x - 2, g, body);                       // legs
  ln(x, g - 4, x + 2, g, body);
  ln(x, g - 8, x + ax1, g - 8 + ay1, body);           // arms
  ln(x, g - 8, x + ax2, g - 8 + ay2, body);
  headAt(x, g - 12);
}

static inline float wave(float speed, float phase = 0) { return sinf(frameMs * speed + phase); }

void rainDrops(int n, int speedDiv, uint32_t color, int skipX0 = 99, int skipX1 = -1, int skipBelow = 99, bool hail = false) {
  for (int i = 0; i < n; i++) {
    int x = (i * 41 + 7) % 64;
    int y = (int)((frameMs / speedDiv + i * 11) % 36) - 3;
    if (x >= skipX0 && x <= skipX1 && y >= skipBelow) continue;  // sheltered by the umbrella
    if (y >= 29) {                                                // splash
      dot(x - 1, 30, dimRGB(color, 140));
      dot(x + 1, 30, dimRGB(color, 140));
      continue;
    }
    if (hail) dot(x, y, color);
    else ln(x, y, x, y + 1, color);
  }
}

// ---------- the scenes ----------
void sceneSunny() {
  drawSun(53, 8, 4);
  canvas.fillRect(0, 30, 64, 2, c565(0xB48C3C));                 // sand
  canvas.fillRect(8, 28, 34, 2, c565(0xFF3C78));                 // towel
  for (int x = 11; x < 42; x += 6) canvas.drawFastVLine(x, 28, 2, c565(0xFFFFFF));
  // lying on his back, head on the left
  ln(14, 26, 27, 26);                                              // body
  ln(27, 26, 31, 22); ln(31, 22, 35, 27);                          // bent leg
  ln(27, 26, 38, 27);                                              // straight leg
  ln(17, 26, 20, 22);                                              // arm up, holding a drink
  canvas.fillRect(20, 19, 2, 3, c565(0x00C8FF));
  dot(21, 18, 0xFFFFFF); dot(22, 17, 0xFFFFFF);                    // straw
  headAt(11, 25);
  canvas.drawFastHLine(10, 24, 3, c565(0x2030D0));                 // tinted sunglasses (black would read as a gap)
  dot(10, 25, 0x2030D0); dot(12, 25, 0x2030D0);
  if ((frameMs / 1800) % 3 == 0) dot(12, 24, 0xFFFFFF);            // glint
}

void sceneNight() {
  canvas.fillCircle(54, 7, 4, c565(0xF0E6A0));                     // crescent moon
  canvas.fillCircle(56, 5, 3, 0);
  static const int8_t ST[][2] = {{30, 3}, {38, 9}, {6, 11}, {20, 7}, {44, 2}, {62, 15}, {27, 13}, {12, 4}};
  for (int i = 0; i < 8; i++)
    if (((frameMs / 450) + i * 3) % 7 != 0) dot(ST[i][0], ST[i][1], i % 3 ? 0x9090A0 : 0xFFFFFF);
  canvas.fillRect(6, 28, 34, 3, c565(0x28408C));                   // bed
  canvas.drawFastVLine(6, 24, 7, c565(0x8C5A32));                  // headboard
  canvas.fillRect(8, 25, 6, 3, c565(0xDCDCDC));                    // pillow
  canvas.fillRect(15, 25, 24, 3, c565(0x7040C0));                  // blanket
  headAt(12, 24);
  for (int i = 0; i < 3; i++) {                                    // floating Zzz
    int ph = (int)((frameMs / 70 + i * 22) % 66);
    if (ph < 50) drawText(15 + ph / 6 + i * 2, 20 - ph / 3, "Z", dimRGB(0x8080FF, 255 - ph * 4));
  }
}

void scenePartly() {
  canvas.fillRect(0, 31, 64, 1, c565(0x2E8B2E));
  drawSun(48, 8, 3);
  int cx = 74 - (int)((frameMs / 90) % 100);
  drawCloud(cx, 9, c565(0xD8D8E0));
  bool up = (frameMs / 350) % 2;
  stander(22, 30, -3, 4, 4, up ? -5 : -3);                          // waving
}

void sceneCloudy() {
  canvas.fillRect(0, 31, 64, 1, c565(0x2E8B2E));
  int a = 70 - (int)((frameMs / 120) % 90), b = 90 - (int)((frameMs / 160) % 110);
  drawCloud(a, 6, c565(0x8A8A96));
  drawCloud(b, 11, c565(0x6A6A76));
  drawCloud(a - 40, 9, c565(0x7A7A86));
  stander(30, 30, -2, 5, 2, 5);                                      // hands down, looking glum
}

void sceneRain(bool hail) {
  drawCloud(10, 3, c565(0x6A6A76));
  drawCloud(33, 2, c565(0x7A7A86));
  drawCloud(55, 3, c565(0x6A6A76));
  canvas.fillRect(0, 31, 64, 1, c565(0x1E3C78));                    // puddle
  const int fx = 28, g = 30, ux = fx + 4, uy = g - 16;
  for (int dy = -7; dy <= 0; dy++)                                   // umbrella canopy
    for (int dx = -7; dx <= 7; dx++)
      if (dx * dx + dy * dy <= 49) dot(ux + dx, uy + dy, (dy == 0 && (dx & 1)) ? 0 : 0xE02828);
  stander(fx, g, 4, -2, -3, 3);                                      // right hand holds the handle
  ln(ux, uy, ux, g - 7, 0xB0B0B0);                                   // handle, beside him
  dot(ux - 1, g - 7, 0xB0B0B0);                                      // hook
  rainDrops(hail ? 18 : 26, hail ? 45 : 25, hail ? 0xFFFFFF : 0x3399FF, ux - 7, ux + 7, uy - 8, hail);
}

void scenePouring() {
  rainDrops(34, 18, 0x3399FF);
  int hy = 20 + (int)lroundf(wave(0.004f) * 1.5f);
  bool flip = (frameMs / 300) % 2;
  ln(29, hy + 3, flip ? 25 : 26, flip ? hy - 3 : hy + 1);           // flailing arms
  ln(31, hy + 3, flip ? 34 : 35, flip ? hy + 1 : hy - 3);
  headAt(30, hy);
  for (int x = 0; x < 64; x++) {                                     // rising water with waves
    int wy = 23 + (int)lroundf(sinf(x * 0.35f + frameMs * 0.004f));
    for (int y = wy; y < 32; y++) dot(x, y, y == wy ? 0x60B0FF : (y < 27 ? 0x1E50B4 : 0x143278));
  }
  if ((frameMs / 900) % 2) { dot(26, 21, 0xA0D0FF); dot(34, 19, 0xA0D0FF); }  // splashes
}

void sceneSnow() {
  canvas.fillRect(0, 29, 64, 3, c565(0xE6E6F0));
  // snowman
  canvas.fillCircle(47, 24, 4, c565(0xFFFFFF));
  canvas.fillCircle(47, 17, 3, c565(0xFFFFFF));
  dot(46, 16, 0x202020); dot(48, 16, 0x202020); dot(49, 17, 0xFF8C00);
  canvas.fillRect(45, 12, 5, 2, c565(0xB02020)); canvas.drawFastHLine(44, 14, 7, c565(0xB02020));
  ln(43, 21, 40, 19, 0x8C5A32); ln(51, 21, 54, 18, 0x8C5A32);
  // figure throwing snowballs
  uint32_t ph = frameMs % 2400;
  bool throwing = ph < 300;
  stander(18, 29, -3, 4, throwing ? 4 : -2, throwing ? -4 : 4);
  dot(17, 21, 0xE02828); dot(19, 21, 0xE02828);                     // scarf
  if (ph >= 300 && ph < 1400) {                                      // snowball arc
    float k = (ph - 300) / 1100.0f;
    int bx = 22 + (int)(k * 21), by = 14 - (int)(sinf(k * PI) * 6) + (int)(k * 2);
    canvas.fillCircle(bx, by, 1, c565(0xFFFFFF));
  } else if (ph >= 1400 && ph < 1700) {                              // poof on the snowman
    dot(43, 15, 0xFFFFFF); dot(42, 13, 0xFFFFFF); dot(44, 12, 0xFFFFFF); dot(41, 16, 0xFFFFFF);
  }
  for (int i = 0; i < 16; i++) {                                     // snowflakes
    int x = (i * 37 + 5 + (int)lroundf(wave(0.002f, i) * 2)) % 64;
    int y = (int)((frameMs / 90 + i * 9) % 32);
    if (y < 29) dot(x, y, 0xFFFFFF);
  }
}

void sceneStorm() {
  uint32_t ph = frameMs % 2600;
  bool flash = ph < 220;
  if (flash) canvas.fillScreen(c565(0x14142A));
  drawCloud(10, 3, c565(0x50505C));
  drawCloud(32, 2, c565(0x5A5A66));
  drawCloud(54, 3, c565(0x50505C));
  canvas.fillRect(0, 31, 64, 1, c565(0x2E5A2E));
  rainDrops(16, 22, 0x3366CC);
  int fx = 20, g = 30;
  stander(fx, g, -4, -5, 4, -5);                                     // arms up in panic
  if (flash || (ph / 150) % 2) {                                     // hair standing on end
    ln(fx - 2, g - 15, fx - 3, g - 17, 0xFFE000);
    ln(fx, g - 15, fx, g - 18, 0xFFE000);
    ln(fx + 2, g - 15, fx + 3, g - 17, 0xFFE000);
  }
  if (flash) {                                                       // the bolt
    uint32_t y = 0xFFF0A0;
    ln(40, 7, 37, 13, y); ln(37, 13, 42, 17, y); ln(42, 17, 38, 24, y); ln(38, 24, 41, 30, y);
    dot(40, 30, 0xFF8000); dot(42, 30, 0xFF8000);
  }
}

void sceneWindy() {
  canvas.fillRect(0, 31, 64, 1, c565(0x2E8B2E));
  canvas.drawFastVLine(9, 7, 24, c565(0x909090));                    // pole
  int fw = 4 + (int)lroundf(wave(0.02f) * 2);                       // flapping flag
  for (int i = 0; i < 4; i++) canvas.drawFastHLine(10, 7 + i, fw - abs(i - 1), c565(0xE02828));
  // blown sideways, hands on the pole
  int w1 = (int)lroundf(wave(0.018f) * 1.5f), w2 = (int)lroundf(wave(0.022f, 1) * 2), w3 = (int)lroundf(wave(0.02f, 2) * 2);
  ln(16, 19, 10, 17); ln(16, 19, 10, 20);                            // arms
  ln(16, 19, 26, 19 + w1);                                           // body
  ln(26, 19 + w1, 34, 16 + w2);                                      // legs
  ln(26, 19 + w1, 34, 22 + w3);
  headAt(14, 16);
  static const int8_t WY[] = {4, 21, 9, 26, 18, 2};
  for (int i = 0; i < 6; i++) {                                      // wind streaks
    int x = (int)((frameMs / 6 + i * 29) % 90) - 12, len = 5 + (i * 3) % 6;
    canvas.drawFastHLine(x, WY[i], len, c565(0x70889C));
  }
  int hx = (int)((frameMs / 12) % 140) - 20;                         // his hat flying away
  if (hx > 34 && hx < 66) {
    int hy = 10 + (int)lroundf(wave(0.01f) * 4);
    canvas.fillRect(hx, hy, 4, 2, c565(0x3C3CC8));
    canvas.drawFastHLine(hx - 1, hy + 2, 6, c565(0x3C3CC8));
  }
}

void sceneFog() {
  canvas.fillRect(0, 31, 64, 1, c565(0x3A4A3A));
  int x = (int)((frameMs / 110) % 84) - 10;
  bool step = (frameMs / 250) % 2;
  uint32_t dim = 0x8C8C8C;
  ln(x, 21, x, 25, dim);
  ln(x, 26, x + (step ? -2 : 1), 30, dim);
  ln(x, 26, x + (step ? 2 : -1), 30, dim);
  ln(x, 22, x + (step ? 2 : -2), 25, dim);
  ln(x, 22, x + (step ? -2 : 2), 25, dim);
  canvas.fillCircle(x, 18, 2, c565(0xB49C82));
  static const int8_t FY[] = {6, 11, 16, 20, 24, 28};
  for (int i = 0; i < 6; i++)                                        // drifting fog bands over him
    for (int fx = 0; fx < 64; fx++)
      if ((fx + frameMs / (140 + i * 30) + i * 7) % 11 < 7) dot(fx, FY[i], i % 2 ? 0x505058 : 0x46464E);
}

// ---------- screen ----------
bool wxDemo = false;  // /api/tune?wxdemo=1 cycles every scene
uint32_t wxDemoStart = 0;
String wxDemoCond;    // /api/tune?wxdemo=windy holds one scene
static const char* const WX_DEMO[] = {"sunny", "clear-night", "partlycloudy", "cloudy", "rainy", "pouring",
                                      "snowy", "lightning", "windy", "fog", "hail"};
static const int WX_DEMO_N = sizeof(WX_DEMO) / sizeof(WX_DEMO[0]);

void drawWeatherScene(const String& cond, const String& label) {
  if (cond == "sunny") sceneSunny();
  else if (cond == "clear-night") sceneNight();
  else if (cond == "partlycloudy") scenePartly();
  else if (cond == "rainy") sceneRain(false);
  else if (cond == "hail") sceneRain(true);
  else if (cond == "pouring") scenePouring();
  else if (cond == "snowy" || cond == "snowy-rainy") sceneSnow();
  else if (cond == "lightning" || cond == "lightning-rainy") sceneStorm();
  else if (cond == "windy" || cond == "windy-variant") sceneWindy();
  else if (cond == "fog") sceneFog();
  else sceneCloudy();

  if (label.length()) {  // temperature (or scene name in demo) on a black tab, top left
    int w = textWidth(label);
    canvas.fillRect(0, 0, w + 2, 7, 0);
    drawText(1, 1, label, 0xFFFFFF);
  }
}

void drawWeather() {
  if (wxDemo) {
    String c = wxDemoCond.length() ? wxDemoCond : String(WX_DEMO[((frameMs - wxDemoStart) / 6000) % WX_DEMO_N]);
    drawWeatherScene(c, upper(c.substring(0, 12)));
    return;
  }
  String t = isnan(L.temp) ? String("") : String((int)lroundf(L.temp)) + "\xC2\xB0";
  drawWeatherScene(L.cond, t);
}
