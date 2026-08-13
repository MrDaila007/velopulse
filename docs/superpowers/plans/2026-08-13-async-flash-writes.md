# Async Flash Writes Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Bound the single largest blocking window a flash save can cause to roughly one flash operation (~90ms) instead of the current 2-3 chained operations, by splitting `StorageManager`'s write path into an async, poll-driven state machine — without changing any existing synchronous caller's behavior.

**Architecture:** `StorageBackend::write()` becomes `beginWrite()`/`pollWrite()` (a two-call async contract). `StorageManager` gains a matching async primitive (`beginSavePayloadAsync`/`pollSave`) that its EXISTING synchronous methods (`saveConfig`, `saveOdometer`, `saveAmbientCalibration`, `saveStorageCounters`, plus the private `writeSlot`) wrap with an internal "start, then drain to completion" call — so every current caller of those methods (all of `loadConfig`/`loadOdometer`'s boot-time defaults/migration writes, every existing native test, `saveAndApplyOdometer`) needs zero changes. Only `AppController`'s three mid-ride/checkpoint callers (`persistOdometer`, `persistAmbientCalibration`, `persistStorageCounters`) switch to the new non-blocking `beginSaveX` entry points, driven forward by a new scheduler task, with the deep-sleep and reboot checkpoints explicitly draining to completion before power-off/reset so no in-flight write is ever lost to those transitions.

**Tech Stack:** C++17, PlatformIO Unity tests (`env:native`), embedded builds (`xiao_ble_sense`, `_128x32`, `_deep_sleep`).

## Global Constraints

- `firmware/lib/domain/*` stays Arduino-independent (compiles under `env:native`).
- `firmware/src/*` (`InternalFsBackend`, `AppController`) only compiles under the `arduino` framework — no native test coverage; verify those changes via `pio run -e xiao_ble_sense[_128x32|_deep_sleep]`, not `pio test -e native`.
- Every task must leave the tree in a fully working, embedded-buildable state — no task may depend on a later task to compile or to behave correctly. This is why the interface change (Task 1) ships with a behavior-preserving (still fully synchronous under the hood) `InternalFsBackend` implementation, and the real two-poll split is isolated to its own task (Task 3) that touches no other file.
- Zero existing native tests should need modification. If a task's diff would require editing an existing test's assertions (not just adding new tests), stop and reconsider the approach — that's a signal the "no behavior change to existing sync callers" boundary has been crossed.
- Only one save may be in flight at a time (`StorageManager` enforces this) — matches this codebase's existing single-slot-queue pattern for BLE commands. A caller that can't start a save because one is already in flight is expected to retry later (every `SavePolicy` is re-evaluated every scheduler tick regardless).
- Commit messages: `<type>: <description>` (e.g. `feat(firmware): ...`), no attribution trailer.
- Run every command from the repo root `/home/user/Documents/velopulse` unless a step says otherwise.

---

### Task 1: `StorageBackend` async interface (behavior-preserving)

**Files:**
- Modify: `firmware/lib/domain/storage_manager.h:72-89` (add `AsyncWriteStatus`, change `StorageBackend`)
- Modify: `firmware/src/internal_fs_backend.h`, `firmware/src/internal_fs_backend.cpp` (reshape to the new interface, fully synchronous under the hood — the real split is Task 3)
- Modify: `firmware/test/test_native/test_main.cpp:276-303` (`MemoryStorageBackend`)
- Test: `firmware/test/test_native/test_main.cpp` (new foundational test)

**Interfaces:**
- Produces: `enum class AsyncWriteStatus : uint8_t { kIdle, kInProgress, kOk, kError };`, `StorageBackend::beginWrite(const char*, const uint8_t*, size_t) -> bool`, `StorageBackend::pollWrite() -> AsyncWriteStatus` — consumed by Task 2's `StorageManager` rewrite.

- [ ] **Step 1: Write the failing test**

Add to `firmware/test/test_native/test_main.cpp`, right after the closing `};` of `class MemoryStorageBackend` (after line 303, before `void putConfigRecord(...)`):

```cpp
void test_memory_backend_async_write_completes_after_configured_polls() {
  MemoryStorageBackend backend;
  backend.polls_to_complete = 3;
  const uint8_t data[] = {1, 2, 3, 4};
  TEST_ASSERT_TRUE(backend.beginWrite("/test_async", data, sizeof(data)));
  TEST_ASSERT_EQUAL(AsyncWriteStatus::kInProgress, backend.pollWrite());
  TEST_ASSERT_EQUAL(AsyncWriteStatus::kInProgress, backend.pollWrite());
  TEST_ASSERT_EQUAL(AsyncWriteStatus::kOk, backend.pollWrite());
  TEST_ASSERT_EQUAL_UINT32(1u, backend.files.count("/test_async"));
  const auto& stored = backend.files.at("/test_async");
  TEST_ASSERT_EQUAL_UINT32(sizeof(data), stored.size());
  TEST_ASSERT_EQUAL_UINT8_ARRAY(data, stored.data(), sizeof(data));
  TEST_ASSERT_EQUAL(AsyncWriteStatus::kIdle, backend.pollWrite());

  // A second beginWrite while one is already pending is rejected.
  backend.polls_to_complete = 1;
  TEST_ASSERT_TRUE(backend.beginWrite("/test_async2", data, sizeof(data)));
  TEST_ASSERT_FALSE(backend.beginWrite("/test_async3", data, sizeof(data)));
}
```

Register it right after `RUN_TEST(test_ambient_calibration_encode_decode_round_trip);` (search for this exact line; it's near the other codec/backend tests):

```cpp
  RUN_TEST(test_memory_backend_async_write_completes_after_configured_polls);
```

- [ ] **Step 2: Run the test to verify it fails**

Run: `cd firmware && pio test -e native -f test_memory_backend_async_write_completes_after_configured_polls`
Expected: build FAILS — `AsyncWriteStatus`, `beginWrite`, `pollWrite`, `polls_to_complete` don't exist yet.

- [ ] **Step 3: Add `AsyncWriteStatus` and change the `StorageBackend` interface**

In `firmware/lib/domain/storage_manager.h`, after the closing `};` of `enum class StorageIoResult` (after line 76), before `class StorageBackend`:

```cpp
enum class AsyncWriteStatus : uint8_t {
  kIdle,
  kInProgress,
  kOk,
  kError,
};
```

Replace the `StorageBackend` class body (lines 78-89):

```cpp
class StorageBackend {
 public:
  virtual ~StorageBackend() = default;
  virtual bool begin() = 0;
  virtual StorageIoResult read(const char* path,
                               uint8_t* output,
                               size_t capacity,
                               size_t& length) = 0;
  // Starts an async write. Returns false only on caller error (a write is
  // already in progress on this backend). The implementation must copy
  // `data` internally -- it will not remain valid past this call. Call
  // pollWrite() repeatedly (e.g. once per scheduler tick) until it returns
  // something other than kInProgress to observe the outcome.
  virtual bool beginWrite(const char* path,
                         const uint8_t* data,
                         size_t length) = 0;
  virtual AsyncWriteStatus pollWrite() = 0;
};
```

- [ ] **Step 4: Reshape `MemoryStorageBackend` to the new interface**

In `firmware/test/test_native/test_main.cpp`, replace the `MemoryStorageBackend` class body (lines 276-303):

```cpp
class MemoryStorageBackend final : public StorageBackend {
 public:
  bool begin() override { return begin_ok; }

  StorageIoResult read(const char* path,
                       uint8_t* output,
                       size_t capacity,
                       size_t& length) override {
    const auto found = files.find(path);
    if (found == files.end()) return StorageIoResult::kNotFound;
    if (found->second.size() > capacity) return StorageIoResult::kError;
    memcpy(output, found->second.data(), found->second.size());
    length = found->second.size();
    return StorageIoResult::kOk;
  }

  bool beginWrite(const char* path, const uint8_t* data,
                  size_t length) override {
    if (write_pending_) return false;
    pending_path_ = path;
    pending_data_.assign(data, data + length);
    polls_remaining_ = polls_to_complete;
    write_pending_ = true;
    return true;
  }

  AsyncWriteStatus pollWrite() override {
    if (!write_pending_) return AsyncWriteStatus::kIdle;
    if (--polls_remaining_ > 0) return AsyncWriteStatus::kInProgress;
    write_pending_ = false;
    if (!write_ok) return AsyncWriteStatus::kError;
    files[pending_path_] = pending_data_;
    return AsyncWriteStatus::kOk;
  }

  // Test-only synchronous seeding helper -- bypasses beginWrite/pollWrite
  // entirely. Used by putXRecord()/corrupt() fixtures below to set up raw
  // slot contents directly.
  bool write(const char* path, const uint8_t* data, size_t length) {
    if (!write_ok) return false;
    files[path] = std::vector<uint8_t>(data, data + length);
    return true;
  }

  void corrupt(const char* path, size_t offset) { files[path][offset] ^= 0x80u; }

  bool begin_ok = true;
  bool write_ok = true;
  size_t polls_to_complete = 1;
  std::map<std::string, std::vector<uint8_t>> files;

 private:
  bool write_pending_ = false;
  std::string pending_path_;
  std::vector<uint8_t> pending_data_;
  size_t polls_remaining_ = 0;
};
```

- [ ] **Step 5: Reshape `InternalFsBackend` to the new interface (still fully synchronous under the hood)**

In `firmware/src/internal_fs_backend.h`, replace the class body:

```cpp
class InternalFsBackend final : public StorageBackend {
 public:
  bool begin() override;
  StorageIoResult read(const char* path,
                       uint8_t* output,
                       size_t capacity,
                       size_t& length) override;
  bool beginWrite(const char* path, const uint8_t* data,
                  size_t length) override;
  AsyncWriteStatus pollWrite() override;

 private:
  bool mounted_ = false;
  bool write_pending_ = false;
  AsyncWriteStatus pending_result_ = AsyncWriteStatus::kIdle;
};
```

In `firmware/src/internal_fs_backend.cpp`, replace the `write()` method (lines 38-49) with:

```cpp
bool InternalFsBackend::beginWrite(const char* path, const uint8_t* data,
                                   size_t length) {
  if (write_pending_) return false;
  // Task 1 keeps this fully synchronous, matching the old write() exactly --
  // the real two-poll remove/write split lands in Task 3, touching only
  // this method's body, not this file's interface shape.
  if (InternalFS.exists(path) && !InternalFS.remove(path)) {
    pending_result_ = AsyncWriteStatus::kError;
    write_pending_ = true;
    return true;
  }
  File file(path, FILE_O_WRITE, InternalFS);
  if (!file) {
    pending_result_ = AsyncWriteStatus::kError;
    write_pending_ = true;
    return true;
  }
  const size_t bytes_written = file.write(data, length);
  file.flush();
  const bool complete = bytes_written == length && file.size() == length;
  file.close();
  pending_result_ = complete ? AsyncWriteStatus::kOk : AsyncWriteStatus::kError;
  write_pending_ = true;
  return true;
}

AsyncWriteStatus InternalFsBackend::pollWrite() {
  if (!write_pending_) return AsyncWriteStatus::kIdle;
  write_pending_ = false;
  return pending_result_;
}
```

- [ ] **Step 6: Run the test to verify it passes**

Run: `cd firmware && pio test -e native -f test_memory_backend_async_write_completes_after_configured_polls`
Expected: PASS.

- [ ] **Step 7: Run the full native suite — must be zero regressions**

Run: `cd firmware && pio test -e native`
Expected: all tests pass (133/133 baseline + 1 new = 134/134). `StorageManager` still calls `backend_.write(...)`, which no longer exists — **this step is expected to reveal a compile error** naming every call site in `firmware/lib/domain/storage_manager.cpp` that still references `backend_.write`. Do not fix these yet; Task 2 does that. If Step 7 does NOT show a compile error here, something in Step 3 was skipped.

Actually run it now to capture the exact error text, then proceed — Task 2 starts from this known-broken state and fixes it as its first step.

- [ ] **Step 8: Build the embedded profile to confirm `InternalFsBackend` compiles standalone**

`InternalFsBackend` itself is now interface-complete, but `AppController`'s `storage_` member (`StorageManager storage_;`, constructed with `storage_backend_`) will fail to link for the same reason as Step 7 until Task 2 fixes `StorageManager`. Skip a full `pio run` here — it will fail for the same known reason. Task 2's Step 8 (below) is the first point a full build succeeds again.

- [ ] **Step 9: Commit**

```bash
git add firmware/lib/domain/storage_manager.h firmware/src/internal_fs_backend.h firmware/src/internal_fs_backend.cpp firmware/test/test_native/test_main.cpp
git commit -m "feat(firmware): reshape StorageBackend into an async beginWrite/pollWrite contract"
```

Note: this commit intentionally leaves `firmware/lib/domain/storage_manager.cpp` referencing the now-removed `backend_.write(...)`, so `pio test -e native` and any embedded build will fail until Task 2 lands. This is a deliberate two-commit sequence within one logical interface migration — Task 2 must be executed immediately after, before treating the branch as buildable.

---

### Task 2: `StorageManager` async primitive + synchronous wrappers

**Files:**
- Modify: `firmware/lib/domain/storage_manager.h:118-184` (`StorageCounters` onward — add async surface)
- Modify: `firmware/lib/domain/storage_manager.cpp:375-646` (`writeSlot` through `saveStorageCounters`)
- Test: `firmware/test/test_native/test_main.cpp` (new async-logic tests)

**Interfaces:**
- Consumes: `StorageBackend::beginWrite`/`pollWrite` (Task 1).
- Produces: `StorageManager::beginSaveOdometer(const OdometerData&) -> bool`, `beginSaveAmbientCalibration(const AmbientCalibrationData&) -> bool`, `beginSaveStorageCounters() -> bool`, `pollSave() -> StorageAsyncStatus`, `drainPendingSave() -> StorageAsyncStatus`, `saveInProgress() const -> bool` — consumed by Task 4's `AppController` rewrite. Existing `saveConfig`/`saveOdometer`/`saveAmbientCalibration`/`saveStorageCounters`/`loadConfig`/`loadOdometer` keep their exact existing signatures and synchronous behavior.

- [ ] **Step 1: Write the failing tests**

Add to `firmware/test/test_native/test_main.cpp`, right after `test_memory_backend_async_write_completes_after_configured_polls` (from Task 1):

```cpp
void test_storage_async_save_completes_over_multiple_polls() {
  MemoryStorageBackend backend;
  StorageManager storage(backend);
  TEST_ASSERT_TRUE(storage.begin());

  backend.polls_to_complete = 2;
  OdometerData data{1234u, 5u};
  TEST_ASSERT_TRUE(storage.beginSaveOdometer(data));
  TEST_ASSERT_TRUE(storage.saveInProgress());
  TEST_ASSERT_EQUAL(StorageAsyncStatus::kInProgress, storage.pollSave());
  TEST_ASSERT_TRUE(storage.saveInProgress());
  TEST_ASSERT_EQUAL(StorageAsyncStatus::kOk, storage.pollSave());
  TEST_ASSERT_FALSE(storage.saveInProgress());
  TEST_ASSERT_EQUAL_UINT32(1u, storage.counters().writes);
  TEST_ASSERT_EQUAL_UINT32(1u, storage.lastOdometerSequence());
}

void test_storage_async_save_serializes_concurrent_begin() {
  MemoryStorageBackend backend;
  StorageManager storage(backend);
  TEST_ASSERT_TRUE(storage.begin());

  backend.polls_to_complete = 2;
  OdometerData odometer{1000u, 1u};
  TEST_ASSERT_TRUE(storage.beginSaveOdometer(odometer));
  TEST_ASSERT_TRUE(storage.saveInProgress());

  AmbientCalibrationData ambient{266u, 1126u, 1u};
  TEST_ASSERT_FALSE(storage.beginSaveAmbientCalibration(ambient));

  TEST_ASSERT_EQUAL(StorageAsyncStatus::kOk, storage.drainPendingSave());
  TEST_ASSERT_TRUE(storage.beginSaveAmbientCalibration(ambient));
}

void test_storage_async_drain_pending_save_blocks_to_completion() {
  MemoryStorageBackend backend;
  StorageManager storage(backend);
  TEST_ASSERT_TRUE(storage.begin());

  backend.polls_to_complete = 2;
  TEST_ASSERT_TRUE(storage.beginSaveStorageCounters());
  TEST_ASSERT_TRUE(storage.saveInProgress());
  TEST_ASSERT_EQUAL(StorageAsyncStatus::kOk, storage.drainPendingSave());
  TEST_ASSERT_FALSE(storage.saveInProgress());
  TEST_ASSERT_EQUAL_UINT32(1u, backend.files.count("/cnt_a"));
}

void test_storage_async_dedup_skip_completes_synchronously() {
  MemoryStorageBackend backend;
  StorageManager storage(backend);
  TEST_ASSERT_TRUE(storage.begin());

  backend.polls_to_complete = 2;
  AmbientCalibrationData data{266u, 1126u, 1u};
  TEST_ASSERT_TRUE(storage.beginSaveAmbientCalibration(data));
  TEST_ASSERT_EQUAL(StorageAsyncStatus::kOk, storage.drainPendingSave());

  const uint32_t writes_before = storage.counters().writes;
  const uint32_t skipped_before = storage.counters().skipped_writes;
  TEST_ASSERT_TRUE(storage.beginSaveAmbientCalibration(data));
  TEST_ASSERT_FALSE(storage.saveInProgress());
  TEST_ASSERT_EQUAL_UINT32(writes_before, storage.counters().writes);
  TEST_ASSERT_EQUAL_UINT32(skipped_before + 1u, storage.counters().skipped_writes);
}

void test_storage_async_write_error_propagates_through_poll() {
  MemoryStorageBackend backend;
  StorageManager storage(backend);
  TEST_ASSERT_TRUE(storage.begin());

  backend.polls_to_complete = 2;
  backend.write_ok = false;
  TEST_ASSERT_TRUE(storage.beginSaveStorageCounters());
  TEST_ASSERT_EQUAL(StorageAsyncStatus::kError, storage.drainPendingSave());
  TEST_ASSERT_EQUAL_UINT32(1u, storage.counters().write_errors);
}
```

Register all five right after `RUN_TEST(test_memory_backend_async_write_completes_after_configured_polls);` (from Task 1):

```cpp
  RUN_TEST(test_storage_async_save_completes_over_multiple_polls);
  RUN_TEST(test_storage_async_save_serializes_concurrent_begin);
  RUN_TEST(test_storage_async_drain_pending_save_blocks_to_completion);
  RUN_TEST(test_storage_async_dedup_skip_completes_synchronously);
  RUN_TEST(test_storage_async_write_error_propagates_through_poll);
```

- [ ] **Step 2: Run the tests to verify they fail**

Run: `cd firmware && pio test -e native -f "test_storage_async_*"`
Expected: build FAILS — `beginSaveOdometer`, `beginSaveAmbientCalibration`, `beginSaveStorageCounters`, `pollSave`, `drainPendingSave`, `saveInProgress`, `StorageAsyncStatus` don't exist yet, and `storage_manager.cpp` still references the removed `backend_.write(...)` from Task 1.

- [ ] **Step 3: Add `StorageAsyncStatus` and the new declarations to the header**

In `firmware/lib/domain/storage_manager.h`, after `AsyncWriteStatus` (added in Task 1, right before `class StorageBackend`), add:

```cpp
enum class StorageAsyncStatus : uint8_t {
  kIdle,
  kInProgress,
  kOk,
  kError,
};
```

Add public method declarations, right after `bool saveStorageCounters();` (in the current header, near line 151):

```cpp
  bool beginSaveOdometer(const OdometerData& odometer);
  bool beginSaveAmbientCalibration(const AmbientCalibrationData& calibration);
  bool beginSaveStorageCounters();
  StorageAsyncStatus pollSave();
  StorageAsyncStatus drainPendingSave();
  bool saveInProgress() const { return save_in_progress_; }
```

Add private method declarations, right after `void loadStorageCounters();` (near line 165):

```cpp
  bool beginWriteSlotAsync(const char* path,
                           const uint8_t* payload,
                           size_t payload_length,
                           uint16_t version,
                           uint32_t sequence);
  bool beginSavePayloadAsync(const char* path_a,
                             const char* path_b,
                             const uint8_t* payload,
                             size_t payload_length,
                             uint16_t version,
                             PayloadKind kind,
                             bool force_write);
```

Add private state members, right after `uint32_t last_odometer_sequence_ = 0;` (near line 183):

```cpp
  bool save_in_progress_ = false;
  bool pending_is_odometer_ = false;
  uint32_t pending_odometer_sequence_ = 0;
```

- [ ] **Step 4: Implement the async primitive in the .cpp**

In `firmware/lib/domain/storage_manager.cpp`, replace `writeSlot` (lines 375-389) with:

```cpp
bool StorageManager::beginWriteSlotAsync(const char* path,
                                         const uint8_t* payload,
                                         size_t payload_length,
                                         uint16_t version,
                                         uint32_t sequence) {
  uint8_t record[kMaximumRecordSize];
  const size_t record_length = encodeRecord(payload, payload_length, version,
                                            sequence, record, sizeof(record));
  if (record_length == 0 || !backend_.beginWrite(path, record, record_length)) {
    ++counters_.write_errors;
    return false;
  }
  save_in_progress_ = true;
  return true;
}

bool StorageManager::writeSlot(const char* path,
                               const uint8_t* payload,
                               size_t payload_length,
                               uint16_t version,
                               uint32_t sequence) {
  if (!beginWriteSlotAsync(path, payload, payload_length, version, sequence)) {
    return false;
  }
  return drainPendingSave() == StorageAsyncStatus::kOk;
}

StorageAsyncStatus StorageManager::pollSave() {
  if (!save_in_progress_) return StorageAsyncStatus::kIdle;
  const AsyncWriteStatus status = backend_.pollWrite();
  if (status == AsyncWriteStatus::kInProgress) return StorageAsyncStatus::kInProgress;
  save_in_progress_ = false;
  if (status == AsyncWriteStatus::kOk) {
    ++counters_.writes;
    if (pending_is_odometer_) last_odometer_sequence_ = pending_odometer_sequence_;
    return StorageAsyncStatus::kOk;
  }
  ++counters_.write_errors;
  return StorageAsyncStatus::kError;
}

StorageAsyncStatus StorageManager::drainPendingSave() {
  constexpr int kMaxDrainPolls = 8;  // the real state machine completes in <=2
  for (int i = 0; i < kMaxDrainPolls && save_in_progress_; ++i) {
    const StorageAsyncStatus status = pollSave();
    if (status != StorageAsyncStatus::kInProgress) return status;
  }
  return save_in_progress_ ? StorageAsyncStatus::kError : StorageAsyncStatus::kIdle;
}
```

Replace `savePayload` (lines 391-446) with:

```cpp
bool StorageManager::beginSavePayloadAsync(const char* path_a,
                                           const char* path_b,
                                           const uint8_t* payload,
                                           size_t payload_length,
                                           uint16_t version,
                                           PayloadKind kind,
                                           bool force_write) {
  if (!mounted_ || save_in_progress_) return false;
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
    case PayloadKind::kStorageCounters:
      readStorageCountersSlot(path_a, a);
      readStorageCountersSlot(path_b, b);
      break;
  }

  const Slot* newest = nullptr;
  if (a.valid && b.valid) {
    newest = isNewer(b.sequence, a.sequence) ? &b : &a;
  } else if (a.valid) {
    newest = &a;
  } else if (b.valid) {
    newest = &b;
  }
  if (!force_write && newest != nullptr &&
      memcmp(newest->payload, payload, payload_length) == 0 &&
      !newest->needs_migration) {
    ++counters_.skipped_writes;
    return true;
  }

  const char* target = path_a;
  uint32_t sequence = 1;
  if (newest != nullptr) sequence = newest->sequence + 1u;
  if (a.valid && !b.valid) {
    target = path_b;
  } else if (!a.valid && b.valid) {
    target = path_a;
  } else if (a.valid && b.valid) {
    target = isNewer(b.sequence, a.sequence) ? path_a : path_b;
  }
  pending_is_odometer_ = (kind == PayloadKind::kOdometer);
  pending_odometer_sequence_ = sequence;
  return beginWriteSlotAsync(target, payload, payload_length, version, sequence);
}

bool StorageManager::savePayload(const char* path_a,
                                 const char* path_b,
                                 const uint8_t* payload,
                                 size_t payload_length,
                                 uint16_t version,
                                 PayloadKind kind,
                                 bool force_write) {
  if (!beginSavePayloadAsync(path_a, path_b, payload, payload_length, version,
                             kind, force_write)) {
    return false;
  }
  if (!save_in_progress_) return true;  // dedup-skip completed synchronously
  return drainPendingSave() == StorageAsyncStatus::kOk;
}
```

Note `writeSlot`/`savePayload` keep their exact original signatures and are called unchanged by `loadConfig`/`loadOdometer` (defaults-write, migration-write) — no other lines in this file need to change for those callers.

- [ ] **Step 5: Add the public async entry points and rewire the sync wrappers**

Replace `saveOdometer` (lines 569-585) with:

```cpp
bool StorageManager::beginSaveOdometer(const OdometerData& odometer) {
  uint8_t payload[kOdometerPayloadSize];
  encodeOdometer(odometer, payload);
  return beginSavePayloadAsync(paths_.odometer_a, paths_.odometer_b, payload,
                               sizeof(payload), kOdometerRecordVersion,
                               PayloadKind::kOdometer, false);
}

bool StorageManager::saveOdometer(const OdometerData& odometer) {
  if (!beginSaveOdometer(odometer)) return false;
  if (!save_in_progress_) return true;
  return drainPendingSave() == StorageAsyncStatus::kOk;
}
```

Replace `saveAmbientCalibration` (lines 629-635) with:

```cpp
bool StorageManager::beginSaveAmbientCalibration(
    const AmbientCalibrationData& calibration) {
  uint8_t payload[kAmbientCalibrationPayloadSize];
  encodeAmbientCalibration(calibration, payload);
  return beginSavePayloadAsync(paths_.ambient_calibration_a,
                               paths_.ambient_calibration_b, payload,
                               sizeof(payload), kAmbientCalibrationRecordVersion,
                               PayloadKind::kAmbientCalibration, false);
}

bool StorageManager::saveAmbientCalibration(
    const AmbientCalibrationData& calibration) {
  if (!beginSaveAmbientCalibration(calibration)) return false;
  if (!save_in_progress_) return true;
  return drainPendingSave() == StorageAsyncStatus::kOk;
}
```

Replace `saveStorageCounters` (lines 637-646) with:

```cpp
bool StorageManager::beginSaveStorageCounters() {
  // Snapshot before encoding: the eventual pollSave() completion will bump
  // counters_.writes as a side effect of the save itself, which must not
  // leak into this payload (it belongs to the *next* save).
  uint8_t payload[kStorageCountersPayloadSize];
  encodeStorageCounters(counters_, payload);
  return beginSavePayloadAsync(paths_.counters_a, paths_.counters_b, payload,
                               sizeof(payload), kStorageCountersRecordVersion,
                               PayloadKind::kStorageCounters, false);
}

bool StorageManager::saveStorageCounters() {
  if (!beginSaveStorageCounters()) return false;
  if (!save_in_progress_) return true;
  return drainPendingSave() == StorageAsyncStatus::kOk;
}
```

`saveConfig` (lines 502-511) is unchanged — it already calls `savePayload`, which is now internally async-backed but externally identical.

- [ ] **Step 6: Run the new tests to verify they pass**

Run: `cd firmware && pio test -e native -f "test_storage_async_*"`
Expected: all 5 PASS.

- [ ] **Step 7: Run the full native suite — zero regressions expected**

Run: `cd firmware && pio test -e native`
Expected: all tests pass (133 baseline + 1 from Task 1 + 5 new = 139/139). Every existing test that calls `saveConfig`/`saveOdometer`/`saveAmbientCalibration`/`saveStorageCounters`/`loadConfig`/`loadOdometer` synchronously must pass with **zero edits** to those tests. If any existing test needed a change to pass, stop — that indicates a behavior change leaked into a synchronous caller, which violates this plan's core constraint.

- [ ] **Step 8: Build all three embedded profiles — first point they succeed since Task 1**

Run: `cd firmware && pio run -e xiao_ble_sense`
Expected: SUCCESS.

Run: `cd firmware && pio run -e xiao_ble_sense_128x32`
Expected: SUCCESS.

Run: `cd firmware && pio run -e xiao_ble_sense_deep_sleep`
Expected: SUCCESS.

- [ ] **Step 9: Commit**

```bash
git add firmware/lib/domain/storage_manager.h firmware/lib/domain/storage_manager.cpp firmware/test/test_native/test_main.cpp
git commit -m "feat(firmware): add StorageManager async save primitive behind existing sync API"
```

---

### Task 3: `InternalFsBackend` — real two-poll split

**Files:**
- Modify: `firmware/src/internal_fs_backend.h`, `firmware/src/internal_fs_backend.cpp`

**Interfaces:**
- Consumes/produces: same `beginWrite`/`pollWrite` signatures established in Task 1 — this task only changes the internal state machine, not the interface shape. No other file changes.

- [ ] **Step 1: Replace the behavior-preserving shim with the real split**

In `firmware/src/internal_fs_backend.h`, replace the private members:

```cpp
 private:
  enum class AsyncWriteState : uint8_t { kIdle, kPendingRemove, kPendingWrite };

  bool mounted_ = false;
  AsyncWriteState write_state_ = AsyncWriteState::kIdle;
  const char* pending_path_ = nullptr;
  uint8_t pending_data_[kMaximumRecordSize] = {};
  size_t pending_length_ = 0;
```

In `firmware/src/internal_fs_backend.cpp`, replace `beginWrite`/`pollWrite` (added in Task 1) with:

```cpp
bool InternalFsBackend::beginWrite(const char* path, const uint8_t* data,
                                   size_t length) {
  if (write_state_ != AsyncWriteState::kIdle) return false;
  if (length > sizeof(pending_data_)) return false;
  memcpy(pending_data_, data, length);
  pending_length_ = length;
  pending_path_ = path;
  write_state_ = AsyncWriteState::kPendingRemove;
  return true;
}

AsyncWriteStatus InternalFsBackend::pollWrite() {
  switch (write_state_) {
    case AsyncWriteState::kIdle:
      return AsyncWriteStatus::kIdle;
    case AsyncWriteState::kPendingRemove: {
      if (InternalFS.exists(pending_path_) && !InternalFS.remove(pending_path_)) {
        write_state_ = AsyncWriteState::kIdle;
        return AsyncWriteStatus::kError;
      }
      write_state_ = AsyncWriteState::kPendingWrite;
      return AsyncWriteStatus::kInProgress;
    }
    case AsyncWriteState::kPendingWrite: {
      write_state_ = AsyncWriteState::kIdle;
      File file(pending_path_, FILE_O_WRITE, InternalFS);
      if (!file) return AsyncWriteStatus::kError;
      const size_t bytes_written = file.write(pending_data_, pending_length_);
      file.flush();
      const bool complete =
          bytes_written == pending_length_ && file.size() == pending_length_;
      file.close();
      return complete ? AsyncWriteStatus::kOk : AsyncWriteStatus::kError;
    }
  }
  return AsyncWriteStatus::kError;  // unreachable
}
```

`pending_path_` holds the raw `const char*` passed in, not a copy — every real caller (`StorageManager::beginWriteSlotAsync`) passes one of `StoragePaths`' `const char*` members, which are string literals with static storage duration, so this is safe without a copy.

- [ ] **Step 2: Build all three embedded profiles**

This file has no native coverage (embedded-only); verify via build success.

Run: `cd firmware && pio run -e xiao_ble_sense`
Expected: SUCCESS.

Run: `cd firmware && pio run -e xiao_ble_sense_128x32`
Expected: SUCCESS.

Run: `cd firmware && pio run -e xiao_ble_sense_deep_sleep`
Expected: SUCCESS.

- [ ] **Step 3: Run the native suite as a final regression pass**

Run: `cd firmware && pio test -e native`
Expected: 139/139 pass (unchanged from Task 2 — this file isn't part of `env:native`).

- [ ] **Step 4: Commit**

```bash
git add firmware/src/internal_fs_backend.h firmware/src/internal_fs_backend.cpp
git commit -m "feat(firmware): split InternalFsBackend's flash write across two polls"
```

---

### Task 4: `AppController` — async save checkpoints

**Files:**
- Modify: `firmware/src/app_controller.h:36-41,57-62,131-132` (task callback decl, persist* decls, member additions)
- Modify: `firmware/src/app_controller.cpp:62-90,142-155,697-735,1109-1123,1398-1487,1768-1786` (constants, task array, deep sleep, both reboot handlers, persist* rewrite)

**Interfaces:**
- Consumes: `StorageManager::beginSaveOdometer`/`beginSaveAmbientCalibration`/`beginSaveStorageCounters`/`pollSave`/`drainPendingSave`/`saveInProgress` (Tasks 2-3).

- [ ] **Step 1: Declare the new task callback, helpers, and member state**

In `firmware/src/app_controller.h`, add a new static task callback after `static void bleTask(void* context, uint32_t now_ms);` (line 41):

```cpp
  static void storageTask(void* context, uint32_t now_ms);
```

Add its instance-method counterpart right after the new `drainStorageSave()` declaration you're about to add below:

```cpp
  void pollStorageSave();
```

Change `bool persistOdometer(OdometerSaveTrigger trigger);` (line 58) and the `persistAmbientCalibration` declaration (lines 60-61) to also declare the new completion/drain helpers, replacing lines 57-62:

```cpp
  void maybePersistOdometer(uint32_t now_ms);
  bool persistOdometer(OdometerSaveTrigger trigger);
  void maybePersistAmbientCalibration(uint32_t now_ms);
  bool persistAmbientCalibration(AmbientCalibrationSaveTrigger trigger,
                                 uint32_t now_ms);
  void persistStorageCounters();
  void completeOdometerSave(bool ok);
  void completeAmbientCalibrationSave(bool ok);
  void completePendingStorageSave(bool ok);
  bool drainStorageSave();
```

Change `ScheduledTask tasks_[6];` (line 131) to:

```cpp
  ScheduledTask tasks_[7];
```

Add new private member state, right after `uint32_t reboot_requested_ms_ = 0;` (line 140):

```cpp
  enum class PendingStorageSave : uint8_t {
    kNone,
    kOdometer,
    kAmbientCalibration,
    kStorageCounters,
  };
  PendingStorageSave pending_storage_save_ = PendingStorageSave::kNone;
  OdometerSaveTrigger pending_odometer_trigger_ = OdometerSaveTrigger::kNone;
  uint64_t pending_odometer_mm_ = 0;
  AmbientCalibrationSaveTrigger pending_ambient_trigger_ =
      AmbientCalibrationSaveTrigger::kNone;
  uint32_t pending_ambient_now_ms_ = 0;
  uint16_t pending_ambient_raw_dark_ = 0;
  uint16_t pending_ambient_raw_bright_ = 0;
  AmbientCalibrationQuality pending_ambient_quality_ =
      AmbientCalibrationQuality::kNarrow;
  bool last_storage_save_ok_ = true;
```

`AmbientCalibrationQuality::kNarrow = 0` is confirmed as the first enumerator in `firmware/lib/domain/ambient_light_calibrator.h:13-16` (the only other value is `kOk = 1`) — the default above is correct as written.

- [ ] **Step 2: Add the storage task budget and register the 7th scheduler task**

In `firmware/src/app_controller.cpp`, replace the budget comment block and constants (lines 69-83):

```cpp
// Scheduler::run() overrun budgets, in microseconds. These are diagnostic
// thresholds surfaced via the "sched" Serial command, not enforced limits --
// the watchdog is what actually protects against a wedged loop. A typical
// internal-flash page erase is ~85ms. Only the "storage" task actually
// performs flash I/O now (StorageManager::pollSave() drives
// InternalFsBackend's two-poll remove/write split); updateState/
// updateAmbient/updateDisplay/updateBattery/updateBle only ever *start* an
// async save (beginSaveX: fast reads + a memcpy, no flash write), so their
// budgets below are now more headroom than their non-flash work needs --
// kept generous pending a real hardware sched measurement rather than
// tightened here.
constexpr uint32_t kSchedPulsesBudgetUs = 2000u;      // ISR ring drain only.
constexpr uint32_t kSchedStateBudgetUs = 150000u;     // trip calc + begin an async save.
constexpr uint32_t kSchedAmbientBudgetUs = 150000u;   // ADC avg + begin an async save.
constexpr uint32_t kSchedBatteryBudgetUs = 150000u;   // ADC read only.
constexpr uint32_t kSchedDisplayBudgetUs = 150000u;   // I2C sendBuffer only.
constexpr uint32_t kSchedBleBudgetUs = 300000u;       // up to 3 chained async-save starts.
constexpr uint32_t kSchedStorageBudgetUs = 150000u;   // one poll of the in-flight save (<=1 flash op).
```

Add a task index constant next to the others (near `constexpr size_t kTaskBle = 5;`):

```cpp
constexpr size_t kTaskStorage = 6;
```

In the constructor (lines 149-155), add the 7th task entry and update the count:

```cpp
      tasks_{{"pulses", 0, 0, pulseTask, this, 0, kSchedPulsesBudgetUs},
             {"state", 100, 0, stateTask, this, 0, kSchedStateBudgetUs},
             {"ambient", 10, 0, ambientTask, this, 0, kSchedAmbientBudgetUs},
             {"battery", 1000, 0, batteryTask, this, 0, kSchedBatteryBudgetUs},
             {"display", 50, 0, displayTask, this, 0, kSchedDisplayBudgetUs},
             {"ble", 100, 0, bleTask, this, 0, kSchedBleBudgetUs},
             {"storage", 50, 0, storageTask, this, 0, kSchedStorageBudgetUs}},
      scheduler_(tasks_, 7, schedulerMicros) {}
```

The `"storage"` task's period (50ms) is fixed and does not vary between normal/low-power modes — it is intentionally not added to `applySchedulerPeriods` (`firmware/lib/domain/power_manager.h`'s `SchedulerPeriods`/`kNormalSchedulerPeriods`/`kLowPowerSchedulerPeriods`): a pending save should complete promptly regardless of ride state, since deep-sleep entry depends on `drainStorageSave()` eventually catching up.

- [ ] **Step 3: Implement `storageTask`, the completion helpers, and `drainStorageSave`**

Find `void AppController::bleTask(void* context, uint32_t now_ms)` in `firmware/src/app_controller.cpp` and add, right after its closing `}`:

```cpp
void AppController::storageTask(void* context, uint32_t now_ms) {
  (void)now_ms;
  static_cast<AppController*>(context)->pollStorageSave();
}
```

Implement `pollStorageSave()` (declared in Step 1) and the completion helpers. Find `void AppController::persistStorageCounters()` in `firmware/src/app_controller.cpp` — its current full body is:

```cpp
void AppController::persistStorageCounters() {
  if (!storage_.mounted()) return;
  const bool ok = storage_.saveStorageCounters();
  Serial.print("Counters save: result=");
  Serial.println(ok ? "OK" : "ERROR");
}
```

Leave this function's body as-is for now (Step 6 below replaces it) and add the new functions right after its closing `}`:

```cpp
void AppController::pollStorageSave() {
  if (pending_storage_save_ == PendingStorageSave::kNone) return;
  const StorageAsyncStatus status = storage_.pollSave();
  if (status == StorageAsyncStatus::kInProgress) return;
  completePendingStorageSave(status == StorageAsyncStatus::kOk);
}

void AppController::completePendingStorageSave(bool ok) {
  switch (pending_storage_save_) {
    case PendingStorageSave::kOdometer:
      completeOdometerSave(ok);
      break;
    case PendingStorageSave::kAmbientCalibration:
      completeAmbientCalibrationSave(ok);
      break;
    case PendingStorageSave::kStorageCounters:
      last_storage_save_ok_ = ok;
      Serial.print("Counters save: result=");
      Serial.println(ok ? "OK" : "ERROR");
      break;
    case PendingStorageSave::kNone:
      break;
  }
  pending_storage_save_ = PendingStorageSave::kNone;
}

bool AppController::drainStorageSave() {
  if (storage_.saveInProgress()) {
    const StorageAsyncStatus status = storage_.drainPendingSave();
    completePendingStorageSave(status == StorageAsyncStatus::kOk);
  }
  return last_storage_save_ok_;
}
```

- [ ] **Step 4: Rewrite `persistOdometer`**

Find `bool AppController::persistOdometer(OdometerSaveTrigger trigger)` and replace its entire body:

```cpp
bool AppController::persistOdometer(OdometerSaveTrigger trigger) {
  if (storage_.saveInProgress()) return false;
  const uint64_t odometer_mm = trip_computer_.snapshot().odometer_mm;
  pending_odometer_trigger_ = trigger;
  pending_odometer_mm_ = odometer_mm;

  OdometerData data;
  data.odometer_mm = odometer_mm;
  data.total_revolutions = trip_computer_.totalRevolutions();
  if (!storage_.mounted() || !storage_.beginSaveOdometer(data)) {
    completeOdometerSave(false);
    return false;
  }
  if (!storage_.saveInProgress()) {
    completeOdometerSave(true);
    return true;
  }
  pending_storage_save_ = PendingStorageSave::kOdometer;
  return true;
}

void AppController::completeOdometerSave(bool ok) {
  last_storage_save_ok_ = ok;
  if (ok) {
    odometer_save_.markSaved(pending_odometer_mm_);
    odometer_save_.acknowledge(pending_odometer_trigger_);
  } else {
    ble_.recordError(ErrorLogCode::kFlashError, ErrorLogSeverity::kError,
                     static_cast<uint16_t>(pending_odometer_trigger_), millis());
  }
  Serial.print("Odo save: trigger=");
  Serial.print(odometerSaveTriggerName(pending_odometer_trigger_));
  Serial.print(", result=");
  Serial.print(ok ? "OK" : "ERROR");
  Serial.print(", sequence=");
  Serial.print(storage_.lastOdometerSequence());
  Serial.print(", writes=");
  Serial.print(storage_.counters().writes);
  Serial.print(", skipped=");
  Serial.print(storage_.counters().skipped_writes);
  Serial.print(", write_errors=");
  Serial.println(storage_.counters().write_errors);
}
```

`odometerSaveTriggerName` is declared in `firmware/lib/domain/odometer_save_policy.h:70` and already used by the current `persistOdometer` at `firmware/src/app_controller.cpp:1425` — the log line above is copied verbatim from that current implementation, field order and wording included.

- [ ] **Step 5: Rewrite `persistAmbientCalibration`**

Find `bool AppController::persistAmbientCalibration(AmbientCalibrationSaveTrigger trigger, uint32_t now_ms)` and replace its entire body:

```cpp
bool AppController::persistAmbientCalibration(AmbientCalibrationSaveTrigger trigger,
                                              uint32_t now_ms) {
  if (storage_.saveInProgress()) return false;
  const AmbientLightCalibrator& calibrator = ambient_light_.calibration();
  AmbientCalibrationData data;
  data.raw_dark = calibrator.rawDark();
  data.raw_bright = calibrator.rawBright();
  data.quality = static_cast<uint8_t>(calibrator.quality());

  pending_ambient_trigger_ = trigger;
  pending_ambient_now_ms_ = now_ms;
  pending_ambient_raw_dark_ = data.raw_dark;
  pending_ambient_raw_bright_ = data.raw_bright;
  pending_ambient_quality_ = calibrator.quality();

  if (!storage_.mounted() || !storage_.beginSaveAmbientCalibration(data)) {
    completeAmbientCalibrationSave(false);
    return false;
  }
  if (!storage_.saveInProgress()) {
    completeAmbientCalibrationSave(true);
    return true;
  }
  pending_storage_save_ = PendingStorageSave::kAmbientCalibration;
  return true;
}

void AppController::completeAmbientCalibrationSave(bool ok) {
  last_storage_save_ok_ = ok;
  if (ok) {
    ambient_light_.markCalibrationPersisted();
    ambient_calibration_save_.acknowledge(pending_ambient_trigger_,
                                          pending_ambient_now_ms_);
  }
  Serial.print("Ambient cal save: trigger=");
  Serial.print(ambientCalibrationSaveTriggerName(pending_ambient_trigger_));
  Serial.print(", result=");
  Serial.print(ok ? "OK" : "ERROR");
  Serial.print(", raw_dark=");
  Serial.print(pending_ambient_raw_dark_);
  Serial.print(", raw_bright=");
  Serial.print(pending_ambient_raw_bright_);
  Serial.print(", quality=");
  Serial.println(ambientCalibrationQualityName(pending_ambient_quality_));
}
```

`ambientCalibrationSaveTriggerName`/`ambientCalibrationQualityName` are declared in `firmware/lib/domain/ambient_calibration_save_policy.h:37` and `firmware/lib/domain/ambient_light_calibrator.h:35`, already used by the current `persistAmbientCalibration` at `firmware/src/app_controller.cpp:1462,1470` — the log line above is copied verbatim from that current implementation.

- [ ] **Step 6: Rewrite `persistStorageCounters`**

Replace its body (already partially covered in Step 3's edit — make sure the final function looks exactly like this, with no leftover old body):

```cpp
void AppController::persistStorageCounters() {
  if (!storage_.mounted() || storage_.saveInProgress()) return;
  if (!storage_.beginSaveStorageCounters()) {
    Serial.println("Counters save: result=ERROR");
    return;
  }
  if (!storage_.saveInProgress()) {
    Serial.println("Counters save: result=OK");
    return;
  }
  pending_storage_save_ = PendingStorageSave::kStorageCounters;
}
```

- [ ] **Step 7: Add `drainStorageSave()` calls at the deep-sleep checkpoint**

Find `void AppController::tryEnterDeepSleep(uint32_t now_ms)` and replace its body (inside the `#if defined(BIKECOMP_FEATURE_DEEP_SLEEP)` branch) with:

```cpp
  if (!config_.deep_sleep_enabled || power_manager_.sleepBlocked()) return;

  // Drain anything still in flight from handlePowerManagerResult's earlier
  // request_deep_sleep_save branch this same tick, so the checks below see
  // an accurate storage_.saveInProgress() and persistOdometer isn't
  // rejected by a save that isn't actually "in the way" anymore.
  drainStorageSave();

  odometer_save_.requestDeepSleepSave();
  const uint64_t odometer_mm = trip_computer_.snapshot().odometer_mm;
  const OdometerSaveTrigger trigger =
      odometer_save_.evaluate(odometer_mm, now_ms);
  if (trigger != OdometerSaveTrigger::kNone) {
    if (!persistOdometer(trigger)) return;
    drainStorageSave();
  }
  ambient_calibration_save_.requestDeepSleepSave();
  maybePersistAmbientCalibration(now_ms);
  drainStorageSave();

  display_.turnOff(now_ms);
  ble_.stopAdvertising();
  wheel_sensor_.suspendInterrupt();

  if (digitalRead(kHallSensePin) == LOW) {
    Serial.println("WARN deep-sleep: hall already LOW; wake on magnet release");
  }

  const bool sense_low = config_.active_edge != 1u;
  // Re-flush counters here: the persistOdometer/maybePersistAmbientCalibration
  // calls above (and the earlier snapshot in handlePowerManagerResult) can
  // still bump counters_.writes/skipped_writes/write_errors, and this is the
  // last chance to capture that before power-off -- drain it fully since an
  // async write left in flight would otherwise be silently lost to System OFF.
  persistStorageCounters();
  drainStorageSave();
  if (kWatchdogEnabled) watchdogFeed();
  deepSleepPrepareAndEnter(kHallSenseNrfGpio, sense_low);
```

Leave the `#else (void)now_ms; #endif` and everything outside this branch untouched. `handlePowerManagerResult`'s own `request_deep_sleep_save` branch (which calls `maybePersistOdometer`/`maybePersistAmbientCalibration`/`persistStorageCounters` before `tryEnterDeepSleep` runs) needs **no changes** — it's a proactive early attempt only; `tryEnterDeepSleep`'s sequence above is what actually guarantees completion before power-off.

- [ ] **Step 8: Add `drainStorageSave()` calls at both reboot checkpoints**

Find `case SerialCommand::kReboot:` and replace its body:

```cpp
      case SerialCommand::kReboot: {
        drainStorageSave();
        odometer_save_.requestRebootSave();
        const bool started = persistOdometer(OdometerSaveTrigger::kReboot);
        const bool odometer_saved = started && drainStorageSave();
        persistStorageCounters();
        drainStorageSave();
        if (!odometer_saved) {
          Serial.println("ERROR reboot storage");
        } else {
          reboot_pending_ = true;
          reboot_requested_ms_ = now_ms;
          Serial.println("OK reboot");
        }
        break;
      }
```

Find `case CommandId::kReboot:` and replace its body:

```cpp
    case CommandId::kReboot: {
      drainStorageSave();
      odometer_save_.requestRebootSave();
      const bool started = persistOdometer(OdometerSaveTrigger::kReboot);
      const bool odometer_saved = started && drainStorageSave();
      if (!odometer_saved) {
        result.status = CommandStatus::kErrStorage;
      } else {
        ambient_calibration_save_.requestRebootSave();
        maybePersistAmbientCalibration(now_ms);
        drainStorageSave();
        reboot_pending_ = true;
        reboot_requested_ms_ = now_ms;
      }
      persistStorageCounters();
      drainStorageSave();
      break;
    }
```

- [ ] **Step 9: Build all three embedded profiles**

`app_controller.cpp` has no native coverage; verify via embedded builds.

Run: `cd firmware && pio run -e xiao_ble_sense`
Expected: SUCCESS. Compare RAM/Flash to Task 3's numbers — expect a small increase from the new task slot and pending-state members.

Run: `cd firmware && pio run -e xiao_ble_sense_128x32`
Expected: SUCCESS.

Run: `cd firmware && pio run -e xiao_ble_sense_deep_sleep`
Expected: SUCCESS.

- [ ] **Step 10: Run the native suite as a final regression pass**

Run: `cd firmware && pio test -e native`
Expected: 139/139 pass (this task doesn't touch `lib/domain`).

- [ ] **Step 11: Commit**

```bash
git add firmware/src/app_controller.h firmware/src/app_controller.cpp
git commit -m "feat(firmware): drive odometer/ambient/counters saves through the async storage path"
```

---

### Task 5: Documentation

**Files:**
- Modify: `tasks/firmware/README.md` (close the "синхронные flash-записи" debt item)
- Modify: `STATUS.md` (update «Готово»/«Ограничения»)

**Interfaces:** none (docs only).

- [ ] **Step 1: Update `tasks/firmware/README.md`**

Find the bullet under «Общие firmware-долги» about `delay(delay_ms)` / synchronous flash writes / `printGpioProbe()` (the one already partially updated by the bound-idle-loop-delay plan, mentioning "синхронные flash-записи (remove+write+flush+close в `InternalFsBackend::write`) прямо на scheduler-пути"). Update it to reflect that flash writes are now async and chunked across the new `"storage"` scheduler task, while `printGpioProbe()`'s `delay(2)` calls remain the one still-open item in this bullet, e.g.:

```markdown
- [x] Сделать flash-записи асинхронными. `StorageBackend::beginWrite`/`pollWrite`
  (`storage_manager.h`) заменили синхронный `write()`; `InternalFsBackend`
  (`internal_fs_backend.{h,cpp}`) разбивает remove+open+write+flush+close на
  два `pollWrite()`-тика. `StorageManager` сохранил старые синхронные
  `saveConfig`/`saveOdometer`/`saveAmbientCalibration`/`saveStorageCounters`
  как тонкие обёртки (begin + drain) для boot-time и non-hot-path вызовов;
  новые `beginSaveOdometer`/`beginSaveAmbientCalibration`/
  `beginSaveStorageCounters` + `pollSave()` используются только на
  mid-ride пути через новую scheduler-задачу `"storage"`
  (`AppController::pollStorageSave`). Deep sleep и оба `reboot`-чекпоинта
  синхронно дренируют (`drainStorageSave()`) любую незавершённую запись перед
  power-off/reset. Остаются: несколько `delay(2)` в `printGpioProbe()`,
  вызываемой из Serial-консоли — не на hot path, вне зоны этого изменения.
```

- [ ] **Step 2: Update `STATUS.md`**

Search the whole file for other mentions of synchronous flash writes / `InternalFsBackend::write` before committing — run `grep -n "InternalFsBackend::write\|синхронные flash-записи\|remove+write+flush+close" STATUS.md` and resolve every hit, not just the first one this step names. Update the «Ограничения» bullet describing the blocking production-loop writes to state they're now async/chunked (mirroring the README wording above, in this file's existing terse style), and add a short bullet under «Готово → Прошивка» describing the same change with file references, matching the style of the neighboring `Watchdog`/`Scheduler task timing`/`StorageCounters` bullets.

- [ ] **Step 3: Commit**

```bash
git add tasks/firmware/README.md STATUS.md
git commit -m "docs: close async flash writes debt in firmware backlog"
```

---

## Self-Review

- **Spec coverage:** the design spec's Architecture, Interface change, Serialization, Deep sleep/reboot, and Testing sections are all implemented: Task 1 = interface, Task 2 = `StorageManager` primitive + sync-wrapper preservation, Task 3 = real `InternalFsBackend` split, Task 4 = `AppController` wiring including the deep-sleep/reboot drain requirement, Task 5 = docs. The spec's "Open Items" (scheduler task period, config-save special-casing) are resolved: period is 50ms fixed (Task 4 Step 2), and `saveConfig` needed no async entry point at all (confirmed while tracing the code — only `writeSlot`/`savePayload` needed to become async-backed, and `saveConfig` already routes through `savePayload`).
- **Deviation from the spec's literal wording, and why:** the spec's Architecture section implied replacing the synchronous `saveOdometer`/`saveAmbientCalibration`/`saveStorageCounters`/`saveConfig` methods with the new `beginSaveX` ones. Tracing the actual call sites (`loadConfig`/`loadOdometer`'s boot-time defaults/migration writes, `saveAndApplyOdometer`, and ~15 existing native tests) showed that keeping the old methods as synchronous wrappers over the same async primitive achieves the identical goal with a dramatically smaller, safer diff — zero existing tests need modification, and boot-time/non-hot-path callers keep working unchanged. This plan implements that refinement instead of a literal rename.
- **Placeholder scan:** no TBD/vague steps; every step has literal code, exact (or explicitly-flagged-as-approximate, with a verification instruction) locations, and literal shell commands. Task 4 Steps 4-5 explicitly instruct the implementer to verify two log-format helper functions' exact current text before assuming the plan's copy is still byte-accurate, since those functions were last touched by an earlier, already-merged plan and could drift further before this one executes — this is a verification instruction, not a placeholder.
- **Type consistency:** `AsyncWriteStatus` (backend-level: kIdle/kInProgress/kOk/kError) and `StorageAsyncStatus` (StorageManager-level: same four values) are intentionally separate types even though they look identical — `StorageBackend` and `StorageManager` are different abstraction layers and neither should depend on the other's enum, matching this codebase's existing pattern of layer-local types (e.g. `StorageIoResult` vs `AsyncWriteStatus` already coexist for the same reason). `PendingStorageSave` (AppController-level: kNone/kOdometer/kAmbientCalibration/kStorageCounters) is a third, distinct enum for the same reason — it tracks *which* save is pending, information `StorageManager` itself has no reason to know.
