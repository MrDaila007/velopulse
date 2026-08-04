#include "app_controller.h"

#include "platform.h"
#include <zephyr/autoconf.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#include "ble_device_info.h"
#include "ble_config_write.h"
#include "ble_telemetry.h"
#include "board_leds.h"
#include "board_pins.h"
#include "config_codec.h"
#include "boot_counter.h"
#include "zephyr_smoke.h"

#ifndef BIKECOMP_HOTPATH_SERIAL
#define BIKECOMP_HOTPATH_SERIAL 0
#endif

#ifndef BIKECOMP_OPEN_PAIRING
#define BIKECOMP_OPEN_PAIRING 0
#endif

namespace bike {
namespace {

constexpr size_t kTaskPulses = 0;
constexpr size_t kTaskState = 1;
constexpr size_t kTaskAmbient = 2;
constexpr size_t kTaskBattery = 3;
constexpr size_t kTaskDisplay = 4;
constexpr size_t kTaskBle = 5;

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
      tasks_{{"pulses", 0, 0, pulseTask, this, 0},
             {"state", 100, 0, stateTask, this, 0},
             {"ambient", 10, 0, ambientTask, this, 0},
             {"battery", 1000, 0, batteryTask, this, 0},
             {"display", 50, 0, displayTask, this, 0},
             {"ble", 100, 0, bleTask, this, 0}},
      scheduler_(tasks_, 6) {}

void AppController::begin() {
  Serial.begin(115200);
  const uint32_t serial_started = millis();
  while (!Serial && static_cast<uint32_t>(millis() - serial_started) < 1500u) yield();

  Serial.println();
  Serial.print("BikeComp FW ");
  Serial.println(CONFIG_FW_VERSION);

  const uint32_t resetreas = platform::resetReasonRaw();
  const ResetReason reset_reason = mapNrfResetReason(resetreas);
  // Clear sticky bits so the next boot sees a fresh reason.
  platform::clearResetReason(resetreas);
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

  Serial.println("Hall reed: D0 drive LOW + D1 sense (two-wire)");
  beginBoardLeds();
  wheel_sensor_.begin(kHallPin, interruptMode(config_.active_edge));
  const bool display_ok = display_.begin(config_);
  Serial.print("OLED 0x3C: ");
  Serial.println(display_ok ? "OK" : "NOT FOUND; counting remains active");
  battery_.begin(config_, millis());
  ambient_light_.begin(millis());
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
  // kSelftestWatchdogOk stays unset until Watchdog is initialized (docs §12.2).
  printDiagnostics();
  ZephyrSmokeContext smoke{};
  smoke.fs_mounted = fs_ok;
  smoke.adc_valid = battery_.snapshot().valid;
  smoke.hall_configured = true;
  smoke.selftest_mask = selftest_mask_;
  runZephyrSmokeChecks(smoke);
}

DiagnosticSnapshot AppController::diagnosticSnapshot() const {
  const int free_heap = platform::freeHeapBytes();
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

void AppController::printHallStatus() const {
  const DiagnosticSnapshot diag = diagnosticSnapshot();
  Serial.print("Hall: pin=");
  Serial.print(kHallPinLabel);
  Serial.print(" (");
  Serial.print(kHallSensePinLabel);
  Serial.print(" / ");
  Serial.print(kHallDrivePinLabel);
  Serial.print("=LOW)");
  Serial.print(" level=");
  Serial.print(wheel_sensor_.pinIsHigh() ? "HIGH" : "LOW");
  Serial.print(", mode=digital");
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
  Serial.println(trip_computer_.revolutions());
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

namespace {

const struct device* hallGpio0() { return DEVICE_DT_GET(DT_NODELABEL(gpio0)); }

bool readHallSenseWithMode(gpio_flags_t mode) {
  const struct device* dev = hallGpio0();
  if (!device_is_ready(dev)) return true;
  gpio_pin_configure(dev, 3, GPIO_INPUT | mode);
  k_msleep(2);
  return gpio_pin_get(dev, 3) != 0;
}

}  // namespace

void AppController::printGpioProbe() {
  wheel_sensor_.suspendInterrupt();

  const bool pullup_high = readHallSenseWithMode(GPIO_PULL_UP);
  const bool float_high = readHallSenseWithMode(GPIO_DISCONNECTED);
  const bool pulldown_high = readHallSenseWithMode(GPIO_PULL_DOWN);

  configureHallPins();
  restoreHallInterrupt();

  Serial.print("GPIO ");
  Serial.print(kHallPinLabel);
  Serial.print(" (");
  Serial.print(kHallSensePinLabel);
  Serial.print(" / ");
  Serial.print(kHallDrivePinLabel);
  Serial.print("=LOW)");
  Serial.print(": PULLUP=");
  Serial.print(pullup_high ? "HIGH" : "LOW");
  Serial.print(", FLOAT=");
  Serial.print(float_high ? "HIGH" : "LOW");
  Serial.print(", PULLDOWN=");
  Serial.println(pulldown_high ? "HIGH" : "LOW");
}

void AppController::maybeLogGpioWatch(uint32_t now_ms) {
  if (!gpio_watch_logging_) return;
  const bool high = wheel_sensor_.pinIsHigh();
  const bool changed = high != gpio_watch_last_high_;
  const bool heartbeat =
      static_cast<uint32_t>(now_ms - gpio_watch_last_ms_) >= 1000u;
  if (!changed && !heartbeat) return;
  gpio_watch_last_ms_ = now_ms;
  gpio_watch_last_high_ = high;
  Serial.print("GPIO ");
  Serial.print(kHallPinLabel);
  Serial.print(" (");
  Serial.print(kHallSensePinLabel);
  Serial.print(" / ");
  Serial.print(kHallDrivePinLabel);
  Serial.print("=LOW)");
  Serial.print(": mode=digital level=");
  Serial.println(high ? "HIGH" : "LOW");
}

void AppController::loop() {
  wheel_sensor_.pollPin();
  const uint32_t now_ms = millis();
  processSerialConsole(now_ms);
  scheduler_.run(now_ms);
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
}

void AppController::processSerialConsole(uint32_t now_ms) {
  if (!Serial) return;

  while (Serial.available() > 0) {
    const int value = Serial.read();
    if (value < 0) break;

    const SerialCommand command =
        serial_command_parser_.feed(static_cast<char>(value));
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
        Serial.println("ERROR hall-analog unsupported on Zephyr two-wire build");
        break;

      case SerialCommand::kHallAnalogStop:
        Serial.println("OK hall-analog logging=0");
        break;

      case SerialCommand::kGpioProbe:
        hall_watch_logging_ = false;
        gpio_watch_logging_ = false;
        printGpioProbe();
        Serial.println("OK gpio-probe");
        break;

      case SerialCommand::kGpioWatch:
        hall_watch_logging_ = false;
        wheel_sensor_.suspendInterrupt();
        configureHallPins();
        gpio_watch_logging_ = true;
        gpio_watch_last_ms_ = now_ms;
        gpio_watch_last_high_ = wheel_sensor_.pinIsHigh();
        Serial.print("GPIO ");
        Serial.print(kHallPinLabel);
        Serial.print(" (");
        Serial.print(kHallSensePinLabel);
        Serial.print(" / ");
        Serial.print(kHallDrivePinLabel);
        Serial.print("=LOW)");
        Serial.print(": mode=digital level=");
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
  for (size_t i = 0; i < kDisplayPageCount; ++i) {
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
  persistOdometer(trigger);
}

bool AppController::persistOdometer(OdometerSaveTrigger trigger) {
  const uint64_t odometer_mm = trip_computer_.snapshot().odometer_mm;

  OdometerData data;
  data.odometer_mm = odometer_mm;
  data.total_revolutions = trip_computer_.totalRevolutions();
  const bool ok = storage_.mounted() && storage_.saveOdometer(data);
  if (ok) {
    odometer_save_.markSaved(odometer_mm);
    // Only clear one-shot pending flags after a successful write; otherwise
    // pause/display-off/critical/usb triggers would be lost on Flash errors.
    odometer_save_.acknowledge(trigger);
  } else {
    ble_.recordError(ErrorLogCode::kFlashError, ErrorLogSeverity::kError,
                     static_cast<uint16_t>(trigger), millis());
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
  return ok;
}

bool AppController::saveAndApplyOdometer(uint64_t odometer_mm,
                                         uint64_t total_revolutions) {
  OdometerData data;
  data.odometer_mm = odometer_mm;
  data.total_revolutions = total_revolutions;
  if (!storage_.mounted() || !storage_.saveOdometer(data)) return false;
  trip_computer_.restorePersistentTotals(odometer_mm, total_revolutions);
  odometer_save_.markSaved(odometer_mm);
  return true;
}


void AppController::processPulses(uint32_t now_ms) {
  maybeLogHallWatch(now_ms);
  maybeLogGpioWatch(now_ms);
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

    ble_.noteMovement();
    display_.noteActivity(now_ms);
    power_manager_.noteActivity(now_ms);
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

void AppController::updateAmbient(uint32_t now_ms) {
  if (!ambient_light_.update(now_ms)) return;
  const AmbientLightSnapshot& ambient = ambient_light_.snapshot();
  display_.setAmbientBrightness(ambient.brightness_pct, ambient.valid);
  if (ambient_raw_logging_) printAmbientLine();
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
      const OdometerSaveTrigger trigger =
          odometer_save_.evaluate(trip_computer_.snapshot().odometer_mm, now_ms);
      if (trigger != OdometerSaveTrigger::kForceSave ||
          !persistOdometer(trigger)) {
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

    case CommandId::kReboot:
      odometer_save_.requestRebootSave();
      if (!persistOdometer(OdometerSaveTrigger::kReboot)) {
        result.status = CommandStatus::kErrStorage;
      } else {
        reboot_pending_ = true;
        reboot_requested_ms_ = now_ms;
      }
      break;

    case CommandId::kSetBatteryCal: {
      DeviceConfig candidate = config_;
      candidate.batt_cal_scale_permille =
          command.params.battery_scale_permille;
      candidate.batt_cal_offset_mv = command.params.battery_offset_mv;
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

void AppController::updateBle(uint32_t now_ms) {
  processPendingConfigWrite(now_ms);
  processPendingSafeCommand(now_ms);
  processPendingDangerousCommand(now_ms);

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
    platform::systemReset();
  }

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
  updateBoardStatusLed(ble_.bleAdvertising(), ble_.bleConnected(), now_ms);
}

}  // namespace bike
