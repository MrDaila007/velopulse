# Ambient Light Auto-Calibration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Passively learn the photoresistor's real per-device `raw_dark`/`raw_bright` ADC range during normal operation, persist it like the odometer (A/B flash slots, survives reboot), and expose a quality flag through debug logging — Arduino firmware target only.

**Architecture:** Two new pure-C++ domain classes (`AmbientLightCalibrator` for the expand-only learning + quality check, `AmbientCalibrationSavePolicy` for save-trigger throttling) plug into the existing `StorageManager` A/B-slot mechanism as a third payload type, then get wired into the Arduino platform layer (`AmbientLightManager`) and the app (`AppController`) following the odometer's existing wiring pattern exactly.

**Tech Stack:** C++17 (Arduino framework for the nRF52840 target, native/Unity for host tests), PlatformIO (`pio test -e native`, `pio run -e xiao_ble_sense -e xiao_ble_sense_128x32`).

## Global Constraints

- Design source of truth: `docs/superpowers/specs/2026-08-06-ambient-light-auto-calibration-design.md`. Read it before starting — this plan implements it as written, including its two addenda (bootstrap semantics, hardware-grounded thresholds).
- Arduino target only (`firmware/`). No changes to `firmware-zephyr/` in this plan.
- Expand-only calibration: `raw_dark` only decreases, `raw_bright` only increases. Never shrink or re-center.
- Calibrator starts from whatever `configure()` is called with at boot (loaded-flash value if present, else the real board's compile-time defaults) — not an empty/unseen sentinel. This was a deliberate, confirmed choice; see the spec's "Bootstrap and known limitation" section.
- Quality thresholds must classify the real hardware factory pair — `BIKECOMP_AMBIENT_RAW_DARK=266`, `BIKECOMP_AMBIENT_RAW_BRIGHT=1126` (`firmware/platformio.ini:20-21`, applies to every `xiao_ble_sense*` env) — as `kOk`. Use `kMinWidth=400`, `kMaxAcceptableDark=600`, `kMinAcceptableBright=900`.
- No UI or BLE exposure of calibration state this iteration — debug log (`Serial.print`) only.
- Debug logging happens at boot (`printLoadInfo("AmbientCalibration", ...)`, matching the existing config/odometer boot log) and on every save attempt (`persistAmbientCalibration`'s trailing `Serial.print` block, matching `persistOdometer`'s). This is a deliberate narrowing of the spec's "on every save and on every quality transition" — a save happens on every actual bound change (subject to the policy's throttle), so its log line already carries the current `quality`; a separate per-tick quality-transition tracker would add state for no practical benefit given the confirmed bootstrap semantics (quality realistically only moves once, if ever, since the calibrator seeds from values that already pass the thresholds).
- No hardware-in-the-loop testing in scope. Domain-layer classes (`AmbientLightCalibrator`, `AmbientCalibrationSavePolicy`, storage codec) get native Unity tests, matching how `AmbientLightModel`/`OdometerSavePolicy`/`StorageManager` are tested today (`firmware/test/test_native/test_main.cpp`). Platform-layer changes (`AmbientLightManager`, `AppController`) are Arduino-dependent and are not natively testable — same boundary the existing code already has (no native test touches `ambient_light_manager.cpp` or `app_controller.cpp` today) — so those tasks are verified by a full PlatformIO build instead (`pio run -e xiao_ble_sense -e xiao_ble_sense_128x32`).
- Coding conventions: `snake_case` methods, trailing `_` on private members, `PascalCase` classes, `kConstantName` constants, `enum class` for states/triggers, everything in `namespace bike`. Match the odometer's existing split of policy (when to save) vs. mechanism (how to write bytes).

---

### Task 1: Share the ADC invalid-rail-margin constant

The calibrator (Task 2) needs to reject the same near-rail ADC readings that `AmbientLightModel` already treats as invalid, so both must read from one shared constant instead of duplicating the magic number `4`. Today it's private to `ambient_light_model.cpp`'s anonymous namespace — move it to the header.

**Files:**
- Modify: `firmware/lib/domain/ambient_light_model.h:1-9`
- Modify: `firmware/lib/domain/ambient_light_model.cpp:1-11`, `firmware/lib/domain/ambient_light_model.cpp:69-71`

**Interfaces:**
- Produces: `bike::kAmbientInvalidRailMargin` (`constexpr uint16_t`, value `4`), usable by any file that includes `ambient_light_model.h`.

- [ ] **Step 1: Move the constant into the header**

In `firmware/lib/domain/ambient_light_model.h`, change:

```cpp
namespace bike {

constexpr uint16_t kAmbientAdcMaximum = 4095u;
constexpr uint32_t kAmbientMinimumLevelDwellMs = 2000u;
```

to:

```cpp
namespace bike {

constexpr uint16_t kAmbientAdcMaximum = 4095u;
constexpr uint16_t kAmbientInvalidRailMargin = 4u;
constexpr uint32_t kAmbientMinimumLevelDwellMs = 2000u;
```

- [ ] **Step 2: Remove the duplicate and use the shared constant**

In `firmware/lib/domain/ambient_light_model.cpp`, remove the anonymous-namespace copy:

```cpp
namespace bike {
namespace {

constexpr uint16_t kInvalidRailMargin = 4u;
constexpr uint16_t kLevelThresholds[] = {150u, 350u, 600u, 800u};
```

becomes:

```cpp
namespace bike {
namespace {

constexpr uint16_t kLevelThresholds[] = {150u, 350u, 600u, 800u};
```

Then update both usages inside `addSample`:

```cpp
  if (raw <= kInvalidRailMargin ||
      raw >= kAmbientAdcMaximum - kInvalidRailMargin ||
      raw_bright_ <= raw_dark_) {
```

becomes:

```cpp
  if (raw <= kAmbientInvalidRailMargin ||
      raw >= kAmbientAdcMaximum - kAmbientInvalidRailMargin ||
      raw_bright_ <= raw_dark_) {
```

- [ ] **Step 3: Run the existing ambient light model tests to confirm no regression**

Run: `cd firmware && pio test -e native`
Expected: full suite PASSES, including `test_ambient_light_model_normalizes_levels_caps_and_contrast` and `test_ambient_light_model_ema_hysteresis_dwell_and_invalid_fallback` — behavior is unchanged, only the constant moved.

- [ ] **Step 4: Commit**

```bash
git add firmware/lib/domain/ambient_light_model.h firmware/lib/domain/ambient_light_model.cpp
git commit -m "refactor: share ambient light invalid-rail margin constant"
```

---

### Task 2: `AmbientLightCalibrator` domain class

**Files:**
- Create: `firmware/lib/domain/ambient_light_calibrator.h`
- Create: `firmware/lib/domain/ambient_light_calibrator.cpp`
- Test: `firmware/test/test_native/test_main.cpp` (add functions + `RUN_TEST` lines; add `#include "ambient_light_calibrator.h"` to the include block at the top)

**Interfaces:**
- Consumes: `bike::kAmbientAdcMaximum`, `bike::kAmbientInvalidRailMargin` (from `ambient_light_model.h`, Task 1).
- Produces (used by Task 5's `AmbientLightManager` and Task 7's `AppController`):
  - `class AmbientLightCalibrator` with `configure(uint16_t raw_dark, uint16_t raw_bright)`, `addSample(uint16_t raw)`, `rawDark() const -> uint16_t`, `rawBright() const -> uint16_t`, `quality() const -> AmbientCalibrationQuality`, `changed() const -> bool`, `markPersisted()`.
  - `enum class AmbientCalibrationQuality : uint8_t { kNarrow = 0, kOk = 1 }`.
  - `const char* ambientCalibrationQualityName(AmbientCalibrationQuality quality)`.

