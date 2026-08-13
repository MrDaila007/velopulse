#ifndef BIKECOMP_HALL_PULLUP
#define BIKECOMP_HALL_PULLUP 1
#endif

#ifndef BIKECOMP_HALL_ACTIVE_EDGE
#define BIKECOMP_HALL_ACTIVE_EDGE (-1)
#endif

#ifndef BIKECOMP_HALL_ANALOG
#define BIKECOMP_HALL_ANALOG 0
#endif

#ifndef BIKECOMP_HALL_ADC_OPEN_MIN
#define BIKECOMP_HALL_ADC_OPEN_MIN 2500
#endif

#ifndef BIKECOMP_HALL_ADC_CLOSED_MAX
#define BIKECOMP_HALL_ADC_CLOSED_MAX 900
#endif

#ifndef BIKECOMP_HALL_TWO_WIRE
#define BIKECOMP_HALL_TWO_WIRE 0
#endif

#include "app_controller.h"

#include <Arduino.h>

#include "ble_device_info.h"
#include "ble_config_write.h"
#include "ble_telemetry.h"
#include "board_leds.h"
#include "board_pins.h"
#include "config_codec.h"
#include "display_profile.h"
#include "board_pins.h"
#include "boot_counter.h"
#include "idle_delay.h"
#include "platform/deep_sleep.h"
#include "platform/watchdog.h"
#include "serial_usb_test.h"
#include "watchdog_config.h"

#ifndef BIKECOMP_HOTPATH_SERIAL
#define BIKECOMP_HOTPATH_SERIAL 0
#endif

#ifndef BIKECOMP_OPEN_PAIRING
#define BIKECOMP_OPEN_PAIRING 0
#endif

// Enabled by default: a wedged main loop otherwise means a dead device until
// manual power-cycle. Set to 0 to disable for debugging a suspected hang
// without the reset masking it.
#ifndef BIKECOMP_FEATURE_WATCHDOG
#define BIKECOMP_FEATURE_WATCHDOG 1
#endif

