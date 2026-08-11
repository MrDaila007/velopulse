# Persist StorageCounters Across Reboot Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make `StorageManager::counters_` (`flash_write_count` and friends, exposed via `GET_DIAGNOSTIC`) survive a reboot instead of resetting to zero every boot, closing the open item in `tasks/firmware/README.md` under «Общие firmware-долги».

**Architecture:** Add a fourth A/B-slot pair (`/cnt_a`, `/cnt_b`) that mirrors the existing `config`/`odometer`/`ambient_calibration` slot pattern already in `firmware/lib/domain/storage_manager.{h,cpp}`. `StorageManager::begin()` loads the newest valid counters record straight into its own `counters_` member (no caller-visible out-param needed, unlike odometer/ambient-calibration which are owned by `AppController`). A new `AppController::persistStorageCounters()` flushes the current snapshot at the same checkpoints already used for odometer/ambient-calibration: deep-sleep-save and both reboot command handlers (Serial `reboot`, BLE `CommandId::kReboot`).

**Tech Stack:** C++17, PlatformIO Unity tests (`env:native`).

## Global Constraints

- `firmware/lib/domain/*` must stay Arduino-independent (compiles under `env:native`).
- The `Slot::payload` buffer (`firmware/lib/domain/storage_manager.cpp:144`) is `uint8_t[kDeviceConfigPayloadSize]` = 48 bytes; the counters payload must fit exactly (it does: 12 × `uint32_t` = 48 bytes).
- Deliberately out of scope: syncing on every distance-triggered odometer save. Counters are diagnostic-only; flushing them only at deep-sleep/reboot checkpoints avoids extra Flash wear while still satisfying "survives reboot".
- Commit messages: `<type>: <description>`, no attribution trailer.
- Run every command from the repo root `/home/user/Documents/velopulse` unless a step says otherwise.

---

### Task 1: Codec — encode/decode `StorageCounters` to a 48-byte payload

**Files:**
- Modify: `firmware/lib/domain/storage_manager.h:11-16` (version/size constants), `:89-96` (`StoragePaths`), `:114-127` (after `StorageCounters`, add codec decls)
- Modify: `firmware/lib/domain/storage_manager.cpp:121-136` (add codec impl next to `encodeAmbientCalibration`)
- Test: `firmware/test/test_native/test_main.cpp`

**Interfaces:**
- Produces: `kStorageCountersRecordVersion` (`uint16_t` = 1), `kStorageCountersPayloadSize` (`size_t` = 48), `encodeStorageCounters(const StorageCounters&, uint8_t[48])`, `decodeStorageCounters(const uint8_t*, size_t, StorageCounters&)` — consumed by Task 2.

- [ ] **Step 1: Write the failing round-trip test**

Add to `firmware/test/test_native/test_main.cpp`, right after `test_ambient_calibration_encode_decode_round_trip` (after line 787):

```cpp
void test_storage_counters_encode_decode_round_trip() {
  StorageCounters original;
  original.writes = 42u;
  original.skipped_writes = 7u;
  original.read_errors = 3u;
  original.write_errors = 1u;
  original.config_slot_recoveries = 2u;
  original.odometer_slot_recoveries = 5u;
  original.ambient_calibration_slot_recoveries = 4u;
  original.config_defaults_restored = 1u;
  original.odometer_defaults_restored = 1u;
  original.ambient_calibration_defaults_restored = 1u;
  original.config_migrations = 6u;
  original.odometer_migrations = 8u;

  uint8_t payload[kStorageCountersPayloadSize];
  encodeStorageCounters(original, payload);

  StorageCounters decoded;
  TEST_ASSERT_TRUE(decodeStorageCounters(payload, sizeof(payload), decoded));
  TEST_ASSERT_EQUAL_UINT32(original.writes, decoded.writes);
  TEST_ASSERT_EQUAL_UINT32(original.skipped_writes, decoded.skipped_writes);
  TEST_ASSERT_EQUAL_UINT32(original.read_errors, decoded.read_errors);
  TEST_ASSERT_EQUAL_UINT32(original.write_errors, decoded.write_errors);
  TEST_ASSERT_EQUAL_UINT32(original.config_slot_recoveries,
                           decoded.config_slot_recoveries);
  TEST_ASSERT_EQUAL_UINT32(original.odometer_slot_recoveries,
                           decoded.odometer_slot_recoveries);
  TEST_ASSERT_EQUAL_UINT32(original.ambient_calibration_slot_recoveries,
                           decoded.ambient_calibration_slot_recoveries);
  TEST_ASSERT_EQUAL_UINT32(original.config_defaults_restored,
                           decoded.config_defaults_restored);
  TEST_ASSERT_EQUAL_UINT32(original.odometer_defaults_restored,
                           decoded.odometer_defaults_restored);
  TEST_ASSERT_EQUAL_UINT32(original.ambient_calibration_defaults_restored,
                           decoded.ambient_calibration_defaults_restored);
  TEST_ASSERT_EQUAL_UINT32(original.config_migrations, decoded.config_migrations);
  TEST_ASSERT_EQUAL_UINT32(original.odometer_migrations, decoded.odometer_migrations);

  TEST_ASSERT_FALSE(decodeStorageCounters(payload, sizeof(payload) - 1, decoded));
}
```

