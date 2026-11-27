# Changelog

All notable changes to this project are documented here.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/).

## [Unreleased]

### Changed

- When the room sensor returns no data, the main screen now shows `00.0` instead of `--.-`
  in the large temperature slot.
- **Larger, bolder main reading** — the room temperature is always drawn at 5× with a second
  1 px pass (4× only for long values such as `-10.5`), filling the screen width and staying
  readable from farther away.
- **No footer on the main screen** — humidity (or boiler water when humidity is unavailable)
  moved into the yellow band where the clock sits; the clock remains on the network, info and
  sleep pages. The seven-segment themes also grew to 40 px high with 5 px strokes.

## [1.1.0] — 2026-11-27

OLED-focused release: the room temperature is now the whole main screen, ten themes share that
layout, and a new device can be set up entirely from the browser.

### Added

- **Captive portal** — the room unit resolves every DNS name to itself while the access point is
  up (`DNSServer`) and answers the Windows/Android/iOS/macOS connectivity probes with a 302, so a
  phone or laptop that joins `KombiPlus` opens the web UI by itself.
- **Web-based first setup** — `/api/status` now reports `setup` (1 when the first setup is not
  done yet) and the web UI opens a setup card with a Wi-Fi scan, SSID and password fields.
  Language and network can be configured entirely from the browser; the encoder is optional.
  The state is stored as `setupdone=` in `config.txt` and only becomes 1 when the device really
  joins the home network (or when the user picks "Not now"), so a failed attempt brings the card
  back on the next boot.
- **Panel test page** — `GET /test` shows a live mirror of the OLED (the real SSD1306
  framebuffer from `/api/oled`, so themes, pages and colors match) next to a virtual encoder and
  button. Turning changes the setpoint, holding the left/right buttons switches the theme, a
  short press changes the page and a long press toggles the mode, all through the same handlers
  the physical controls use. The page works even without a panel attached, because the sketch
  now renders the display buffer regardless.
- **CI:** `.github/workflows/build.yml` compiles both sketches with arduino-cli
  (ESP8266 core 3.1.2) on every push.

### Fixed

- **Wi-Fi scan from the web UI** — the scan no longer runs blocking inside the HTTP handler
  (which could stall or reset the request in AP+STA mode). `/api/scan?start=1` starts an
  asynchronous scan and the page polls until results arrive; failures now return `ok:0`
  instead of an empty list.
- **Saving Wi-Fi from the web UI** — `/api/wifi` reports `ok:0` when the settings file cannot be
  written instead of pretending success, the restart is delayed so the response is delivered,
  and the page shows the result before reloading.

### Changed

- **Ten OLED themes on one layout** — `klasik`, `ince`, `dolu`, `kutu`, `nokta`, `dijital`,
  `neon`, `panel`, `minimal` and `retro` all keep the main screen the same; a theme changes only
  the status icons (`ikon=`: line, thin, filled, boxed, dotted) and the large temperature font
  (`font=`: GFX or seven-segment digits). Every theme file is generated on first boot.
- **First encoder turn only enters edit mode** — while the room temperature is on screen the
  first turn no longer changes the target; it opens the blinking target view for 10 s, and only
  the following turns step the setpoint. An accidental bump cannot alter the setting.
- **OLED status band now shows links, time and state** — separate icons for the room sensor,
  the boiler link and Wi-Fi are always drawn (broken link / sensor `!` / crossed waves on
  failure), plus mode-off, sleep and boost when active; the current `HH:MM` (or `--:--` before
  NTP) sits at the far right with the page dots beside it.
- **Main page is built around the room temperature** — the temperature is drawn at 4×, centred,
  with a small `C` beside it; the `ISITIYOR`/`BEKLIYOR` status text is gone (the value blinks
  while there is heat demand) and humidity/boiler water are only drawn when data exists, so a
  missing humidity reading no longer reserves space. The target is hidden by default: turning
  the encoder draws it blinking in the same large slot for 10 s, then the room temperature
  returns.
- **OLED themes: yellow top band is a status area** — two-colour SSD1306 modules paint
  the first 16 px yellow, so that band now shows icons for sensor error, no boiler link, AP
  mode, sleep and boost, with the page dots on the right. Page-title text (`TERMOSTAT`,
  `NETWORK`, …) is no longer drawn. The body starts at y=16 (`OLED_BAND_H`).
- **Theme switching without the theme page** — the dedicated theme page is gone. Hold the
  encoder button and turn right/left for the next/previous theme, or hold the left/right
  buttons on `/test`; the visible page redraws with the new theme immediately.
  `GET /api/theme?step=1` (or `-1`) drives the same path. At that point a single `klasik` layout
  was built in while the theme structure was reworked; `/themes/active.txt` still stores the choice.
