# Ambient Light Auto-Calibration — Design

Status: approved
Scope: Arduino firmware target only (`firmware/`). Zephyr port (`firmware-zephyr/`) is a follow-up, tracked separately once this lands and is validated on hardware.

## Problem

The photoresistor brightness sensor is normalized against `raw_dark`/`raw_bright` bounds
(`firmware/src/ambient_light_manager.h:15-21`, macros `BIKECOMP_AMBIENT_RAW_DARK`=100,
`BIKECOMP_AMBIENT_RAW_BRIGHT`=3900). These are compile-time constants, never measured on the
actual unit, never persisted, and there's no way to tell after the fact whether a given device's
factory calibration was actually reasonable for its sensor/enclosure.

## Goal

Passively learn the true min/max raw ADC range per-device during normal operation, persist it
like the odometer (survives reboot/power-loss, wear-leveled), and record a quality signal so it's
possible to tell — via debug log, for now — whether the observed calibration range looks
trustworthy or still too narrow to trust.

## Non-goals (this iteration)

- No user-facing UI or BLE exposure of calibration state — debug log only.
- No active/guided calibration flow (pointing the device at dark/light on demand).
- No Zephyr implementation.
- No shrinking/re-centering of the learned range — expansion only.

## Architecture

Two new pieces, following existing domain/platform layering:

1. **Domain logic** — `AmbientLightCalibrator` (`firmware/lib/domain/ambient_light_calibrator.{h,cpp}`),
   pure C++, no Arduino/Zephyr headers, unit-testable like `AmbientLightModel`.
2. **Storage** — a new payload type on the existing generic `StorageManager`
   (`firmware/lib/domain/storage_manager.{h,cpp}`), following the odometer's A/B-slot pattern
   exactly.

Wiring point: `AmbientLightManager::update()` (`firmware/src/ambient_light_manager.cpp`) already
computes a trimmed-average `raw` sample and feeds it to `model_.addSample(...)`. It will also feed
the same raw sample to the calibrator — but only when the sample is a genuine sensor reading, not
the `addSample(0u, ...)` sentinel used when the presence check fails (no sensor detected,
`ambient_light_manager.cpp:56-65`).

## Domain logic — `AmbientLightCalibrator`

```
class AmbientLightCalibrator {
 public:
  void configure(uint16_t raw_dark, uint16_t raw_bright);
  void addSample(uint16_t raw);  // only valid, non-sentinel samples

  uint16_t rawDark() const;
  uint16_t rawBright() const;
  CalibrationQuality quality() const;  // kOk | kNarrow
  bool changed() const;                // bounds moved since markPersisted()
  void markPersisted();
};
```

- **Expand-only update rule:** `raw_dark = min(raw_dark, raw)`, `raw_bright = max(raw_bright, raw)`.
  Starting point is whatever `configure()` was called with (loaded-from-flash value, or the
  existing compile-time defaults if nothing valid was loaded).
- **Sample admission:** a sample only reaches the calibrator if it's within the same valid-rail
  window `AmbientLightModel::addSample` already uses to decide validity
  (`raw > kInvalidRailMargin && raw < kAmbientAdcMaximum - kInvalidRailMargin`,
  `ambient_light_model.cpp:69-71`). This excludes the presence-check-failure sentinel (raw=0) and a
  stuck-at-rail sensor from corrupting the learned range.
- **Quality criteria** (constants placed next to `kInvalidRailMargin`/`kAmbientAdcMaximum` in
  `ambient_light_model.h`):
  - `kOk` requires **all** of:
    - width: `raw_bright - raw_dark >= kMinCalibrationWidth`
    - `raw_dark <= kMaxAcceptableDark`
    - `raw_bright >= kMinAcceptableBright`
  - Otherwise `kNarrow`.
  - Reference values, grounded in the values actually flashed to hardware — `firmware/platformio.ini`
    sets `BIKECOMP_AMBIENT_RAW_DARK=266` / `BIKECOMP_AMBIENT_RAW_BRIGHT=1126` (width 860) for every
    real board env (`xiao_ble_sense` and its variants); the 100/3900 fallback in the header is only
    used if those build flags are absent, which doesn't happen on real builds. Thresholds must
    treat the real factory pair as passing (consistent with the bootstrap decision above):
    `kMinWidth = 400`, `kMaxAcceptableDark = 600`, `kMinAcceptableBright = 900`. Exact numbers can
    still be retuned during implementation, but must keep 266/1126 classified as `kOk`.