Register it next to the other codec tests, right after `RUN_TEST(test_ambient_calibration_encode_decode_round_trip);` (`firmware/test/test_native/test_main.cpp:3095`):

```cpp
  RUN_TEST(test_storage_counters_encode_decode_round_trip);
```

- [ ] **Step 2: Run the test to verify it fails**

Run: `cd firmware && pio test -e native -f test_storage_counters_encode_decode_round_trip`
Expected: build FAILS — `kStorageCountersPayloadSize`/`encodeStorageCounters`/`decodeStorageCounters` don't exist.

- [ ] **Step 3: Add the constants and declarations**

In `firmware/lib/domain/storage_manager.h`, after `constexpr size_t kAmbientCalibrationPayloadSize = 5;` (line 16):

```cpp
constexpr uint16_t kStorageCountersRecordVersion = 1;
constexpr size_t kStorageCountersPayloadSize = 48;
```

After the closing `};` of `struct StorageCounters` (line 127), before `class StorageManager`:

```cpp
void encodeStorageCounters(const StorageCounters& counters,
                           uint8_t output[kStorageCountersPayloadSize]);
bool decodeStorageCounters(const uint8_t* input, size_t length,
                           StorageCounters& counters);
```

- [ ] **Step 4: Implement the codec**

In `firmware/lib/domain/storage_manager.cpp`, after `decodeAmbientCalibration` (after line 136):

```cpp
void encodeStorageCounters(const StorageCounters& counters,
                           uint8_t output[kStorageCountersPayloadSize]) {
  writeU32(output + 0, counters.writes);
  writeU32(output + 4, counters.skipped_writes);
  writeU32(output + 8, counters.read_errors);
  writeU32(output + 12, counters.write_errors);
  writeU32(output + 16, counters.config_slot_recoveries);
  writeU32(output + 20, counters.odometer_slot_recoveries);
  writeU32(output + 24, counters.ambient_calibration_slot_recoveries);
  writeU32(output + 28, counters.config_defaults_restored);
  writeU32(output + 32, counters.odometer_defaults_restored);
  writeU32(output + 36, counters.ambient_calibration_defaults_restored);
  writeU32(output + 40, counters.config_migrations);
  writeU32(output + 44, counters.odometer_migrations);
}

bool decodeStorageCounters(const uint8_t* input, size_t length,
                           StorageCounters& counters) {
  if (input == nullptr || length != kStorageCountersPayloadSize) return false;
  counters.writes = readU32(input + 0);
  counters.skipped_writes = readU32(input + 4);
  counters.read_errors = readU32(input + 8);
  counters.write_errors = readU32(input + 12);
  counters.config_slot_recoveries = readU32(input + 16);
  counters.odometer_slot_recoveries = readU32(input + 20);
  counters.ambient_calibration_slot_recoveries = readU32(input + 24);
  counters.config_defaults_restored = readU32(input + 28);
  counters.odometer_defaults_restored = readU32(input + 32);
  counters.ambient_calibration_defaults_restored = readU32(input + 36);
  counters.config_migrations = readU32(input + 40);
  counters.odometer_migrations = readU32(input + 44);
  return true;
}
```

