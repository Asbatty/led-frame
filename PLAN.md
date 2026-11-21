# LED wall frame — project plan

A framed 64x32 RGB LED panel in the kitchen/living room, driven by an ESP32 over Wi-Fi,
showing live information from Home Assistant.

## Hardware (done)
- Waveshare 64x32 P4 HUB75 panel + ESP32-WROOM-32E DevKitC (USB: CP2102N)
- Pins: R1 25, G1 26, B1 27, R2 14, **G2 33** (moved off strapping pin 12), B2 13,
  A 23, B 19, C 5, D 17, CLK 16, LAT 4, OE 15, E unused
- Power: 5 V 5 A supply. Firmware caps brightness so peak draw stays under ~4 A.
  To add: 1000 µF capacitor across the panel's 5 V/GND; later power the ESP32 from the same supply (VIN).

## Architecture
```
 met.no weather, sensors, people ─► Home Assistant (Core install on a home server)
                                         │  MQTT discovery + state
                                         ▼
                               Mosquitto broker (Docker on the Mac, port 1883)
                                         │
                                         ▼
                ESP32 frame firmware  ◄── phone page http://matrix.local (backup control)
```
- Custom Arduino firmware (ESP32 HUB75 DMA library), draws into an off-screen canvas each frame (no flicker).
- Frame finds the broker by its mDNS `.local` name, not by IP (the server IP can change).
- One MQTT "device discovery" payload creates all entities in HA automatically.

## Home Assistant entities the frame will create
| Entity | Purpose |
|---|---|
| `light.led_frame` | On/off + brightness |
| `select.led_frame_mode` | Auto rotation, Clock & weather, Message, Countdown, Art |
| `text.led_frame_message` | Message to scroll |
| `notify.led_frame` | Temporary alerts (pop up, then return to previous screen) |
| `datetime.led_frame_countdown` + `text.led_frame_countdown_name` | Countdown target |
| diagnostics | Wi-Fi signal, uptime, IP |

HA side lives in one package: `packages/led_frame.yaml` (weather push, rain alerts, Basil screen, schedules).

## Screens (v1)
1. **Clock & weather** — big time, current conditions icon + temp from `weather.forecast_home`,
   today's high/low. **Rain icon** whenever rain is expected in the next few hours.
2. **Message board** — scrolling text from the phone page or HA (`text.set_value`).
3. **Countdown** — event name + days left (set from the phone page or HA).
4. **Ambient art** — slow colour waves with a small clock.
5. **Basil food** — only joins the rotation when `sensor.basil_food_days_left` < 10.

## Alerts (v1)
- **Rain coming** — pop-up warning (icon + "RAIN IN 30 MIN") when the hourly forecast says rain is near.
- Other alerts can be added later by calling `notify.send_message` on `notify.led_frame`.

## Defaults (change any time)
- Dim from 22:00–07:00; off when everyone in the house is away.
- Brightness cap ~200/255 to suit the 5 A supply.

## Build steps
1. **Broker** — Mosquitto in Docker on the Mac (auto-restart), with a dedicated user/password for the frame; add the MQTT integration in HA.
2. **Firmware core** — Wi-Fi setup portal (no password in code), `matrix.local` phone page, settings saved to flash, OTA updates over Wi-Fi.
3. **MQTT + discovery** — frame appears in HA with the entities above; reconnects cleanly.
4. **Screens** — clock & weather, message, countdown, art, Basil; auto rotation.
5. **HA package** — weather + rain push, rain alert, Basil rule, night/away schedule. Checked with `check_config`, committed to the Home Assistant config repo, restart timed away from the cat feeder meal times.
6. **Framing** — test a thin 3D-printed diffuser; adjust colours/brightness for it.