- **Bootstrap and known limitation:** the calibrator's starting `raw_dark`/`raw_bright` are
  whatever `configure()` receives at boot — the loaded-from-flash calibration if one exists,
  otherwise the existing compile-time defaults (100/3900). This was confirmed deliberately: no
  separate "empty/unseen" bootstrap state. Consequence — since those defaults already satisfy the
  `kOk` thresholds above, and updates are expand-only, `quality` reads `kOk` starting from the
  first sample and can never revert to `kNarrow`. It does not certify that *this specific unit*
  has observed genuine darkness/brightness yet. Read `quality` together with the logged
  `raw_dark`/`raw_bright` drift over time, not as a standalone trust signal — the numeric bounds
  are the actually useful signal this iteration produces.

## Storage

New payload type on `StorageManager`, mirroring the odometer exactly
(`firmware/lib/domain/storage_manager.cpp:106-119` for the pattern to copy):

- `struct AmbientCalibrationData { uint16_t raw_dark; uint16_t raw_bright; uint8_t quality; }`
- `encodeAmbientCalibration` / `decodeAmbientCalibration` free functions, wrapped in the standard
  `RecordHeader` (magic, version, payload_len, sequence, crc32).
- `kAmbientCalibrationRecordVersion = 1`. No migration path needed yet (first version), but the
  codec follows the same version-field convention as odometer/config for future-proofing.
- A/B redundant slots: `/alc_a`, `/alc_b` added to `StoragePaths`, same newest-by-sequence
  selection and older-slot-write-target logic as odometer (`storage_manager.cpp:220-280`).

**Load:** at `AmbientLightManager::begin()` / app init, read the calibration record. If valid and
`quality == kOk`, pass `raw_dark`/`raw_bright` into `model_.configure(...)` and
`calibrator_.configure(...)` instead of the compile-time defaults. If invalid or `kNarrow`, fall
back to compile-time defaults and let the calibrator keep learning from there.

## Save policy

A new, deliberately small `AmbientCalibrationSavePolicy` — **not** a reuse of
`OdometerSavePolicy`, since the trigger shape is different (no distance-based triggers apply
here). Saves when:

- `calibrator.changed()` is true, **and**
- at least `kMinSaveIntervalMs` (throttle, e.g. 5 minutes) has passed since the last save, **or**
- one of the existing app-level save signals fires: deep sleep entry, reboot, USB disconnect
  (the same events `OdometerSavePolicy` already reacts to — this policy is just another consumer
  of those existing triggers, not a new signal source).

This avoids flash wear from writing on every sample while still capturing the calibration before
power loss.

## Debug visibility (this iteration only)

On every save, and on every `quality` transition, log: `raw_dark`, `raw_bright`, `quality`,
and whether the active bounds came from flash or compile-time defaults. Match the existing
odometer debug-log format/call site conventions found in the codebase during implementation. No
UI or BLE surface is added in this iteration — see Non-goals.

## Testing

- Unit tests for `AmbientLightCalibrator`: expand-only behavior, sample admission/rejection at
  rail boundaries, quality transitions (kNarrow → kOk and the reverse-never-happens invariant
  since bounds only expand).
- Unit tests for the calibration codec: encode/decode round-trip, matching the existing pattern
  used for odometer/config codec tests.
- No hardware-in-the-loop testing in scope for this iteration.

## Follow-up (explicitly out of scope here)

- Zephyr port of `AmbientLightCalibrator` wiring + storage payload
  (`firmware-zephyr/app/src/services/ambient_light_manager.{h,cpp}`, `littlefs_backend`).
- Any UI/BLE exposure of calibration quality, if it turns out to be needed in practice.