- [ ] **Step 5: Run the test to verify it passes**

Run: `cd firmware && pio test -e native -f test_storage_counters_encode_decode_round_trip`
Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add firmware/lib/domain/storage_manager.h firmware/lib/domain/storage_manager.cpp firmware/test/test_native/test_main.cpp
git commit -m "feat(firmware): add StorageCounters wire codec"
```

---

### Task 2: StorageManager — A/B slot, load-on-begin, explicit save

**Files:**
- Modify: `firmware/lib/domain/storage_manager.h:89-96` (`StoragePaths`), `:129-172` (`StorageManager`)
- Modify: `firmware/lib/domain/storage_manager.cpp:151-155` (`begin`), `:286-337` (`savePayload`), add new private/public methods
- Test: `firmware/test/test_native/test_main.cpp`

**Interfaces:**
- Consumes: `encodeStorageCounters`/`decodeStorageCounters` (Task 1).
- Produces: `StorageManager::saveStorageCounters()` (public, returns `bool`) — consumed by Task 3's `AppController::persistStorageCounters()`. `counters()` (already public, `firmware/lib/domain/storage_manager.h:144`) now reflects the persisted snapshot immediately after `begin()`.

- [ ] **Step 1: Write the failing "absent record" test**

Add to `firmware/test/test_native/test_main.cpp`, right after `test_ambient_calibration_defaults_without_flash_write_then_alternates_slots` (after line 826):

```cpp
void test_storage_counters_absent_record_defaults_without_flash_write() {
  MemoryStorageBackend backend;
  StorageManager storage(backend);
  TEST_ASSERT_TRUE(storage.begin());
  TEST_ASSERT_EQUAL_UINT32(0u, storage.counters().writes);
  TEST_ASSERT_EQUAL_UINT32(0u, backend.files.count("/cnt_a"));
  TEST_ASSERT_EQUAL_UINT32(0u, backend.files.count("/cnt_b"));
}

void test_storage_counters_persist_across_reboot() {
  MemoryStorageBackend backend;
  StorageManager storage(backend);
  TEST_ASSERT_TRUE(storage.begin());

  OdometerData odometer{1234u, 5u};
  TEST_ASSERT_TRUE(storage.saveOdometer(odometer));
  odometer.odometer_mm = 2000u;
  TEST_ASSERT_TRUE(storage.saveOdometer(odometer));

  const uint32_t writes_before_save = storage.counters().writes;
  TEST_ASSERT_TRUE(writes_before_save >= 2u);
  TEST_ASSERT_TRUE(storage.saveStorageCounters());
  TEST_ASSERT_EQUAL_UINT32(1u, backend.files.count("/cnt_a"));

  StorageManager reloaded(backend);
  TEST_ASSERT_TRUE(reloaded.begin());
  TEST_ASSERT_EQUAL_UINT32(writes_before_save, reloaded.counters().writes);
}

void test_storage_counters_recovers_from_corrupt_slot_and_keeps_read_error() {
  MemoryStorageBackend backend;
  StorageCounters slot_a;
  slot_a.writes = 10u;
  putStorageCountersRecord(backend, "/cnt_a", slot_a, 3u);

  StorageCounters slot_b;
  slot_b.writes = 9u;
  putStorageCountersRecord(backend, "/cnt_b", slot_b, 2u);
  backend.corrupt("/cnt_a", kRecordHeaderSize);

  StorageManager storage(backend);
  TEST_ASSERT_TRUE(storage.begin());
  // Slot A (sequence 3, newest) is corrupt; falls back to slot B (sequence 2)
  // and keeps the read_errors bump this boot's failed slot A read caused.
  TEST_ASSERT_EQUAL_UINT32(9u, storage.counters().writes);
  TEST_ASSERT_EQUAL_UINT32(1u, storage.counters().read_errors);
}
```

Add the seeding helper next to `putOdometerRecord` (after line 328, inside the same anonymous namespace):

```cpp
void putStorageCountersRecord(MemoryStorageBackend& backend,
                              const char* path,
                              const StorageCounters& counters,
                              uint32_t sequence,
                              uint16_t version = kStorageCountersRecordVersion) {
  uint8_t payload[kStorageCountersPayloadSize];
  uint8_t record[kMaximumRecordSize];
  encodeStorageCounters(counters, payload);
  const size_t length = encodeRecord(payload, sizeof(payload), version,
                                     sequence, record, sizeof(record));
  backend.write(path, record, length);
}
```

Register all three new tests next to the ambient calibration storage test, right after `RUN_TEST(test_ambient_calibration_defaults_without_flash_write_then_alternates_slots);` (`firmware/test/test_native/test_main.cpp:3096`):

```cpp
  RUN_TEST(test_storage_counters_absent_record_defaults_without_flash_write);
  RUN_TEST(test_storage_counters_persist_across_reboot);
  RUN_TEST(test_storage_counters_recovers_from_corrupt_slot_and_keeps_read_error);
