# Bound the Low-Power Idle Loop Delay Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Bound the single largest blocking window in `AppController::loop()` — the low-power-idle `delay(delay_ms)` that can currently block for up to ~1 second — to a small, fixed chunk, so pulse polling, the Serial console, and the scheduler get serviced far more often without changing overall idle power behavior.

**Architecture:** Extract the "how long to actually block for" decision into a pure, native-testable function `clampIdleDelayMs(requested_delay_ms, max_chunk_ms)` in a new `firmware/lib/domain/idle_delay.{h,cpp}` file (mirrors how `Scheduler`'s timing logic already lives in `lib/domain`, Arduino-independent). `AppController::loop()` calls it before its existing `delay(delay_ms)`. Because `loop()` is called back-to-back in a tight Arduino main loop, chunking the wait doesn't change the *total* time spent idle before the next due task — it only shrinks the longest single unresponsive window from ~1000 ms to `kMaxIdleDelayChunkMs` (50 ms).

**Tech Stack:** C++17, PlatformIO Unity tests (`env:native`).

## Global Constraints

- `firmware/lib/domain/*` must stay Arduino-independent (compiles under `env:native`, no `Arduino.h`, no `delay()`/`millis()` calls inside the pure function itself).
- Explicitly out of scope for this plan (call this out, don't attempt it): the synchronous `InternalFsBackend::write` flash I/O (`remove+write+flush+close`) that runs on the scheduler path during odometer/ambient-calibration/counters saves, and the `delay(2)` calls inside `printGpioProbe()` (`firmware/src/app_controller.cpp:519,536,541`, only reachable from the Serial console, not the hot loop). Both would need a real async/state-machine redesign of `InternalFsBackend` and deserve their own dedicated plan — bounding them here without characterizing that API first would violate "no placeholders."
- Commit messages: `<type>: <description>`, no attribution trailer.
- Run every command from the repo root `/home/user/Documents/velopulse` unless a step says otherwise.

---

### Task 1: `clampIdleDelayMs` — pure, native-tested helper

**Files:**
- Create: `firmware/lib/domain/idle_delay.h`
- Create: `firmware/lib/domain/idle_delay.cpp`
- Test: `firmware/test/test_native/test_main.cpp`

**Interfaces:**
- Produces: `uint32_t clampIdleDelayMs(uint32_t requested_delay_ms, uint32_t max_chunk_ms)` — consumed by Task 2's `AppController::loop()`.

- [ ] **Step 1: Write the failing test**

Add to `firmware/test/test_native/test_main.cpp`, right after the last scheduler test, `test_scheduler_duration_wraps_safely_across_micros_rollover` (search for it; it's the function immediately before `RUN_TEST(test_scheduler_period_and_wrap);` at line 3120 — add the new test function near the other scheduler test *definitions*, not the RUN_TEST block):

```cpp
void test_clamp_idle_delay_bounds_to_max_chunk() {
  TEST_ASSERT_EQUAL_UINT32(50u, clampIdleDelayMs(1000u, 50u));
  TEST_ASSERT_EQUAL_UINT32(30u, clampIdleDelayMs(30u, 50u));
  TEST_ASSERT_EQUAL_UINT32(50u, clampIdleDelayMs(50u, 50u));
  TEST_ASSERT_EQUAL_UINT32(0u, clampIdleDelayMs(0u, 50u));
  TEST_ASSERT_EQUAL_UINT32(1000u, clampIdleDelayMs(1000u, 0u));
}
```

Add the include next to the other domain headers, after `#include "display_power.h"` and before `#include "odometer_save_policy.h"` (`firmware/test/test_native/test_main.cpp:32-33`):

```cpp
#include "idle_delay.h"
```

Register the test right after `RUN_TEST(test_scheduler_duration_wraps_safely_across_micros_rollover);` (`firmware/test/test_native/test_main.cpp:3126`):

```cpp
  RUN_TEST(test_clamp_idle_delay_bounds_to_max_chunk);
```

- [ ] **Step 2: Run the test to verify it fails**

Run: `cd firmware && pio test -e native -f test_clamp_idle_delay_bounds_to_max_chunk`
Expected: build FAILS — `idle_delay.h` doesn't exist / `clampIdleDelayMs` is undeclared.

- [ ] **Step 3: Create the header**

Create `firmware/lib/domain/idle_delay.h`:

```cpp
#pragma once

#include <stdint.h>

namespace bike {

// Bounds a requested idle wait to at most max_chunk_ms, so a caller doing
// `delay(clampIdleDelayMs(requested, cap))` in a loop never blocks longer
// than one chunk at a time while still reaching the same total wait.
// max_chunk_ms == 0 disables clamping (returns requested_delay_ms unchanged).
uint32_t clampIdleDelayMs(uint32_t requested_delay_ms, uint32_t max_chunk_ms);

}  // namespace bike
```

- [ ] **Step 4: Implement it**

Create `firmware/lib/domain/idle_delay.cpp`:

```cpp
#include "idle_delay.h"

namespace bike {

uint32_t clampIdleDelayMs(uint32_t requested_delay_ms, uint32_t max_chunk_ms) {
  if (max_chunk_ms == 0u) return requested_delay_ms;
  return requested_delay_ms < max_chunk_ms ? requested_delay_ms : max_chunk_ms;
}

}  // namespace bike
```

- [ ] **Step 5: Run the test to verify it passes**

Run: `cd firmware && pio test -e native -f test_clamp_idle_delay_bounds_to_max_chunk`
Expected: PASS.

- [ ] **Step 6: Run the full native suite to confirm no regressions**

Run: `cd firmware && pio test -e native`
Expected: all tests pass.

- [ ] **Step 7: Commit**

```bash
git add firmware/lib/domain/idle_delay.h firmware/lib/domain/idle_delay.cpp firmware/test/test_native/test_main.cpp
git commit -m "feat(firmware): add clampIdleDelayMs helper for bounding blocking waits"
```

---

### Task 2: Use the clamp in `AppController::loop()`

**Files:**
- Modify: `firmware/src/app_controller.cpp:59-86` (constants block), `:609-629` (`AppController::loop`)
- Modify: `tasks/firmware/README.md:76-84` (update the loop-blocking bullet), `STATUS.md` (update the matching «Ограничения» bullet)

**Interfaces:**
- Consumes: `clampIdleDelayMs` (Task 1).

- [ ] **Step 1: Add the include and the tunable constant**

In `firmware/src/app_controller.cpp`, add the include near the other local includes (check the existing `#include` block above line 59 and add alphabetically, e.g. next to any `#include "..._policy.h"` line).

Add the constant to the existing anonymous-namespace tunables block, after `constexpr uint32_t kSchedBleBudgetUs = 300000u;       // up to 3 chained flash writes.` (`firmware/src/app_controller.cpp:82`):

```cpp
constexpr uint32_t kMaxIdleDelayChunkMs = 50u;  // bound a single delay() in low-power idle.
```

- [ ] **Step 2: Use the clamp in `loop()`**

Replace the low-power-idle branch in `AppController::loop()` (`firmware/src/app_controller.cpp:620-625`):

```cpp
  if (power_manager_.systemMode() == SystemPowerMode::kLowPowerIdle) {
    const uint32_t next_due = scheduler_.nextDueMs(now_ms);
    if (next_due > now_ms && next_due != UINT32_MAX) {
      const uint32_t delay_ms =
          clampIdleDelayMs(next_due - now_ms, kMaxIdleDelayChunkMs);
      if (delay_ms > 1u) delay(delay_ms);
    }
  } else {
    yield();
  }
```

- [ ] **Step 3: Build all three embedded profiles**

`app_controller.cpp` isn't part of `env:native`; verify via embedded builds.

Run: `cd firmware && pio run -e xiao_ble_sense`
Expected: SUCCESS.

Run: `cd firmware && pio run -e xiao_ble_sense_128x32`
Expected: SUCCESS.

Run: `cd firmware && pio run -e xiao_ble_sense_deep_sleep`
Expected: SUCCESS.

- [ ] **Step 4: Re-run the native suite as a final regression pass**

Run: `cd firmware && pio test -e native`
Expected: all pass.

- [ ] **Step 5: Update `tasks/firmware/README.md`**

In the «Общие firmware-долги» section (`tasks/firmware/README.md:76-84`), the bullet currently reads (in part): "Найдено: `delay(delay_ms)` до ~1000 мс в low-power idle (`src/app_controller.cpp:579`)...". Update it to note the idle branch is now bounded, while keeping the flash-write and `printGpioProbe` sub-items open, e.g.:

```markdown
- [ ] Исключить длительные блокировки из production loop и проверить ISR review.
  Low-power idle `delay()` теперь ограничен `kMaxIdleDelayChunkMs` = 50 мс
  (`clampIdleDelayMs`, `idle_delay.{h,cpp}`) вместо до ~1000 мс. Остаются: синхронные
  flash-записи (remove+write+flush+close в `InternalFsBackend::write`) прямо на
  scheduler-пути при автосохранении одометра/ambient-калибровки/counters; несколько
  `delay(2)` в `printGpioProbe()`, вызываемой из Serial-консоли (`:519,536,541`).
  watchdog-таймаут 8 с и `sched` (см. 2.1) остаются сетью безопасности и
  измерительным инструментом для оставшихся блокировок.
```

- [ ] **Step 6: Update `STATUS.md`**

In the «Ограничения» section, update the sentence describing "`delay(delay_ms)` до ~1000 мс в low-power idle между задачами планировщика" to state it's now bounded to 50 ms via `clampIdleDelayMs`, keeping the rest of that bullet (flash writes, `printGpioProbe`) unchanged — mirror the phrasing style already used for the watchdog bullet immediately above it.

- [ ] **Step 7: Commit**

```bash
git add firmware/src/app_controller.cpp tasks/firmware/README.md STATUS.md
git commit -m "fix(firmware): bound low-power idle delay to 50ms chunks"
```

---

## Self-Review

- **Spec coverage:** the low-power-idle `delay(delay_ms)` item from `tasks/firmware/README.md:76-84` ("Найдено: `delay(delay_ms)` до ~1000 мс в low-power idle") is addressed by Tasks 1-2. The remaining two sub-items in that same bullet (synchronous flash writes, `printGpioProbe` delay(2)) are explicitly deferred with rationale in Global Constraints — not silently dropped.
- **Placeholder scan:** no TBD/vague steps; every step has literal code, exact file:line anchors, and literal shell commands.
- **Type consistency:** `clampIdleDelayMs(uint32_t, uint32_t) -> uint32_t` matches the types already flowing through `loop()` (`scheduler_.nextDueMs(now_ms)` returns `uint32_t`, `next_due - now_ms` is `uint32_t`, `delay_ms` was already `uint32_t`) — no new conversions introduced.
- **Behavior-preservation note:** total idle duration before the next due task is unchanged (chunking sums to the same wait); only the longest single unresponsive window shrinks from ~1000 ms to 50 ms. This is called out explicitly in the Architecture section so a reviewer doesn't mistake it for a power-consumption change.
