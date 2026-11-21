#pragma once
// ---- Drawing: off-screen canvas, colours and icons ----
// Every frame is drawn into `canvas`, then copied to the panel in one go,
// so the screen never shows a half-drawn or cleared frame.
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

MatrixPanel_I2S_DMA* display = nullptr;
GFXcanvas16 canvas(PANEL_W, PANEL_H);
uint32_t frameMs = 0;  // millis() of the frame being drawn

static inline uint16_t c565(uint32_t rgb) {
  return ((rgb >> 8) & 0xF800) | ((rgb >> 5) & 0x07E0) | ((rgb >> 3) & 0x001F);
}

static inline uint32_t dimRGB(uint32_t rgb, uint8_t k) {
  uint32_t r = ((rgb >> 16) & 0xFF) * k / 255, g = ((rgb >> 8) & 0xFF) * k / 255, b = (rgb & 0xFF) * k / 255;
  return (r << 16) | (g << 8) | b;
}

uint32_t hsvRGB(int h, uint8_t s, uint8_t v) {
  h %= 360;
  if (h < 0) h += 360;
  int region = h / 60, rem = (h - region * 60) * 255 / 60;
  uint8_t p = v * (255 - s) / 255;
  uint8_t q = v * (255 - s * rem / 255) / 255;
  uint8_t t = v * (255 - s * (255 - rem) / 255) / 255;
  uint8_t r, g, b;
  switch (region) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    default: r = v; g = p; b = q; break;
  }
  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

bool panelFlip = PANEL_FLIP_180;  // panel mounted upside down in the frame

void pushFrame() {
  if (!panelFlip) {
    display->drawRGBBitmap(0, 0, canvas.getBuffer(), PANEL_W, PANEL_H);
    return;
  }
  // 180° rotation is the pixel buffer in reverse order
  static uint16_t rotated[PANEL_W * PANEL_H];
  const uint16_t* src = canvas.getBuffer();
  for (int i = 0, n = PANEL_W * PANEL_H; i < n; i++) rotated[n - 1 - i] = src[i];
  display->drawRGBBitmap(0, 0, rotated, PANEL_W, PANEL_H);
}

// Small bitmaps: one string per row, '1' = lit
void drawRows(int x, int y, const char* const rows[], int n, uint16_t c) {
  for (int j = 0; j < n; j++)
    for (int i = 0; rows[j][i]; i++)
      if (rows[j][i] == '1') canvas.drawPixel(x + i, y + j, c);
}

static const char* const ICON_HEART[] = {"01010", "11111", "11111", "01110", "00100"};
static const char* const ICON_PLANE[] = {"0001000", "1111110", "0001000"};
static const char* const ICON_CAT[]   = {"1000001", "1100011", "1111111", "1010101", "1111111", "0111110"};
static const char* const ICON_BELL[]  = {"0001000", "0011100", "0111110", "0111110", "0111110", "1111111", "0001000"};
static const char* const ICON_DROP[]  = {"010", "010", "111", "111", "010"};

// ---- Weather icons, centred on (cx, cy), about 14x13 px ----
void drawCloud(int cx, int cy, uint16_t c) {
  canvas.fillCircle(cx - 3, cy + 1, 3, c);
  canvas.fillCircle(cx + 1, cy - 1, 4, c);
  canvas.fillCircle(cx + 4, cy + 1, 3, c);
  canvas.fillRect(cx - 5, cy + 1, 11, 4, c);
}

void drawSun(int cx, int cy, int r) {
  canvas.fillCircle(cx, cy, r, c565(0xFFC800));
  int ray = (frameMs / 500) % 2 ? r + 2 : r + 3;
  for (int a = 0; a < 8; a++) {
    float an = a * PI / 4 + frameMs / 2500.0f;
    canvas.drawPixel(cx + lroundf(cosf(an) * ray), cy + lroundf(sinf(an) * ray), c565(0xFF9600));
  }
}

void drawWeatherIcon(const String& cond, int cx, int cy) {
  uint16_t grey = c565(0x9A9AA8), white = c565(0xD8D8E0), blue = c565(0x3399FF);
  if (cond == "sunny") {
    drawSun(cx, cy, 3);
  } else if (cond == "clear-night") {
    canvas.fillCircle(cx, cy, 4, c565(0xF0E6A0));
    canvas.fillCircle(cx + 2, cy - 2, 3, 0);
  } else if (cond == "partlycloudy") {
    drawSun(cx - 2, cy - 2, 2);
    drawCloud(cx + 1, cy + 2, white);
  } else if (cond == "rainy" || cond == "pouring") {
    drawCloud(cx, cy - 1, grey);
    for (int i = 0; i < 3; i++) {
      int y = cy + 5 + (int)((frameMs / 120 + i * 2) % 3);
      canvas.drawFastVLine(cx - 4 + i * 4, y, 2, blue);
    }
  } else if (cond == "snowy" || cond == "snowy-rainy" || cond == "hail") {
    drawCloud(cx, cy - 1, grey);
    for (int i = 0; i < 4; i++) {
      int y = cy + 5 + (int)((frameMs / 300 + i) % 3);
      canvas.drawPixel(cx - 5 + i * 3, y, white);
    }
  } else if (cond == "lightning" || cond == "lightning-rainy") {
    drawCloud(cx, cy - 1, grey);
    if ((frameMs / 300) % 4 != 0) {
      uint16_t y = c565(0xFFE000);
      canvas.drawLine(cx + 1, cy + 4, cx - 1, cy + 6, y);
      canvas.drawLine(cx - 1, cy + 6, cx + 1, cy + 6, y);
      canvas.drawLine(cx + 1, cy + 6, cx - 1, cy + 8, y);
    }
  } else if (cond == "fog" || cond == "windy" || cond == "windy-variant") {
    for (int i = 0; i < 4; i++) {
      int off = (int)((frameMs / 200 + i * 3) % 4);
      canvas.drawFastHLine(cx - 6 + off, cy - 4 + i * 3, 10, grey);
    }
  } else {  // cloudy, exceptional, unknown
    drawCloud(cx, cy, grey);
  }
}