```

- [ ] **Step 2: Run the tests to verify they fail**

Run: `cd firmware && pio test -e native -f test_storage_counters_*`
Expected: build FAILS — `saveStorageCounters`, `StoragePaths::counters_a/b`, `PayloadKind::kStorageCounters` don't exist yet.

- [ ] **Step 3: Add the slot paths and the new PayloadKind**

In `firmware/lib/domain/storage_manager.h`, inside `struct StoragePaths` (after `ambient_calibration_b`, line 95):

```cpp
  const char* counters_a = "/cnt_a";
  const char* counters_b = "/cnt_b";
```

Change the `PayloadKind` enum (line 149):

```cpp
  enum class PayloadKind : uint8_t { kConfig, kOdometer, kAmbientCalibration, kStorageCounters };
```

Add method declarations. Public (after `bool saveAmbientCalibration(...)`, line 141):

```cpp
  bool saveStorageCounters();
```

Private (after `void readAmbientCalibrationSlot(...)`, line 153):

```cpp
  void readStorageCountersSlot(const char* path, Slot& slot);
  void loadStorageCounters();
```

- [ ] **Step 4: Implement `readStorageCountersSlot`**

In `firmware/lib/domain/storage_manager.cpp`, after `readAmbientCalibrationSlot` (after line 268):

```cpp
void StorageManager::readStorageCountersSlot(const char* path, Slot& slot) {
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
  if (!decodeRecord(record, record_length, kStorageCountersRecordVersion,
                    kStorageCountersPayloadSize, decoded)) {
    ++counters_.read_errors;
    return;
  }

  StorageCounters restored;
  if (!decodeStorageCounters(decoded.payload, decoded.header.payload_len,
                             restored)) {
    ++counters_.read_errors;
    return;
  }
  encodeStorageCounters(restored, slot.payload);
  slot.sequence = decoded.header.sequence;
  slot.version = decoded.header.version;
  slot.needs_migration = false;
  slot.valid = true;
}
```

- [ ] **Step 5: Add the `kStorageCounters` case to `savePayload`'s switch**

In `firmware/lib/domain/storage_manager.cpp`, inside `savePayload`'s `switch (kind)` (after the `case PayloadKind::kAmbientCalibration:` block, before its closing `}` at line 309):

```cpp
    case PayloadKind::kStorageCounters:
      readStorageCountersSlot(path_a, a);
      readStorageCountersSlot(path_b, b);
      break;
