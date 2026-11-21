// Waveshare 64x32 P4 RGB LED matrix (HUB75) on an ESP32-DevKitC (ESP32-WROOM-32E)
// Library: "ESP32 HUB75 LED MATRIX PANEL DMA Display" (Library Manager)
// Board:   Tools > Board > esp32 > "ESP32 Dev Module"

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#define PANEL_WIDTH    64
#define PANEL_HEIGHT   32
#define PANELS_CHAINED 1

// ---- ESP32 (WROOM-32E) pins: the library's standard wiring ----
// Ribbon wire colour (counting from the brown wire at the top) in brackets.
#define R1_PIN  25   // 1  brown
#define G1_PIN  26   // 2  red
#define B1_PIN  27   // 3  orange
                     // 4  yellow  -> GND
#define R2_PIN  14   // 5  green
#define G2_PIN  33   // 6  blue   (moved off GPIO12, a boot strapping pin)
#define B2_PIN  13   // 7  purple
#define E_PIN   -1   // 8  grey    -> not connected (32-row panel)
#define A_PIN   23   // 9  white
#define B_PIN   19   // 10 black
#define C_PIN    5   // 11 brown
#define D_PIN   17   // 12 red
#define CLK_PIN 16   // 13 orange
#define LAT_PIN  4   // 14 yellow
#define OE_PIN  15   // 15 green
                     // 16 blue    -> GND

MatrixPanel_I2S_DMA *display = nullptr;

void colorTest() {
  uint16_t colors[] = {
    display->color565(255, 0, 0),
    display->color565(0, 255, 0),
    display->color565(0, 0, 255),
    display->color565(255, 255, 255),
  };
  const char *names[] = {"red", "green", "blue", "white"};
  for (int i = 0; i < 4; i++) {
    Serial.printf("Filling %s\n", names[i]);
    display->fillScreen(colors[i]);
    delay(1000);
  }
  display->clearScreen();
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("HUB75 matrix test starting");

  HUB75_I2S_CFG::i2s_pins pins = {
    R1_PIN, G1_PIN, B1_PIN, R2_PIN, G2_PIN, B2_PIN,
    A_PIN, B_PIN, C_PIN, D_PIN, E_PIN,
    LAT_PIN, OE_PIN, CLK_PIN
  };
  HUB75_I2S_CFG cfg(PANEL_WIDTH, PANEL_HEIGHT, PANELS_CHAINED, pins);
  // If you see ghosting/flicker, try: cfg.clkphase = false;
  // If colors look scrambled, try:    cfg.driver = HUB75_I2S_CFG::FM6126A;

  display = new MatrixPanel_I2S_DMA(cfg);
  if (!display->begin()) {
    Serial.println("Matrix init FAILED (check pins / PSRAM settings)");
    while (true) delay(1000);
  }
  display->setBrightness8(60);  // 0-255; keep low while powering from a small supply
  display->clearScreen();

  colorTest();
  display->setTextWrap(false);
  display->setTextSize(1);
}

void loop() {
  static int x = PANEL_WIDTH;
  static uint8_t hue = 0;
  const char *msg = "Hello from ESP32!";
  int textWidth = strlen(msg) * 6;   // 6 px per char at size 1

  display->clearScreen();

  // Rainbow-ish color that shifts over time
  uint8_t r = (sin(hue * 0.05) + 1) * 127;
  uint8_t g = (sin(hue * 0.05 + 2.09) + 1) * 127;
  uint8_t b = (sin(hue * 0.05 + 4.18) + 1) * 127;
  display->setTextColor(display->color565(r, g, b));
  display->setCursor(x, 12);
  display->print(msg);

  // Static frame so you can see the edges of the panel
  display->drawRect(0, 0, PANEL_WIDTH, PANEL_HEIGHT, display->color565(0, 0, 80));

  x--;
  if (x < -textWidth) x = PANEL_WIDTH;
  hue++;
  delay(30);
}
