# Changelog

All notable changes to VeloPulse / BikeComp are documented here.
Version numbers follow [`version.toml`](version.toml); git tags use `v` + semver
(with optional `-beta.N` / `-alpha.N` pre-release suffix).

## [0.2.0-beta.1] — 2026-08-03

First public **beta** release: ride computer firmware (Arduino / nRF52840), Android
companion app, and BLE protocol 1.1.

### Firmware (Arduino) 0.2.0

- Trip computer: speed, distance, odometer, ride states, OLED UI (128×64 and 128×32).
- BLE GATT protocol v1.1: config, telemetry, commands, companion time/weather write.
- Power manager: display auto-off, aggressive BLE power save, deep sleep (optional build).
- Redundant flash storage for config and odometer.

### Mobile app 1.1.0

- BLE scan, pair, live dashboard, settings draft/sync.
- Companion sync: clock and Open-Meteo weather on device OLED (city picker, Minsk included).
- Maintenance commands, session log export, firmware migration backup.
- Settings footer: app, device firmware, and protocol versions.

### Known limitations (beta)

- iOS build is CI-only; primary field testing target is Android.
- Zephyr firmware (`0.2.0-zephyr`) is in migration; this release ships **Arduino** HEX/APK.
- Hardware gate and long-ride validation are not complete — see `STATUS.md`.

### Upgrade

1. Flash `firmware-128x64/firmware.hex` (or `firmware-128x32` for 32 px OLED) via UF2/Adafruit
   bootloader or `pio run -t upload`.
2. Install **`bikecomp-mobile-v0.2.0-beta.1.apk`** (Android), pair once, disable aggressive power save if
   BLE discovery fails.
3. Optional: enable time/weather in app **Settings → Время и погода на экране**.

### Release assets

- `firmware-128x64/` — Arduino firmware for SSD1306 128×64 (default XIAO build).
- `firmware-128x32/` — same firmware for 128×32 OLED profile.
- `bikecomp-mobile-v0.2.0-beta.1.apk` — Android companion app (minSdk 24).

[0.2.0-beta.1]: https://github.com/MrDaila007/velopulse/releases/tag/v0.2.0-beta.1
