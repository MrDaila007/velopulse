# BLE Connection Indicator (Э2.7) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Show a small "BLE" indicator on the OLED when a phone/companion is connected, closing the last open item of Э2.7 (`tasks/firmware/README.md`).

**Architecture:** `DisplaySnapshot`/`DisplayFrame` (both in `firmware/include/types.h`) get a `ble_connected` bool that flows: `AppController::updateDisplay()` reads `BleManager::bleConnected()` → `DisplayFormatter::format()` copies it into the frame → `drawDisplayFrame()` in the shared `display_layout.cpp` renderer draws the literal text `"BLE"` when set, nothing otherwise. Because the field defaults to `false`, every existing pixel-golden frame is unaffected; only a new `ble_connected` scenario is added.

**Tech Stack:** C++17, PlatformIO Unity tests (`env:native`), Python `unittest` + Pillow pixel-golden harness (`simulator/`).

## Global Constraints

- `firmware/lib/domain/*` must stay Arduino-independent (compiles under `env:native`, `-I lib/domain`, no `Arduino.h`).
- `firmware/src/*` (AppController, BleManager) only compiles under the `arduino` framework — it has no native test coverage; verify those changes via `pio run -e xiao_ble_sense[_128x32|_deep_sleep]` builds, not `pio test -e native`.
- Don't touch existing golden PNGs in `simulator/golden/` — the new scenario must be additive so all 22 current goldens stay byte-identical.
- Commit messages: `<type>: <description>` (e.g. `feat(firmware): ...`), no attribution trailer.
- Run every command from the repo root `/home/user/Documents/velopulse` unless a step says otherwise.

---

### Task 1: Propagate `ble_connected` through the data model

**Files:**
- Modify: `firmware/include/types.h:49-72` (`DisplaySnapshot`, `DisplayFrame`)
- Modify: `firmware/lib/domain/display_formatter.cpp:54-138` (`DisplayFormatter::format`)
- Test: `firmware/test/test_native/test_main.cpp` (new test near line 1657, registered near line 3158)

**Interfaces:**
- Produces: `DisplaySnapshot::ble_connected` (bool, default `false`), `DisplayFrame::ble_connected` (bool, default `false`) — consumed by Task 2's renderer and Task 3's `AppController` wiring.

- [ ] **Step 1: Write the failing native test**

Add this test right after `test_display_formatter_battery_and_value_limits` (around `firmware/test/test_native/test_main.cpp:1677`):

```cpp
void test_display_formatter_ble_indicator() {
  DisplaySnapshot snapshot;
  snapshot.trip.ride_state = RideState::kIdle;

  DisplayFrame disconnected = DisplayFormatter::format(snapshot, DisplayPage::kTrip);
  TEST_ASSERT_FALSE(disconnected.ble_connected);

  snapshot.ble_connected = true;
  DisplayFrame connected = DisplayFormatter::format(snapshot, DisplayPage::kTrip);
  TEST_ASSERT_TRUE(connected.ble_connected);
}
```

Register it next to the other formatter tests, right after `RUN_TEST(test_display_formatter_low_battery_warning);` (`firmware/test/test_native/test_main.cpp:3159`):

```cpp
  RUN_TEST(test_display_formatter_ble_indicator);
```

- [ ] **Step 2: Run the test to verify it fails**

Run: `cd firmware && pio test -e native -f test_display_formatter_ble_indicator`
Expected: build FAILS — `DisplaySnapshot`/`DisplayFrame` have no member `ble_connected`.

- [ ] **Step 3: Add the field to both structs**

In `firmware/include/types.h`, inside `struct DisplaySnapshot` (after `bool companion_weather_stale = false;`, before the closing `};` at line 59):

```cpp
  bool ble_connected = false;
```

Inside `struct DisplayFrame` (after `bool low_battery_warning = false;`, before the closing `};` at line 72):

```cpp
  bool ble_connected = false;
```

- [ ] **Step 4: Propagate the field in the formatter**

In `firmware/lib/domain/display_formatter.cpp`, inside `DisplayFormatter::format`, right after `DisplayFrame frame;` (line 57):

```cpp
  frame.ble_connected = snapshot.ble_connected;
```

- [ ] **Step 5: Run the test to verify it passes**

Run: `cd firmware && pio test -e native -f test_display_formatter_ble_indicator`
Expected: PASS.

- [ ] **Step 6: Run the full native suite to confirm no regressions**

Run: `cd firmware && pio test -e native`
Expected: all tests pass (128/128 — 127 existing + the new one).

- [ ] **Step 7: Commit**

```bash
git add firmware/include/types.h firmware/lib/domain/display_formatter.cpp firmware/test/test_native/test_main.cpp
git commit -m "feat(firmware): propagate BLE connection state into DisplayFrame"
```

