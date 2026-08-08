# Zephyr Z5 Parity Catch-Up Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Bring `firmware-zephyr/` back to functional parity with `firmware/` (Arduino) for the three features that shipped on Arduino after Z3/Z4 acceptance (commit `e1d2c27`) but were never ported: PowerManager FSM, the Zephyr deep-sleep adapter, and BLE Companion Sync (clock/weather on OLED).

**Architecture:** All three features already exist as complete, working code in `firmware/` — two of them (`power_manager.cpp`, `companion_snapshot.cpp`) are plain domain modules that only need to be added to the Zephyr build and wired into `firmware-zephyr/app/src/app_controller.cpp`, mirroring the exact call sites already present in `firmware/src/app_controller.cpp`. `firmware/include/` (types, `ble_protocol.h`) is a shared include path for both builds, so the wire-format changes (Companion Write UUID, `DisplaySnapshot` companion fields) are already visible to Zephyr — only the Zephyr-side GATT table and `BleManager` shim need new code. Deep sleep needs one genuinely new file, `platform/deep_sleep_zephyr.cpp`, implementing the existing `firmware/src/platform/deep_sleep.h` contract with Zephyr/nrfx calls instead of Arduino/Adafruit calls.

**Tech Stack:** C++17, Zephyr RTOS 4.1 (`west build`, `ztest`/twister), nrfx HAL (`hal/nrf_power.h`, `hal/nrf_gpio.h`), CMake.

## Global Constraints

- Do not change `protocol/` wire formats — `kBleCompanionWriteUuid`, `CompanionSnapshotPacket` layout, and `PROTO_MINOR=1` are already fixed by the Arduino implementation; Zephyr must match them exactly, not reinvent them.
- Business logic stays in `firmware/lib/domain/`; Zephyr files only adapt platform APIs (GPIO, BLE, storage) behind the same interfaces Arduino uses (`docs/08-zephyr-migration.md` §2).
- `firmware-zephyr` must keep building in both `CONFIG_BIKECOMP_ZEPHYR_BLE_STUB` variants (real BLE and stub) — every new `BleManager` method needs an implementation in **both** `services/ble_manager_zephyr.cpp` and `services/ble_manager_stub.cpp`.
- USB serial regression harness (`serial_usb_test.cpp`, Arduino `test-on`/`test-off`) is explicitly **out of scope** for this plan — user decision 2026-08-04 was to defer it. Do not port `usb_test_mode_` or `UsbTestHooks`. Leave it recorded as open debt in `tasks/firmware/zephyr.md` Z5.4.
- Verify each task with `cd firmware-zephyr && ./scripts/build.sh` (west build) and `west build -b native_sim -p -- -DCONF_FILE=... ` is not used here — this project's test target is `west twister -T tests/domain` (or `cd firmware-zephyr && make test` per `tasks/firmware/zephyr.md`). Use whichever the repo's existing CI invokes; check `.github/workflows/ci.yml` if unsure, don't guess a new command.

---

### Task 1: PowerManager FSM parity in Zephyr AppController

**Files:**
- Modify: `firmware-zephyr/cmake/domain_sources.cmake`
- Modify: `firmware-zephyr/app/CMakeLists.txt`
- Modify: `firmware-zephyr/app/src/app_controller.h`
- Modify: `firmware-zephyr/app/src/app_controller.cpp`
- Modify: `firmware-zephyr/app/src/services/ble_manager.h`
- Modify: `firmware-zephyr/app/src/services/ble_manager_zephyr.cpp`
- Modify: `firmware-zephyr/app/src/services/ble_manager_stub.cpp`
- Modify: `firmware-zephyr/app/src/services/display_manager.h`
- Create: `firmware-zephyr/tests/domain/src/test_power_manager.cpp`
- Modify: `firmware-zephyr/tests/domain/CMakeLists.txt`

**Interfaces:**
- Consumes: `bike::PowerManager`, `PowerManagerConfig`, `PowerManagerInput`, `PowerManagerUpdateResult`, `SchedulerPeriods`, `SystemPowerMode`, `PowerSleepBlockReason` from `firmware/lib/domain/power_manager.h` (already exists, unmodified).
- Produces: `AppController::power_manager_` member and `BleManager::applyPowerSaveAdvertising(bool)` / `BleManager::stopAdvertising()`, which Task 2 (deep sleep) calls directly.

- [ ] **Step 1: Consolidate the duplicated Zephyr domain-source lists**

`firmware-zephyr/app/CMakeLists.txt` currently hardcodes its own copy of the 29-module domain list instead of reusing `firmware-zephyr/cmake/domain_sources.cmake` (which `tests/domain/CMakeLists.txt` already uses). That duplication is exactly how this whole Z5 gap happened silently — fix it now so a module added once can't be forgotten in the other list again.

In `firmware-zephyr/cmake/domain_sources.cmake`, add the new module to the existing list (alphabetical, matches current ordering):

```cmake
  ${BIKECOMP_DOMAIN_DIR}/page_carousel.cpp
  ${BIKECOMP_DOMAIN_DIR}/power_manager.cpp
  ${BIKECOMP_DOMAIN_DIR}/protocol_codec.cpp
```

Then replace the inline list in `firmware-zephyr/app/CMakeLists.txt` (lines 6–46) with:

```cmake
set(FIRMWARE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../firmware)

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/domain_sources.cmake)
set(DOMAIN_DIR ${BIKECOMP_DOMAIN_DIR})
set(DOMAIN_SOURCES ${BIKECOMP_DOMAIN_SOURCES})

set(PLATFORM_SOURCES
  src/platform/time_console.cpp
  src/platform/adc_io.cpp
  src/platform/littlefs_backend.cpp
)

set(SERVICE_SOURCES
  src/services/wheel_sensor.cpp
  src/services/battery_manager.cpp
  src/services/ambient_light_manager.cpp
  src/services/board_leds.cpp
  src/smoke/zephyr_smoke.cpp
)
```

Leave everything below `SERVICE_SOURCES` (the BLE/display stub `if()` blocks, `target_sources`, `target_include_directories`, etc.) untouched — `DOMAIN_SOURCES` and `DOMAIN_DIR` still resolve to the same values, just sourced from one file.

- [ ] **Step 2: Verify the CMake change alone still configures**

Run: `cd firmware-zephyr && ./scripts/build.sh`
Expected: build succeeds (now compiling `power_manager.cpp` into `app`, unused so far — no link errors, only possibly an "unused" warning which is not fatal here since only `-Werror=return-type` is enabled).

- [ ] **Step 3: Add `PowerManager` member and config wiring to `AppController`**

In `firmware-zephyr/app/src/app_controller.h`, add the include and member (mirrors `firmware/src/app_controller.h` exactly):

