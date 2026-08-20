# Changelog

All notable changes to VeloPulse / BikeComp are documented here.
Version numbers follow [`version.toml`](version.toml); git tags use `v` + semver
(with optional `-beta.N` / `-alpha.N` pre-release suffix).

## [Unreleased]

### Firmware (Arduino) 0.2.0

- **BLE CSC client:** dual-role central + peripheral. Pair a CYCPLUS C3 (cadence)
  or S3 (speed) — or any CSC `0x1816` sensor. Bond in `/csc_a` `/csc_b`; Serial
  `csc-pair` / `csc-forget` / `csc-status`.
- **C3 cadence:** RPM on OLED page CAD and Telemetry v2 `cadence_x10`. Hall stays
  the speed source.
- **S3 speed:** wheel revolutions feed trip/odometer/speed using
  `wheel_circumference_mm`. Fresh S3 data (under 4 s) suppresses Hall so distance
  is not doubled; Hall resumes after disconnect or silence.

### Protocol 1.2

- Telemetry `struct_version = 2` is 44 bytes (v1 36-byte prefix remains valid).
  `csc_flags` reports connection, C3 crank, S3 wheel, pairing, and live speed source.

### Mobile / web

- Dashboard shows cadence and C3/S3 status chips.

## [0.2.0-beta.3] — 2026-08-06

Third **beta** release: fixes live speed and max speed stuck at zero while distance,
average speed, and pulse counting continued to work.

### Firmware (Arduino) 0.2.0

- **Speed fix:** restore Hall sensor ISR edge capture (polling-only regression since
  `0.2.0-beta.1`) so pulse intervals are measured reliably.
- **Speed fix:** tighten `SpeedIntervalGuard` so post-pause cadence is not stretched
  back to stale long gaps; reset the guard when movement resumes from idle/pause.
- **Pulse filter:** reject zero-length accepted intervals; clamp invalid `max_speed_kmh`
  when computing overspeed floor.

### Mobile app 1.1.0+5

- No functional mobile changes; build number bumped for release traceability.

### Upgrade

1. Flash `firmware-128x64/firmware.hex` (or `firmware-128x32` for 32 px OLED) — **required**
   if live speed showed `0.0` while trip distance and average speed were updating.
2. Install **`bikecomp-mobile-v0.2.0-beta.3.apk`** over beta.2 (optional; same app
   features as `1.1.0+4`).

### Release assets

- `firmware-128x64/` — Arduino firmware for SSD1306 128×64.
- `firmware-128x32/` — Arduino firmware for 128×32 OLED profile.
- `firmware-zephyr/` — experimental Zephyr `zephyr.hex`, `zephyr.uf2`, `zephyr.elf`.
- `bikecomp-mobile-v0.2.0-beta.3.apk` — Android companion app.

[0.2.0-beta.3]: https://github.com/MrDaila007/velopulse/releases/tag/v0.2.0-beta.3

## [0.2.0-beta.2] — 2026-08-05

Second **beta** release: CI/release pipeline hardening, Zephyr Z5 Arduino parity catch-up,
and companion-sync display layout fixes.

### Firmware (Arduino) 0.2.0

- No functional firmware changes since `0.2.0-beta.1`; Arduino HEX/APK versions unchanged.

### Firmware (Zephyr) 0.2.0-zephyr — experimental

- **Z5 parity:** PowerManager FSM, deep-sleep adapter, BLE Companion Sync (clock/weather on OLED).
- Domain ztest suite expanded (`test_power_manager`, `test_companion_snapshot`).
- CI now builds and attaches `firmware-zephyr/zephyr.{hex,uf2,elf}` — **field validation still pending**.

### Mobile app 1.1.0+4

- Formatting-only delta since beta.1 (no feature changes in this tag).

### Tooling / CI

- Full CI green: Simulator goldens, Mobile `dart format`, Zephyr `west build` (u8g2 in west workspace).
- Release workflow: Zephyr artifacts, per-tag changelog notes, `workflow_dispatch` with explicit tag.

### Known limitations (beta)

- Primary field target remains **Arduino firmware + Android APK**.
- Zephyr build is bundled for early testers; treat as experimental until hardware gate passes.
- iOS build is CI-only; see `STATUS.md` for acceptance gaps.

### Upgrade

1. Flash `firmware-128x64/firmware.hex` (or `firmware-128x32` for 32 px OLED) — same Arduino build as beta.1
   unless you already run `dev` tip.
2. Install **`bikecomp-mobile-v0.2.0-beta.2.apk`** over beta.1 (same app version `1.1.0`, build `+4`).
3. Optional Zephyr testers: flash `firmware-zephyr/zephyr.uf2` via double-tap bootloader.

### Release assets

- `firmware-128x64/` — Arduino firmware for SSD1306 128×64.
- `firmware-128x32/` — Arduino firmware for 128×32 OLED profile.
- `firmware-zephyr/` — experimental Zephyr `zephyr.hex`, `zephyr.uf2`, `zephyr.elf`.
- `bikecomp-mobile-v0.2.0-beta.2.apk` — Android companion app.

[0.2.0-beta.2]: https://github.com/MrDaila007/velopulse/releases/tag/v0.2.0-beta.2

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
