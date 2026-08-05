#pragma once

#include "ambient_light_manager.h"
#include "battery_manager.h"
#include "ble_manager.h"
#include "companion_snapshot.h"
#include "config.h"
#include "diagnostics.h"
#include "display_manager.h"
#include "littlefs_backend.h"
#include "odometer_save_policy.h"
#include "platform/deep_sleep.h"
#include "power_manager.h"
#include "pulse_filter.h"
#include "ride_state.h"
#include "scheduler.h"
#include "serial_console.h"
#include "serial_profile.h"
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

  DiagnosticSnapshot diagnosticSnapshot() const;

 private:
  static void pulseTask(void* context, uint32_t now_ms);
  static void stateTask(void* context, uint32_t now_ms);
  static void ambientTask(void* context, uint32_t now_ms);
  static void displayTask(void* context, uint32_t now_ms);
  static void batteryTask(void* context, uint32_t now_ms);
  static void bleTask(void* context, uint32_t now_ms);

  void processPulses(uint32_t now_ms);
  void updateState(uint32_t now_ms);
  void updateAmbient(uint32_t now_ms);
  void updateDisplay(uint32_t now_ms);
  void updateBattery(uint32_t now_ms);
  void updateBle(uint32_t now_ms);
  bool ensureBleInitialized(uint32_t now_ms);
  void processPendingConfigWrite(uint32_t now_ms);
  void processPendingSafeCommand(uint32_t now_ms);
  void processPendingDangerousCommand(uint32_t now_ms);
  void processPendingCompanionWrite(uint32_t now_ms);
  void processSerialConsole(uint32_t now_ms);
  bool applyLoadConfigHex(const char* hex);
  void dumpConfig() const;
  void applyConfig(const DeviceConfig& config);
  void configurePowerManager();
  PowerManagerInput buildPowerManagerInput(uint32_t now_ms) const;
  void applySchedulerPeriods(uint32_t now_ms);
  void handlePowerManagerResult(const PowerManagerUpdateResult& result,
                                uint32_t now_ms);
  void updatePowerManager(uint32_t now_ms);
  void tryEnterDeepSleep(uint32_t now_ms);
  void printPowerStatus() const;
  void printStatus();
  void applyRideUpdate(const RideUpdate& update, uint32_t now_ms);
  void maybePersistOdometer(uint32_t now_ms);
  bool persistOdometer(OdometerSaveTrigger trigger);
  bool saveAndApplyOdometer(uint64_t odometer_mm, uint64_t total_revolutions);
  bool loadConfigFromWire(const uint8_t payload[kDeviceConfigPayloadSize]);
  void printDiagnostics() const;
  void printAmbientLine() const;
  void printDisplayState() const;
  void printHallStatus() const;
  void maybeLogHallWatch(uint32_t now_ms);
  void printGpioProbe();
  void maybeLogGpioWatch(uint32_t now_ms);
  void restoreHallInterrupt();
  void applyHallEdge(uint8_t active_edge);

  DeviceConfig config_;
  LittleFsBackend storage_backend_;
  StorageManager storage_;
  OdometerSavePolicy odometer_save_;
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
  CompanionState companion_state_ = {};
  SerialCommandParser serial_command_parser_;
  PendingWireV1Reader pending_wire_v1_;
  ScheduledTask tasks_[6];
  Scheduler scheduler_;
  uint8_t selftest_mask_ = 0;
  uint32_t sensor_test_started_ms_ = 0;
  uint32_t sensor_test_duration_ms_ = 0;
  uint32_t logged_isr_overflow_ = 0;
  uint32_t logged_sensor_stuck_ = 0;
  bool critical_battery_active_ = false;
  bool reboot_pending_ = false;
  uint32_t reboot_requested_ms_ = 0;
  bool ble_init_pending_ = false;
  BleBootSeed ble_boot_seed_ = {};
  bool boot_config_recovered_ = false;
  ResetReason boot_reset_reason_ = ResetReason::kUnknown;
  bool ambient_raw_logging_ = false;
  bool hall_watch_logging_ = false;
  uint32_t hall_watch_last_ms_ = 0;
  uint32_t hall_watch_last_pulses_ = 0;
  bool hall_watch_last_pin_high_ = true;
  bool gpio_watch_logging_ = false;
  uint32_t gpio_watch_last_ms_ = 0;
  bool gpio_watch_last_high_ = true;
};

}  // namespace bike