```

- [ ] **Step 6: Implement `loadStorageCounters` and wire it into `begin()`**

In `firmware/lib/domain/storage_manager.cpp`, replace `StorageManager::begin()` (lines 151-155):

```cpp
bool StorageManager::begin() {
  mounted_ = backend_.begin();
  if (!mounted_) {
    ++counters_.read_errors;
    return false;
  }
  loadStorageCounters();
  return true;
}
```

Add `loadStorageCounters` right after `readStorageCountersSlot` (from Step 4):

```cpp
void StorageManager::loadStorageCounters() {
  const uint32_t read_errors_before = counters_.read_errors;
  Slot a;
  Slot b;
  readStorageCountersSlot(paths_.counters_a, a);
  readStorageCountersSlot(paths_.counters_b, b);
  const uint32_t read_errors_during = counters_.read_errors - read_errors_before;

  const Slot* selected = nullptr;
  if (a.valid && b.valid) {
    selected = isNewer(b.sequence, a.sequence) ? &b : &a;
  } else if (a.valid) {
    selected = &a;
  } else if (b.valid) {
    selected = &b;
  }
  if (selected == nullptr) return;

  StorageCounters restored;
  if (!decodeStorageCounters(selected->payload, kStorageCountersPayloadSize,
                             restored)) {
    return;
  }
  // Preserve this boot's own read-error bump instead of discarding it under
  // the wholesale overwrite below.
  restored.read_errors += read_errors_during;
  counters_ = restored;
}
```

- [ ] **Step 7: Implement `saveStorageCounters`**

In `firmware/lib/domain/storage_manager.cpp`, after `saveAmbientCalibration` (after line 526):

```cpp
bool StorageManager::saveStorageCounters() {
  // Snapshot before encoding: writeSlot() below will bump counters_.writes
  // as a side effect of the save itself, which must not leak into this
  // payload (it belongs to the *next* save).
  uint8_t payload[kStorageCountersPayloadSize];
  encodeStorageCounters(counters_, payload);
  return savePayload(paths_.counters_a, paths_.counters_b, payload,
                     sizeof(payload), kStorageCountersRecordVersion,
                     PayloadKind::kStorageCounters, false);
}
```

- [ ] **Step 8: Run the tests to verify they pass**

Run: `cd firmware && pio test -e native -f test_storage_counters_*`
Expected: all 4 PASS (encode/decode round-trip from Task 1, plus the 3 new ones).

- [ ] **Step 9: Run the full native suite**

Run: `cd firmware && pio test -e native`
Expected: all tests pass (131/131 — 127 baseline + 1 from the BLE indicator plan if already applied, + 3 new here; if run standalone, 130/130).

- [ ] **Step 10: Commit**

```bash
git add firmware/lib/domain/storage_manager.h firmware/lib/domain/storage_manager.cpp firmware/test/test_native/test_main.cpp
git commit -m "feat(firmware): persist StorageCounters in a new /cnt_a|b A/B slot"
```

---

### Task 3: Flush counters at deep-sleep and reboot checkpoints

**Files:**
- Modify: `firmware/src/app_controller.h:60-61` (new private method decl)
- Modify: `firmware/src/app_controller.cpp:671-686` (`handlePowerManagerResult`), `:1095-1104` (Serial `kReboot`), `:1739-1749` (BLE `CommandId::kReboot`)
- Modify: `tasks/firmware/README.md:72-74` (close the bullet), `STATUS.md` (update «Ограничения» / «Готово»)

**Interfaces:**
- Consumes: `StorageManager::saveStorageCounters()` (Task 2).

- [ ] **Step 1: Declare the helper**

In `firmware/src/app_controller.h`, after `bool persistAmbientCalibration(AmbientCalibrationSaveTrigger trigger, uint32_t now_ms);` (line 61):

```cpp
  void persistStorageCounters();
```

- [ ] **Step 2: Implement the helper**

In `firmware/src/app_controller.cpp`, right after `persistAmbientCalibration` (after line 1451):

```cpp
void AppController::persistStorageCounters() {
  if (!storage_.mounted()) return;
  const bool ok = storage_.saveStorageCounters();
  Serial.print("Counters save: result=");
  Serial.println(ok ? "OK" : "ERROR");
}
```

- [ ] **Step 3: Flush at the deep-sleep-save checkpoint**

In `handlePowerManagerResult` (lines 671-678), inside the `if (result.request_deep_sleep_save)` block, after `maybePersistAmbientCalibration(now_ms);`:

```cpp
  if (result.request_deep_sleep_save) {
    odometer_save_.requestDeepSleepSave();
    maybePersistOdometer(now_ms);
    ambient_calibration_save_.requestDeepSleepSave();
    maybePersistAmbientCalibration(now_ms);
    persistStorageCounters();
  }