- [ ] **Step 1: Write the failing tests**

Add near the other ambient-light tests in `firmware/test/test_native/test_main.cpp` (after `test_ambient_light_model_ema_hysteresis_dwell_and_invalid_fallback`, around line 1274):

```cpp
void test_ambient_light_calibrator_expands_bounds_and_tracks_changed() {
  AmbientLightCalibrator calibrator;
  calibrator.configure(266, 1126);
  TEST_ASSERT_EQUAL_UINT16(266u, calibrator.rawDark());
  TEST_ASSERT_EQUAL_UINT16(1126u, calibrator.rawBright());
  TEST_ASSERT_FALSE(calibrator.changed());

  calibrator.addSample(500);  // inside current bounds, no change
  TEST_ASSERT_EQUAL_UINT16(266u, calibrator.rawDark());
  TEST_ASSERT_EQUAL_UINT16(1126u, calibrator.rawBright());
  TEST_ASSERT_FALSE(calibrator.changed());

  calibrator.addSample(150);  // new low
  TEST_ASSERT_EQUAL_UINT16(150u, calibrator.rawDark());
  TEST_ASSERT_TRUE(calibrator.changed());

  calibrator.addSample(2000);  // new high
  TEST_ASSERT_EQUAL_UINT16(2000u, calibrator.rawBright());

  calibrator.markPersisted();
  TEST_ASSERT_FALSE(calibrator.changed());

  calibrator.addSample(100);  // lower again after persisting
  TEST_ASSERT_EQUAL_UINT16(100u, calibrator.rawDark());
  TEST_ASSERT_TRUE(calibrator.changed());
}

void test_ambient_light_calibrator_rejects_rail_samples() {
  AmbientLightCalibrator calibrator;
  calibrator.configure(266, 1126);

  calibrator.addSample(0);      // presence-check-failure-style sentinel
  calibrator.addSample(4);      // at the rail margin, still rejected
  calibrator.addSample(4095);   // pinned bright rail
  calibrator.addSample(4091);   // at the rail margin, still rejected
  TEST_ASSERT_EQUAL_UINT16(266u, calibrator.rawDark());
  TEST_ASSERT_EQUAL_UINT16(1126u, calibrator.rawBright());
  TEST_ASSERT_FALSE(calibrator.changed());

  calibrator.addSample(5);      // just past the margin, admitted
  calibrator.addSample(4090);   // just past the margin, admitted
  TEST_ASSERT_EQUAL_UINT16(5u, calibrator.rawDark());
  TEST_ASSERT_EQUAL_UINT16(4090u, calibrator.rawBright());
}

void test_ambient_light_calibrator_quality_checks_width_and_absolute_bounds() {
  AmbientLightCalibrator calibrator;

  calibrator.configure(266, 1126);  // real factory default must read as kOk
  TEST_ASSERT_EQUAL(AmbientCalibrationQuality::kOk, calibrator.quality());

  calibrator.configure(1700, 2400);  // width 700 ok, but dark too high
  TEST_ASSERT_EQUAL(AmbientCalibrationQuality::kNarrow, calibrator.quality());

  calibrator.configure(200, 700);  // dark ok, but bright too low
  TEST_ASSERT_EQUAL(AmbientCalibrationQuality::kNarrow, calibrator.quality());

  calibrator.configure(500, 700);  // width 200, below minimum
  TEST_ASSERT_EQUAL(AmbientCalibrationQuality::kNarrow, calibrator.quality());

  calibrator.configure(200, 1200);  // width 1000, dark 200, bright 1200: all pass
  TEST_ASSERT_EQUAL(AmbientCalibrationQuality::kOk, calibrator.quality());

  calibrator.configure(700, 700);  // degenerate zero-width range
  TEST_ASSERT_EQUAL(AmbientCalibrationQuality::kNarrow, calibrator.quality());
}
```

Add the include near the other domain includes (alphabetical, right after `ambient_light_model.h` is not present as its own include today — insert before `battery_model.h`):

```cpp
#include "ambient_light_calibrator.h"
#include "ambient_light_model.h"
```

Register the tests in `main()`, right after the existing ambient light model `RUN_TEST` lines (around line 2744):

```cpp
  RUN_TEST(test_ambient_light_model_ema_hysteresis_dwell_and_invalid_fallback);
  RUN_TEST(test_ambient_light_calibrator_expands_bounds_and_tracks_changed);
  RUN_TEST(test_ambient_light_calibrator_rejects_rail_samples);
  RUN_TEST(test_ambient_light_calibrator_quality_checks_width_and_absolute_bounds);
```

- [ ] **Step 2: Run to verify it fails**

Run: `cd firmware && pio test -e native`
Expected: build FAILS — `ambient_light_calibrator.h: No such file or directory`.

- [ ] **Step 3: Create the header**

Write `firmware/lib/domain/ambient_light_calibrator.h`:

```cpp
#pragma once

#include <stdint.h>

#include "ambient_light_model.h"

namespace bike {

constexpr uint16_t kAmbientCalibrationMinWidth = 400u;
constexpr uint16_t kAmbientCalibrationMaxAcceptableDark = 600u;
constexpr uint16_t kAmbientCalibrationMinAcceptableBright = 900u;

enum class AmbientCalibrationQuality : uint8_t {
  kNarrow = 0,
  kOk = 1,
};

class AmbientLightCalibrator {
 public:
  void configure(uint16_t raw_dark, uint16_t raw_bright);
  void addSample(uint16_t raw);

  uint16_t rawDark() const { return raw_dark_; }
  uint16_t rawBright() const { return raw_bright_; }
  AmbientCalibrationQuality quality() const;
  bool changed() const { return changed_; }
  void markPersisted() { changed_ = false; }

 private:
  uint16_t raw_dark_ = 0;
  uint16_t raw_bright_ = kAmbientAdcMaximum;
  bool changed_ = false;
};

const char* ambientCalibrationQualityName(AmbientCalibrationQuality quality);

}  // namespace bike
```

- [ ] **Step 4: Create the implementation**

Write `firmware/lib/domain/ambient_light_calibrator.cpp`:

```cpp
#include "ambient_light_calibrator.h"

namespace bike {

void AmbientLightCalibrator::configure(uint16_t raw_dark, uint16_t raw_bright) {
  raw_dark_ = raw_dark;
  raw_bright_ = raw_bright;
  changed_ = false;
}

void AmbientLightCalibrator::addSample(uint16_t raw) {
  if (raw <= kAmbientInvalidRailMargin ||
      raw >= kAmbientAdcMaximum - kAmbientInvalidRailMargin) {
    return;
  }
  if (raw < raw_dark_) {
    raw_dark_ = raw;
    changed_ = true;
  }
  if (raw > raw_bright_) {
    raw_bright_ = raw;
    changed_ = true;
  }
}

AmbientCalibrationQuality AmbientLightCalibrator::quality() const {
  if (raw_bright_ <= raw_dark_) return AmbientCalibrationQuality::kNarrow;
  const uint16_t width = static_cast<uint16_t>(raw_bright_ - raw_dark_);
  if (width < kAmbientCalibrationMinWidth) return AmbientCalibrationQuality::kNarrow;
  if (raw_dark_ > kAmbientCalibrationMaxAcceptableDark) {
    return AmbientCalibrationQuality::kNarrow;
  }
  if (raw_bright_ < kAmbientCalibrationMinAcceptableBright) {
    return AmbientCalibrationQuality::kNarrow;
  }
  return AmbientCalibrationQuality::kOk;
}

const char* ambientCalibrationQualityName(AmbientCalibrationQuality quality) {
  return quality == AmbientCalibrationQuality::kOk ? "ok" : "narrow";
}

}  // namespace bike
```

