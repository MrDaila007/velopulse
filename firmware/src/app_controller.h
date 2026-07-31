#pragma once

#include "ambient_light_manager.h"
#include "battery_manager.h"
#include "ble_manager.h"
#include "config.h"
#include "diagnostics.h"
#include "display_manager.h"
#include "internal_fs_backend.h"
#include "odometer_save_policy.h"
#include "pulse_filter.h"
#include "ride_state.h"
#include "scheduler.h"
#include "serial_console.h"
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

  void processPulses(uint32_t now_ms);
  void updateState(uint32_t now_ms);
  void updateAmbient(uint32_t now_ms);
  void updateDisplay(uint32_t now_ms);
  void updateBattery(uint32_t now_ms);
  void updateBle(uint32_t now_ms);
  void processPendingConfigWrite(uint32_t now_ms);
  void processPendingSafeCommand(uint32_t now_ms);
  void processPendingDangerousCommand(uint32_t now_ms);
  void processSerialConsole(uint32_t now_ms);
  void dumpConfig() const;
  void applyConfig(const DeviceConfig& config);
  void applyRideUpdate(const RideUpdate& update, uint32_t now_ms);
  void maybePersistOdometer(uint32_t now_ms);
  bool persistOdometer(OdometerSaveTrigger trigger);
  bool saveAndApplyOdometer(uint64_t odometer_mm,
                            uint64_t total_revolutions);
  void printDiagnostics() const;
  void printAmbientLine() const;
  void printDisplayState() const;

  DeviceConfig config_;
  InternalFsBackend storage_backend_;
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
  SerialCommandParser serial_command_parser_;
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
  bool ambient_raw_logging_ = false;
};

}  // namespace bike
