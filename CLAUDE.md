# LED Frame — notes for Claude

Framed 64×32 HUB75 LED display on an ESP32-WROOM-32E, controlled from Home Assistant (MQTT discovery)
and a phone page at http://matrix.local. See README.md for the overview. Private details for this
install (IPs, hosts, accounts) are in CLAUDE.local.md (git-ignored).

## Layout
- `firmware/LedFrame/` — the Arduino sketch. Header-only modules, all compiled as one unit via LedFrame.ino:
  - `config.h` pins, panel defaults, time zone, `FW_VERSION` (bump on every change)
  - `state.h` saved settings (Preferences) + live data from HA; modes list (append new modes at the end)
  - `gfx.h` canvas, colours, icons, `pushFrame()` (handles the 180° flip)
  - `font.h` 3×5 font (`drawText`, `textWidth`, UTF-8 ° supported)
  - `scenes.h` stick-figure weather scenes (`drawWeatherScene(cond, label)`)
  - `screens.h` clock/message/countdown/Basil/art screens, alerts, auto rotation (`AUTO_ORDER`)
  - `web.h` phone page (PROGMEM HTML), `/api/*`, `/update`, ArduinoOTA
  - `mqtt.h` discovery payload, state topics, command handling
  - `secrets.h` (git-ignored) MQTT + OTA passwords; template in `secrets.example.h`
- `tools/flash.ps1` build + install over Wi-Fi (or `-Usb COM5`); `tools/snapshot.py` PNG of the live frame
- `MatrixTest/` original wiring test sketch; `PLAN.md` original project plan

## Build / install / check
- Build + update over Wi-Fi: `powershell -ExecutionPolicy Bypass -File tools\flash.ps1 [-Frame <ip>]`
- FQBN `esp32:esp32:esp32:PartitionScheme=min_spiffs` (app is ~1.24 MB; default partition is too small).
- arduino-cli lives at `%LOCALAPPDATA%\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe`.
- USB uploads usually need the BOOT button held; prefer Wi-Fi updates. espota/ArduinoOTA fails from
  Windows (firewall blocks the callback) — `flash.ps1` uses the HTTP `/update` endpoint instead.
- Verify drawings without a camera: `python tools/snapshot.py <host> out.png`, then view the PNG.
  `/api/tune?wxdemo=<state>` holds a weather scene; `/api/tune?freeze=1` freezes the art.

## Drawing rules learned on this panel
- Black = LED off: never use black for detail on dark backgrounds (it reads as a hole). Use lit colours.
- Avoid channel values below ~48 for anything that animates at low brightness (flicker).
- Keep `clkphase = false` (PANEL_CLKPHASE); the default ghosts lit pixels into black neighbours.
- Text is uppercase 3×5 (4 px per char incl. gap); scale 2 for headline text. Panel is 64×32.

## Conventions
- Bump `FW_VERSION` in config.h for every firmware change.
- Never commit `secrets.h`, `build/`, or `CLAUDE.local.md`.
- Home Assistant config lives on the Mac mini (see CLAUDE.local.md); changing it needs the user's
  explicit approval, a `check_config` run, and restarts timed away from the cat feeder's meal times.
