#pragma once
// ---- Hardware and fixed settings for the LED wall frame ----

#define FW_VERSION "0.3.4"

// Panel drive defaults (can be overridden over Wi-Fi, saved in flash).
// Fewer colour bits = no ultra-short LED pulses, so dim colours don't flicker.
#define PANEL_COLOR_DEPTH 8
#define PANEL_REFRESH     90
// This panel latches data on the other clock edge: with the library default (true)
// lit pixels ghost into black neighbours when much of the screen is lit (Art screen).
#define PANEL_CLKPHASE    false
// Panel is mounted upside down in the frame; change with /api/tune?flip=0|1
#define PANEL_FLIP_180    true
#define HOSTNAME   "matrix"            // http://matrix.local and OTA name
#define SETUP_AP   "LED-Frame-Setup"   // Wi-Fi setup hotspot name

#define PANEL_W 64
#define PANEL_H 32

// HUB75 wiring (ESP32-WROOM-32E). Ribbon colour, counted from the brown wire.
#define PIN_R1  25   // 1  brown
#define PIN_G1  26   // 2  red
#define PIN_B1  27   // 3  orange
#define PIN_R2  14   // 5  green
#define PIN_G2  33   // 6  blue   (moved off strapping pin 12)
#define PIN_B2  13   // 7  purple
#define PIN_E   -1   // 8  grey   (unused on 32-row panels)
#define PIN_A   23   // 9  white
#define PIN_B   19   // 10 black
#define PIN_C    5   // 11 brown
#define PIN_D   17   // 12 red
#define PIN_CLK 16   // 13 orange
#define PIN_LAT  4   // 14 yellow
#define PIN_OE  15   // 15 green

// 5 V 5 A supply: never drive the panel past this (0-255).
#define BRIGHTNESS_CAP 200

// America/Chicago, with automatic daylight saving.
#define TZ_POSIX "CST6CDT,M3.2.0,M11.1.0"

#define FRAME_MS 33   // ~30 fps