```cpp
#include "power_manager.h"
```

(place it alphabetically with the other includes — after `platform.h`/`odometer_save_policy.h`, before `pulse_filter.h`)

Add to the private method block, next to the other `void ...(uint32_t now_ms)` declarations:

```cpp
  void configurePowerManager();
  PowerManagerInput buildPowerManagerInput(uint32_t now_ms) const;
  void applySchedulerPeriods(uint32_t now_ms);
  void handlePowerManagerResult(const PowerManagerUpdateResult& result,
                                uint32_t now_ms);
  void updatePowerManager(uint32_t now_ms);
  void printPowerStatus() const;
  void printStatus();
```

Add to the member list, next to `BleManager ble_;`:

```cpp
  PowerManager power_manager_;
```

- [ ] **Step 4: Implement the PowerManager wiring in `app_controller.cpp`**

Add near the top of `firmware-zephyr/app/src/app_controller.cpp`, in the anonymous namespace with the other task-index constants (there currently are none named — check `tasks_[6]` initializer in the constructor for the existing order: `pulses, state, ambient, battery, display, ble`) add:

```cpp
namespace {
constexpr size_t kTaskPulses = 0;
constexpr size_t kTaskState = 1;
constexpr size_t kTaskAmbient = 2;
constexpr size_t kTaskBattery = 3;
constexpr size_t kTaskDisplay = 4;
constexpr size_t kTaskBle = 5;
}  // namespace
```

(If an anonymous namespace with other helpers already exists near the top of the file, add these constants inside it instead of creating a second one.)

Append these method definitions (copy verbatim from `firmware/src/app_controller.cpp:564-668`, which is platform-agnostic — it only calls `Serial.print*`, `scheduler_.setTaskPeriod`, `ble_.applyPowerSaveAdvertising`, all of which already exist or are added in this task):

```cpp
void AppController::configurePowerManager() {
  PowerManagerConfig pm_config;
  pm_config.power_save_mode = config_.power_save_mode;
  pm_config.deep_sleep_enabled = config_.deep_sleep_enabled;
  pm_config.deep_sleep_timeout_s = config_.deep_sleep_timeout_s;
  power_manager_.configure(pm_config);
  power_manager_.setBleAlwaysAdvertise(config_.ble_always_advertise);
}

PowerManagerInput AppController::buildPowerManagerInput(uint32_t now_ms) const {
  PowerManagerInput input;
  input.ride_state = ride_state_.state();
  input.display_power = display_.powerState();
  input.now_ms = now_ms;
  input.ble_connected = ble_.bleConnected();
  input.charging = battery_.snapshot().charge_status == ChargeStatus::kCharging;
  input.sensor_test_active = ble_.sensorTestActive();
  input.display_test_active = display_.displayTestActive();
  return input;
}

void AppController::applySchedulerPeriods(uint32_t now_ms) {
  const SchedulerPeriods periods = power_manager_.schedulerPeriods();
  scheduler_.setTaskPeriod(kTaskPulses, periods.pulses_ms, now_ms);
  scheduler_.setTaskPeriod(kTaskState, periods.state_ms, now_ms);
  scheduler_.setTaskPeriod(kTaskAmbient, periods.ambient_ms, now_ms);
  scheduler_.setTaskPeriod(kTaskBattery, periods.battery_ms, now_ms);
  scheduler_.setTaskPeriod(kTaskDisplay, periods.display_ms, now_ms);
  scheduler_.setTaskPeriod(kTaskBle, periods.ble_ms, now_ms);
}

void AppController::handlePowerManagerResult(
    const PowerManagerUpdateResult& result, uint32_t now_ms) {
  if (result.request_deep_sleep_save) {
    odometer_save_.requestDeepSleepSave();
    maybePersistOdometer(now_ms);
  }
  if (result.mode_changed &&
      power_manager_.systemMode() == SystemPowerMode::kLowPowerIdle) {
    ble_.applyPowerSaveAdvertising(power_manager_.aggressiveBlePowerSave());
  }
  // NOTE: result.request_enter_deep_sleep is handled by AppController::tryEnterDeepSleep
  // added in Task 2 of docs/superpowers/plans/2026-08-04-zephyr-parity-z5.md.
}

void AppController::updatePowerManager(uint32_t now_ms) {
  if (ble_.bleConnected()) display_.noteActivity(now_ms);
  const PowerManagerUpdateResult result =
      power_manager_.update(buildPowerManagerInput(now_ms));
  handlePowerManagerResult(result, now_ms);
  applySchedulerPeriods(now_ms);
  ble_.applyPowerSaveAdvertising(power_manager_.aggressiveBlePowerSave());
}

void AppController::printPowerStatus() const {
  Serial.print("Power: mode=");
  Serial.print(systemPowerModeName(power_manager_.systemMode()));
  Serial.print(", deep_sleep_armed=");
  Serial.print(power_manager_.deepSleepArmed() ? '1' : '0');
  Serial.print(", blocked=");
  Serial.print(powerSleepBlockReasonName(power_manager_.blockReason()));
  Serial.print(", display_off_ms=");
  Serial.print(power_manager_.displayOffSinceMs());
  Serial.print(", low_power_entries=");
  Serial.println(power_manager_.lowPowerIdleEntryCount());
}

void AppController::printStatus() {
  const TripSnapshot trip = trip_computer_.snapshot();
  const BatterySnapshot battery = battery_.snapshot();
  const DiagnosticSnapshot diag = diagnosticSnapshot();
  const char* ride = "IDLE";
  switch (trip.ride_state) {
    case RideState::kMoving: ride = "MOVING"; break;
    case RideState::kPaused: ride = "PAUSED"; break;
    default: break;
  }
  const char* display = "bright";
  switch (display_.powerState()) {
    case DisplayPowerState::kDim: display = "dim"; break;
    case DisplayPowerState::kOff: display = "off"; break;
    default: break;
  }
  Serial.print("Status: speed_x100=");
  Serial.print(trip.speed_x100);
  Serial.print(" avg_speed_x100=");
  Serial.print(trip.average_speed_x100);
  Serial.print(" max_speed_x100=");
  Serial.print(trip.max_speed_x100);
  Serial.print(" trip_mm=");
  Serial.print(trip.trip_distance_mm);
  Serial.print(" rev=");
  Serial.print(trip.revolutions);
  Serial.print(" moving_ms=");
  Serial.print(trip.moving_time_ms);
  Serial.print(" ride=");
  Serial.print(ride);
  Serial.print(" battery_mv=");
  Serial.print(battery.millivolts);
  Serial.print(" battery_pct=");
  Serial.print(battery.percent);
  Serial.print(" battery_valid=");
  Serial.print(battery.valid ? 1 : 0);
  Serial.print(" usb=");
  Serial.print(battery.usb_present ? 1 : 0);
  Serial.print(" display=");
  Serial.print(display);
  Serial.print(" raw_pulses=");
  Serial.print(diag.raw_pulse_count);
  Serial.print(" debounce_rej=");
  Serial.print(diag.rejected_debounce);
  Serial.print(" overspeed_rej=");
  Serial.print(diag.rejected_overspeed);
  Serial.print(" power_mode=");
  Serial.print(systemPowerModeName(power_manager_.systemMode()));
  Serial.print(" deep_sleep_armed=");
  Serial.println(power_manager_.deepSleepArmed() ? 1 : 0);
}
```