namespace bike {
namespace {

constexpr size_t kTaskPulses = 0;
constexpr size_t kTaskState = 1;
constexpr size_t kTaskAmbient = 2;
constexpr size_t kTaskBattery = 3;
constexpr size_t kTaskDisplay = 4;
constexpr size_t kTaskBle = 5;
constexpr size_t kTaskStorage = 6;

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

// Bounds how long AppController::loop()'s low-power-idle branch can ever
// sleep in one delay() call, regardless of what kLowPowerSchedulerPeriods
// (power_manager.h) says. Raising those periods for power savings won't take
// effect beyond this cap unless this constant is raised too.
constexpr uint32_t kMaxIdleDelayChunkMs = 50u;  // bound a single delay() in low-power idle.

uint32_t schedulerMicros() { return static_cast<uint32_t>(micros()); }

constexpr bool kWatchdogEnabled = (BIKECOMP_FEATURE_WATCHDOG != 0);

int interruptMode(uint8_t active_edge) {
  if (active_edge == 1) return RISING;
  if (active_edge == 2) return CHANGE;
  return FALLING;
}

void printUint64(uint64_t value) {
  char buffer[21];
  char* cursor = buffer + sizeof(buffer);
  *--cursor = '\0';
  do {
    *--cursor = static_cast<char>('0' + value % 10u);
    value /= 10u;
  } while (value != 0);
  Serial.print(cursor);
}

uint16_t saturateErrorDetail(uint32_t value) {
  return value > 0xFFFFu ? 0xFFFFu : static_cast<uint16_t>(value);
}

void printLoadInfo(const char* label,
                   const StorageLoadInfo& info,
                   bool load_ok) {
  Serial.print(label);
  Serial.print(": source=");
  Serial.print(storageSourceName(info.source));
  Serial.print(", sequence=");
  Serial.print(info.sequence);
  Serial.print(", version=");
  Serial.print(info.from_version);
  Serial.print(", recovered=");
  Serial.print(info.recovered ? "yes" : "no");
  Serial.print(", defaults_restored=");
  Serial.print(info.source == StorageSource::kDefaults ? "yes" : "no");
  Serial.print(", migrated=");
  Serial.print(info.migrated ? "yes" : "no");
  if (info.migrated) {
    Serial.print(", migration_written=");
    Serial.print(info.migration_written ? "yes" : "no");
  }
  Serial.print(", status=");
  Serial.println(load_ok ? "OK" : "ERROR");
}

}  // namespace

AppController::AppController()
    : storage_(storage_backend_),
      pulse_filter_(PulseFilterConfig{config_.wheel_circumference_mm,
                                      config_.max_speed_kmh,
                                      config_.debounce_ms,
                                      500}),
      ride_state_(static_cast<uint32_t>(config_.stop_timeout_s) * 1000u),
      tasks_{{"pulses", 0, 0, pulseTask, this, 0, kSchedPulsesBudgetUs},
             {"state", 100, 0, stateTask, this, 0, kSchedStateBudgetUs},
             {"ambient", 10, 0, ambientTask, this, 0, kSchedAmbientBudgetUs},
             {"battery", 1000, 0, batteryTask, this, 0, kSchedBatteryBudgetUs},
             {"display", 50, 0, displayTask, this, 0, kSchedDisplayBudgetUs},
             {"ble", 100, 0, bleTask, this, 0, kSchedBleBudgetUs},
             {"storage", 50, 0, storageTask, this, 0, kSchedStorageBudgetUs}},
      scheduler_(tasks_, 7, schedulerMicros) {}

void AppController::begin() {
  // CONFIG/RREN/CRV are write-locked once the WDT is running, so configure
  // it before anything else — but do not start it yet: the boot sequence
  // below has multi-hundred-ms blocking steps (Serial wait, flash mount,
  // SoftDevice enable) that a running watchdog would trip on.
  if (kWatchdogEnabled) watchdogConfigure(BIKECOMP_WDT_TIMEOUT_MS);

  Serial.begin(115200);
  const uint32_t serial_started = millis();
  while (!Serial && static_cast<uint32_t>(millis() - serial_started) < 1500u) yield();

  Serial.println();
  Serial.print("BikeComp FW ");
  Serial.println(FW_VERSION);

  beginBoardLeds();

  const uint32_t resetreas = NRF_POWER->RESETREAS;
  const ResetReason reset_reason = mapNrfResetReason(resetreas);
  // Clear sticky bits so the next boot sees a fresh reason.
  NRF_POWER->RESETREAS = resetreas;
  Serial.print("reset_reason=");
  Serial.print(static_cast<unsigned>(reset_reason));
  Serial.print(" (RESETREAS=0x");
  Serial.print(resetreas, HEX);
  Serial.println(')');

  const bool fs_ok = storage_.begin();
  Serial.print("Flash FS: ");
  Serial.println(fs_ok ? "OK" : "MOUNT FAILED");

  uint16_t boot_count = 0;
  if (fs_ok) {
    boot_count = loadAndIncrementBootCount(storage_backend_);
  }
  Serial.print("boot_count=");
  Serial.println(boot_count);

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
  Serial.print("Odometer value: ");
  printUint64(trip_computer_.snapshot().odometer_mm);
  Serial.print(" mm, total revolutions: ");
  printUint64(trip_computer_.totalRevolutions());
  Serial.println();

  odometer_save_.configure(config_.odometer_save_interval_m);
  odometer_save_.markSaved(trip_computer_.snapshot().odometer_mm);

  pulse_filter_.configure(PulseFilterConfig{
      config_.wheel_circumference_mm, config_.max_speed_kmh,
      config_.debounce_ms, 500});
  ride_state_ = RideStateMachine(
      static_cast<uint32_t>(config_.stop_timeout_s) * 1000u);

  Serial.println("Hall: reed D0 <-> D1 (drive LOW)");
  wheel_sensor_.begin(kHallPin, interruptMode(config_.active_edge));
  const bool display_ok = display_.begin(config_);
  Serial.print("OLED 0x3C: ");
  Serial.println(display_ok ? "OK" : "NOT FOUND; counting remains active");
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
  Serial.print("Battery: ");
  Serial.print(battery_.snapshot().millivolts);
  Serial.print(" mV, ");
  Serial.print(battery_.snapshot().percent);
  Serial.println("%");
  odometer_save_.noteUsbPresent(battery_.snapshot().usb_present);
  odometer_save_.noteBatteryPercent(battery_.snapshot().percent,
                                    battery_.snapshot().valid);
  ride_state_.reset(millis());
  odometer_save_.noteRideState(ride_state_.state(), millis());
  odometer_save_.noteDisplayPower(display_.powerState());
  configurePowerManager();
  applySchedulerPeriods(millis());
  if (deepSleepWakeFromSleep(resetreas)) {
    Serial.print("wake_source=");
    Serial.println(deepSleepWakeSourceName());
  }

  BleBootSeed ble_seed;
  ble_seed.config_from_flash =
      config_ok && config_info.source != StorageSource::kDefaults;
  ble_seed.display_ok = display_ok;
  ble_seed.fs_ok = fs_ok;
  ble_seed.usb_connected = battery_.snapshot().usb_present;
  ble_seed.battery_percent = battery_.snapshot().percent;
  ble_seed.reset_reason = static_cast<uint8_t>(reset_reason);
  ble_seed.boot_count = boot_count;
  ble_seed.boot_ms = millis();
  ble_seed.open_pairing_always = (BIKECOMP_OPEN_PAIRING != 0);
  const bool ble_ok = ble_.begin(config_, ble_seed);
  Serial.print("BLE GATT: ");
  Serial.println(ble_ok ? "OK" : "INIT FAILED");
  if (ble_ok) {
    const uint32_t now_ms = millis();
    if (!display_ok) {
      ble_.recordError(ErrorLogCode::kI2cTimeout, ErrorLogSeverity::kError,
                       kDisplayI2cAddress, now_ms);
    }
    if (!fs_ok) {
      ble_.recordError(ErrorLogCode::kFlashError, ErrorLogSeverity::kError,
                       0, now_ms);
    }
    if (config_info.recovered) {
      ble_.recordError(ErrorLogCode::kConfigCrc, ErrorLogSeverity::kWarn,
                       0, now_ms);
    }
    if (reset_reason == ResetReason::kWatchdog) {
      ble_.recordError(ErrorLogCode::kWatchdogReset, ErrorLogSeverity::kError,
                       0, now_ms);
    }
  }

  selftest_mask_ = 0;
  if (display_ok) selftest_mask_ |= kSelftestDisplayOk;
  selftest_mask_ |= kSelftestHallPinOk;
  if (battery_.snapshot().valid) selftest_mask_ |= kSelftestAdcOk;
  if (fs_ok) selftest_mask_ |= kSelftestFsOk;
  if (ble_seed.config_from_flash) selftest_mask_ |= kSelftestConfigValid;
  if (ble_ok) selftest_mask_ |= kSelftestBleOk;
  // Start last: every blocking step above (Serial wait, flash mount, boot
  // counter write, SoftDevice enable) has now run, so the WDT timeout only
  // has to cover steady-state loop() work from here on.
  if (kWatchdogEnabled) {
    watchdogStart();
    if (watchdogStarted()) selftest_mask_ |= kSelftestWatchdogOk;
  }
  printDiagnostics();
}

DiagnosticSnapshot AppController::diagnosticSnapshot() const {
  const int free_heap = dbgHeapFree();
  return buildDiagnosticSnapshot(makeDiagnosticSources(
      storage_.counters(), pulse_filter_.counters(),
      wheel_sensor_.rawPulseCount(), wheel_sensor_.overflowCount(),
      free_heap > 0 ? static_cast<uint32_t>(free_heap) : 0u,
      display_.isOk() ? 0u : 1u, selftest_mask_));
}

void AppController::printDiagnostics() const {
  const DiagnosticSnapshot diag = diagnosticSnapshot();
  uint8_t payload[kDiagnosticPayloadSize];
  encodeDiagnosticPayload(diag, payload);

  Serial.print("Diagnostics: pulses=");
  Serial.print(diag.raw_pulse_count);
  Serial.print(", debounce=");
  Serial.print(diag.rejected_debounce);
  Serial.print(", overspeed=");
  Serial.print(diag.rejected_overspeed);
  Serial.print(", isr_ovf=");
  Serial.print(diag.isr_overflow);
  Serial.print(", flash_writes=");
  Serial.print(diag.flash_write_count);
  Serial.print(", heap/16=");
  Serial.print(diag.free_heap_units);
  Serial.print(", i2c_err=");
  Serial.print(diag.i2c_error_count);
  Serial.print(", selftest=0x");
  if (diag.selftest_mask < 0x10u) Serial.print('0');
  Serial.print(diag.selftest_mask, HEX);
  Serial.print(", payload=");
  for (size_t i = 0; i < kDiagnosticPayloadSize; ++i) {
    if (payload[i] < 0x10u) Serial.print('0');
    Serial.print(payload[i], HEX);
  }
  Serial.println();

  const AmbientLightSnapshot& ambient = ambient_light_.snapshot();
  Serial.print("Ambient: enabled=");
  Serial.print(ambient_light_.enabled() ? 1 : 0);
  Serial.print(", valid=");
  Serial.print(ambient.valid ? 1 : 0);
  Serial.print(", raw=");
  Serial.print(ambient.raw);
  Serial.print(", filtered=");
  Serial.print(ambient.filtered_raw);
  Serial.print(", auto_pct=");
  Serial.print(ambient.brightness_pct);
  Serial.print(", effective_pct=");
  Serial.println(display_.effectiveBrightnessPct());
}

void AppController::printAmbientLine() const {
  const AmbientLightSnapshot& ambient = ambient_light_.snapshot();
  Serial.print("Ambient: enabled=");
  Serial.print(ambient_light_.enabled() ? 1 : 0);
  Serial.print(", valid=");
  Serial.print(ambient.valid ? 1 : 0);
  Serial.print(", raw=");
  Serial.print(ambient.raw);
  Serial.print(", filtered=");
  Serial.print(ambient.filtered_raw);
  Serial.print(", auto_pct=");
  Serial.print(ambient.brightness_pct);
  Serial.print(", effective_pct=");
  Serial.println(display_.effectiveBrightnessPct());
}

void AppController::printHallStatus() const {
  const DiagnosticSnapshot diag = diagnosticSnapshot();
  Serial.print("Hall: pin=");
  Serial.print(kHallPinLabel);
#if BIKECOMP_HALL_TWO_WIRE
  Serial.print(" (");
  Serial.print(kHallSensePinLabel);
  Serial.print(" / ");
  Serial.print(kHallDrivePinLabel);
  Serial.print("=LOW)");
#endif
  Serial.print(" level=");
  Serial.print(wheel_sensor_.pinIsHigh() ? "HIGH" : "LOW");
#if BIKECOMP_HALL_TWO_WIRE
  Serial.print(", mode=digital");
#elif BIKECOMP_HALL_PULLUP
  Serial.print(", pull=internal");
#else
  Serial.print(", pull=none");
#endif
  Serial.print(", nrf=");
  Serial.print(kHallNrfPin);
  Serial.print(", edge=");
  switch (config_.active_edge) {
    case 0:
      Serial.print("FALLING");
      break;
    case 1:
      Serial.print("RISING");
      break;
    default:
      Serial.print("CHANGE");
      break;
  }
  Serial.print(", raw_pulses=");
  Serial.print(diag.raw_pulse_count);
  Serial.print(", accepted=");
  Serial.print(pulse_filter_.counters().accepted);
  Serial.print(", debounce_rej=");
  Serial.print(diag.rejected_debounce);
  Serial.print(", overspeed_rej=");
  Serial.print(diag.rejected_overspeed);
  Serial.print(", isr_ovf=");
  Serial.print(diag.isr_overflow);
  Serial.print(", ride=");
  switch (ride_state_.state()) {
    case RideState::kIdle:
      Serial.print("IDLE");
      break;
    case RideState::kMoving:
      Serial.print("MOVING");
      break;
    case RideState::kPaused:
      Serial.print("PAUSED");
      break;
  }
  Serial.print(", revolutions=");
  Serial.print(trip_computer_.revolutions());
  const TripSnapshot trip = trip_computer_.snapshot();
  Serial.print(", speed_x100=");
  Serial.print(trip.speed_x100);
  Serial.print(", max_speed_x100=");
  Serial.print(trip.max_speed_x100);
  Serial.print(", gap_corr=");
  Serial.print(speed_calculator_.speedIntervalCorrectedCount());
  Serial.print(", last_interval_us=");
  Serial.print(last_accepted_interval_us_);
#if BIKECOMP_HALL_ANALOG
  Serial.print(", mode=analog, adc=");
  Serial.print(wheel_sensor_.lastAnalogRaw());
  Serial.print(", open_th=");
  Serial.print(BIKECOMP_HALL_ADC_OPEN_MIN);
  Serial.print(", closed_th=");
  Serial.print(BIKECOMP_HALL_ADC_CLOSED_MAX);
#endif
  Serial.println();
}

void AppController::printHallAnalogLine() const {
  Serial.print("Hall analog: pin=");
  Serial.print(kHallPinLabel);
  Serial.print(", adc=");
  Serial.print(wheel_sensor_.lastAnalogRaw());
  Serial.print(", level=");
  Serial.print(wheel_sensor_.pinIsHigh() ? "HIGH" : "LOW");
  Serial.print(", raw_pulses=");
  Serial.print(wheel_sensor_.rawPulseCount());
  Serial.print(", accepted=");
  Serial.println(pulse_filter_.counters().accepted);
}

void AppController::maybeLogHallAnalog(uint32_t now_ms) {
  if (!hall_analog_logging_) return;
  const uint16_t raw = wheel_sensor_.lastAnalogRaw();
  const bool pin_high = wheel_sensor_.pinIsHigh();
  const bool changed =
      raw != hall_analog_last_raw_ || pin_high != hall_watch_last_pin_high_;
  const bool heartbeat =
      static_cast<uint32_t>(now_ms - hall_analog_last_ms_) >= 1000u;
  if (!changed && !heartbeat) return;
  hall_analog_last_ms_ = now_ms;
  hall_analog_last_raw_ = raw;
  hall_watch_last_pin_high_ = pin_high;
  printHallAnalogLine();
}

void AppController::maybeLogHallWatch(uint32_t now_ms) {
  if (!hall_watch_logging_) return;
  const uint32_t pulses = wheel_sensor_.rawPulseCount();
  const bool pin_high = wheel_sensor_.pinIsHigh();
  const bool changed =
      pulses != hall_watch_last_pulses_ || pin_high != hall_watch_last_pin_high_;
  const bool heartbeat =
      static_cast<uint32_t>(now_ms - hall_watch_last_ms_) >= 2000u;
  if (!changed && !heartbeat) return;
  hall_watch_last_ms_ = now_ms;
  hall_watch_last_pulses_ = pulses;
  hall_watch_last_pin_high_ = pin_high;
  printHallStatus();
}

void AppController::restoreHallInterrupt() {
  wheel_sensor_.resumeInterrupt(interruptMode(config_.active_edge));
}

void AppController::applyHallEdge(uint8_t active_edge) {
  if (active_edge > 2u) return;
  config_.active_edge = active_edge;
  wheel_sensor_.begin(kHallPin, interruptMode(active_edge));
}

void AppController::printGpioProbe() {
  wheel_sensor_.suspendInterrupt();

#if BIKECOMP_HALL_ANALOG
#if BIKECOMP_HALL_PULLUP
  pinMode(kHallPin, INPUT_PULLUP);
#else
  pinMode(kHallPin, INPUT);
#endif
  delay(2);
  wheel_sensor_.pollPin();
  restoreHallInterrupt();

  Serial.print("GPIO ");
  Serial.print(kHallPinLabel);
  Serial.print(": mode=analog, ADC=");
  Serial.print(wheel_sensor_.lastAnalogRaw());
  Serial.print(", level=");
  Serial.print(wheel_sensor_.pinIsHigh() ? "HIGH" : "LOW");
  Serial.print(" (open>=");
  Serial.print(BIKECOMP_HALL_ADC_OPEN_MIN);
  Serial.print(", closed<=");
  Serial.print(BIKECOMP_HALL_ADC_CLOSED_MAX);
  Serial.println(')');
#else
  configureHallPins(kHallPin);
  delay(2);

  auto readMode = [](uint8_t pin, uint8_t mode) -> bool {
    configureHallPins(kHallPin);
    pinMode(pin, mode);
    delay(2);
    return digitalRead(pin) == HIGH;
  };

  const bool pullup_high = readMode(kHallPin, INPUT_PULLUP);
  const bool float_high = readMode(kHallPin, INPUT);
  const bool pulldown_high = readMode(kHallPin, INPUT_PULLDOWN);

  configureHallPins(kHallPin);
  restoreHallInterrupt();

  Serial.print("GPIO ");
  Serial.print(kHallPinLabel);
#if BIKECOMP_HALL_TWO_WIRE
  Serial.print(" (");
  Serial.print(kHallSensePinLabel);
  Serial.print(" / ");
  Serial.print(kHallDrivePinLabel);
  Serial.print("=LOW)");
#endif
  Serial.print(": PULLUP=");
  Serial.print(pullup_high ? "HIGH" : "LOW");
  Serial.print(", FLOAT=");
  Serial.print(float_high ? "HIGH" : "LOW");
  Serial.print(", PULLDOWN=");
  Serial.println(pulldown_high ? "HIGH" : "LOW");
#endif
}

void AppController::maybeLogGpioWatch(uint32_t now_ms) {
  if (!gpio_watch_logging_) return;
  const bool high = digitalRead(kHallPin) == HIGH;
  const bool changed = high != gpio_watch_last_high_;
  const bool heartbeat =
      static_cast<uint32_t>(now_ms - gpio_watch_last_ms_) >= 1000u;
  if (!changed && !heartbeat) return;
  gpio_watch_last_ms_ = now_ms;
  gpio_watch_last_high_ = high;
  Serial.print("GPIO ");
  Serial.print(kHallPinLabel);
#if BIKECOMP_HALL_TWO_WIRE
  Serial.print(" (");
  Serial.print(kHallSensePinLabel);
  Serial.print(" / ");
  Serial.print(kHallDrivePinLabel);
  Serial.print("=LOW)");
#endif
  Serial.print(": mode=INPUT level=");
  Serial.println(high ? "HIGH" : "LOW");
}

void AppController::printDisplayState() const {
  Serial.print("Display: power=");
  switch (display_.powerState()) {
    case DisplayPowerState::kBright:
      Serial.print("bright");
      break;
    case DisplayPowerState::kDim:
      Serial.print("dim");
      break;
    case DisplayPowerState::kOff:
      Serial.print("off");
      break;
  }
  Serial.print(", effective_pct=");
  Serial.println(display_.effectiveBrightnessPct());
}

void AppController::loop() {
  // Feed unconditionally and first: this is what actually protects against a
  // wedged loop() task, whether the wedge is here or in a scheduler task
  // called below. watchdogFeed() itself no-ops if the WDT was never started.
  if (kWatchdogEnabled) watchdogFeed();
  wheel_sensor_.pollPin();
  const uint32_t now_ms = millis();
  processSerialConsole(now_ms);
  scheduler_.run(now_ms);
  updatePowerManager(now_ms);

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
}

void AppController::configurePowerManager() {
  PowerManagerConfig pm_config;
  pm_config.power_save_mode = config_.power_save_mode;
  pm_config.deep_sleep_enabled = config_.deep_sleep_enabled;
  pm_config.deep_sleep_timeout_s = config_.deep_sleep_timeout_s;
  power_manager_.configure(pm_config);
  power_manager_.setBleAlwaysAdvertise(config_.ble_always_advertise);
}

PowerManagerInput AppController::buildPowerManagerInput(
    uint32_t now_ms) const {
  PowerManagerInput input;
  input.ride_state = ride_state_.state();
  if (usb_test_mode_) {
    input.display_power = usb_test_display_power_;
    input.now_ms = usb_test_now_ms_;
  } else {
    input.display_power = display_.powerState();
    input.now_ms = now_ms;
  }
  input.ble_connected = usb_test_mode_ ? false : ble_.bleConnected();
  input.charging =
      battery_.snapshot().charge_status == ChargeStatus::kCharging;
  input.sensor_test_active =
      usb_test_mode_ ? false : ble_.sensorTestActive();
  input.display_test_active =
      usb_test_mode_ ? false : display_.displayTestActive();
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
    drainStorageSave();
    ambient_calibration_save_.requestDeepSleepSave();
    maybePersistAmbientCalibration(now_ms);
    drainStorageSave();
    persistStorageCounters();
  }
  if (result.mode_changed &&
      power_manager_.systemMode() == SystemPowerMode::kLowPowerIdle) {
    ble_.applyPowerSaveAdvertising(power_manager_.aggressiveBlePowerSave());
  }
  if (result.request_enter_deep_sleep) {
    tryEnterDeepSleep(now_ms);
  }
}

