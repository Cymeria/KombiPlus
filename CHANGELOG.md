# Changelog

All notable changes to this project are documented here.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/).

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

