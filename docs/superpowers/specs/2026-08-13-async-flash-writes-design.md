# Async Flash Writes — Design

**Status:** Approved for planning
**Date:** 2026-08-13

## Context

`firmware/src/internal_fs_backend.cpp`'s `InternalFsBackend::write()` performs, synchronously and on the caller's stack:

```
if (exists(path)) remove(path);
open(path, FILE_O_WRITE);
write(data, length);
flush();
close();
```

This runs on the Adafruit nRF52 Arduino core (`platform = seeedboards`, `board = seeed-xiao-afruitnrf52-nrf52840`) via `Adafruit_LittleFS`/`InternalFileSystem`, which wraps the upstream `littlefs` library. `littlefs` is synchronous by design — there is no async variant, and no async NVMC (flash controller) driver is exposed by this SDK. A page erase on nRF52840 internal flash is documented by Nordic at ~85-90ms typical; a small-file rewrite (as every one of this project's fixed-path A/B slot saves is) very likely triggers an erase on the freed block before the new write can land, since flash cannot be reprogrammed in place without erasing first.

`StorageManager` (`firmware/lib/domain/storage_manager.cpp`) calls this synchronously from four save paths — `saveConfig` (:502), `saveOdometer` (:569), `saveAmbientCalibration` (:629), `saveStorageCounters` (:637) — all funneled through the shared `savePayload` (:391) → `writeSlot` (:375) pair. `AppController` (`firmware/src/app_controller.cpp`) calls these from the scheduler path (`persistOdometer` :1407, `persistAmbientCalibration` :1448, `persistStorageCounters` :1474) whenever a `*SavePolicy` decides a save is due — most commonly every `odometer_save_interval_m` (default 500m) while riding, or on a throttled ambient-calibration change.

Today's worst case is the full `remove+open+write+flush+close` chain running back-to-back in one scheduler tick — potentially two chained flash operations (a reclaim-triggered erase inside `remove`, another inside `write`/`flush`) landing in the same tick, unmeasured on real hardware but plausibly ~180-270ms given the ~90ms-per-erase datasheet figure. This has never been characterized on real hardware; the existing `kSchedStateBudgetUs` etc. (`app_controller.cpp`) already reserve 150ms of headroom "under 1 flash write" as a defensive estimate, not a measurement.

This blocks `loop()` — pulse polling, display refresh, BLE telemetry, Serial console — for however long it takes, mid-ride, every ~500m.

## Goal

Bound the single largest blocking window a save can cause to roughly one flash operation (~90ms), instead of the current potential 2-3 chained operations, by splitting each write's LittleFS calls across scheduler ticks. This targets the mid-ride save path specifically — that's the only context where the blocking is user-visible and costly (a dropped wheel pulse, a display stutter, a delayed BLE notify).

## Non-Goals

- **Zero blocking / true concurrency.** littlefs's flash calls are blocking hardware operations with no way to interrupt them mid-flight on this SDK. Only a second execution context (a FreeRTOS task) could remove blocking from `loop()` entirely, and that was explicitly rejected for this iteration — no RTOS-task precedent exists anywhere in this codebase yet, InternalFS's thread-safety isn't documented by Adafruit, and Bluefruit's own bonding storage may touch the same filesystem from a different context. That's a bigger, separately-scoped decision if the chunked approach turns out insufficient after real hardware measurement.
- **Read path.** `loadConfig`/`loadOdometer`/`loadAmbientCalibration`/`loadStorageCounters` and `StorageManager::begin()` stay fully synchronous. They only run once at boot, before a ride starts and before anything else is competing for the loop — the existing behavior there is fine.
- **Reducing the erase's actual duration.** ~90ms is a hardware property of nRF52840 NVMC, not something software can shrink. This design only controls *when* that cost is paid, not *how much* it costs.

## Architecture

### The chunk boundary

`InternalFsBackend`'s write sequence splits at its natural midpoint into two ticks:

- **Tick 1:** `remove(path)` if it exists.
- **Tick 2:** `open` + `write` + `flush` + `close`, determining success/failure.

This is the coarsest split available — littlefs gives no finer-grained hook into the erase/program cycle — but it caps any single tick at one flash operation instead of stacking both, roughly halving to thirding the worst-case single blocking window.

### `StorageBackend` interface

Replaces the single synchronous `write()`:

```cpp
enum class AsyncWriteStatus : uint8_t { kIdle, kInProgress, kOk, kError };

class StorageBackend {
 public:
  virtual ~StorageBackend() = default;
  virtual bool begin() = 0;
  virtual StorageIoResult read(const char* path, uint8_t* output,
                               size_t capacity, size_t& length) = 0;
  // Starts an async write. Returns false only on caller error (a write is
  // already in progress). Copies `data` internally — see below.
  virtual bool beginWrite(const char* path, const uint8_t* data,
                          size_t length) = 0;
  // Advances the write by exactly one step. Call once per tick until it
  // returns something other than kInProgress. Each call may block for up
  // to one flash operation (~90ms worst case), never more than one.
  virtual AsyncWriteStatus pollWrite() = 0;
};
```

**Payload ownership:** the caller's record buffer (a stack-local `uint8_t[kMaximumRecordSize]` inside `writeSlot`) does not survive to the next tick. `InternalFsBackend::beginWrite` must copy the payload into an internal fixed-size buffer (`kMaximumRecordSize` = 64 bytes today) before returning, and hold the path pointer (all callers pass `const char*` literals from `StoragePaths`, which are static string constants — safe to hold without copying).

### `StorageManager` async surface

The existing `savePayload`/`writeSlot` machinery — dedup-by-content check, A/B newest-slot selection, sequence numbering — is entirely reads (fast, no erase) and stays synchronous exactly as it is today. Only the final "actually write" step becomes async. New public surface:

```cpp
bool beginSaveConfig(const DeviceConfig& config);
bool beginSaveOdometer(const OdometerData& odometer);
bool beginSaveAmbientCalibration(const AmbientCalibrationData& calibration);
bool beginSaveStorageCounters();
StorageAsyncStatus pollSave();     // advance the in-flight save, if any
bool saveInProgress() const;
```

`beginSaveX` runs `savePayload`'s existing decision logic synchronously (unchanged), and if a write is actually needed, calls `backend_.beginWrite(...)` and returns `true` to mean "started" (not "succeeded" — that's what `pollSave()` eventually reports). If the value was unchanged (dedup skip), it completes synchronously as it does today — `beginSaveX` returns `true` and `pollSave()` immediately reports `kOk` on the next call, no flash touched, matching today's `skipped_writes` behavior exactly.

