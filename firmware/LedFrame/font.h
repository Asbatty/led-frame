#pragma once
// ---- 3x5 pixel font (the same one used in the design mockups) ----
// Each glyph is 15 chars, 3 per row, top to bottom. Lowercase prints as uppercase.

struct Glyph { uint8_t c; const char* b; };
static const Glyph FONT[] = {
  {'0', "111101101101111"}, {'1', "010110010010111"}, {'2', "111001111100111"}, {'3', "111001111001111"},
  {'4', "101101111001001"}, {'5', "111100111001111"}, {'6', "111100111101111"}, {'7', "111001001001001"},
  {'8', "111101111101111"}, {'9', "111101111001111"},
  {'A', "010101111101101"}, {'B', "110101110101110"}, {'C', "011100100100011"}, {'D', "110101101101110"},
  {'E', "111100110100111"}, {'F', "111100110100100"}, {'G', "011100101101011"}, {'H', "101101111101101"},
  {'I', "111010010010111"}, {'J', "001001001101010"}, {'K', "101101110101101"}, {'L', "100100100100111"},
  {'M', "101111111101101"}, {'N', "110101101101101"}, {'O', "010101101101010"}, {'P', "110101110100100"},
  {'Q', "010101101110011"}, {'R', "110101110101101"}, {'S', "011100010001110"}, {'T', "111010010010010"},
  {'U', "101101101101111"}, {'V', "101101101101010"}, {'W', "101101111111101"}, {'X', "101101010101101"},
  {'Y', "101101010010010"}, {'Z', "111001010100111"},
  {' ', "000000000000000"}, {':', "000010000010000"}, {'.', "000000000000010"}, {',', "000000000010100"},
  {'!', "010010010000010"}, {'?', "111001010000010"}, {'-', "000000111000000"}, {'+', "000010111010000"},
  {'/', "001001010100100"}, {'\'', "010010000000000"}, {'"', "101101000000000"}, {'%', "101001010100101"},
  {'#', "101111101111101"}, {'&', "010101010101011"}, {'(', "001010010010001"}, {')', "100010010010100"},
  {'=', "000111000111000"}, {'<', "001010100010001"}, {'>', "100010001010100"}, {'*', "000101010101000"},
  {'_', "000000000000111"}, {';', "000010000010100"}, {'@', "010101111100011"}, {'$', "011110010011110"},
  {0xB0, "010101010000000"},  // degree sign
};

const char* glyphFor(int c) {
  if (c >= 'a' && c <= 'z') c -= 32;
  for (const Glyph& g : FONT) if (g.c == c) return g.b;
  return "111001010000010";  // '?'
}

// Read one character from a UTF-8 string; returns -1 for bytes to skip.
int nextChar(const String& s, unsigned& i) {
  uint8_t c = s[i++];
  if (c < 0x80) return c;
  if (c == 0xC2 && i < s.length() && (uint8_t)s[i] == 0xB0) { i++; return 0xB0; }
  while (i < s.length() && ((uint8_t)s[i] & 0xC0) == 0x80) i++;  // other symbols/emoji
  return '?';
}

int textWidth(const String& s, int k = 1) {
  int n = 0;
  for (unsigned i = 0; i < s.length();) if (nextChar(s, i) >= 0) n++;
  return n ? n * 4 * k - k : 0;
}

uint16_t colorAt(uint32_t color, int x) {
  if (color == COLOR_RAINBOW) return c565(hsvRGB(x * 5 + frameMs / 12, 255, 255));
  return c565(color);
}

// Draws text at (x, y); k = scale (1 = 3x5, 2 = 6x10). Returns the x after the text.
int drawText(int x, int y, const String& s, uint32_t color, int k = 1) {
  for (unsigned i = 0; i < s.length();) {
    int ch = nextChar(s, i);
    if (ch < 0) continue;
    if (x > PANEL_W) break;
    if (x + 3 * k >= 0) {
      const char* g = glyphFor(ch);
      for (int r = 0; r < 5; r++)
        for (int q = 0; q < 3; q++)
          if (g[r * 3 + q] == '1') {
            int X = x + q * k, Y = y + r * k;
            uint16_t c = colorAt(color, X);
            if (k == 1) canvas.drawPixel(X, Y, c);
            else canvas.fillRect(X, Y, k, k, c);
          }
    }
    x += 4 * k;
  }
  return x;
}

void drawTextCentered(int y, const String& s, uint32_t color, int k = 1) {
  drawText((PANEL_W - textWidth(s, k)) / 2, y, s, color, k);
}
