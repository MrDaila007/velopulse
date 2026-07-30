#pragma once

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
  static void displayTask(void* context, uint32_t now_ms);
  static void batteryTask(void* context, uint32_t now_ms);
  static void bleTask(void* context, uint32_t now_ms);

  void processPulses(uint32_t now_ms);
  void updateState(uint32_t now_ms);
  void updateDisplay(uint32_t now_ms);
  void updateBattery(uint32_t now_ms);
  void updateBle(uint32_t now_ms);
  void processPendingConfigWrite();
  void applyConfig(const DeviceConfig& config);
  void applyRideUpdate(const RideUpdate& update, uint32_t now_ms);
  void maybePersistOdometer(uint32_t now_ms);
  void printDiagnostics() const;

  DeviceConfig config_;
  InternalFsBackend storage_backend_;
  StorageManager storage_;
  OdometerSavePolicy odometer_save_;
  WheelSensor wheel_sensor_;
  PulseFilter pulse_filter_;
  SpeedCalculator speed_calculator_;
  TripComputer trip_computer_;
  RideStateMachine ride_state_;
  BatteryManager battery_;
  DisplayManager display_;
  BleManager ble_;
  ScheduledTask tasks_[5];
  Scheduler scheduler_;
  uint8_t selftest_mask_ = 0;
};

}  // namespace bike