---

### Task 2: Draw the indicator and cover it with a pixel-golden scenario

**Files:**
- Modify: `firmware/lib/domain/display_layout.cpp:8-127` (`draw128x32`, `draw128x64`)
- Modify: `simulator/firmware_renderer.cpp:52-125` (`makeScenario`)
- Modify: `simulator/draw_bikecomp.py:21-22` (`SCENARIO_ORDER`/`GOLDEN_SCENARIOS`)
- Modify: `simulator/tests/test_display.py` (new test)
- Create: `simulator/golden/128x32/ble_connected.png`, `simulator/golden/128x64/ble_connected.png`

**Interfaces:**
- Consumes: `DisplayFrame::ble_connected` (Task 1).
- Produces: `drawBleIndicator(DisplayCanvas&, const DisplayFrame&)` — internal to `display_layout.cpp`, no other file calls it directly.

- [ ] **Step 1: Register the new scenario in the renderer harness**

In `simulator/firmware_renderer.cpp`, inside `makeScenario`, add a branch right after the `else if (name == "low_battery")` block (after line 96, before `else if (name == "speed_0")`):

```cpp
  } else if (name == "ble_connected") {
    page = DisplayPage::kTrip;
    snapshot.ble_connected = true;
```

- [ ] **Step 2: Register the scenario name in the Python side**

In `simulator/draw_bikecomp.py`, change line 22 from:

```python
GOLDEN_SCENARIOS = SCENARIO_ORDER + ("idle", "paused", "battery_unknown", "low_battery")
```

to:

```python
GOLDEN_SCENARIOS = SCENARIO_ORDER + (
    "idle", "paused", "battery_unknown", "low_battery", "ble_connected",
)
```

- [ ] **Step 3: Write the failing Python assertion test**

Add to `simulator/tests/test_display.py`, right after `test_burn_in_offset_moves_shared_layout_for_both_profiles` (before `test_all_scenarios_match_golden_pixels_for_both_profiles`):

```python
    def test_ble_indicator_shown_only_when_connected(self):
        for display_height in DISPLAY_HEIGHTS:
            with self.subTest(display_height=display_height):
                connected = firmware_commands("ble_connected", display_height)
                self.assertIn(["TEXT", "56", "7", "BLE"], connected)
                trip = firmware_commands("trip", display_height)
                self.assertNotIn(["TEXT", "56", "7", "BLE"], trip)
```

- [ ] **Step 4: Run the test to verify it fails**