- [ ] **Step 5: Run to verify it passes**

Run: `cd firmware && pio test -e native`
Expected: PASS, including the three new tests.

- [ ] **Step 6: Commit**

```bash
git add firmware/lib/domain/ambient_light_calibrator.h firmware/lib/domain/ambient_light_calibrator.cpp firmware/test/test_native/test_main.cpp
git commit -m "feat: add AmbientLightCalibrator domain class"
```

---

### Task 3: Storage support for `AmbientCalibrationData`

Adds a third payload type to `StorageManager`, following the odometer's A/B-slot pattern. `savePayload`'s `bool config_payload` parameter becomes a `PayloadKind` enum so it can dispatch to a third slot reader.

**Files:**
- Modify: `firmware/lib/domain/storage_manager.h` (whole file structure shown below — see step 1 for the exact diffs)
- Modify: `firmware/lib/domain/storage_manager.cpp`
- Test: `firmware/test/test_native/test_main.cpp`

**Interfaces:**
- Consumes: `encodeRecord`/`decodeRecord` (existing, `storage_manager.h`), the existing `Slot` struct (buffer already 48 bytes, large enough for this 5-byte payload — no change needed).
- Produces (used by Task 7's `AppController`):
  - `struct AmbientCalibrationData { uint16_t raw_dark = 0; uint16_t raw_bright = 0; uint8_t quality = 0; }`
  - `void encodeAmbientCalibration(const AmbientCalibrationData&, uint8_t output[kAmbientCalibrationPayloadSize])`
  - `bool decodeAmbientCalibration(const uint8_t* input, size_t length, AmbientCalibrationData&)`
  - `StorageManager::loadAmbientCalibration(AmbientCalibrationData&, StorageLoadInfo&) -> bool`
  - `StorageManager::saveAmbientCalibration(const AmbientCalibrationData&) -> bool`
  - `StoragePaths::ambient_calibration_a` / `::ambient_calibration_b` (defaults `"/alc_a"` / `"/alc_b"`)

- [ ] **Step 1: Write the failing tests**

Add near the other storage tests in `firmware/test/test_native/test_main.cpp`, after `test_odometer_alternates_and_recovers_older_slot` (around line 767):

```cpp
void test_ambient_calibration_encode_decode_round_trip() {
  AmbientCalibrationData original{123u, 3800u, 1u};
  uint8_t payload[kAmbientCalibrationPayloadSize];
  encodeAmbientCalibration(original, payload);

  AmbientCalibrationData decoded;
  TEST_ASSERT_TRUE(decodeAmbientCalibration(payload, sizeof(payload), decoded));
  TEST_ASSERT_EQUAL_UINT16(original.raw_dark, decoded.raw_dark);
  TEST_ASSERT_EQUAL_UINT16(original.raw_bright, decoded.raw_bright);
  TEST_ASSERT_EQUAL_UINT8(original.quality, decoded.quality);

  TEST_ASSERT_FALSE(decodeAmbientCalibration(payload, sizeof(payload) - 1, decoded));
}

void test_ambient_calibration_defaults_without_flash_write_then_alternates_slots() {
  MemoryStorageBackend backend;
  StorageManager storage(backend);
  TEST_ASSERT_TRUE(storage.begin());

  AmbientCalibrationData loaded;
  StorageLoadInfo info;
  TEST_ASSERT_TRUE(storage.loadAmbientCalibration(loaded, info));
  TEST_ASSERT_EQUAL(StorageSource::kDefaults, info.source);
  TEST_ASSERT_EQUAL_UINT16(0u, loaded.raw_dark);
  // Unlike config/odometer, an absent calibration record must NOT be
  // eagerly written -- the caller decides the real bootstrap values.
  TEST_ASSERT_EQUAL_UINT32(0u, backend.files.count("/alc_a"));
  TEST_ASSERT_EQUAL_UINT32(0u, backend.files.count("/alc_b"));
  TEST_ASSERT_EQUAL_UINT32(0u, storage.counters().writes);

  AmbientCalibrationData saved{266u, 1126u,
                               static_cast<uint8_t>(AmbientCalibrationQuality::kOk)};
  TEST_ASSERT_TRUE(storage.saveAmbientCalibration(saved));
  TEST_ASSERT_EQUAL_UINT32(1u, backend.files.count("/alc_a"));

  StorageManager reloaded(backend);
  TEST_ASSERT_TRUE(reloaded.begin());
  AmbientCalibrationData from_flash;
  TEST_ASSERT_TRUE(reloaded.loadAmbientCalibration(from_flash, info));
  TEST_ASSERT_EQUAL(StorageSource::kSlotA, info.source);
  TEST_ASSERT_EQUAL_UINT16(266u, from_flash.raw_dark);
  TEST_ASSERT_EQUAL_UINT16(1126u, from_flash.raw_bright);

  from_flash.raw_dark = 200u;
  TEST_ASSERT_TRUE(reloaded.saveAmbientCalibration(from_flash));
  TEST_ASSERT_EQUAL_UINT32(1u, backend.files.count("/alc_b"));

  // Saving the same value again is a no-op (dedup by content).
  const uint32_t writes_before = reloaded.counters().writes;
  TEST_ASSERT_TRUE(reloaded.saveAmbientCalibration(from_flash));
  TEST_ASSERT_EQUAL_UINT32(writes_before, reloaded.counters().writes);
}
```

Register in `main()`, after `RUN_TEST(test_odometer_alternates_and_recovers_older_slot);` (around line 2704):

```cpp
  RUN_TEST(test_odometer_alternates_and_recovers_older_slot);
  RUN_TEST(test_ambient_calibration_encode_decode_round_trip);
  RUN_TEST(test_ambient_calibration_defaults_without_flash_write_then_alternates_slots);
```

- [ ] **Step 2: Run to verify it fails**

Run: `cd firmware && pio test -e native`
Expected: build FAILS — `AmbientCalibrationData`/`encodeAmbientCalibration`/`loadAmbientCalibration` undeclared.

- [ ] **Step 3: Extend `storage_manager.h`**

Add the payload version/size constants right after the existing ones:

```cpp
constexpr uint32_t kRecordMagic = 0x50434B42u;
constexpr uint16_t kConfigRecordVersion = 2;
constexpr uint16_t kOdometerRecordVersion = 2;
constexpr uint16_t kAmbientCalibrationRecordVersion = 1;
constexpr size_t kRecordHeaderSize = 16;
constexpr size_t kOdometerPayloadSize = 16;
constexpr size_t kAmbientCalibrationPayloadSize = 5;
constexpr size_t kMaximumRecordSize = 64;
```

Add the struct and codec declarations right after `decodeOdometer`'s declaration:

```cpp
struct AmbientCalibrationData {
  uint16_t raw_dark = 0;
  uint16_t raw_bright = 0;
  uint8_t quality = 0;
};

void encodeAmbientCalibration(const AmbientCalibrationData& calibration,
                              uint8_t output[kAmbientCalibrationPayloadSize]);
bool decodeAmbientCalibration(const uint8_t* input,
                              size_t length,
                              AmbientCalibrationData& calibration);
```

Add the two new paths to `StoragePaths`:

```cpp
struct StoragePaths {
  const char* config_a = "/cfg_a";
  const char* config_b = "/cfg_b";
  const char* odometer_a = "/odo_a";
  const char* odometer_b = "/odo_b";
  const char* ambient_calibration_a = "/alc_a";
  const char* ambient_calibration_b = "/alc_b";
};
```

Add two counters to `StorageCounters`:

```cpp
struct StorageCounters {
  uint32_t writes = 0;
  uint32_t skipped_writes = 0;
  uint32_t read_errors = 0;
  uint32_t write_errors = 0;
  uint32_t config_slot_recoveries = 0;
  uint32_t odometer_slot_recoveries = 0;
  uint32_t ambient_calibration_slot_recoveries = 0;
  uint32_t config_defaults_restored = 0;
  uint32_t odometer_defaults_restored = 0;
  uint32_t ambient_calibration_defaults_restored = 0;
  uint32_t config_migrations = 0;
  uint32_t odometer_migrations = 0;
};
```

Add the two public methods to `StorageManager`, right after `saveOdometer`'s declaration:

```cpp
  bool loadOdometer(OdometerData& odometer, StorageLoadInfo& info);
  bool saveOdometer(const OdometerData& odometer);
  bool loadAmbientCalibration(AmbientCalibrationData& calibration,
                              StorageLoadInfo& info);
  bool saveAmbientCalibration(const AmbientCalibrationData& calibration);
```

Change the private section: add a `PayloadKind` enum, a new slot reader, and change `savePayload`'s bool parameter to that enum:

```cpp
 private:
  struct Slot;
  enum class PayloadKind : uint8_t { kConfig, kOdometer, kAmbientCalibration };

  void readConfigSlot(const char* path, Slot& slot);
  void readOdometerSlot(const char* path, Slot& slot);
  void readAmbientCalibrationSlot(const char* path, Slot& slot);
  bool writeSlot(const char* path,
                 const uint8_t* payload,
                 size_t payload_length,
                 uint16_t version,
                 uint32_t sequence);
  bool savePayload(const char* path_a,
                   const char* path_b,
                   const uint8_t* payload,
                   size_t payload_length,
                   uint16_t version,
                   PayloadKind kind,
                   bool force_write);
```

- [ ] **Step 4: Implement the codec functions in `storage_manager.cpp`**

Add right after `decodeOdometer`:

```cpp
void encodeAmbientCalibration(const AmbientCalibrationData& calibration,
                              uint8_t output[kAmbientCalibrationPayloadSize]) {
  writeU16(output, calibration.raw_dark);
  writeU16(output + 2, calibration.raw_bright);
  output[4] = calibration.quality;
}

bool decodeAmbientCalibration(const uint8_t* input,
                              size_t length,
                              AmbientCalibrationData& calibration) {
  if (input == nullptr || length != kAmbientCalibrationPayloadSize) return false;
  calibration.raw_dark = readU16(input);
  calibration.raw_bright = readU16(input + 2);
  calibration.quality = input[4];
  return true;
}
```

- [ ] **Step 5: Implement `readAmbientCalibrationSlot`**

Add right after `readOdometerSlot`'s closing brace:

```cpp
void StorageManager::readAmbientCalibrationSlot(const char* path, Slot& slot) {
  slot = Slot{};
  uint8_t record[kMaximumRecordSize];
  size_t record_length = 0;
  const StorageIoResult result =
      backend_.read(path, record, sizeof(record), record_length);
  if (result == StorageIoResult::kNotFound) return;
  slot.present = true;
  if (result != StorageIoResult::kOk) {
    ++counters_.read_errors;
    return;
  }

  DecodedRecord decoded;
  if (!decodeRecord(record, record_length, kAmbientCalibrationRecordVersion,
                    kAmbientCalibrationPayloadSize, decoded)) {
    ++counters_.read_errors;
    return;
  }

  AmbientCalibrationData calibration;
  if (!decodeAmbientCalibration(decoded.payload, decoded.header.payload_len,
                                calibration)) {
    ++counters_.read_errors;
    return;
  }
  encodeAmbientCalibration(calibration, slot.payload);
  slot.sequence = decoded.header.sequence;
  slot.version = decoded.header.version;
  slot.needs_migration = false;
  slot.valid = true;
}
```

- [ ] **Step 6: Update `savePayload` to dispatch on `PayloadKind`**

Change:

```cpp
bool StorageManager::savePayload(const char* path_a,
                                 const char* path_b,
                                 const uint8_t* payload,
                                 size_t payload_length,
                                 uint16_t version,
                                 bool config_payload,
                                 bool force_write) {
  if (!mounted_) return false;
  Slot a;
  Slot b;
  if (config_payload) {
    readConfigSlot(path_a, a);
    readConfigSlot(path_b, b);
  } else {
    readOdometerSlot(path_a, a);
    readOdometerSlot(path_b, b);
  }
```

to:

```cpp
bool StorageManager::savePayload(const char* path_a,
                                 const char* path_b,
                                 const uint8_t* payload,
                                 size_t payload_length,
                                 uint16_t version,
                                 PayloadKind kind,
                                 bool force_write) {
  if (!mounted_) return false;
  Slot a;
  Slot b;
  switch (kind) {
    case PayloadKind::kConfig:
      readConfigSlot(path_a, a);
      readConfigSlot(path_b, b);
      break;
    case PayloadKind::kOdometer:
      readOdometerSlot(path_a, a);
      readOdometerSlot(path_b, b);
      break;
    case PayloadKind::kAmbientCalibration:
      readAmbientCalibrationSlot(path_a, a);
      readAmbientCalibrationSlot(path_b, b);
      break;
  }
```

- [ ] **Step 7: Update the two existing `savePayload` call sites**

In `saveConfig`:

```cpp
  return savePayload(paths_.config_a, paths_.config_b, payload,
                     sizeof(payload), kConfigRecordVersion, true, false);
```

becomes:

```cpp
  return savePayload(paths_.config_a, paths_.config_b, payload,
                     sizeof(payload), kConfigRecordVersion,
                     PayloadKind::kConfig, false);
```

In `loadConfig`'s migration branch:

```cpp
      info.migration_written =
          savePayload(paths_.config_a, paths_.config_b, selected->payload,
                      kDeviceConfigPayloadSize, kConfigRecordVersion, true,
                      true);
```

becomes:

```cpp
      info.migration_written =
          savePayload(paths_.config_a, paths_.config_b, selected->payload,
                      kDeviceConfigPayloadSize, kConfigRecordVersion,
                      PayloadKind::kConfig, true);
```

In `saveOdometer`:

```cpp
  const bool ok = savePayload(paths_.odometer_a, paths_.odometer_b, payload,
                              sizeof(payload), kOdometerRecordVersion, false,
                              false);
```

becomes:

```cpp
  const bool ok = savePayload(paths_.odometer_a, paths_.odometer_b, payload,
                              sizeof(payload), kOdometerRecordVersion,
                              PayloadKind::kOdometer, false);
```

In `loadOdometer`'s migration branch:

```cpp
      info.migration_written =
          savePayload(paths_.odometer_a, paths_.odometer_b, selected->payload,
                      kOdometerPayloadSize, kOdometerRecordVersion, false,
                      true);
```

becomes:

```cpp
      info.migration_written =
          savePayload(paths_.odometer_a, paths_.odometer_b, selected->payload,
                      kOdometerPayloadSize, kOdometerRecordVersion,
                      PayloadKind::kOdometer, true);
```

- [ ] **Step 8: Implement `loadAmbientCalibration` and `saveAmbientCalibration`**

Add right after `saveOdometer`'s closing brace:

```cpp
bool StorageManager::loadAmbientCalibration(AmbientCalibrationData& calibration,
                                            StorageLoadInfo& info) {
  info = StorageLoadInfo{};
  if (!mounted_) return false;
  Slot a;
  Slot b;
  readAmbientCalibrationSlot(paths_.ambient_calibration_a, a);
  readAmbientCalibrationSlot(paths_.ambient_calibration_b, b);
  const Slot* selected = nullptr;
  if (a.valid && b.valid) {
    selected = isNewer(b.sequence, a.sequence) ? &b : &a;
  } else if (a.valid) {
    selected = &a;
  } else if (b.valid) {
    selected = &b;
  }
  if (selected != nullptr) {
    if (!decodeAmbientCalibration(selected->payload,
                                  kAmbientCalibrationPayloadSize, calibration)) {
      return false;
    }
    info.source = selected == &a ? StorageSource::kSlotA : StorageSource::kSlotB;
    info.sequence = selected->sequence;
    info.from_version = selected->version;
    info.recovered = selected == &a ? (b.present && !b.valid)
                                    : (a.present && !a.valid);
    if (info.recovered) ++counters_.ambient_calibration_slot_recoveries;
    return true;
  }

  // Unlike config/odometer, do not eagerly write a default record here --
  // the caller (AppController) decides the real bootstrap raw_dark/raw_bright
  // and persists explicitly once the save policy decides to.
  calibration = AmbientCalibrationData{};
  info.source = StorageSource::kDefaults;
  info.recovered = a.present || b.present;
  ++counters_.ambient_calibration_defaults_restored;
  info.sequence = 0;
  info.from_version = kAmbientCalibrationRecordVersion;
  return true;
}

bool StorageManager::saveAmbientCalibration(const AmbientCalibrationData& calibration) {
  uint8_t payload[kAmbientCalibrationPayloadSize];
  encodeAmbientCalibration(calibration, payload);
  return savePayload(paths_.ambient_calibration_a, paths_.ambient_calibration_b,
                     payload, sizeof(payload), kAmbientCalibrationRecordVersion,
                     PayloadKind::kAmbientCalibration, false);
}
```

- [ ] **Step 9: Run to verify it passes**

Run: `cd firmware && pio test -e native`
Expected: PASS, full suite including the two new storage tests.

- [ ] **Step 10: Commit**

```bash
git add firmware/lib/domain/storage_manager.h firmware/lib/domain/storage_manager.cpp firmware/test/test_native/test_main.cpp
git commit -m "feat: add ambient calibration payload type to StorageManager"
```

---

### Task 4: `AmbientCalibrationSavePolicy` domain class

Mirrors `OdometerSavePolicy`'s split of policy vs. mechanism, but simplified: only a time-throttled change trigger plus three one-shot triggers (deep sleep, reboot, USB disconnect). No distance/pause/battery triggers — those don't apply to calibration data.

**Files:**
- Create: `firmware/lib/domain/ambient_calibration_save_policy.h`
- Create: `firmware/lib/domain/ambient_calibration_save_policy.cpp`
- Test: `firmware/test/test_native/test_main.cpp`

**Interfaces:**
- Consumes: nothing beyond `<stdint.h>`.
- Produces (used by Task 7's `AppController`):
  - `enum class AmbientCalibrationSaveTrigger : uint8_t { kNone, kThrottledChange, kDeepSleep, kReboot, kUsbDisconnect }`
  - `class AmbientCalibrationSavePolicy` with `noteUsbPresent(bool)`, `requestDeepSleepSave()`, `requestRebootSave()`, `evaluate(bool calibration_changed, uint32_t now_ms) const -> AmbientCalibrationSaveTrigger`, `acknowledge(AmbientCalibrationSaveTrigger trigger, uint32_t now_ms)`.
  - `const char* ambientCalibrationSaveTriggerName(AmbientCalibrationSaveTrigger trigger)`
  - `constexpr uint32_t kAmbientCalibrationMinSaveIntervalMs = 300000u;`

- [ ] **Step 1: Write the failing tests**

Add near the odometer save-policy tests in `firmware/test/test_native/test_main.cpp`, after `test_odometer_save_flash_error_keeps_oneshot_pending` (around line 1541):

```cpp
void test_ambient_calibration_save_throttles_changes_and_prioritizes_one_shots() {
  AmbientCalibrationSavePolicy policy;

  // No change yet: nothing to save.
  TEST_ASSERT_EQUAL(AmbientCalibrationSaveTrigger::kNone, policy.evaluate(false, 0));

  // First observed change saves immediately (no prior save to throttle against).
  TEST_ASSERT_EQUAL(AmbientCalibrationSaveTrigger::kThrottledChange,
                    policy.evaluate(true, 0));
  policy.acknowledge(AmbientCalibrationSaveTrigger::kThrottledChange, 1000);

  // Too soon after the last save: throttled even though it changed again.
  TEST_ASSERT_EQUAL(AmbientCalibrationSaveTrigger::kNone,
                    policy.evaluate(true, 1000 + 299999u));
  // Exactly at the interval: allowed.
  TEST_ASSERT_EQUAL(AmbientCalibrationSaveTrigger::kThrottledChange,
                    policy.evaluate(true, 1000 + 300000u));

  // No change at all: never saves regardless of elapsed time.
  TEST_ASSERT_EQUAL(AmbientCalibrationSaveTrigger::kNone,
                    policy.evaluate(false, 1000 + 999999u));
}

void test_ambient_calibration_save_one_shot_triggers_and_usb_disconnect() {
  AmbientCalibrationSavePolicy policy;

  policy.requestDeepSleepSave();
  TEST_ASSERT_EQUAL(AmbientCalibrationSaveTrigger::kDeepSleep,
                    policy.evaluate(false, 0));
  policy.requestRebootSave();
  // Reboot outranks a still-pending deep sleep request.
  TEST_ASSERT_EQUAL(AmbientCalibrationSaveTrigger::kReboot, policy.evaluate(false, 0));
  policy.acknowledge(AmbientCalibrationSaveTrigger::kReboot, 0);
  // Deep sleep request is still pending after acknowledging reboot only.
  TEST_ASSERT_EQUAL(AmbientCalibrationSaveTrigger::kDeepSleep,
                    policy.evaluate(false, 0));
  policy.acknowledge(AmbientCalibrationSaveTrigger::kDeepSleep, 0);
  TEST_ASSERT_EQUAL(AmbientCalibrationSaveTrigger::kNone, policy.evaluate(false, 0));

  // First noteUsbPresent call only establishes the baseline, no trigger.
  policy.noteUsbPresent(true);
  TEST_ASSERT_EQUAL(AmbientCalibrationSaveTrigger::kNone, policy.evaluate(false, 0));
  policy.noteUsbPresent(false);
  TEST_ASSERT_EQUAL(AmbientCalibrationSaveTrigger::kUsbDisconnect,
                    policy.evaluate(false, 0));
  policy.acknowledge(AmbientCalibrationSaveTrigger::kUsbDisconnect, 0);
  TEST_ASSERT_EQUAL(AmbientCalibrationSaveTrigger::kNone, policy.evaluate(false, 0));
}
```

Register in `main()`, after `RUN_TEST(test_odometer_save_flash_error_keeps_oneshot_pending);` (around line 2757):

```cpp
  RUN_TEST(test_odometer_save_flash_error_keeps_oneshot_pending);
  RUN_TEST(test_ambient_calibration_save_throttles_changes_and_prioritizes_one_shots);
  RUN_TEST(test_ambient_calibration_save_one_shot_triggers_and_usb_disconnect);
```

Add the include alphabetically, right before `ambient_light_model.h`:

```cpp
#include "ambient_calibration_save_policy.h"
#include "ambient_light_calibrator.h"
#include "ambient_light_model.h"
```

- [ ] **Step 2: Run to verify it fails**

Run: `cd firmware && pio test -e native`
Expected: build FAILS — `ambient_calibration_save_policy.h: No such file or directory`.

- [ ] **Step 3: Create the header**

Write `firmware/lib/domain/ambient_calibration_save_policy.h`:

```cpp
#pragma once

#include <stdint.h>

namespace bike {

constexpr uint32_t kAmbientCalibrationMinSaveIntervalMs = 300000u;

enum class AmbientCalibrationSaveTrigger : uint8_t {
  kNone = 0,
  kThrottledChange,
  kDeepSleep,
  kReboot,
  kUsbDisconnect,
};

class AmbientCalibrationSavePolicy {
 public:
  void noteUsbPresent(bool usb_present);
  void requestDeepSleepSave();
  void requestRebootSave();

  AmbientCalibrationSaveTrigger evaluate(bool calibration_changed,
                                         uint32_t now_ms) const;
  void acknowledge(AmbientCalibrationSaveTrigger trigger, uint32_t now_ms);

 private:
  uint32_t last_saved_ms_ = 0;
  bool has_saved_ = false;
  bool usb_present_ = false;
  bool usb_known_ = false;
  bool usb_disconnect_pending_ = false;
  bool deep_sleep_pending_ = false;
  bool reboot_pending_ = false;
};

const char* ambientCalibrationSaveTriggerName(AmbientCalibrationSaveTrigger trigger);

}  // namespace bike
```

- [ ] **Step 4: Create the implementation**

Write `firmware/lib/domain/ambient_calibration_save_policy.cpp`:

```cpp
#include "ambient_calibration_save_policy.h"

namespace bike {

void AmbientCalibrationSavePolicy::noteUsbPresent(bool usb_present) {
  if (!usb_known_) {
    usb_present_ = usb_present;
    usb_known_ = true;
    return;
  }
  if (usb_present_ && !usb_present) usb_disconnect_pending_ = true;
  usb_present_ = usb_present;
}

void AmbientCalibrationSavePolicy::requestDeepSleepSave() {
  deep_sleep_pending_ = true;
}

void AmbientCalibrationSavePolicy::requestRebootSave() { reboot_pending_ = true; }

AmbientCalibrationSaveTrigger AmbientCalibrationSavePolicy::evaluate(
    bool calibration_changed, uint32_t now_ms) const {
  if (reboot_pending_) return AmbientCalibrationSaveTrigger::kReboot;
  if (deep_sleep_pending_) return AmbientCalibrationSaveTrigger::kDeepSleep;
  if (usb_disconnect_pending_) return AmbientCalibrationSaveTrigger::kUsbDisconnect;
  if (calibration_changed &&
      (!has_saved_ || static_cast<uint32_t>(now_ms - last_saved_ms_) >=
                          kAmbientCalibrationMinSaveIntervalMs)) {
    return AmbientCalibrationSaveTrigger::kThrottledChange;
  }
  return AmbientCalibrationSaveTrigger::kNone;
}

void AmbientCalibrationSavePolicy::acknowledge(
    AmbientCalibrationSaveTrigger trigger, uint32_t now_ms) {
  switch (trigger) {
    case AmbientCalibrationSaveTrigger::kReboot:
      reboot_pending_ = false;
      break;
    case AmbientCalibrationSaveTrigger::kDeepSleep:
      deep_sleep_pending_ = false;
      break;
    case AmbientCalibrationSaveTrigger::kUsbDisconnect:
      usb_disconnect_pending_ = false;
      break;
    case AmbientCalibrationSaveTrigger::kThrottledChange:
    case AmbientCalibrationSaveTrigger::kNone:
      break;
  }
  last_saved_ms_ = now_ms;
  has_saved_ = true;
}

const char* ambientCalibrationSaveTriggerName(AmbientCalibrationSaveTrigger trigger) {
  switch (trigger) {
    case AmbientCalibrationSaveTrigger::kThrottledChange:
      return "throttled_change";
    case AmbientCalibrationSaveTrigger::kDeepSleep:
      return "deep_sleep";
    case AmbientCalibrationSaveTrigger::kReboot:
      return "reboot";
    case AmbientCalibrationSaveTrigger::kUsbDisconnect:
      return "usb_disconnect";
    default:
      return "none";
  }
}

}  // namespace bike
```

- [ ] **Step 5: Run to verify it passes**

Run: `cd firmware && pio test -e native`
Expected: PASS, full suite including the two new save-policy tests.

- [ ] **Step 6: Commit**

```bash
git add firmware/lib/domain/ambient_calibration_save_policy.h firmware/lib/domain/ambient_calibration_save_policy.cpp firmware/test/test_native/test_main.cpp
git commit -m "feat: add AmbientCalibrationSavePolicy domain class"
```

---

### Task 5: Wire `AmbientLightCalibrator` into `AmbientLightManager`

Arduino platform layer. `begin()` gains two parameters so `AppController` (Task 6) can supply the correct bootstrap bounds (loaded-from-flash or compile-time default) instead of `AmbientLightManager` hardcoding the macros itself. `update()` feeds every genuine sensor sample to the calibrator too — but not the presence-check-failure sentinel.

No native test for this task — `AmbientLightManager` includes `<Arduino.h>` and is only compiled by the embedded PlatformIO envs, same boundary the file already has today. Verified by a full embedded build instead (Task 8).

**Files:**
- Modify: `firmware/src/ambient_light_manager.h`
- Modify: `firmware/src/ambient_light_manager.cpp`

**Interfaces:**
- Consumes: `bike::AmbientLightCalibrator` (Task 2).
- Produces (used by Task 6/7's `AppController`):
  - `AmbientLightManager::begin(uint32_t now_ms, uint16_t raw_dark, uint16_t raw_bright)` (signature change — was `begin(uint32_t now_ms)`)
  - `AmbientLightManager::calibration() const -> const AmbientLightCalibrator&`
  - `AmbientLightManager::markCalibrationPersisted()`

- [ ] **Step 1: Update the header**

In `firmware/src/ambient_light_manager.h`, change the `begin` declaration and add the new members:

```cpp
class AmbientLightManager {
 public:
  void begin(uint32_t now_ms);
  bool update(uint32_t now_ms);

  const AmbientLightSnapshot& snapshot() const { return model_.snapshot(); }
  bool enabled() const { return BIKECOMP_AMBIENT_LIGHT != 0; }

 private:
  AmbientLightModel model_;
  uint32_t last_sample_ms_ = 0;
  uint32_t power_started_ms_ = 0;
  bool powered_ = false;
  bool presence_checked_ = false;
};
```

becomes:

```cpp
class AmbientLightManager {
 public:
  void begin(uint32_t now_ms, uint16_t raw_dark, uint16_t raw_bright);
  bool update(uint32_t now_ms);

  const AmbientLightSnapshot& snapshot() const { return model_.snapshot(); }
  bool enabled() const { return BIKECOMP_AMBIENT_LIGHT != 0; }
  const AmbientLightCalibrator& calibration() const { return calibrator_; }
  void markCalibrationPersisted() { calibrator_.markPersisted(); }

 private:
  AmbientLightModel model_;
  AmbientLightCalibrator calibrator_;
  uint32_t last_sample_ms_ = 0;
  uint32_t power_started_ms_ = 0;
  bool powered_ = false;
  bool presence_checked_ = false;
};
```

And update the include list at the top:

```cpp
#include <Arduino.h>

#include "ambient_light_model.h"
```

becomes:

```cpp
#include <Arduino.h>

#include "ambient_light_calibrator.h"
#include "ambient_light_model.h"
```

- [ ] **Step 2: Update `begin()` in the .cpp**

Change:

```cpp
void AmbientLightManager::begin(uint32_t now_ms) {
  model_.configure(BIKECOMP_AMBIENT_RAW_DARK,
                   BIKECOMP_AMBIENT_RAW_BRIGHT, now_ms);
#if BIKECOMP_AMBIENT_LIGHT
```

to:

```cpp
void AmbientLightManager::begin(uint32_t now_ms, uint16_t raw_dark,
                                uint16_t raw_bright) {
  model_.configure(raw_dark, raw_bright, now_ms);
  calibrator_.configure(raw_dark, raw_bright);
#if BIKECOMP_AMBIENT_LIGHT
```

- [ ] **Step 3: Feed genuine samples to the calibrator in `update()`**

Change the final line of `update()`:

```cpp
  const uint16_t average =
      static_cast<uint16_t>((sum - minimum - maximum + 7u) / 14u);
  return model_.addSample(average, now_ms);
#endif
```

to:

```cpp
  const uint16_t average =
      static_cast<uint16_t>((sum - minimum - maximum + 7u) / 14u);
  calibrator_.addSample(average);
  return model_.addSample(average, now_ms);
#endif
```

Leave the presence-check-failure branch (`return model_.addSample(0u, now_ms);`, a few lines above) untouched — that sentinel must not reach the calibrator.

- [ ] **Step 4: Commit**

```bash
git add firmware/src/ambient_light_manager.h firmware/src/ambient_light_manager.cpp
git commit -m "feat: wire AmbientLightCalibrator into AmbientLightManager"
```

(Build verification happens in Task 8, after `AppController` is updated to match the new `begin()` signature — the project won't compile again until Task 6 lands, same as any multi-file signature change.)

---

### Task 6: `AppController` — load and seed calibration at boot

**Files:**
- Modify: `firmware/src/app_controller.h`
- Modify: `firmware/src/app_controller.cpp`

**Interfaces:**
- Consumes: `StorageManager::loadAmbientCalibration` (Task 3), `AmbientLightManager::begin(now_ms, raw_dark, raw_bright)` (Task 5), `AmbientCalibrationSavePolicy` (Task 4, member only — used fully in Task 7).
- Produces: `AppController` now boots with `ambient_light_.calibration()` seeded from the newest valid flash record when its `quality` is `kOk`, otherwise from `BIKECOMP_AMBIENT_RAW_DARK`/`BIKECOMP_AMBIENT_RAW_BRIGHT`.

- [ ] **Step 1: Add the include and member variable**

In `firmware/src/app_controller.h`, add the include alphabetically:

```cpp
#include "ambient_light_manager.h"
#include "battery_manager.h"
```

becomes:

```cpp
#include "ambient_calibration_save_policy.h"
#include "ambient_light_manager.h"
#include "battery_manager.h"
```

Add the member next to `odometer_save_`:

```cpp
  StorageManager storage_;
  OdometerSavePolicy odometer_save_;
```

becomes:

```cpp
  StorageManager storage_;
  OdometerSavePolicy odometer_save_;
  AmbientCalibrationSavePolicy ambient_calibration_save_;
```

- [ ] **Step 2: Load the calibration record at boot**

In `firmware/src/app_controller.cpp`, `AppController::begin()`, change:

```cpp
  StorageLoadInfo config_info;
  StorageLoadInfo odometer_info;
  bool config_ok = false;
  bool odometer_ok = false;
  OdometerData odometer;
  if (fs_ok) {
    config_ok = storage_.loadConfig(config_, config_info);
    odometer_ok = storage_.loadOdometer(odometer, odometer_info);
    if (odometer_ok) {
      trip_computer_.restorePersistentTotals(
          odometer.odometer_mm, odometer.total_revolutions);
    }
  }
  printLoadInfo("Config", config_info, config_ok);
#if BIKECOMP_HALL_ACTIVE_EDGE >= 0
  config_.active_edge = static_cast<uint8_t>(BIKECOMP_HALL_ACTIVE_EDGE);
#endif
  printLoadInfo("Odometer", odometer_info, odometer_ok);
```

to:

```cpp
  StorageLoadInfo config_info;
  StorageLoadInfo odometer_info;
  StorageLoadInfo calibration_info;
  bool config_ok = false;
  bool odometer_ok = false;
  bool calibration_ok = false;
  OdometerData odometer;
  AmbientCalibrationData calibration;
  if (fs_ok) {
    config_ok = storage_.loadConfig(config_, config_info);
    odometer_ok = storage_.loadOdometer(odometer, odometer_info);
    if (odometer_ok) {
      trip_computer_.restorePersistentTotals(
          odometer.odometer_mm, odometer.total_revolutions);
    }
    calibration_ok = storage_.loadAmbientCalibration(calibration, calibration_info);
  }
  printLoadInfo("Config", config_info, config_ok);
#if BIKECOMP_HALL_ACTIVE_EDGE >= 0
  config_.active_edge = static_cast<uint8_t>(BIKECOMP_HALL_ACTIVE_EDGE);
#endif
  printLoadInfo("Odometer", odometer_info, odometer_ok);
  printLoadInfo("AmbientCalibration", calibration_info, calibration_ok);
```

- [ ] **Step 3: Seed `AmbientLightManager` with the right bootstrap bounds**

Still in `begin()`, change:

```cpp
  battery_.begin(config_, millis());
  ambient_light_.begin(millis());
```

to:

```cpp
  battery_.begin(config_, millis());
  uint16_t ambient_raw_dark = BIKECOMP_AMBIENT_RAW_DARK;
  uint16_t ambient_raw_bright = BIKECOMP_AMBIENT_RAW_BRIGHT;
  if (calibration_ok && calibration_info.source != StorageSource::kDefaults &&
      static_cast<AmbientCalibrationQuality>(calibration.quality) ==
          AmbientCalibrationQuality::kOk) {
    ambient_raw_dark = calibration.raw_dark;
    ambient_raw_bright = calibration.raw_bright;
  }
  ambient_light_.begin(millis(), ambient_raw_dark, ambient_raw_bright);
```

- [ ] **Step 4: Build to verify it compiles**

Run: `cd firmware && pio run -e xiao_ble_sense -e xiao_ble_sense_128x32`
Expected: SUCCESS. This is the first point since Task 5 where the project compiles again (the `AmbientLightManager::begin` signature change and this call site now match). Task 4's `AmbientCalibrationSavePolicy` member compiles but is otherwise unused until Task 7 — expect an "unused" type of warning at most, not an error (the codebase's `-Wno-unused-variable`/`-Wno-unused-function` build flags in `firmware/platformio.ini` already suppress those); if the build instead reports a hard error, stop and fix it before continuing to Task 7.

- [ ] **Step 5: Commit**

```bash
git add firmware/src/app_controller.h firmware/src/app_controller.cpp
git commit -m "feat: load and seed ambient light calibration at boot"
```

---

### Task 7: `AppController` — persist calibration on change, deep sleep, reboot, USB disconnect

**Files:**
- Modify: `firmware/src/app_controller.h`
- Modify: `firmware/src/app_controller.cpp`

**Interfaces:**
- Consumes: `AmbientLightManager::calibration()` / `::markCalibrationPersisted()` (Task 5), `StorageManager::saveAmbientCalibration` (Task 3), `AmbientCalibrationSavePolicy::evaluate`/`acknowledge`/`noteUsbPresent`/`requestDeepSleepSave`/`requestRebootSave` (Task 4).
- Produces: calibration bounds get written to flash under the same throttling/one-shot triggers described in the spec; a debug log line on every save attempt.

- [ ] **Step 1: Declare the new private methods**

In `firmware/src/app_controller.h`, change:

```cpp
  void maybePersistOdometer(uint32_t now_ms);
  bool persistOdometer(OdometerSaveTrigger trigger);
```

to:

```cpp
  void maybePersistOdometer(uint32_t now_ms);
  bool persistOdometer(OdometerSaveTrigger trigger);
  void maybePersistAmbientCalibration(uint32_t now_ms);
  bool persistAmbientCalibration(AmbientCalibrationSaveTrigger trigger,
                                 uint32_t now_ms);
```

- [ ] **Step 2: Implement `maybePersistAmbientCalibration` and `persistAmbientCalibration`**

In `firmware/src/app_controller.cpp`, add right after `persistOdometer`'s closing brace (before `saveAndApplyOdometer`):

```cpp
void AppController::maybePersistAmbientCalibration(uint32_t now_ms) {
  if (usb_test_mode_) return;
  const bool changed = ambient_light_.calibration().changed();
  const AmbientCalibrationSaveTrigger trigger =
      ambient_calibration_save_.evaluate(changed, now_ms);
  if (trigger == AmbientCalibrationSaveTrigger::kNone) return;
  persistAmbientCalibration(trigger, now_ms);
}

bool AppController::persistAmbientCalibration(AmbientCalibrationSaveTrigger trigger,
                                              uint32_t now_ms) {
  const AmbientLightCalibrator& calibrator = ambient_light_.calibration();
  AmbientCalibrationData data;
  data.raw_dark = calibrator.rawDark();
  data.raw_bright = calibrator.rawBright();
  data.quality = static_cast<uint8_t>(calibrator.quality());
  const bool ok = storage_.mounted() && storage_.saveAmbientCalibration(data);
  if (ok) {
    ambient_light_.markCalibrationPersisted();
    ambient_calibration_save_.acknowledge(trigger, now_ms);
  }

  Serial.print("Ambient cal save: trigger=");
  Serial.print(ambientCalibrationSaveTriggerName(trigger));
  Serial.print(", result=");
  Serial.print(ok ? "OK" : "ERROR");
  Serial.print(", raw_dark=");
  Serial.print(data.raw_dark);
  Serial.print(", raw_bright=");
  Serial.print(data.raw_bright);
  Serial.print(", quality=");
  Serial.println(ambientCalibrationQualityName(calibrator.quality()));
  return ok;
}
```

- [ ] **Step 3: Hook the throttled-change trigger into `updateAmbient`**

Change:

```cpp
void AppController::updateAmbient(uint32_t now_ms) {
  if (!ambient_light_.update(now_ms)) return;
  const AmbientLightSnapshot& ambient = ambient_light_.snapshot();
  display_.setAmbientBrightness(ambient.brightness_pct, ambient.valid);
  if (ambient_raw_logging_) printAmbientLine();
}
```

to:

```cpp
void AppController::updateAmbient(uint32_t now_ms) {
  if (!ambient_light_.update(now_ms)) return;
  const AmbientLightSnapshot& ambient = ambient_light_.snapshot();
  display_.setAmbientBrightness(ambient.brightness_pct, ambient.valid);
  if (ambient_raw_logging_) printAmbientLine();
  maybePersistAmbientCalibration(now_ms);
}
```

- [ ] **Step 4: Hook the deep-sleep trigger (two call sites)**

In `handlePowerManagerResult`, change:

```cpp
  if (result.request_deep_sleep_save) {
    odometer_save_.requestDeepSleepSave();
    maybePersistOdometer(now_ms);
  }
```

to:

```cpp
  if (result.request_deep_sleep_save) {
    odometer_save_.requestDeepSleepSave();
    maybePersistOdometer(now_ms);
    ambient_calibration_save_.requestDeepSleepSave();
    maybePersistAmbientCalibration(now_ms);
  }
```

In `tryEnterDeepSleep`, change:

```cpp
  odometer_save_.requestDeepSleepSave();
  const uint64_t odometer_mm = trip_computer_.snapshot().odometer_mm;
  const OdometerSaveTrigger trigger =
      odometer_save_.evaluate(odometer_mm, now_ms);
  if (trigger != OdometerSaveTrigger::kNone &&
      !persistOdometer(trigger)) {
    return;
  }

  display_.turnOff(now_ms);
```

to:

```cpp
  odometer_save_.requestDeepSleepSave();
  const uint64_t odometer_mm = trip_computer_.snapshot().odometer_mm;
  const OdometerSaveTrigger trigger =
      odometer_save_.evaluate(odometer_mm, now_ms);
  if (trigger != OdometerSaveTrigger::kNone &&
      !persistOdometer(trigger)) {
    return;
  }
  ambient_calibration_save_.requestDeepSleepSave();
  maybePersistAmbientCalibration(now_ms);

  display_.turnOff(now_ms);
```

(A failed calibration save does not block entering deep sleep — unlike the odometer, calibration data is diagnostic, not critical; `maybePersistAmbientCalibration` is `void` and its result is intentionally not checked here.)

- [ ] **Step 5: Hook the USB-disconnect trigger**

In `updateBattery`, change:

```cpp
  if (!usb_test_mode_) {
    odometer_save_.noteUsbPresent(snapshot.usb_present);
    odometer_save_.noteBatteryPercent(snapshot.percent, snapshot.valid);
  }
  ble_.noteUsbPresent(snapshot.usb_present);
  maybePersistOdometer(now_ms);
```

to:

```cpp
  if (!usb_test_mode_) {
    odometer_save_.noteUsbPresent(snapshot.usb_present);
    odometer_save_.noteBatteryPercent(snapshot.percent, snapshot.valid);
    ambient_calibration_save_.noteUsbPresent(snapshot.usb_present);
  }
  ble_.noteUsbPresent(snapshot.usb_present);
  maybePersistOdometer(now_ms);
  maybePersistAmbientCalibration(now_ms);
```

- [ ] **Step 6: Hook the reboot trigger**

Change:

```cpp
    case CommandId::kReboot:
      odometer_save_.requestRebootSave();
      if (!persistOdometer(OdometerSaveTrigger::kReboot)) {
        result.status = CommandStatus::kErrStorage;
      } else {
        reboot_pending_ = true;
        reboot_requested_ms_ = now_ms;
      }
      break;
```

to:

```cpp
    case CommandId::kReboot:
      odometer_save_.requestRebootSave();
      if (!persistOdometer(OdometerSaveTrigger::kReboot)) {
        result.status = CommandStatus::kErrStorage;
      } else {
        ambient_calibration_save_.requestRebootSave();
        maybePersistAmbientCalibration(now_ms);
        reboot_pending_ = true;
        reboot_requested_ms_ = now_ms;
      }
      break;
```

- [ ] **Step 7: Build to verify it compiles**

Run: `cd firmware && pio run -e xiao_ble_sense -e xiao_ble_sense_128x32`
Expected: SUCCESS, no errors or new warnings.

- [ ] **Step 8: Commit**

```bash
git add firmware/src/app_controller.h firmware/src/app_controller.cpp
git commit -m "feat: persist ambient light calibration on change, deep sleep, reboot, USB disconnect"
```

---

### Task 8: Full verification

**Files:** none (verification only).

- [ ] **Step 1: Run the full native domain test suite**

Run: `cd firmware && pio test -e native`
Expected: PASS — every test, including all new ones from Tasks 2-4, plus the full pre-existing suite (regression check for Task 1's constant move and Task 3's `savePayload` signature change).

- [ ] **Step 2: Run the full embedded build for both display variants**

Run: `cd firmware && pio run -e xiao_ble_sense -e xiao_ble_sense_128x32`
Expected: SUCCESS for both environments — this is the only check that exercises `AmbientLightManager` and `AppController`'s Arduino-dependent code (Tasks 5-7), since those aren't reachable from the native test suite.

- [ ] **Step 3: Confirm nothing was left uncommitted**

Run: `git status`
Expected: clean working tree — every task already committed its own changes.
