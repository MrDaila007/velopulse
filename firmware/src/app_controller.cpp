#include "app_controller.h"

#include <Arduino.h>

#include "ble_device_info.h"
#include "ble_telemetry.h"
#include "board_pins.h"
#include "boot_counter.h"

#ifndef BIKECOMP_HOTPATH_SERIAL
#define BIKECOMP_HOTPATH_SERIAL 0
#endif

#ifndef BIKECOMP_OPEN_PAIRING
#define BIKECOMP_OPEN_PAIRING 1
#endif

namespace bike {
namespace {

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
      tasks_{{"pulses", 0, 0, pulseTask, this, 0},
             {"state", 100, 0, stateTask, this, 0},
             {"battery", 1000, 0, batteryTask, this, 0},
             {"display", 50, 0, displayTask, this, 0},
             {"ble", 100, 0, bleTask, this, 0}},
      scheduler_(tasks_, 5) {}

void AppController::begin() {
  Serial.begin(115200);
  const uint32_t serial_started = millis();
  while (!Serial && static_cast<uint32_t>(millis() - serial_started) < 1500u) yield();

  Serial.println();
  Serial.print("BikeComp FW ");
  Serial.println(FW_VERSION);

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
  printLoadInfo("Odometer", odometer_info, odometer_ok);
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

  Serial.println("Hall simulator: button D0 -> GND");
  wheel_sensor_.begin(kHallPin, interruptMode(config_.active_edge));
  const bool display_ok = display_.begin(config_);
  Serial.print("OLED 0x3C: ");
  Serial.println(display_ok ? "OK" : "NOT FOUND; counting remains active");
  battery_.begin(config_, millis());
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

  selftest_mask_ = 0;
  if (display_ok) selftest_mask_ |= kSelftestDisplayOk;
  selftest_mask_ |= kSelftestHallPinOk;
  if (battery_.snapshot().valid) selftest_mask_ |= kSelftestAdcOk;
  if (fs_ok) selftest_mask_ |= kSelftestFsOk;
  if (ble_seed.config_from_flash) selftest_mask_ |= kSelftestConfigValid;
  if (ble_ok) selftest_mask_ |= kSelftestBleOk;
  // kSelftestWatchdogOk stays unset until Watchdog is initialized (docs §12.2).
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
}

void AppController::loop() {
  wheel_sensor_.pollPin();
  scheduler_.run(millis());
  yield();
}

void AppController::pulseTask(void* context, uint32_t now_ms) {
  static_cast<AppController*>(context)->processPulses(now_ms);
}

void AppController::stateTask(void* context, uint32_t now_ms) {
  static_cast<AppController*>(context)->updateState(now_ms);
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

void AppController::applyRideUpdate(const RideUpdate& update, uint32_t now_ms) {
  if (update.moving_delta_ms != 0) trip_computer_.addMovingTime(update.moving_delta_ms);
  trip_computer_.setRideState(update.state);
  if (update.state != RideState::kMoving) trip_computer_.setCurrentSpeed(0);
  odometer_save_.noteRideState(update.state, now_ms);
}

void AppController::maybePersistOdometer(uint32_t now_ms) {
  const uint64_t odometer_mm = trip_computer_.snapshot().odometer_mm;
  const OdometerSaveTrigger trigger =
      odometer_save_.evaluate(odometer_mm, now_ms);
  if (trigger == OdometerSaveTrigger::kNone) return;

  OdometerData data;
  data.odometer_mm = odometer_mm;
  data.total_revolutions = trip_computer_.totalRevolutions();
  const bool ok = storage_.mounted() && storage_.saveOdometer(data);
  if (ok) {
    odometer_save_.markSaved(odometer_mm);
    // Only clear one-shot pending flags after a successful write; otherwise
    // pause/display-off/critical/usb triggers would be lost on Flash errors.
    odometer_save_.acknowledge(trigger);
  }

  Serial.print("Odo save: trigger=");
  Serial.print(odometerSaveTriggerName(trigger));
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

void AppController::processPulses(uint32_t now_ms) {
  PulseEvent event;
  while (wheel_sensor_.pop(event)) {
    const PulseDecision decision = pulse_filter_.process(event.timestamp_us, event.returned_passive);
    if (!decision.accepted) {
#if BIKECOMP_HOTPATH_SERIAL
      Serial.print("Pulse rejected: ");
      Serial.println(static_cast<unsigned>(decision.rejection));
#endif
      continue;
    }

    display_.noteActivity(now_ms);
    odometer_save_.noteDisplayPower(display_.powerState());
    applyRideUpdate(ride_state_.onPulse(now_ms), now_ms);
    uint16_t speed = 0;
    if (!decision.first_pulse) {
      speed = speed_calculator_.onInterval(config_.wheel_circumference_mm,
                                           decision.interval_us,
                                           event.timestamp_us,
                                           config_.smoothing_enabled,
                                           config_.smoothing_window);
    }
    trip_computer_.onRevolution(config_.wheel_circumference_mm, speed);
    maybePersistOdometer(now_ms);

#if BIKECOMP_HOTPATH_SERIAL
    Serial.print("Revolution ");
    Serial.print(trip_computer_.snapshot().revolutions);
    Serial.print(", speed x100=");
    Serial.println(speed);
#endif
  }
}

void AppController::updateState(uint32_t now_ms) {
  applyRideUpdate(ride_state_.update(now_ms), now_ms);
  const uint16_t speed = speed_calculator_.updateForTimeout(
      micros(), static_cast<uint32_t>(config_.stop_timeout_s) * 1000000u);
  if (ride_state_.state() == RideState::kMoving) trip_computer_.setCurrentSpeed(speed);
  maybePersistOdometer(now_ms);
}

void AppController::updateDisplay(uint32_t now_ms) {
  if (display_.updatePower(now_ms)) {
    odometer_save_.noteDisplayPower(display_.powerState());
  }
  maybePersistOdometer(now_ms);

  DisplaySnapshot snapshot;
  snapshot.trip = trip_computer_.snapshot();
  snapshot.battery = battery_.snapshot();
  display_.render(snapshot);
}

void AppController::updateBattery(uint32_t now_ms) {
  if (!battery_.update(now_ms)) return;
  const BatterySnapshot& snapshot = battery_.snapshot();
  odometer_save_.noteUsbPresent(snapshot.usb_present);
  odometer_save_.noteBatteryPercent(snapshot.percent, snapshot.valid);
  ble_.noteUsbPresent(snapshot.usb_present);
  maybePersistOdometer(now_ms);
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

void AppController::updateBle(uint32_t now_ms) {
  TelemetryBuildInput input;
  input.trip = trip_computer_.snapshot();
  input.battery = battery_.snapshot();
  input.display_on = display_.powerState() != DisplayPowerState::kOff;
  input.smoothing_enabled = config_.smoothing_enabled;
  input.units_imperial = config_.units_imperial;
  input.had_pulse = ride_state_.hasPulse();
  input.last_pulse_ms = ride_state_.lastPulseMs();
  input.now_ms = now_ms;
  ble_.serviceTelemetry(input, now_ms);
}

}  // namespace bike