`counters_.writes`/`write_errors` bump only when `pollSave()` observes the underlying `pollWrite()` reach `kOk`/`kError` — not at `beginSaveX` time. This matters for `saveStorageCounters()` specifically: it must still snapshot `counters_` into the payload *before* `beginWrite` is called (same "snapshot precedes the write's own increment" rule established when counters persistence was built), now more clearly separated in time since the snapshot and the eventual increment are ticks apart.

### Serialization

Only one save in flight at a time, matching this codebase's existing single-slot-queue pattern (BLE safe/dangerous commands already work this way). If a `beginSaveX` is called while another save is in progress, it does nothing and returns `false` — no new request queue. This is safe because every `maybePersistX` caller in `AppController` already re-evaluates its `*SavePolicy` every tick; a save that couldn't start this tick because another was in flight gets naturally retried once that one finishes, since the underlying odometer/ambient-calibration value hasn't been marked "saved" yet.

### `AppController` wiring

A new scheduler task (added to the existing `ScheduledTask` array alongside `kTaskPulses`/`kTaskState`/etc.) calls `storage_.pollSave()` every tick whenever `storage_.saveInProgress()` is true — this is what actually drives the two-tick state machine forward. The existing `persistOdometer`/`persistAmbientCalibration`/`persistStorageCounters` helpers change from "call the blocking save, immediately check the bool and log/acknowledge" to "call `beginSaveX`, remember which trigger is pending, and move the acknowledge/error-logging logic to wherever `pollSave()` reports completion" (a new small completion-handling function, since the trigger being acknowledged depends on which of the three save types just finished).

### Deep sleep and reboot: the one place this stays synchronous

`tryEnterDeepSleep` (`app_controller.cpp:697`) and both reboot handlers (`SerialCommand::kReboot` :1109, `CommandId::kReboot` :1768) currently call the persist helpers and then immediately power off (`deepSleepPrepareAndEnter`) or arm a 250ms reset timer. If saves become async, either path could complete *before* the in-flight write lands on flash — silently losing exactly the data these checkpoints were built to protect (this would directly undo the deep-sleep/reboot guarantees the StorageCounters persistence work just established).

Fix: add `StorageManager::drainPendingSave(uint32_t deadline_ms)` — a bounded busy-poll (calls `pollWrite()`/`pollSave()` back-to-back with no scheduler yielding) that blocks until the in-flight save completes or the deadline is exceeded. Called only at these three checkpoints, immediately after the relevant `beginSaveX` call and before the power-off/reset step. Blocking here is intentional and harmless — the device is seconds from powering off or resetting either way, so there's no responsiveness cost to protect. The deadline should be comfortably inside the 8s watchdog budget (a two-tick save at ~90ms/tick is ~180ms worst case; a deadline in the 500ms-1s range leaves large margin while still guaranteeing forward progress if something is wrong).

## Testing Strategy

- **`InternalFsBackend`'s real two-tick behavior:** embedded-build-only, same as every other Arduino-dependent change in this codebase — verified by successful builds of all three profiles, not native tests.
- **`StorageManager`'s async state logic:** new native test double, `SlowMemoryStorageBackend`, configurable to take N `pollWrite()` calls before completing (N≥1, default matching the real two-tick shape). Enables native tests for: serialization (a second `beginSaveX` while one is in flight returns false), counters bumping only on completion not on `beginSaveX`, the dedup-skip fast path still completing in one poll, and `drainPendingSave`'s timeout behavior (using a backend double that never completes, to confirm the deadline is honored).
- **`AppController` wiring** (the new scheduler task, the three checkpoint call sites): embedded-build-only, as with all other `app_controller.cpp` changes in this codebase.

## Open Items for the Implementation Plan

- Exact period for the new "drive the async save" scheduler task — should be short enough to make forward progress promptly (existing `kTaskState` period is a reasonable starting point to mirror) but this needs a concrete value chosen during planning, not left implicit.
- Whether `beginSaveConfig` needs any special handling for the factory-reset dangerous-command path (which currently blocks on `saveConfig` before proceeding) — likely no different treatment needed since that path is already gated behind a slow two-phase BLE confirm/nonce flow where a couple hundred ms of extra latency is invisible, but should be confirmed explicitly rather than assumed during planning.
- Real hardware measurement of `sched`'s `max_us` for the storage-related scheduler tasks after this change lands, to replace the current defensive-estimate budgets with real numbers (tracked as existing hardware debt, not new scope here).