```

- [ ] **Step 4: Flush at the Serial `reboot` checkpoint**

At lines 1095-1104, inside the `case SerialCommand::kReboot:` success branch:

```cpp
      case SerialCommand::kReboot:
        odometer_save_.requestRebootSave();
        if (!persistOdometer(OdometerSaveTrigger::kReboot)) {
          Serial.println("ERROR reboot storage");
        } else {
          persistStorageCounters();
          reboot_pending_ = true;
          reboot_requested_ms_ = now_ms;
          Serial.println("OK reboot");
        }
        break;
```

- [ ] **Step 5: Flush at the BLE `kReboot` checkpoint**

At lines 1739-1749, inside `case CommandId::kReboot:`:

```cpp
    case CommandId::kReboot:
      odometer_save_.requestRebootSave();
      if (!persistOdometer(OdometerSaveTrigger::kReboot)) {
        result.status = CommandStatus::kErrStorage;
      } else {
        ambient_calibration_save_.requestRebootSave();
        maybePersistAmbientCalibration(now_ms);
        persistStorageCounters();
        reboot_pending_ = true;
        reboot_requested_ms_ = now_ms;
      }
      break;
```

- [ ] **Step 6: Build all three embedded profiles**

`app_controller.cpp` isn't part of `env:native`; verify via embedded builds.

Run: `cd firmware && pio run -e xiao_ble_sense`
Expected: SUCCESS.

Run: `cd firmware && pio run -e xiao_ble_sense_128x32`
Expected: SUCCESS.

Run: `cd firmware && pio run -e xiao_ble_sense_deep_sleep`
Expected: SUCCESS.

- [ ] **Step 7: Re-run the native suite as a final regression pass**

Run: `cd firmware && pio test -e native`
Expected: all pass.

- [ ] **Step 8: Update `tasks/firmware/README.md`**

Change the bullet under «Общие firmware-долги» (`tasks/firmware/README.md:72-74`, currently `- [ ] Персистировать StorageCounters между reboot...`) to a closed one, e.g.:

```markdown
- [x] Персистировать `StorageCounters` между reboot: новый A/B слот `/cnt_a|b`
  (`storage_manager.{h,cpp}`), payload version 1, 48 байт. `StorageManager::begin()`
  восстанавливает `counters_` из свежего слота; `AppController::persistStorageCounters()`
  сохраняет снимок перед deep sleep и перед reboot (Serial и BLE `CommandId::kReboot`).
```

- [ ] **Step 9: Update `STATUS.md`**

In the «Ограничения» section, change the sentence "`flash_write_count` и остальные `StorageCounters` RAM-only — сбрасываются при каждом reboot, серимализации/загрузки нет." to reflect the new persisted behavior (checkpoints: deep sleep, reboot; not every write) — mirror the style of the neighboring `OdometerSavePolicy` bullet under «Готово → Прошивка».

- [ ] **Step 10: Commit**

```bash
git add firmware/src/app_controller.h firmware/src/app_controller.cpp tasks/firmware/README.md STATUS.md
git commit -m "feat(firmware): flush StorageCounters at deep-sleep and reboot checkpoints"
```

---

## Self-Review

- **Spec coverage:** `tasks/firmware/README.md:72-74` ("Персистировать StorageCounters между reboot... сейчас поле counters_ только инкрементируется в памяти") is fully addressed by Tasks 1-3.
- **Placeholder scan:** no TBD/vague steps; every step has literal code, exact file:line anchors, and literal shell commands.
- **Type consistency:** `StorageCounters` (12 × `uint32_t`) is used unchanged from its existing definition (`storage_manager.h:114-127`) — no new fields added, only a codec and slot pair for the existing struct. `saveStorageCounters()` takes no arguments and reads `counters_` directly, unlike `saveOdometer`/`saveAmbientCalibration` which take an explicit `Data` struct — this asymmetry is intentional (see Task 2 Step 7's comment) because `StorageCounters` already lives inside `StorageManager` itself, whereas odometer/ambient data are owned by `AppController`.
- **Known trade-off, stated explicitly:** counters are flushed only at deep-sleep/reboot checkpoints, not on every distance-triggered odometer save — so a counters value from a power-loss event (not a clean reboot) reflects the last checkpoint, not the exact moment of loss. This matches the diagnostic (non-critical) nature of these counters and avoids adding Flash wear; call this out if a future task wants tighter accuracy.