- **Web UI scan helper** — the first-setup card and the Wi-Fi card now share one `scanInto()`
  helper instead of duplicating the `/api/scan` list rendering.
- The product is named **KombiPlus** in the firmware banner, web UI, READMEs and banner image.
- Room-unit wiring documentation now matches the firmware defaults: **OLED SDA = D5, SCL = D6**
  and the encoder on **D1/D2** (CLK/DT). Pin comments corrected (D5 = GPIO14, D6 = GPIO12).

## [1.0.0] — 2026-11-19

First stable release. Both devices report `FW 1.0.0`.

### Added

- **Two-node architecture** — room unit (`oda_termostati`) and boiler relay unit
  (`kombi_rolesi`) talking over line-based ASCII on TCP 8266.
- **Room unit hardware support** — DHT11 / DHT22 / DS18B20, 0.96" SSD1306 OLED (128×64, I²C),
  KY‑040 rotary encoder (setpoint, pages, mode).
- **OLED theme engine** — 10 themes (`klasik`, `minimal`, `ters`, `kutu`, `genis`, `gece`,
  `retro`, `durum`, `kompakt`, `dev`) stored as editable `/themes/*.txt` files on LittleFS,
  with a centered status line and a framed 2× setpoint.
- **Embedded web UI** (`web_ui.h`) — mobile-first layout: setpoint, mode, boost, sleep mode,
  relay behaviour, hysteresis, calibration, frost protection, system info and history chart.
- **Wi-Fi network scanner** — `/api/scan` plus a "Scan networks" list in *Wi-Fi settings*, so a
  network can be picked instead of typed.
- **Multilingual interface (8 languages)** — English, Turkish, Russian, Spanish, Portuguese,
  Japanese, Korean and Chinese in the embedded web UI. On first boot the room unit asks for a
  language (OLED: turn to pick, press to confirm; web UI: full-screen picker) and stores it as
  `lang=` in `/config.txt`; it is also exposed by `/api/set?lang=xx` and `/api/status`. OLED
  labels are localized for en/tr/es/pt because the built-in font is ASCII-only; other languages
  fall back to English on the display.
- **First-boot picker only on new devices** — the language question is tied to a `langset=` marker
  next to `lang=`, so a unit updating from an earlier firmware keeps its settings, boots into
  English without a prompt and can pick a language later in the *Language* card. A board with no
  `/config.txt` is asked again on every boot until a language is chosen.
- **Telegram in the selected language** — command names and bot replies follow the UI language
  (`/статус`, `/温度 22.5`, `/睡眠温度 18` …), while English and the original Turkish commands stay
  valid as aliases, and `/help` lists the command set of the active language.
- **Wi-Fi settings** — home network SSID/password; the access point stays up for the relay unit.
- **Boost** — +1.5 °C for 5 / 15 / 30 minutes.
- **Sleep mode** — alternative setpoint inside an NTP-based time window.
- **Frost protection** — heating demand below 5 °C even when the mode is off.
- **MQTT** — retained `kombi/status` JSON, `kombi/set` commands, `kombi/result` acknowledgement.
- **Telegram bot** — status and control, restricted to the configured chat ID.
- **LittleFS storage** — `/config.txt` settings, `/history.csv` hourly samples, `/themes/*.txt`
  themes, `/fs` LittleFSExplorer file manager (MIT, vendored).
- **History chart** — 24 h / 7 d / 30 d, manual sample, clear.
- **OTA updates** (STA only) and mDNS `termostat.local`.
- **Fail-safe relay** — the relay applies a demand only while the last command is younger than
  5 minutes; the behaviour (open, or hold last demand) is chosen in the web UI and stored in the
  relay's EEPROM.
- **Relay unit** — active-low/active-high selection, optional DS18B20 boiler flow sensor,
  status LED, verbose serial log, always-boots-open contact.
- **Hardware-free demo** — `demo/demo.html` renders the OLED pages and the real web UI, with a
  mock API; `demo/build_demo.ps1` regenerates it from `web_ui.h`.
- **Documentation** — 8-language README set, wiring diagrams, architecture diagram and
  per-device manuals.

### Changed

- Fail-safe timeout is **5 minutes** (was 30 seconds) in firmware, web UI text and docs.
- Room temperature block is centered in the web UI on phone and desktop layouts.
- "Open full screen" in the demo opens an in-page overlay instead of a separate file.
- OLED status text is centered; the setpoint frame dropped its label.

### Fixed

- LittleFSExplorer upload now closes the file on an aborted upload.
- OLED 4 Hz path uses fixed buffers instead of `String` allocations.