void AppController::tryEnterDeepSleep(uint32_t now_ms) {
#if defined(BIKECOMP_FEATURE_DEEP_SLEEP) && BIKECOMP_FEATURE_DEEP_SLEEP
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
  // A true System OFF resets the WDT along with the rest of the core, so
  // this feed only matters if the SoC is emulating System OFF (e.g. a
  // debugger attached) instead of actually entering it — in that case the
  // WDT keeps counting and this buys time before a spurious watchdog reset.
  if (kWatchdogEnabled) watchdogFeed();
  deepSleepPrepareAndEnter(kHallSenseNrfGpio, sense_low);
#else
  (void)now_ms;
#endif
}

void AppController::updatePowerManager(uint32_t now_ms) {
  const PowerManagerUpdateResult result =
      power_manager_.update(buildPowerManagerInput(now_ms));
  handlePowerManagerResult(result, now_ms);
  if (!usb_test_mode_) {
    applySchedulerPeriods(now_ms);
    ble_.applyPowerSaveAdvertising(power_manager_.aggressiveBlePowerSave());
  }
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

void AppController::printSchedulerStats() const {
  for (const ScheduledTask& task : tasks_) {
    Serial.print("Sched: name=");
    Serial.print(task.name);
    Serial.print(" runs=");
    Serial.print(task.run_count);
    Serial.print(" last_us=");
    Serial.print(task.last_duration_us);
    Serial.print(" max_us=");
    Serial.print(task.max_duration_us);
    Serial.print(" budget_us=");
    Serial.print(task.budget_us);
    Serial.print(" overruns=");
    Serial.println(task.overrun_count);
  }
}

void AppController::printStatus() {
  const TripSnapshot trip = trip_computer_.snapshot();
  const BatterySnapshot battery = battery_.snapshot();
  const DiagnosticSnapshot diag = diagnosticSnapshot();
  const char* ride = "IDLE";
  switch (trip.ride_state) {
    case RideState::kMoving:
      ride = "MOVING";
      break;
    case RideState::kPaused:
      ride = "PAUSED";
      break;
    default:
      break;
  }
  const char* display = "bright";
  switch (display_.powerState()) {
    case DisplayPowerState::kDim:
      display = "dim";
      break;
    case DisplayPowerState::kOff:
      display = "off";
      break;
    default:
      break;
  }
  Serial.print("Status: speed_x100=");
  Serial.print(trip.speed_x100);
  Serial.print(" avg_speed_x100=");
  Serial.print(trip.average_speed_x100);
  Serial.print(" max_speed_x100=");
  Serial.print(trip.max_speed_x100);
  Serial.print(" trip_mm=");
  Serial.print(trip.trip_distance_mm);
  Serial.print(" odo_mm=");
  printUint64(trip.odometer_mm);
  Serial.print(" rev=");
  Serial.print(trip.revolutions);
  Serial.print(" total_rev=");
  printUint64(trip_computer_.totalRevolutions());
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
  Serial.print(" hall=");
  Serial.print(wheel_sensor_.pinIsHigh() ? "HIGH" : "LOW");
  Serial.print(" raw_pulses=");
  Serial.print(diag.raw_pulse_count);
  Serial.print(" accepted=");
  Serial.print(pulse_filter_.counters().accepted);
  Serial.print(" debounce_rej=");
  Serial.print(diag.rejected_debounce);
  Serial.print(" overspeed_rej=");
  Serial.print(diag.rejected_overspeed);
  Serial.print(" power_mode=");
  Serial.print(systemPowerModeName(power_manager_.systemMode()));
  Serial.print(" deep_sleep_armed=");
  Serial.print(power_manager_.deepSleepArmed() ? 1 : 0);
  Serial.print(" usb_test=");
  Serial.println(usb_test_mode_ ? 1 : 0);
  const uint32_t now_ms = millis();
  const CompanionHeaderView companion = companion_state_.header(now_ms);
  Serial.print(" clock=");
  Serial.print(companion.valid ? companion.text : "--");
  Serial.print(" clock_valid=");
  Serial.print(companion.valid ? 1 : 0);
  Serial.print(" clock_stale=");
  Serial.print(companion.stale ? 1 : 0);
  const CompanionWeatherView weather = companion_state_.weather(now_ms);
  Serial.print(" weather_temp=");
  Serial.print(weather.valid ? weather.temp : "--");
  Serial.print(" weather_rain=");
  Serial.print(weather.valid ? weather.rain : "--");
  Serial.print(" weather_valid=");
  Serial.println(weather.valid ? 1 : 0);
}

void AppController::applyAcceptedPulse(const PulseDecision& decision,
                                       uint32_t timestamp_us,
                                       uint32_t now_ms) {
  if (!usb_test_mode_) {
    ble_.noteMovement();
    power_manager_.noteActivity(now_ms);
    display_.noteActivity(now_ms);
    odometer_save_.noteDisplayPower(display_.powerState());
  }
  const RideState before_pulse = ride_state_.state();
  applyRideUpdate(ride_state_.onPulse(now_ms), now_ms);
  if (before_pulse != RideState::kMoving) {
    speed_calculator_.resetIntervalGuard();
  }
  uint16_t speed = 0;
  if (decision.interval_us > 0) {
    last_accepted_interval_us_ = decision.interval_us;
    const bool smooth =
        usb_test_mode_ ? usb_test_smoothing_enabled_ : config_.smoothing_enabled;
    speed = speed_calculator_.onInterval(
        config_.wheel_circumference_mm, decision.interval_us, timestamp_us,
        smooth, config_.smoothing_window);
  }
  if (usb_test_mode_) {
    trip_computer_.onRevolutionForTest(config_.wheel_circumference_mm, speed);
  } else {
    trip_computer_.onRevolution(config_.wheel_circumference_mm, speed);
  }
  if (!usb_test_mode_) {
    maybePersistOdometer(now_ms);
  }
}

void AppController::enterUsbTestMode(uint32_t now_ms) {
  if (!usb_test_backup_valid_) {
    usb_test_backup_trip_ = trip_computer_.snapshot();
    usb_test_backup_total_revolutions_ = trip_computer_.totalRevolutions();
    usb_test_backup_valid_ = true;
  }
  usb_test_mode_ = true;
  usb_test_line_len_ = 0;
  resetUsbTestSession(now_ms);
}

void AppController::exitUsbTestMode() {
  if (usb_test_backup_valid_) {
    trip_computer_.restoreSnapshot(usb_test_backup_trip_,
                                 usb_test_backup_total_revolutions_);
    odometer_save_.markSaved(usb_test_backup_trip_.odometer_mm);
    usb_test_backup_valid_ = false;
  }
  usb_test_mode_ = false;
  usb_test_line_len_ = 0;
  if (usb_test_smoothing_saved_) {
    config_.smoothing_enabled = usb_test_smoothing_enabled_;
  }
}

void AppController::resetUsbTestSession(uint32_t now_ms) {
  pulse_filter_.reset();
  speed_calculator_.reset();
  ride_state_.reset(now_ms);
  trip_computer_.resetTrip();
  usb_test_last_ts_us_ = 1000000u;
  usb_test_has_timestamp_ = false;
  usb_test_display_power_ = DisplayPowerState::kBright;
  usb_test_now_ms_ = now_ms;
  usb_test_smoothing_enabled_ = config_.smoothing_enabled;
  usb_test_smoothing_saved_ = true;
  PowerManagerConfig pm_config;
  pm_config.power_save_mode = config_.power_save_mode;
  pm_config.deep_sleep_enabled = config_.deep_sleep_enabled;
  pm_config.deep_sleep_timeout_s = config_.deep_sleep_timeout_s;
  power_manager_.configure(pm_config);
}

bool AppController::injectUsbTestPulse(uint32_t interval_us,
                                       uint32_t now_ms,
                                       char* detail,
                                       size_t detail_len) {
  uint32_t timestamp_us = usb_test_last_ts_us_;
  if (!usb_test_has_timestamp_) {
    usb_test_has_timestamp_ = true;
  } else {
    timestamp_us = usb_test_last_ts_us_ + interval_us;
    usb_test_now_ms_ += interval_us / 1000u;
    if (usb_test_now_ms_ <= now_ms) {
      usb_test_now_ms_ = now_ms + 1u;
    }
  }
  usb_test_last_ts_us_ = timestamp_us;

  const PulseDecision decision =
      pulse_filter_.process(timestamp_us, true);
  if (!decision.accepted) {
    snprintf(detail, detail_len, "rejected=%u",
             static_cast<unsigned>(decision.rejection));
    return true;
  }
  applyAcceptedPulse(decision, timestamp_us, usb_test_now_ms_);
  snprintf(detail, detail_len, "interval=%lu speed_x100=%u rev=%lu",
           decision.first_pulse ? 0UL
                                : static_cast<unsigned long>(decision.interval_us),
           trip_computer_.snapshot().speed_x100,
           static_cast<unsigned long>(trip_computer_.snapshot().revolutions));
  return true;
}

void AppController::fillUsbTestSnapshot(UsbTestSnapshot& out) {
  out = {};
  const TripSnapshot trip = trip_computer_.snapshot();
  out.speed_x100 = trip.speed_x100;
  out.revolutions = trip.revolutions;
  out.ride_state = trip.ride_state;
  out.accepted_pulses = pulse_filter_.counters().accepted;
  out.rejected_debounce = pulse_filter_.counters().rejected_debounce;
  out.rejected_overspeed = pulse_filter_.counters().rejected_overspeed;
  out.power_mode = power_manager_.systemMode();
  out.deep_sleep_armed = power_manager_.deepSleepArmed();
}

void AppController::usbHookReset(void* context, uint32_t now_ms) {
  static_cast<AppController*>(context)->resetUsbTestSession(now_ms);
}

bool AppController::usbHookInject(void* context,
                                  uint32_t interval_us,
                                  uint32_t now_ms,
                                  char* detail,
                                  size_t detail_len) {
  return static_cast<AppController*>(context)->injectUsbTestPulse(
      interval_us, now_ms, detail, detail_len);
}

void AppController::usbHookSmooth(void* context, bool enabled) {
  static_cast<AppController*>(context)->usb_test_smoothing_enabled_ = enabled;
}

void AppController::usbHookSnapshot(void* context, UsbTestSnapshot* out) {
  static_cast<AppController*>(context)->fillUsbTestSnapshot(*out);
}

bool AppController::usbHookPowerFixture(void* context,
                                       DisplayPowerState display_power,
                                       uint32_t now_ms) {
  auto* self = static_cast<AppController*>(context);
  self->usb_test_display_power_ = display_power;
  self->usb_test_now_ms_ = now_ms;
  return true;
}

void AppController::usbHookUpdatePower(void* context, uint32_t now_ms) {
  auto* self = static_cast<AppController*>(context);
  PowerManagerInput input;
  input.ride_state = self->ride_state_.state();
  input.display_power = self->usb_test_display_power_;
  input.ble_connected = false;
  input.charging = false;
  input.sensor_test_active = false;
  input.display_test_active = false;
  input.now_ms = now_ms;
  self->power_manager_.update(input);
  self->usb_test_now_ms_ = now_ms;
}

void AppController::usbHookSetPowerSave(void* context, bool enabled) {
  auto* self = static_cast<AppController*>(context);
  PowerManagerConfig pm_config;
  pm_config.power_save_mode = enabled;
  pm_config.deep_sleep_enabled = self->config_.deep_sleep_enabled;
  pm_config.deep_sleep_timeout_s = self->config_.deep_sleep_timeout_s;
  self->power_manager_.configure(pm_config);
  self->power_manager_.setBleAlwaysAdvertise(self->config_.ble_always_advertise);
}

UsbTestHooks AppController::usbTestHooks() {
  UsbTestHooks hooks = {};
  hooks.context = this;
  hooks.reset = usbHookReset;
  hooks.inject_pulse = usbHookInject;
  hooks.set_smoothing = usbHookSmooth;
  hooks.snapshot = usbHookSnapshot;
  hooks.set_power_fixture = usbHookPowerFixture;
  hooks.update_power = usbHookUpdatePower;
  hooks.set_power_save = usbHookSetPowerSave;
  return hooks;
}

void AppController::processUsbTestLine(const char* line, uint32_t now_ms) {
  const UsbTestResult result = handleUsbTestLine(line, usbTestHooks(), now_ms);
  char buffer[kUsbTestResponseMax + 8] = {};
  formatUsbTestResult(result, buffer, sizeof(buffer));
  Serial.println(buffer);
}

void AppController::processSerialConsole(uint32_t now_ms) {
  if (!Serial) return;

  while (Serial.available() > 0) {
    const int value = Serial.read();
    if (value < 0) break;
    const char ch = static_cast<char>(value);

    if (usb_test_mode_) {
      if (ch == '\r' || ch == '\n') {
        if (usb_test_line_len_ > 0) {
          usb_test_line_[usb_test_line_len_] = '\0';
          if (strcmp(usb_test_line_, "test-off") == 0) {
            exitUsbTestMode();
            Serial.println("OK test-off");
          } else {
            processUsbTestLine(usb_test_line_, now_ms);
            usb_test_line_len_ = 0;
          }
        }
      } else if (usb_test_line_len_ + 1u < kUsbTestLineMax) {
        usb_test_line_[usb_test_line_len_++] = ch;
      }
      continue;
    }

    const SerialCommand command = serial_command_parser_.feed(ch);
    if (command != SerialCommand::kNone &&
        command != SerialCommand::kUnknown) {
      power_manager_.noteActivity(now_ms);
    }
    switch (command) {
      case SerialCommand::kNone:
        break;

      case SerialCommand::kOpenPairing:
        ble_.openPairingWindow(kDefaultPairingWindowMs / 1000u, now_ms);
        Serial.println("OK open-pairing duration_s=300");
        break;

      case SerialCommand::kDumpConfig:
        dumpConfig();
        break;

      case SerialCommand::kResetOdometer:
        Serial.println(saveAndApplyOdometer(0, 0)
                           ? "OK reset-odo"
                           : "ERROR reset-odo storage");
        break;

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

      case SerialCommand::kSelftest:
        printDiagnostics();
        Serial.print("OK selftest mask=0x");
        if (selftest_mask_ < 0x10u) Serial.print('0');
        Serial.println(selftest_mask_, HEX);
        break;

      case SerialCommand::kAmbientRaw:
        ambient_raw_logging_ = true;
        printAmbientLine();
        Serial.println("OK ambient-raw logging=1");
        break;

      case SerialCommand::kAmbientStop:
        ambient_raw_logging_ = false;
        Serial.println("OK ambient-raw logging=0");
        break;

      case SerialCommand::kDisplayState:
        printDisplayState();
        Serial.println("OK display-state");
        break;

      case SerialCommand::kWakeDisplay:
        display_.noteActivity(now_ms);
        printDisplayState();
        Serial.println("OK wake-display");
        break;

      case SerialCommand::kHallStatus:
        printHallStatus();
        Serial.println("OK hall-status");
        break;

      case SerialCommand::kHallWatch:
        hall_analog_logging_ = false;
        hall_watch_logging_ = true;
        hall_watch_last_ms_ = now_ms;
        hall_watch_last_pulses_ = wheel_sensor_.rawPulseCount();
        hall_watch_last_pin_high_ = wheel_sensor_.pinIsHigh();
        printHallStatus();
        Serial.println("OK hall-watch logging=1");
        break;

      case SerialCommand::kHallStop:
        hall_watch_logging_ = false;
        Serial.println("OK hall-watch logging=0");
        break;

      case SerialCommand::kHallRising:
        applyHallEdge(1);
        printHallStatus();
        Serial.println("OK hall-rising");
        break;

      case SerialCommand::kHallFalling:
        applyHallEdge(0);
        printHallStatus();
        Serial.println("OK hall-falling");
        break;

      case SerialCommand::kHallChange:
        applyHallEdge(2);
        printHallStatus();
        Serial.println("OK hall-change");
        break;

      case SerialCommand::kHallAnalog:
        hall_watch_logging_ = false;
        hall_analog_logging_ = true;
        hall_analog_last_ms_ = now_ms;
        hall_analog_last_raw_ = wheel_sensor_.lastAnalogRaw();
        hall_watch_last_pin_high_ = wheel_sensor_.pinIsHigh();
        printHallAnalogLine();
        Serial.println("OK hall-analog logging=1");
        break;

      case SerialCommand::kHallAnalogStop:
        hall_analog_logging_ = false;
        Serial.println("OK hall-analog logging=0");
        break;

      case SerialCommand::kGpioProbe:
        hall_watch_logging_ = false;
        hall_analog_logging_ = false;
        gpio_watch_logging_ = false;
        printGpioProbe();
        Serial.println("OK gpio-probe");
        break;

      case SerialCommand::kGpioWatch:
        hall_watch_logging_ = false;
        wheel_sensor_.suspendInterrupt();
        configureHallPins(kHallPin);
        gpio_watch_logging_ = true;
        gpio_watch_last_ms_ = now_ms;
        gpio_watch_last_high_ = digitalRead(kHallPin) == HIGH;
        Serial.print("GPIO ");
        Serial.print(kHallPinLabel);
#if BIKECOMP_HALL_TWO_WIRE
        Serial.print(" (");
        Serial.print(kHallSensePinLabel);
        Serial.print(" / ");
        Serial.print(kHallDrivePinLabel);
        Serial.print("=LOW)");
#endif
        Serial.print(": mode=");
#if BIKECOMP_HALL_TWO_WIRE
        Serial.print("digital");
#elif BIKECOMP_HALL_PULLUP
        Serial.print("PULLUP");
#else
        Serial.print("INPUT");
#endif
        Serial.print(" level=");
        Serial.println(gpio_watch_last_high_ ? "HIGH" : "LOW");
        Serial.println("OK gpio-watch logging=1 (ISR off)");
        break;

      case SerialCommand::kGpioStop:
        gpio_watch_logging_ = false;
        restoreHallInterrupt();
        Serial.println("OK gpio-watch logging=0 (polling on)");
        break;

      case SerialCommand::kPowerStatus:
        printPowerStatus();
        Serial.println("OK power-status");
        break;

      case SerialCommand::kStatus:
        printStatus();
        Serial.println("OK status");
        break;

      case SerialCommand::kSchedStats:
        printSchedulerStats();
        Serial.println("OK sched");
        break;

      case SerialCommand::kWdtHang:
        if (!kWatchdogEnabled) {
          Serial.println("ERROR wdt-hang watchdog-disabled");
          break;
        }
        // Deliberately wedges loop() to verify the watchdog actually
        // resets a hung device. Nothing after this line runs again until
        // the WDT timeout fires; the SoftDevice/BLE task keeps running
        // independently, so this specifically exercises the loop()-task
        // starvation case the feed-from-loop() placement is meant to catch.
        Serial.print("WARN wdt-hang: spinning, expect reset in ~");
        Serial.print(BIKECOMP_WDT_TIMEOUT_MS);
        Serial.println("ms");
        Serial.flush();
        while (true) {
        }
        break;

      case SerialCommand::kTestOn:
        enterUsbTestMode(now_ms);
        Serial.println("OK test-on");
        break;

      case SerialCommand::kTestOff:
        exitUsbTestMode();
        Serial.println("OK test-off");
        break;

      case SerialCommand::kUnknown:
        Serial.println("ERROR unknown-command");
        break;
    }
  }
}

void AppController::dumpConfig() const {
  uint8_t payload[kDeviceConfigPayloadSize];
  encodeDeviceConfig(config_, payload);

  Serial.println("Config:");
  Serial.print("  wheel_circumference_mm=");
  Serial.println(config_.wheel_circumference_mm);
  Serial.print("  max_speed_kmh=");
  Serial.println(config_.max_speed_kmh);
  Serial.print("  stop_timeout_s=");
  Serial.println(config_.stop_timeout_s);
  Serial.print("  display_timeout_s=");
  Serial.println(config_.display_timeout_s);
  Serial.print("  deep_sleep_timeout_s=");
  Serial.println(config_.deep_sleep_timeout_s);
  Serial.print("  brightness_pct=");
  Serial.println(config_.brightness_pct);
  Serial.print("  page_switch_period_s=");
  Serial.println(config_.page_switch_period_s);
  Serial.print("  enabled_pages_mask=0x");
  Serial.println(config_.enabled_pages_mask, HEX);
  Serial.print("  low_battery_pct=");
  Serial.println(config_.low_battery_pct);
  Serial.print("  odometer_save_interval_m=");
  Serial.println(config_.odometer_save_interval_m);
  Serial.print("  smoothing_window=");
  Serial.println(config_.smoothing_window);
  Serial.print("  debounce_ms=");
  Serial.println(config_.debounce_ms);
  Serial.print("  active_edge=");
  Serial.println(config_.active_edge);
  Serial.print("  pinned_page=");
  Serial.println(config_.pinned_page);
  Serial.print("  batt_cal_scale_permille=");
  Serial.println(config_.batt_cal_scale_permille);
  Serial.print("  batt_cal_offset_mv=");
  Serial.println(config_.batt_cal_offset_mv);
  Serial.print("  page_order=");
  for (size_t i = 0; i < kConfigurablePageOrderCount; ++i) {
    if (i != 0) Serial.print(',');
    Serial.print(config_.page_order[i]);
  }
  Serial.println();
  Serial.print("  device_name=");
  Serial.println(config_.device_name);
  Serial.print("  flags=");
  Serial.print(config_.smoothing_enabled ? '1' : '0');
  Serial.print(config_.auto_page_switch ? '1' : '0');
  Serial.print(config_.display_auto_off ? '1' : '0');
  Serial.print(config_.ble_always_advertise ? '1' : '0');
  Serial.print(config_.units_imperial ? '1' : '0');
  Serial.print(config_.sensor_invert ? '1' : '0');
  Serial.print(config_.power_save_mode ? '1' : '0');
  Serial.println(config_.deep_sleep_enabled ? '1' : '0');
  Serial.print("  wire_v1=");
  for (size_t i = 0; i < kDeviceConfigPayloadSize; ++i) {
    if (payload[i] < 0x10u) Serial.print('0');
    Serial.print(payload[i], HEX);
  }
  Serial.println();
  Serial.println("OK dump-config");
}

void AppController::pulseTask(void* context, uint32_t now_ms) {
  static_cast<AppController*>(context)->processPulses(now_ms);
}

void AppController::stateTask(void* context, uint32_t now_ms) {
  static_cast<AppController*>(context)->updateState(now_ms);
}

void AppController::ambientTask(void* context, uint32_t now_ms) {
  static_cast<AppController*>(context)->updateAmbient(now_ms);
}

void AppController::displayTask(void* context, uint32_t now_ms) {
  static_cast<AppController*>(context)->updateDisplay(now_ms);
}

void AppController::batteryTask(void* context, uint32_t now_ms) {
  static_cast<AppController*>(context)->updateBattery(now_ms);
}

void AppController::bleTask(void* context, uint32_t now_ms) {
  static_cast<AppController*>(context)->updateBle(now_ms);
}

void AppController::storageTask(void* context, uint32_t now_ms) {
  (void)now_ms;
  static_cast<AppController*>(context)->pollStorageSave();
}

void AppController::applyRideUpdate(const RideUpdate& update, uint32_t now_ms) {
  if (update.moving_delta_ms != 0) trip_computer_.addMovingTime(update.moving_delta_ms);
  trip_computer_.setRideState(update.state);
  if (update.state != RideState::kMoving) trip_computer_.setCurrentSpeed(0);
  if (!usb_test_mode_) {
    odometer_save_.noteRideState(update.state, now_ms);
  }
}

void AppController::maybePersistOdometer(uint32_t now_ms) {
  if (usb_test_mode_) return;
  const uint64_t odometer_mm = trip_computer_.snapshot().odometer_mm;
  const OdometerSaveTrigger trigger =
      odometer_save_.evaluate(odometer_mm, now_ms);
  if (trigger == OdometerSaveTrigger::kNone) return;
  persistOdometer(trigger);
}

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

bool AppController::saveAndApplyOdometer(uint64_t odometer_mm,
                                         uint64_t total_revolutions) {
  drainStorageSave();
  OdometerData data;
  data.odometer_mm = odometer_mm;
  data.total_revolutions = total_revolutions;
  if (!storage_.mounted() || !storage_.saveOdometer(data)) return false;
  trip_computer_.restorePersistentTotals(odometer_mm, total_revolutions);
  odometer_save_.markSaved(odometer_mm);
  return true;
}


void AppController::processPulses(uint32_t now_ms) {
  maybeLogGpioWatch(now_ms);
  maybeLogHallWatch(now_ms);
  maybeLogHallAnalog(now_ms);
  PulseEvent event;
  while (wheel_sensor_.pop(event)) {
    const PulseDecision decision =
        pulse_filter_.process(event.timestamp_us, event.returned_passive);
    if (!decision.accepted) {
#if BIKECOMP_HOTPATH_SERIAL
      Serial.print("Pulse rejected: ");
      Serial.println(static_cast<unsigned>(decision.rejection));
#endif
      continue;
    }

    applyAcceptedPulse(decision, event.timestamp_us, now_ms);

#if BIKECOMP_HOTPATH_SERIAL
    Serial.print("Revolution ");
    Serial.print(trip_computer_.snapshot().revolutions);
    Serial.print(", speed x100=");
    Serial.println(trip_computer_.snapshot().speed_x100);
#endif
  }
}

void AppController::updateState(uint32_t now_ms) {
  const uint32_t effective_ms = usb_test_mode_ ? usb_test_now_ms_ : now_ms;
  applyRideUpdate(ride_state_.update(effective_ms), effective_ms);
  const uint32_t now_us = usb_test_mode_ ? usb_test_last_ts_us_ : micros();
  const uint16_t speed = speed_calculator_.updateForTimeout(
      now_us, static_cast<uint32_t>(config_.stop_timeout_s) * 1000000u);
  if (ride_state_.state() == RideState::kMoving && speed_calculator_.hasPulse()) {
    trip_computer_.setCurrentSpeed(speed);
  }
  maybePersistOdometer(effective_ms);
}

void AppController::updateAmbient(uint32_t now_ms) {
  if (!ambient_light_.update(now_ms)) return;
  const AmbientLightSnapshot& ambient = ambient_light_.snapshot();
  display_.setAmbientBrightness(ambient.brightness_pct, ambient.valid);
  if (ambient_raw_logging_) printAmbientLine();
  maybePersistAmbientCalibration(now_ms);
}

void AppController::updateDisplay(uint32_t now_ms) {
  if (!usb_test_mode_ && display_.updatePower(now_ms)) {
    odometer_save_.noteDisplayPower(display_.powerState());
  }
  maybePersistOdometer(now_ms);

  DisplaySnapshot snapshot;
  snapshot.trip = trip_computer_.snapshot();
  snapshot.battery = battery_.snapshot();
  snapshot.ble_connected = ble_.bleConnected();
  const CompanionHeaderView companion = companion_state_.header(now_ms);
  if (companion.valid) {
    snprintf(snapshot.companion_header, sizeof(snapshot.companion_header), "%s",
             companion.text);
    snapshot.companion_header_valid = true;
    snapshot.companion_header_stale = companion.stale;
  }
  const CompanionWeatherView weather = companion_state_.weather(now_ms);
  if (weather.valid) {
    snprintf(snapshot.companion_weather_temp,
             sizeof(snapshot.companion_weather_temp), "%s", weather.temp);
    snprintf(snapshot.companion_weather_rain, sizeof(snapshot.companion_weather_rain),
             "%s", weather.rain);
    snapshot.companion_weather_valid = true;
    snapshot.companion_weather_stale = weather.stale;
  }
  display_.render(snapshot);
}

void AppController::updateBattery(uint32_t now_ms) {
  if (!battery_.update(now_ms)) return;
  const BatterySnapshot& snapshot = battery_.snapshot();
  if (!usb_test_mode_) {
    odometer_save_.noteUsbPresent(snapshot.usb_present);
    odometer_save_.noteBatteryPercent(snapshot.percent, snapshot.valid);
    ambient_calibration_save_.noteUsbPresent(snapshot.usb_present);
  }
  ble_.noteUsbPresent(snapshot.usb_present);
  maybePersistOdometer(now_ms);
  maybePersistAmbientCalibration(now_ms);
  const bool critical = snapshot.valid && snapshot.percent <= 5u;
  if (critical && !critical_battery_active_) {
    ble_.recordError(ErrorLogCode::kCriticalBattery, ErrorLogSeverity::kWarn,
                     snapshot.percent, now_ms);
  }
  critical_battery_active_ = critical;
#if BIKECOMP_HOTPATH_SERIAL
  Serial.print("Battery raw=");
  Serial.print(battery_.lastRawAverage());
  Serial.print(", spread=");
  Serial.print(battery_.lastRawSpread());
  Serial.print(", mV=");
  Serial.print(snapshot.millivolts);
  Serial.print(", pct=");
  Serial.print(snapshot.percent);
  Serial.print(", usb=");
  Serial.println(snapshot.usb_present ? 1 : 0);
#endif
}

void AppController::applyConfig(const DeviceConfig& new_config) {
  const uint8_t previous_edge = config_.active_edge;
  config_ = new_config;

  pulse_filter_.configure(PulseFilterConfig{
      config_.wheel_circumference_mm, config_.max_speed_kmh,
      config_.debounce_ms, 500});
  ride_state_.setStopTimeoutMs(
      static_cast<uint32_t>(config_.stop_timeout_s) * 1000u);
  odometer_save_.configure(config_.odometer_save_interval_m);
  display_.applyRuntimeConfig(config_, millis());
  battery_.applyRuntimeConfig(config_, millis());
  configurePowerManager();
  if (config_.active_edge != previous_edge) {
    wheel_sensor_.begin(kHallPin, interruptMode(config_.active_edge));
  }
}

void AppController::processPendingConfigWrite(uint32_t now_ms) {
  if (!ble_.hasPendingConfigWrite()) return;

  uint8_t payload[kConfigurationSize] = {};
  if (!ble_.takePendingConfigWrite(payload)) return;

  ConfigWriteResult result = {};
  const ConfigWriteParseResult parsed = parseConfigWritePayload(
      payload, kConfigurationSize);
  if (!parsed.ok) {
    result.status = parsed.status;
    result.field_id = parsed.field_id;
    ble_.recordError(ErrorLogCode::kConfigWriteRejected,
                     ErrorLogSeverity::kWarn, parsed.field_id, now_ms);
    ble_.publishConfigWriteResult(result);
    ble_.completePendingConfigWrite();
    return;
  }

  if (!deviceConfigsEqual(config_, parsed.config)) {
    applyConfig(parsed.config);
  }

  drainStorageSave();
  const bool saved = storage_.mounted() && storage_.saveConfig(config_);
  if (!saved) {
    result.status = CommandStatus::kErrStorage;
    ble_.recordError(ErrorLogCode::kFlashError, ErrorLogSeverity::kError,
                     kCommandResultConfigWriteId, now_ms);
    ble_.publishConfigWriteResult(result);
    ble_.completePendingConfigWrite();
    return;
  }

  ble_.publishAppliedConfig(config_, /*config_valid=*/true);
  result.status = CommandStatus::kOk;
  ble_.publishConfigWriteResult(result);
  ble_.completePendingConfigWrite();
}

void AppController::processPendingSafeCommand(uint32_t now_ms) {
  if (!ble_.hasPendingSafeCommand()) return;

  SafeCommandParseResult command;
  if (!ble_.takePendingSafeCommand(command)) return;

  BleCommandResult result;
  switch (command.command_id) {
    case CommandId::kResetTrip:
      trip_computer_.resetTrip();
      break;
    case CommandId::kResetMaxSpeed:
      trip_computer_.resetMaxSpeed();
      break;
    case CommandId::kForceSave: {
      odometer_save_.requestForceSave();
      drainStorageSave();
      const OdometerSaveTrigger trigger =
          odometer_save_.evaluate(trip_computer_.snapshot().odometer_mm, now_ms);
      const bool started = persistOdometer(trigger);
      if (trigger != OdometerSaveTrigger::kForceSave || !started ||
          !drainStorageSave()) {
        result.status = CommandStatus::kErrStorage;
      }
      break;
    }
    case CommandId::kDisplayOn:
      display_.noteActivity(now_ms);
      odometer_save_.noteDisplayPower(display_.powerState());
      break;
    case CommandId::kDisplayOff:
      display_.turnOff(now_ms);
      odometer_save_.noteDisplayPower(display_.powerState());
      break;
    case CommandId::kDisplayTest:
      if (!display_.isOk()) {
        result.status = CommandStatus::kErrHardware;
      } else {
        display_.showTestPattern(command.params.display_test_pattern, now_ms);
        odometer_save_.noteDisplayPower(display_.powerState());
      }
      break;
    case CommandId::kSensorTestStart:
      sensor_test_started_ms_ = now_ms;
      sensor_test_duration_ms_ =
          static_cast<uint32_t>(command.params.sensor_test_duration_s) * 1000u;
      ble_.setSensorTestActive(true);
      break;
    case CommandId::kSensorTestStop:
      sensor_test_duration_ms_ = 0;
      ble_.setSensorTestActive(false);
      break;
    case CommandId::kBatteryTest:
      if (!battery_.runTest(now_ms)) {
        result.status = CommandStatus::kErrHardware;
      }
      break;
    case CommandId::kStartDiagnostic:
      printDiagnostics();
      result.detail = selftest_mask_;
      break;
    case CommandId::kGetDiagnostic: {
      const DiagnosticSnapshot diagnostic = diagnosticSnapshot();
      encodeDiagnosticPayload(diagnostic, result.payload);
      result.payload_len = kDiagnosticPayloadSize;
      break;
    }
    default:
      result.status = CommandStatus::kErrUnknownCommand;
      result.detail = static_cast<uint8_t>(CommandFieldId::kCommandId);
      break;
  }

  ble_.completePendingSafeCommand();
  ble_.publishSafeCommandResult(command.command_id, result);
}

void AppController::processPendingDangerousCommand(uint32_t now_ms) {
  if (!ble_.hasPendingDangerousCommand()) return;

  DangerousCommandParseResult command;
  if (!ble_.takePendingDangerousCommand(command)) return;

  BleCommandResult result;
  bool clear_bonds_after_result = false;
  switch (command.command_id) {
    case CommandId::kResetOdometer:
      if (!saveAndApplyOdometer(0, 0)) {
        result.status = CommandStatus::kErrStorage;
      }
      break;

    case CommandId::kFactoryReset: {
      const uint64_t old_odometer_mm = trip_computer_.snapshot().odometer_mm;
      const uint64_t old_total_revolutions = trip_computer_.totalRevolutions();
      if (!saveAndApplyOdometer(0, 0)) {
        result.status = CommandStatus::kErrStorage;
        break;
      }
      const DeviceConfig defaults;
      drainStorageSave();
      if (!storage_.mounted() || !storage_.saveConfig(defaults)) {
        saveAndApplyOdometer(old_odometer_mm, old_total_revolutions);
        result.status = CommandStatus::kErrStorage;
        break;
      }
      trip_computer_.resetTrip();
      applyConfig(defaults);
      ble_.publishAppliedConfig(config_, /*config_valid=*/true);
      ble_.openPairingWindow(kDefaultPairingWindowMs / 1000u, now_ms);
      clear_bonds_after_result = true;
      break;
    }

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

    case CommandId::kSetBatteryCal: {
      DeviceConfig candidate = config_;
      candidate.batt_cal_scale_permille =
          command.params.battery_scale_permille;
      candidate.batt_cal_offset_mv = command.params.battery_offset_mv;
      drainStorageSave();
      if (!storage_.mounted() || !storage_.saveConfig(candidate)) {
        result.status = CommandStatus::kErrStorage;
      } else {
        applyConfig(candidate);
        ble_.publishAppliedConfig(config_, /*config_valid=*/true);
      }
      break;
    }

    case CommandId::kSetOdometer:
      if (!saveAndApplyOdometer(
              static_cast<uint64_t>(command.params.odometer_m) * 1000u,
              trip_computer_.totalRevolutions())) {
        result.status = CommandStatus::kErrStorage;
      }
      break;

    case CommandId::kOpenPairingWindow:
      ble_.openPairingWindow(command.params.pairing_window_duration_s, now_ms);
      break;

    default:
      result.status = CommandStatus::kErrUnknownCommand;
      result.detail = static_cast<uint8_t>(CommandFieldId::kCommandId);
      break;
  }

  if (result.status == CommandStatus::kErrStorage) {
    ble_.recordError(ErrorLogCode::kFlashError, ErrorLogSeverity::kError,
                     static_cast<uint16_t>(command.command_id), now_ms);
  }
  ble_.completePendingDangerousCommand();
  ble_.publishDangerousCommandResult(command.command_id, result);
  if (clear_bonds_after_result) ble_.clearBonds();
}

void AppController::processPendingCompanionWrite(uint32_t now_ms) {
  CompanionSnapshotPacket packet = {};
  if (!ble_.takePendingCompanionWrite(packet)) return;
  companion_state_.apply(packet, now_ms);
}

void AppController::updateBle(uint32_t now_ms) {
  processPendingConfigWrite(now_ms);
  processPendingSafeCommand(now_ms);
  processPendingDangerousCommand(now_ms);
  processPendingCompanionWrite(now_ms);

  const uint32_t overflow = wheel_sensor_.overflowCount();
  if (overflow != logged_isr_overflow_) {
    logged_isr_overflow_ = overflow;
    ble_.recordError(ErrorLogCode::kIsrOverflow, ErrorLogSeverity::kWarn,
                     saturateErrorDetail(overflow), now_ms);
  }
  const uint32_t stuck = pulse_filter_.counters().rejected_stuck;
  if (stuck != logged_sensor_stuck_) {
    logged_sensor_stuck_ = stuck;
    ble_.recordError(ErrorLogCode::kSensorStuck, ErrorLogSeverity::kWarn,
                     saturateErrorDetail(stuck), now_ms);
  }

  if (ble_.sensorTestActive() && sensor_test_duration_ms_ != 0 &&
      static_cast<uint32_t>(now_ms - sensor_test_started_ms_) >=
          sensor_test_duration_ms_) {
    sensor_test_duration_ms_ = 0;
    ble_.setSensorTestActive(false);
  }

  if (reboot_pending_ &&
      static_cast<uint32_t>(now_ms - reboot_requested_ms_) >= 250u) {
    drainStorageSave();
    NVIC_SystemReset();
  }

  TelemetryBuildInput input;
  input.trip = trip_computer_.snapshot();
  input.battery = battery_.snapshot();
  input.display_on = display_.powerState() != DisplayPowerState::kOff;
  input.ble_connected = ble_.bleConnected();
  input.low_power_idle =
      power_manager_.systemMode() == SystemPowerMode::kLowPowerIdle;
  input.deep_sleep_pending =
      power_manager_.deepSleepArmed() && config_.deep_sleep_enabled;
  input.smoothing_enabled = config_.smoothing_enabled;
  input.units_imperial = config_.units_imperial;
  input.had_pulse = ride_state_.hasPulse();
  input.last_pulse_ms = ride_state_.lastPulseMs();
  input.now_ms = now_ms;
  ble_.serviceTelemetry(input, now_ms);
  updateBoardStatusLed(ble_.bleAdvertising(), ble_.bleConnected(), now_ms);
}

}  // namespace bike