Run: `cd simulator && .venv/bin/python -m unittest tests.test_display.DisplaySimulatorTest.test_ble_indicator_shown_only_when_connected -v`
Expected: FAIL — `assertIn` cannot find `["TEXT", "56", "7", "BLE"]` because nothing draws it yet (the renderer rebuilds automatically via `_ensure_renderer()`'s mtime check).

- [ ] **Step 5: Implement the indicator in the shared renderer**

In `firmware/lib/domain/display_layout.cpp`, add constants next to the existing ones (after line 11, `constexpr int16_t kBatteryIconTipX = 119;`):

```cpp
constexpr int16_t kBleIndicatorX = 56;
constexpr int16_t kBleIndicatorY = 7;
```

Add the drawing helper next to `drawBatteryLabels` (after its closing `}` at line 46):

```cpp
void drawBleIndicator(DisplayCanvas& canvas, const DisplayFrame& frame) {
  if (!frame.ble_connected) return;
  canvas.drawText(kBleIndicatorX, kBleIndicatorY, "BLE");
}
```

Call it from `draw128x32` (after `canvas.setFont(DisplayFont::kSmall);` at line 77, before `drawBatteryLabels(...)`):

```cpp
  drawBleIndicator(canvas, frame);
```

Call it from `draw128x64` (after `canvas.drawText(0, 7, frame.units);` at line 98, before `drawBattery128x64(canvas, frame);`):

```cpp
  drawBleIndicator(canvas, frame);
```

- [ ] **Step 6: Run the test to verify it passes**

Run: `cd simulator && .venv/bin/python -m unittest tests.test_display.DisplaySimulatorTest.test_ble_indicator_shown_only_when_connected -v`
Expected: PASS.

Note: `test_all_scenarios_match_golden_pixels_for_both_profiles` and `test_frames_are_visible_and_distinct_for_both_profiles` will now fail because `ble_connected` was added to `GOLDEN_SCENARIOS` but has no golden PNG yet — that's expected until Step 7.

- [ ] **Step 7: Generate the two new golden frames**

Run:
```bash
cd simulator
xvfb-run -a .venv/bin/python render_frames.py --scenario ble_connected --display-height 32 --scale 1 --output golden/128x32/ble_connected.png
xvfb-run -a .venv/bin/python render_frames.py --scenario ble_connected --display-height 64 --scale 1 --output golden/128x64/ble_connected.png
```

Open both PNGs (or use `render_frames.py --scenario ble_connected --display-height 64 --scale 6 --output /tmp/preview.png` for a readable copy) and confirm "BLE" is legible and doesn't overlap the units/battery text.

- [ ] **Step 8: Run the full simulator suite**

Run: `./simulator/test.sh`
Expected: all groups pass, now with **24 golden frames** (12 scenarios × 2 profiles).

- [ ] **Step 9: Commit**

```bash
git add firmware/lib/domain/display_layout.cpp simulator/firmware_renderer.cpp simulator/draw_bikecomp.py simulator/tests/test_display.py simulator/golden/128x32/ble_connected.png simulator/golden/128x64/ble_connected.png
git commit -m "feat(firmware): draw BLE connection indicator in shared OLED layout"
```

---

### Task 3: Wire the live BLE state into AppController and close the docs

**Files:**
- Modify: `firmware/src/app_controller.cpp:1512-1538` (`AppController::updateDisplay`)
- Modify: `tasks/firmware/README.md:32-36` (item 2.7)
- Modify: `STATUS.md` (remove the "Нет BLE-индикатора" bullet under «Ограничения», add a line under «Готово»)

**Interfaces:**
- Consumes: `DisplaySnapshot::ble_connected` (Task 1), `BleManager::bleConnected() const` (`firmware/src/ble_manager.h:72`, already implemented).

- [ ] **Step 1: Wire the live value**

In `firmware/src/app_controller.cpp`, inside `AppController::updateDisplay`, right after `snapshot.battery = battery_.snapshot();` (line 1520):

```cpp
  snapshot.ble_connected = ble_.bleConnected();
```

- [ ] **Step 2: Build all three embedded profiles**

This file isn't part of `env:native` (`platformio.ini` only adds `-I lib/domain` there), so the only verification is a successful embedded build:

Run: `cd firmware && pio run -e xiao_ble_sense`
Expected: SUCCESS. Note the reported RAM/Flash size (compare against the `191 028 Б` / `17 968 Б` baseline in `STATUS.md` — a few bytes of growth from one new `bool` field is expected).

Run: `cd firmware && pio run -e xiao_ble_sense_128x32`
Expected: SUCCESS.

Run: `cd firmware && pio run -e xiao_ble_sense_deep_sleep`
Expected: SUCCESS.

- [ ] **Step 3: Re-run the native and simulator suites as a final regression pass**

Run: `cd firmware && pio test -e native`
Expected: 128/128 pass.

Run: `./simulator/test.sh`
Expected: all groups pass (24 golden frames).

- [ ] **Step 4: Update `tasks/firmware/README.md`**

Change the 2.7 line (`tasks/firmware/README.md:32-36`) from an open checkbox describing the missing wiring to a closed one, e.g.:

```markdown
- [x] 2.7 Завершить DisplayManager. BLE-индикатор реализован: `DisplaySnapshot`/
  `DisplayFrame` (`include/types.h`) содержат `ble_connected`, `display_layout.cpp`
  рисует `"BLE"` при подключении, `AppController::updateDisplay()` подключает
  `BleManager::bleConnected()`. Simulator gate: сценарий `ble_connected`,
  24 golden-кадра.
```

- [ ] **Step 5: Update `STATUS.md`**

Remove the bullet starting with "Нет BLE-индикатора на экране (Э2.7)" from the «Ограничения» section, and add a short line to «Готово → Прошивка» describing the same wiring (mirror the style of the neighboring `DisplayBurnInGuard`/`AmbientLightManager` bullets — one paragraph, file references, no prose beyond what Steps 1–2 above actually did).

- [ ] **Step 6: Commit**

```bash
git add firmware/src/app_controller.cpp tasks/firmware/README.md STATUS.md
git commit -m "feat(firmware): wire live BLE connection state into OLED indicator (closes Э2.7)"
```

---

## Self-Review

- **Spec coverage:** `tasks/firmware/README.md:32-36` (Э2.7 missing BLE indicator) is fully addressed — data model (Task 1), rendering + pixel-golden coverage (Task 2), live wiring + docs (Task 3).
- **Placeholder scan:** no TBD/"add appropriate"/"similar to" — every step has literal code or literal commands.
- **Type consistency:** `ble_connected` is `bool` throughout (`DisplaySnapshot`, `DisplayFrame`); `drawBleIndicator` takes `(DisplayCanvas&, const DisplayFrame&)` matching the existing `drawBatteryLabels`/`drawBatteryIcon` helper signatures in the same file; scenario name `"ble_connected"` is identical across `firmware_renderer.cpp`, `draw_bikecomp.py`, `test_display.py`, and the two golden filenames.