(`printStatus` drops the Arduino `hall=`, `total_rev=`, `usb_test=` fields that depend on `wheel_sensor_.pinIsHigh()` two-wire status and the deferred USB test harness — those are not present/relevant in the current Zephyr `AppController`. Keep the rest field-for-field so `tools/` scripts that already parse Arduino's `status` line output can be reused later without a rewrite.)

- [ ] **Step 5: Call the new wiring from `begin()`, `loop()`, `applyConfig()`, and `processPulses()`**

In `begin()`, after `odometer_save_.noteDisplayPower(display_.powerState());` (the existing last line of the boot sequence, right before the BLE seed block), add:

```cpp
  configurePowerManager();
  applySchedulerPeriods(millis());
```

In `loop()` (currently just `wheel_sensor_.pollPin(); ...; scheduler_.run(now_ms); yield();`), replace the trailing `yield();` with:

```cpp
  updatePowerManager(now_ms);

  if (power_manager_.systemMode() == SystemPowerMode::kLowPowerIdle) {
    const uint32_t next_due = scheduler_.nextDueMs(now_ms);
    if (next_due > now_ms && next_due != UINT32_MAX) {
      const uint32_t delay_ms = next_due - now_ms;
      if (delay_ms > 1u) k_msleep(delay_ms);
    }
  } else {
    yield();
  }
```

(Use `k_msleep` — the Zephyr kernel sleep — instead of Arduino's `delay()`; confirm `Scheduler::nextDueMs` exists on the shared `Scheduler` class in `firmware/lib/domain/scheduler.h`. If it doesn't exist yet, add it there as a small shared addition: `uint32_t nextDueMs(uint32_t now_ms) const;` returning the minimum of all `tasks_[i].next_due_ms`, since this is domain code shared by both builds and Arduino's `b3d39ff` already relies on it existing — check `firmware/lib/domain/scheduler.h` before writing a Zephyr-only duplicate.)

In `applyConfig(const DeviceConfig& new_config)`, after the existing `battery_.applyRuntimeConfig(config_, millis());` line, add:

```cpp
  configurePowerManager();
```

In `processPulses(uint32_t now_ms)`, in the accepted-pulse branch (after the existing `ble_.noteMovement();` / `display_.noteActivity(now_ms);` calls — do not restructure the function, just add one line), add:

```cpp
  power_manager_.noteActivity(now_ms);
```

In `processSerialConsole`, add two cases immediately before `case SerialCommand::kUnknown:` (currently at line 586):

```cpp
      case SerialCommand::kPowerStatus:
        printPowerStatus();
        Serial.println("OK power-status");
        break;

      case SerialCommand::kStatus:
        printStatus();
        Serial.println("OK status");
        break;

```

- [ ] **Step 6: Add `applyPowerSaveAdvertising` / `stopAdvertising` to the Zephyr `BleManager` shim**

In `firmware-zephyr/app/src/services/ble_manager.h`, add to the public interface, next to `bool bleConnected() const;`:

```cpp
  void applyPowerSaveAdvertising(bool aggressive_power_save);
  void stopAdvertising();
```

In `firmware-zephyr/app/src/services/ble_manager_zephyr.cpp`, add a file-scope flag next to `bool g_advertising_restart_pending = false;`:

```cpp
bool g_aggressive_ble_power_save = false;
```

Then add the two method definitions near the other `void BleManager::...` definitions (after `noteMovement`, matching Arduino's `firmware/src/ble_manager.cpp:526-545` logic but using the Zephyr connection/advertising state already tracked in this file, `g_active_conn` and the existing static `startAdvertising()`/`stopAdvertising()` free functions):

```cpp
void BleManager::stopAdvertising() {
  bike::stopAdvertising();  // calls the existing static free function in this TU
}

void BleManager::applyPowerSaveAdvertising(bool aggressive_power_save) {
  if (!ok_) return;
  const bool was_aggressive = g_aggressive_ble_power_save;
  g_aggressive_ble_power_save = aggressive_power_save;
  if (aggressive_power_save) {
    stopAdvertising();
    return;
  }
  if (was_aggressive && g_active_conn == nullptr) {
    startAdvertising();
  }
}
```

Since `stopAdvertising()`/`startAdvertising()` are free functions in the anonymous namespace of the same translation unit and `BleManager::stopAdvertising()` is a same-named method, qualify the free-function call however this file already disambiguates other same-name collisions (check how `noteMovement()` at line 703-709 calls the free `startAdvertising()` — copy that exact pattern instead of inventing `bike::stopAdvertising()`, which will not compile since the free function isn't in `namespace bike` explicitly, it's just inside the outer `namespace bike { namespace { ... } }`).

- [ ] **Step 7: Add matching no-op stubs to `ble_manager_stub.cpp`**

In `firmware-zephyr/app/src/services/ble_manager_stub.cpp`, add next to the other one-line stubs like `void BleManager::noteMovement() {}`:

```cpp
void BleManager::stopAdvertising() {}
void BleManager::applyPowerSaveAdvertising(bool aggressive_power_save) {
  (void)aggressive_power_save;
}
```

- [ ] **Step 8: Add `displayTestActive()` to the Zephyr `DisplayManager`**

In `firmware-zephyr/app/src/services/display_manager.h`, add next to `bool isOk() const { return display_ok_; }`:

```cpp
  bool displayTestActive() const { return test_active_; }
```

(`test_active_` is already a member declared unconditionally, outside the `#ifndef BIKECOMP_ZEPHYR_DISPLAY_STUB` block, so this compiles in both the real and stub display variants without further changes.)

- [ ] **Step 9: Build and fix any compile errors**

Run: `cd firmware-zephyr && ./scripts/build.sh`
Expected: clean build. If `Scheduler::nextDueMs` doesn't exist yet (Step 5 caveat), add it to `firmware/lib/domain/scheduler.h`/`.cpp` first — it is shared domain code so this also benefits Arduino's build (which already assumes it exists per the `b3d39ff` diff), rebuild, then continue.

- [ ] **Step 10: Write the ztest coverage for `power_manager`**

Create `firmware-zephyr/tests/domain/src/test_power_manager.cpp`, mirroring `firmware/test/test_native/test_main.cpp:1736-1808` (three scenarios: power-save mode enters/exits low-power idle, deep-sleep timeout of 0 plus a BLE-connected block reason, and the deep-sleep save request firing once armed):

```cpp
#include <zephyr/ztest.h>

#include "power_manager.h"

using namespace bike;

ZTEST(power_manager, test_power_save_and_timeout) {
  PowerManager manager;
  PowerManagerConfig config;
  config.power_save_mode = true;
  config.deep_sleep_enabled = false;
  config.deep_sleep_timeout_s = 900;
  manager.configure(config);

  PowerManagerInput input;
  input.display_power = DisplayPowerState::kOff;
  input.now_ms = 1000;
  PowerManagerUpdateResult result = manager.update(input);
  zassert_true(result.mode_changed, "mode changed on entry");
  zassert_equal(static_cast<int>(SystemPowerMode::kLowPowerIdle),
                static_cast<int>(manager.systemMode()), "enters low power idle");
  zassert_false(manager.deepSleepArmed(), "deep sleep not armed yet");

  manager.noteActivity(2000);
  zassert_equal(static_cast<int>(SystemPowerMode::kNormal),
                static_cast<int>(manager.systemMode()), "activity exits low power");

  config.power_save_mode = false;
  config.deep_sleep_timeout_s = 60;
  manager.configure(config);
  input.now_ms = 0;
  manager.update(input);
  zassert_equal(static_cast<int>(SystemPowerMode::kNormal),
                static_cast<int>(manager.systemMode()), "normal before timeout");

  input.now_ms = 59000;
  manager.update(input);
  zassert_equal(static_cast<int>(SystemPowerMode::kNormal),
                static_cast<int>(manager.systemMode()), "still normal at 59s");

  input.now_ms = 60000;
  result = manager.update(input);
  zassert_equal(static_cast<int>(SystemPowerMode::kLowPowerIdle),
                static_cast<int>(manager.systemMode()), "idle at 60s timeout");
  zassert_true(manager.deepSleepArmed(), "deep sleep armed at timeout");
}

ZTEST(power_manager, test_deep_sleep_timeout_zero_and_ble_block) {
  PowerManager manager;
  PowerManagerConfig config;
  config.deep_sleep_timeout_s = 0;
  manager.configure(config);

  PowerManagerInput input;
  input.display_power = DisplayPowerState::kOff;
  input.now_ms = 100000;
  manager.update(input);
  zassert_equal(static_cast<int>(SystemPowerMode::kNormal),
                static_cast<int>(manager.systemMode()), "zero timeout never idles");

  config.deep_sleep_timeout_s = 60;
  manager.configure(config);
  input.ble_connected = true;
  manager.update(input);
  zassert_equal(static_cast<int>(SystemPowerMode::kNormal),
                static_cast<int>(manager.systemMode()), "BLE connection blocks idle");
  zassert_equal(static_cast<int>(PowerSleepBlockReason::kBleConnected),
                static_cast<int>(manager.blockReason()), "block reason is BLE");
}

ZTEST(power_manager, test_deep_sleep_save_request) {
  PowerManager manager;
  PowerManagerConfig config;
  config.deep_sleep_enabled = true;
  config.deep_sleep_timeout_s = 10;
  manager.configure(config);

  PowerManagerInput input;
  input.display_power = DisplayPowerState::kOff;
  input.now_ms = 0;
  manager.update(input);
  PowerManagerUpdateResult result = manager.update(input);
  zassert_false(result.request_deep_sleep_save, "no save request before timeout");

  input.now_ms = 10000;
  result = manager.update(input);
  zassert_true(result.request_deep_sleep_save, "save requested at timeout");
  zassert_true(manager.deepSleepArmed(), "armed at timeout");
}

ZTEST_SUITE(power_manager, NULL, NULL, NULL, NULL, NULL);
```

Register it in `firmware-zephyr/tests/domain/CMakeLists.txt`, adding to `target_sources(testbinary PRIVATE ...)`:

```cmake
    src/test_power_manager.cpp
```

- [ ] **Step 11: Run the ztest suite**

Run: `cd firmware-zephyr && make test` (per `tasks/firmware/zephyr.md` Z4.1b convention)
Expected: all `power_manager` ZTEST cases pass alongside the existing `codec_storage`/`motion`/`protocol`/`commands` suites.

- [ ] **Step 12: Commit**

```bash
git add firmware-zephyr/cmake/domain_sources.cmake firmware-zephyr/app/CMakeLists.txt \
  firmware-zephyr/app/src/app_controller.h firmware-zephyr/app/src/app_controller.cpp \
  firmware-zephyr/app/src/services/ble_manager.h firmware-zephyr/app/src/services/ble_manager_zephyr.cpp \
  firmware-zephyr/app/src/services/ble_manager_stub.cpp firmware-zephyr/app/src/services/display_manager.h \
  firmware-zephyr/tests/domain/src/test_power_manager.cpp firmware-zephyr/tests/domain/CMakeLists.txt \
  firmware/lib/domain/scheduler.h firmware/lib/domain/scheduler.cpp
git commit -m "feat(zephyr): port PowerManager FSM from Arduino (Z5.1)"
```

---

### Task 2: Zephyr deep-sleep adapter (Z5.2)

**Files:**
- Create: `firmware-zephyr/app/src/platform/deep_sleep_zephyr.cpp`
- Modify: `firmware-zephyr/app/include/board_pins.h`
- Modify: `firmware-zephyr/app/Kconfig`
- Modify: `firmware-zephyr/app/prj.conf`
- Modify: `firmware-zephyr/app/CMakeLists.txt`
- Modify: `firmware-zephyr/app/src/app_controller.h`
- Modify: `firmware-zephyr/app/src/app_controller.cpp`
- Modify: `firmware-zephyr/app/src/services/ble_manager_zephyr.cpp`

**Interfaces:**
- Consumes: `AppController::power_manager_`, `handlePowerManagerResult` from Task 1; `firmware/src/platform/deep_sleep.h` interface (`deepSleepWakeFromSleep()`, `deepSleepPrepareAndEnter(uint32_t, bool)`, `deepSleepWakeSourceName()`), unmodified.
- Produces: `g_deep_sleep_supported` flips to `true` in `ble_manager_zephyr.cpp` when `CONFIG_BIKECOMP_DEEP_SLEEP=y`.

- [ ] **Step 1: Add raw nRF GPIO pin constants to Zephyr's `board_pins.h`**

The Arduino build uses raw nRF52 pin numbers (`kHallSenseNrfGpio = 3u`, i.e. P0.03) for `nrf_gpio_cfg_sense_input`. Zephyr's `board_pins.h` currently only has devicetree alias indices, not raw pin numbers. Add, matching `firmware/include/board_pins.h`:

```cpp
// Raw nRF52840 GPIO numbers for deep-sleep SENSE config (mirrors firmware/include/board_pins.h).
constexpr uint32_t kHallSenseNrfGpio = 3u;   // P0.03
constexpr uint32_t kHallDriveNrfGpio = 2u;   // P0.02
constexpr uint32_t kBoardChargeNrfGpio = 17u;  // P0.17
```

- [ ] **Step 2: Add the `CONFIG_BIKECOMP_DEEP_SLEEP` Kconfig option**

In `firmware-zephyr/app/Kconfig`, add after `BIKECOMP_ZEPHYR_DISPLAY_STUB`:

```
config BIKECOMP_DEEP_SLEEP
	bool "Enable deep sleep (System OFF) adapter"
	default y
	help
	  Mirrors Arduino BIKECOMP_FEATURE_DEEP_SLEEP. Code path is compiled in;
	  actual entry still requires config_.deep_sleep_enabled at runtime and
	  is gated by the same hardware bench verification as the Arduino build
	  (see tasks/firmware/README.md "Общие firmware-долги").
```

In `firmware-zephyr/app/prj.conf`, add near the other `CONFIG_BIKECOMP_*` lines:

```
CONFIG_BIKECOMP_DEEP_SLEEP=y
CONFIG_POWEROFF=y
```

In `firmware-zephyr/app/CMakeLists.txt`, add alongside the other `if(CONFIG_BIKECOMP_...)` compile-definition blocks:

```cmake
if(CONFIG_BIKECOMP_DEEP_SLEEP)
  target_compile_definitions(app PRIVATE BIKECOMP_FEATURE_DEEP_SLEEP=1)
else()
  target_compile_definitions(app PRIVATE BIKECOMP_FEATURE_DEEP_SLEEP=0)
endif()
```

Add the new source file to `SERVICE_SOURCES` (or `PLATFORM_SOURCES`, matching where `time_console.cpp` lives — use `PLATFORM_SOURCES` since it's a `platform/` adapter):

```cmake
set(PLATFORM_SOURCES
  src/platform/time_console.cpp
  src/platform/adc_io.cpp
  src/platform/littlefs_backend.cpp
  src/platform/deep_sleep_zephyr.cpp
)
```

- [ ] **Step 3: Implement `deep_sleep_zephyr.cpp`**

Create `firmware-zephyr/app/src/platform/deep_sleep_zephyr.cpp`, implementing the exact same interface as `firmware/src/platform/deep_sleep_nrf52.cpp`, using nrfx HAL headers already proven available in this build (`ble_manager_zephyr.cpp` already includes `<hal/nrf_power.h>` via `time_console.cpp`) plus `<hal/nrf_gpio.h>` for sense-pin config:

```cpp
#include "platform/deep_sleep.h"

#include <hal/nrf_gpio.h>
#include <hal/nrf_power.h>

#include "board_pins.h"

namespace bike {
namespace {

char g_wake_source[8] = "reset";

void setWakeSource(const char* value) {
  for (size_t i = 0; i < sizeof(g_wake_source); ++i) {
    g_wake_source[i] = value[i];
    if (value[i] == '\0') break;
  }
}

}  // namespace

bool deepSleepWakeFromSleep() {
  const uint32_t reason = NRF_POWER->RESETREAS;
  if ((reason & POWER_RESETREAS_OFF_Msk) != 0u) {
    setWakeSource("hall");
    return true;
  }
  if ((reason & POWER_RESETREAS_VBUS_Msk) != 0u) {
    setWakeSource("usb");
    return true;
  }
  return false;
}

bool deepSleepPrepareAndEnter(uint32_t hall_nrf_gpio, bool sense_low) {
#if BIKECOMP_HALL_TWO_WIRE
  nrf_gpio_cfg_input(kHallDriveNrfGpio, NRF_GPIO_PIN_NOPULL);
#endif

  const auto sense = sense_low ? NRF_GPIO_PIN_SENSE_LOW : NRF_GPIO_PIN_SENSE_HIGH;
  nrf_gpio_cfg_sense_input(hall_nrf_gpio, NRF_GPIO_PIN_PULLUP, sense);

  // USB VBUS wake is handled by the SoC; CHG line is an additional reserve path.
  nrf_gpio_cfg_sense_input(kBoardChargeNrfGpio, NRF_GPIO_PIN_NOPULL,
                           NRF_GPIO_PIN_SENSE_LOW);

  nrf_power_system_off(NRF_POWER);
  return false;
}

const char* deepSleepWakeSourceName() { return g_wake_source; }

}  // namespace bike
```

This is a direct nrfx-register port of the proven Arduino implementation (same RESETREAS bits, same SENSE config, same pins) — it does not depend on `sd_softdevice`/Bluefruit (`nrf_power_system_off` is the raw HAL call; Arduino's `sd_power_system_off()` is the SoftDevice-wrapped equivalent, unavailable here since Zephyr's BT stack is not the Nordic SoftDevice).

- [ ] **Step 4: Wire `tryEnterDeepSleep` into `app_controller`**

In `firmware-zephyr/app/src/app_controller.h`, add the include and declaration:

```cpp
#include "platform/deep_sleep.h"
```
```cpp
  void tryEnterDeepSleep(uint32_t now_ms);
```

In `firmware-zephyr/app/src/app_controller.cpp`, replace the placeholder comment left by Task 1's `handlePowerManagerResult` with the real call:

```cpp
  if (result.request_enter_deep_sleep) {
    tryEnterDeepSleep(now_ms);
  }
```

Add the method definition (adapted from `firmware/src/app_controller.cpp:619-645`, replacing `digitalRead`/`Serial.println` warn check and GPIO pin macro with the Zephyr equivalents already used elsewhere in this file — check how `wheel_sensor_` exposes pin state, e.g. `wheel_sensor_.pinIsHigh()`, and reuse that instead of a raw `digitalRead`):

```cpp
void AppController::tryEnterDeepSleep(uint32_t now_ms) {
#if defined(BIKECOMP_FEATURE_DEEP_SLEEP) && BIKECOMP_FEATURE_DEEP_SLEEP
  if (!config_.deep_sleep_enabled || power_manager_.sleepBlocked()) return;

  odometer_save_.requestDeepSleepSave();
  const uint64_t odometer_mm = trip_computer_.snapshot().odometer_mm;
  const OdometerSaveTrigger trigger = odometer_save_.evaluate(odometer_mm, now_ms);
  if (trigger != OdometerSaveTrigger::kNone && !persistOdometer(trigger)) {
    return;
  }

  display_.turnOff(now_ms);
  ble_.stopAdvertising();
  wheel_sensor_.suspendInterrupt();

  const bool sense_low = config_.active_edge != 1u;
  deepSleepPrepareAndEnter(kHallSenseNrfGpio, sense_low);
#else
  (void)now_ms;
#endif
}
```

Confirm `WheelSensor::suspendInterrupt()` already exists in `firmware-zephyr/app/src/services/wheel_sensor.h` (it should, since `wheel_sensor.cpp` is fully ported per Z1.1) — if the method name differs, use whatever the Zephyr `WheelSensor` actually exposes for disabling the GPIO callback.

In `begin()`, right after the existing `const uint32_t resetreas = platform::resetReasonRaw();` block finishes printing, add the wake-source print (mirrors Arduino, uses the new `deep_sleep.h` include added above):

```cpp
  if (deepSleepWakeFromSleep()) {
    Serial.print("wake_source=");
    Serial.println(deepSleepWakeSourceName());
  }
```

- [ ] **Step 5: Flip `g_deep_sleep_supported` in the Zephyr BLE Device Info**

In `firmware-zephyr/app/src/services/ble_manager_zephyr.cpp`, both places that currently hardcode `g_deep_sleep_supported = false;` (lines ~601 in `seedGattBuffers`, and any other reset) should reflect the compiled-in flag instead:

```cpp
  g_deep_sleep_supported = static_cast<bool>(IS_ENABLED(CONFIG_BIKECOMP_DEEP_SLEEP));
```

Do this once in `seedGattBuffers` where the flag is first set (do not add it to `refreshDeviceInfoForRead`/`refreshDeviceInfoLiveFields` call sites — those only *read* the flag, they don't set it, per the existing Arduino pattern where `g_deep_sleep_supported = kDeepSleepCompiledIn;` is set once at init and again on disconnect reset in `ble_manager.cpp:404`; find and update the equivalent second reset site in the Zephyr file if one exists).

- [ ] **Step 6: Build**

Run: `cd firmware-zephyr && ./scripts/build.sh`
Expected: clean build. `nrf_gpio_cfg_sense_input`/`nrf_power_system_off` symbols resolve from the nrfx HAL already linked for `time_console.cpp`'s `nrf_power_gpregret_set`/`nrf_power_usbregstatus_vbusdet_get` calls — if the linker can't find `nrf_power_system_off`, check the exact symbol name in the nrfx version vendored by this Zephyr SDK (`grep -rn "nrf_power_system_off" $ZEPHYR_BASE/../modules/hal/nordic/nrfx/hal/nrf_power.h` or wherever the SDK is installed) and use the exact name found there.

- [ ] **Step 7: ztest / twister sanity**

Run: `cd firmware-zephyr && make test`
Expected: existing suites still pass (deep sleep entry itself is not host-testable — it halts the SoC — so there is no new ztest here; this step only guards against a regression in the files touched).

- [ ] **Step 8: Commit**

```bash
git add firmware-zephyr/app/src/platform/deep_sleep_zephyr.cpp firmware-zephyr/app/include/board_pins.h \
  firmware-zephyr/app/Kconfig firmware-zephyr/app/prj.conf firmware-zephyr/app/CMakeLists.txt \
  firmware-zephyr/app/src/app_controller.h firmware-zephyr/app/src/app_controller.cpp \
  firmware-zephyr/app/src/services/ble_manager_zephyr.cpp
git commit -m "feat(zephyr): port deep sleep adapter from Arduino (Z5.2)"
```

**Note:** exactly like Arduino's own deep sleep (`tasks/firmware/README.md`: "код; полевая верификация — Э7"), this ships code-complete but hardware-unverified. Do not mark Z5.2 hardware-verified in `tasks/firmware/zephyr.md` until a real System OFF / GPIO wake cycle has been confirmed on the XIAO bench, same bar as Arduino.

---

### Task 3: BLE Companion Sync parity in Zephyr (Z5.3)

**Files:**
- Modify: `firmware-zephyr/cmake/domain_sources.cmake`
- Modify: `firmware-zephyr/app/src/services/ble_manager.h`
- Modify: `firmware-zephyr/app/src/services/ble_manager_zephyr.cpp`
- Modify: `firmware-zephyr/app/src/services/ble_manager_stub.cpp`
- Modify: `firmware-zephyr/app/src/app_controller.h`
- Modify: `firmware-zephyr/app/src/app_controller.cpp`
- Create: `firmware-zephyr/tests/domain/src/test_companion_snapshot.cpp`
- Modify: `firmware-zephyr/tests/domain/CMakeLists.txt`

**Interfaces:**
- Consumes: `bike::CompanionSnapshotPacket`, `CompanionState`, `CompanionHeaderView`, `CompanionWeatherView`, `decodeCompanionSnapshot`, `encodeCompanionSnapshot`, `kCompanionSnapshotSize` from `firmware/lib/domain/companion_snapshot.h`; `kBleCompanionWriteUuid` from `firmware/include/ble_protocol.h` (both already visible to Zephyr via the shared `firmware/include` path — no changes needed there).
- Produces: `AppController::companion_state_` populated for `DisplaySnapshot` rendering, which the already-shared `display_formatter.cpp`/`display_layout.cpp` domain modules pick up automatically.

- [ ] **Step 1: Add `companion_snapshot.cpp` to the domain source list**

In `firmware-zephyr/cmake/domain_sources.cmake`, add:

```cmake
  ${BIKECOMP_DOMAIN_DIR}/companion_snapshot.cpp
  ${BIKECOMP_DOMAIN_DIR}/config_codec.cpp
```

(insert alphabetically before `config_codec.cpp`; this file feeds both `app/CMakeLists.txt` and `tests/domain/CMakeLists.txt` after Task 1 Step 1's consolidation, so no other CMake file needs touching).

- [ ] **Step 2: Add the Companion Write GATT characteristic**

In `firmware-zephyr/app/src/services/ble_manager_zephyr.cpp`, add the UUID next to the other `BIKECOMP_UUID_*_VAL` macros:

```cpp
#define BIKECOMP_UUID_COMPANION_WRITE_VAL \
  BT_UUID_128_ENCODE(0x7c9a000b, 0x4b7d, 0x4f2e, 0x9c1a, 0x2e6d5f8b31a4)
```

Add the static UUID object next to `error_log_uuid`:

```cpp
static const struct bt_uuid_128 companion_write_uuid =
    BT_UUID_INIT_128(BIKECOMP_UUID_COMPANION_WRITE_VAL);
```

Add `#include "companion_snapshot.h"` to the includes block.

Add a pending-write holder next to `g_pending_config_write`-style globals:

```cpp
struct PendingCompanionWrite {
  enum class State : uint8_t { kIdle = 0, kPending };
  State state = State::kIdle;
  CompanionSnapshotPacket packet = {};
};

PendingCompanionWrite g_pending_companion_write = {};
```

Add the write handler next to `onConfigWrite`:

```cpp
void onCompanionWrite(struct bt_conn* conn, const uint8_t* data, uint16_t len) {
  (void)conn;
  CompanionSnapshotPacket packet = {};
  if (!decodeCompanionSnapshot(data, len, packet)) return;
  g_pending_companion_write.packet = packet;
  g_pending_companion_write.state = PendingCompanionWrite::State::kPending;
}
```

Add the GATT write callback next to `writeConfig`:

```cpp
ssize_t writeCompanion(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                       const void* buf, uint16_t len, uint16_t offset,
                       uint8_t flags) {
  (void)attr;
  (void)flags;
  if (offset != 0) {
    return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
  }
  if (len != kCompanionSnapshotSize) {
    return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
  }
  onCompanionWrite(conn, static_cast<const uint8_t*>(buf), len);
  return len;
}
```

Append the characteristic at the **end** of `BT_GATT_SERVICE_DEFINE` (after the Error Log CCC, before the closing `);`) so every existing `BikecompAttrIndex` value stays numerically stable — write-only characteristics need no CCC descriptor, matching `config_write`:

```cpp
    BT_GATT_CCC(nullptr, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
    BT_GATT_CHARACTERISTIC(&companion_write_uuid.uuid, BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_WRITE_ENCRYPT, nullptr, writeCompanion,
                           nullptr));
```

(the trailing `);` moves from the current last line, which currently ends the Error Log CCC — replace that line's terminator accordingly).

- [ ] **Step 3: Expose the pending write on `BleManager`**

In `firmware-zephyr/app/src/services/ble_manager.h`, add `#include "companion_snapshot.h"` and, next to `bool bleConnected() const;`:

```cpp
  bool hasPendingCompanionWrite() const;
  bool takePendingCompanionWrite(CompanionSnapshotPacket& out);
```

In `ble_manager_zephyr.cpp`, add the implementations near the other `bool BleManager::...` methods:

```cpp
bool BleManager::hasPendingCompanionWrite() const {
  return g_pending_companion_write.state ==
         PendingCompanionWrite::State::kPending;
}

bool BleManager::takePendingCompanionWrite(CompanionSnapshotPacket& out) {
  if (!hasPendingCompanionWrite()) return false;
  out = g_pending_companion_write.packet;
  g_pending_companion_write.state = PendingCompanionWrite::State::kIdle;
  return true;
}
```

In `ble_manager_stub.cpp`, add:

```cpp
bool BleManager::hasPendingCompanionWrite() const { return false; }
bool BleManager::takePendingCompanionWrite(CompanionSnapshotPacket& out) {
  (void)out;
  return false;
}
```

- [ ] **Step 4: Wire `CompanionState` into `AppController`**

In `firmware-zephyr/app/src/app_controller.h`, add `#include "companion_snapshot.h"`, the method declaration next to `processPendingDangerousCommand`:

```cpp
  void processPendingCompanionWrite(uint32_t now_ms);
```

and the member next to `power_manager_` (from Task 1):

```cpp
  CompanionState companion_state_ = {};
```

In `firmware-zephyr/app/src/app_controller.cpp`, add the method (identical to Arduino, platform-agnostic):

```cpp
void AppController::processPendingCompanionWrite(uint32_t now_ms) {
  CompanionSnapshotPacket packet = {};
  if (!ble_.takePendingCompanionWrite(packet)) return;
  companion_state_.apply(packet, now_ms);
}
```

Call it from `updateBle(uint32_t now_ms)`, next to the other `processPending*` calls:

```cpp
  processPendingCompanionWrite(now_ms);
```

- [ ] **Step 5: Populate `DisplaySnapshot` and extend `printStatus`**

In `updateDisplay(uint32_t now_ms)`, where `DisplaySnapshot snapshot;` is built (before `display_.render(snapshot);`), add:

```cpp
  const CompanionHeaderView companion = companion_state_.header(now_ms);
  if (companion.valid) {
    snprintf(snapshot.companion_header, sizeof(snapshot.companion_header), "%s",
             companion.text);
    snapshot.companion_header_valid = true;
    snapshot.companion_header_stale = companion.stale;
  }
  const CompanionWeatherView weather = companion_state_.weather(now_ms);
  if (weather.valid) {
    snprintf(snapshot.companion_weather_temp, sizeof(snapshot.companion_weather_temp),
             "%s", weather.temp);
    snprintf(snapshot.companion_weather_rain, sizeof(snapshot.companion_weather_rain),
             "%s", weather.rain);
    snapshot.companion_weather_valid = true;
    snapshot.companion_weather_stale = weather.stale;
  }
```

In `printStatus()` (added in Task 1), append before the final `Serial.println(...)` call:

```cpp
  const uint32_t now_ms = millis();
  const CompanionHeaderView companion = companion_state_.header(now_ms);
  Serial.print(" clock=");
  Serial.print(companion.valid ? companion.text : "--");
  Serial.print(" clock_valid=");
  Serial.print(companion.valid ? 1 : 0);
  const CompanionWeatherView weather = companion_state_.weather(now_ms);
  Serial.print(" weather_temp=");
  Serial.print(weather.valid ? weather.temp : "--");
  Serial.print(" weather_rain=");
  Serial.println(weather.valid ? weather.rain : "--");
```

(adjust the previous last line of `printStatus` from `Serial.println(...)` to `Serial.print(...)` so this block's final `Serial.println` is the true line terminator — do not emit two newlines).

- [ ] **Step 6: Build**

Run: `cd firmware-zephyr && ./scripts/build.sh`
Expected: clean build with 8 GATT characteristics now registered (verify via `grep -c BT_GATT_CHARACTERISTIC firmware-zephyr/app/src/services/ble_manager_zephyr.cpp` returning 8).

- [ ] **Step 7: Write the ztest coverage for `companion_snapshot`**

Create `firmware-zephyr/tests/domain/src/test_companion_snapshot.cpp`, mirroring `firmware/test/test_native/test_main.cpp:2569-2616` and reusing the existing `protocol/fixtures/companion_v1_nominal.hex` fixture (already present in the repo, added alongside the Arduino feature) via the shared `fixture_loader`:

```cpp
#include <zephyr/ztest.h>

#include <vector>

#include "companion_snapshot.h"
#include "fixture_loader.h"

using namespace bike;
using bike::test_support::loadFixtureHex;

ZTEST(companion_snapshot, test_fixture_roundtrip) {
  std::vector<uint8_t> fixture_hex;
  zassert_true(loadFixtureHex("companion_v1_nominal", fixture_hex), "load fixture");
  CompanionSnapshotPacket decoded = {};
  zassert_true(
      decodeCompanionSnapshot(fixture_hex.data(), fixture_hex.size(), decoded),
      "decode");
  zassert_equal(1, decoded.struct_version, "struct_version");
  zassert_equal(1704067200u, decoded.unix_time, "unix_time");
  zassert_equal(180, decoded.tz_offset_min, "tz_offset_min");
  zassert_equal(185, decoded.temp_c_x10, "temp_c_x10");
  zassert_equal(40, decoded.pop_pct, "pop_pct");
  zassert_equal(0x03, decoded.flags, "flags");
  zassert_equal(1704070800u, decoded.valid_until, "valid_until");

  uint8_t encoded[kCompanionSnapshotSize] = {};
  encodeCompanionSnapshot(decoded, encoded);
  zassert_mem_equal(fixture_hex.data(), encoded, kCompanionSnapshotSize,
                    "round-trip encode");
}

ZTEST(companion_snapshot, test_clock_and_header_stale) {
  CompanionSnapshotPacket packet = {};
  packet.struct_version = 1;
  packet.unix_time = 1704067200u;
  packet.tz_offset_min = 180;
  packet.temp_c_x10 = 185;
  packet.pop_pct = 40;
  packet.flags = kCompanionFlagTimeValid | kCompanionFlagWeatherValid;
  packet.valid_until = 1704070800u;

  CompanionState state;
  state.apply(packet, 1000u);
  const CompanionHeaderView header = state.header(1000u);
  zassert_true(header.valid, "header valid");
  zassert_false(header.stale, "header not stale immediately");
  zassert_str_equal("03:00", header.text, "formatted local time");

  const CompanionWeatherView weather = state.weather(1000u);
  zassert_true(weather.valid, "weather valid");
  zassert_str_equal("+18.5C", weather.temp, "formatted temp");
  zassert_str_equal("R40%", weather.rain, "formatted rain");

  const CompanionHeaderView stale = state.header(3700000u);
  zassert_true(stale.stale, "header stale after 3700s");
  const CompanionWeatherView stale_weather = state.weather(3700000u);
  zassert_true(stale_weather.stale, "weather stale after 3700s");
}

ZTEST_SUITE(companion_snapshot, NULL, NULL, NULL, NULL, NULL);
```

Register it in `firmware-zephyr/tests/domain/CMakeLists.txt`:

```cmake
    src/test_companion_snapshot.cpp
```

- [ ] **Step 8: Run the ztest suite**

Run: `cd firmware-zephyr && make test`
Expected: `companion_snapshot` suite passes; all pre-existing suites (`codec_storage`, `motion`, `protocol`, `commands`, `power_manager` from Task 1) still pass.

- [ ] **Step 9: Commit**

```bash
git add firmware-zephyr/cmake/domain_sources.cmake firmware-zephyr/app/src/services/ble_manager.h \
  firmware-zephyr/app/src/services/ble_manager_zephyr.cpp firmware-zephyr/app/src/services/ble_manager_stub.cpp \
  firmware-zephyr/app/src/app_controller.h firmware-zephyr/app/src/app_controller.cpp \
  firmware-zephyr/tests/domain/src/test_companion_snapshot.cpp firmware-zephyr/tests/domain/CMakeLists.txt
git commit -m "feat(zephyr): port BLE Companion Sync clock/weather from Arduino (Z5.3)"
```

---

### Task 4: Sync parity docs and checklists (Z5.6)

**Files:**
- Modify: `tasks/firmware/zephyr.md`
- Modify: `docs/08-zephyr-migration.md`

**Interfaces:**
- Consumes: nothing new — this task only updates prose/checkboxes to match the code state produced by Tasks 1–3.

- [ ] **Step 1: Check off the completed Z5 sub-tasks**

In `tasks/firmware/zephyr.md`, under the "Z5 — Синхронизация с Arduino" section added 2026-08-04, check `Z5.1`, `Z5.2`, `Z5.3`, and `Z5.7` (the ztest additions from Tasks 1–3). Leave `Z5.4` (USB harness) unchecked with its existing note — it is out of scope per the 2026-08-04 decision. Add one line under Z5.2 noting deep sleep is code-complete but hardware-unverified on Zephyr, matching Arduino's own status.

- [ ] **Step 2: Update the parity matrix in `docs/08-zephyr-migration.md`**

Flip the four rows added 2026-08-04 (`BLE GATT characteristics`, `PowerManager FSM`, `Deep sleep`, `BLE Companion Sync`) from ✗ to ✓, update the `BLE GATT characteristics` row to `✓ (8, +Companion)` for both columns, and update the "Обновлено" date at the top of the file to the date this task is executed. Leave `USB serial regression harness` as ✗ with a note pointing at `tasks/firmware/zephyr.md` Z5.4.

- [ ] **Step 3: Commit**

```bash
git add tasks/firmware/zephyr.md docs/08-zephyr-migration.md
git commit -m "docs: mark Zephyr Z5 parity catch-up complete (Z5.1-Z5.3, Z5.7)"
```

---

## Self-Review Notes

- **Spec coverage:** Z5.1 (PowerManager) → Task 1. Z5.2 (deep sleep) → Task 2. Z5.3 (Companion Sync) → Task 3. Z5.4 (USB harness) → explicitly deferred per user decision, recorded in Global Constraints and Task 4 Step 1. Z5.5 (app_controller.cpp diff) → fully covered by Tasks 1–3, which together port every non-USB-test line of the `b3d39ff`/`965da46` diffs. Z5.6 (docs) → Task 4. Z5.7 (ztest) → Task 1 Step 10, Task 3 Step 7.
- **Known open risk:** `nrf_power_system_off` exact symbol name and `Scheduler::nextDueMs` existence are flagged inline (Task 2 Step 6, Task 1 Step 5) as the two points most likely to need a small on-the-spot adjustment once building against the real Zephyr SDK checkout — both have a documented fallback (grep the SDK header; add the method to shared `scheduler.h` if missing).
- **Hardware gate:** none of these three tasks close `Z3.9` (Android hardware gate) or add a new hardware gate beyond the deep-sleep bench verification already tracked for Arduino — this plan is software-parity only, consistent with the rest of `firmware-zephyr`'s existing "code complete, bench pending" pattern.
