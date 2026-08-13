#pragma once

#include "ambient_calibration_save_policy.h"
#include "ambient_light_manager.h"
#include "battery_manager.h"
#include "ble_manager.h"
#include "companion_snapshot.h"
#include "config.h"
#include "diagnostics.h"
#include "display_manager.h"
#include "internal_fs_backend.h"
#include "odometer_save_policy.h"
#include "power_manager.h"
#include "pulse_filter.h"
#include "ride_state.h"
#include "scheduler.h"
#include "serial_console.h"
#include "serial_usb_test.h"
#include "speed_calculator.h"
#include "storage_manager.h"
#include "trip_computer.h"
#include "wheel_sensor.h"

namespace bike {

class AppController {
 public:
  AppController();
  void begin();
  void loop();

  // Snapshot for Serial dump / future GET_DIAGNOSTIC (§6.1).
  DiagnosticSnapshot diagnosticSnapshot() const;

 private:
  static void pulseTask(void* context, uint32_t now_ms);
  static void stateTask(void* context, uint32_t now_ms);
  static void ambientTask(void* context, uint32_t now_ms);
  static void displayTask(void* context, uint32_t now_ms);
  static void batteryTask(void* context, uint32_t now_ms);
  static void bleTask(void* context, uint32_t now_ms);
  static void storageTask(void* context, uint32_t now_ms);

  void processPulses(uint32_t now_ms);
  void updateState(uint32_t now_ms);
  void updateAmbient(uint32_t now_ms);
  void updateDisplay(uint32_t now_ms);
  void updateBattery(uint32_t now_ms);
  void updateBle(uint32_t now_ms);
  void processPendingConfigWrite(uint32_t now_ms);
  void processPendingSafeCommand(uint32_t now_ms);
  void processPendingDangerousCommand(uint32_t now_ms);
  void processPendingCompanionWrite(uint32_t now_ms);
  void processSerialConsole(uint32_t now_ms);
  void dumpConfig() const;
  void applyConfig(const DeviceConfig& config);
  void applyRideUpdate(const RideUpdate& update, uint32_t now_ms);
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
  void pollStorageSave();
  bool saveAndApplyOdometer(uint64_t odometer_mm,
                            uint64_t total_revolutions);
  void printDiagnostics() const;
  void printAmbientLine() const;
  void printDisplayState() const;
  void printHallStatus() const;
  void printHallAnalogLine() const;
  void maybeLogHallWatch(uint32_t now_ms);
  void maybeLogHallAnalog(uint32_t now_ms);
  void printGpioProbe();
  void maybeLogGpioWatch(uint32_t now_ms);
  void restoreHallInterrupt();
  void applyHallEdge(uint8_t active_edge);
  void configurePowerManager();
  PowerManagerInput buildPowerManagerInput(uint32_t now_ms) const;
  void updatePowerManager(uint32_t now_ms);
  void applySchedulerPeriods(uint32_t now_ms);
  void handlePowerManagerResult(const PowerManagerUpdateResult& result,
                                uint32_t now_ms);
  void tryEnterDeepSleep(uint32_t now_ms);
  void printPowerStatus() const;
  void printSchedulerStats() const;
  void printStatus();
  void processUsbTestLine(const char* line, uint32_t now_ms);
  void resetUsbTestSession(uint32_t now_ms);
  bool injectUsbTestPulse(uint32_t interval_us,
                          uint32_t now_ms,
                          char* detail,
                          size_t detail_len);
  void fillUsbTestSnapshot(UsbTestSnapshot& out);
  void applyAcceptedPulse(const PulseDecision& decision,
                          uint32_t timestamp_us,
                          uint32_t now_ms);

  void exitUsbTestMode();
  void enterUsbTestMode(uint32_t now_ms);

  static void usbHookReset(void* context, uint32_t now_ms);
  static bool usbHookInject(void* context,
                            uint32_t interval_us,
                            uint32_t now_ms,
                            char* detail,
                            size_t detail_len);
  static void usbHookSmooth(void* context, bool enabled);
  static void usbHookSnapshot(void* context, UsbTestSnapshot* out);
  static bool usbHookPowerFixture(void* context,
                                  DisplayPowerState display_power,
                                  uint32_t now_ms);
  static void usbHookUpdatePower(void* context, uint32_t now_ms);
  static void usbHookSetPowerSave(void* context, bool enabled);
  UsbTestHooks usbTestHooks();

  DeviceConfig config_;
  InternalFsBackend storage_backend_;
  StorageManager storage_;
  OdometerSavePolicy odometer_save_;
  AmbientCalibrationSavePolicy ambient_calibration_save_;
  WheelSensor wheel_sensor_;
  PulseFilter pulse_filter_;
  SpeedCalculator speed_calculator_;
  TripComputer trip_computer_;
  RideStateMachine ride_state_;
  AmbientLightManager ambient_light_;
  BatteryManager battery_;
  DisplayManager display_;
  BleManager ble_;
  PowerManager power_manager_;
  SerialCommandParser serial_command_parser_;
  ScheduledTask tasks_[7];
  Scheduler scheduler_;
  uint8_t selftest_mask_ = 0;
  uint32_t sensor_test_started_ms_ = 0;
  uint32_t sensor_test_duration_ms_ = 0;
  uint32_t logged_isr_overflow_ = 0;
  uint32_t logged_sensor_stuck_ = 0;
  bool critical_battery_active_ = false;
  bool reboot_pending_ = false;
  uint32_t reboot_requested_ms_ = 0;
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
  bool ambient_raw_logging_ = false;
  bool hall_watch_logging_ = false;
  uint32_t hall_watch_last_ms_ = 0;
  uint32_t hall_watch_last_pulses_ = 0;
  bool hall_watch_last_pin_high_ = true;
  uint32_t last_accepted_interval_us_ = 0;
  bool hall_analog_logging_ = false;
  uint32_t hall_analog_last_ms_ = 0;
  uint16_t hall_analog_last_raw_ = 0;
  bool gpio_watch_logging_ = false;
  uint32_t gpio_watch_last_ms_ = 0;
  bool gpio_watch_last_high_ = true;
  bool usb_test_mode_ = false;
  bool usb_test_smoothing_saved_ = false;
  bool usb_test_smoothing_enabled_ = true;
  char usb_test_line_[kUsbTestLineMax] = {};
  size_t usb_test_line_len_ = 0;
  uint32_t usb_test_last_ts_us_ = 1000000u;
  bool usb_test_has_timestamp_ = false;
  DisplayPowerState usb_test_display_power_ = DisplayPowerState::kBright;
  uint32_t usb_test_now_ms_ = 0;
  bool usb_test_backup_valid_ = false;
  TripSnapshot usb_test_backup_trip_ = {};
  uint64_t usb_test_backup_total_revolutions_ = 0;
  CompanionState companion_state_ = {};
};

}  // namespace bike
