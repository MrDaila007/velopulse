#pragma once

#include "battery_manager.h"
#include "config.h"
#include "display_manager.h"
#include "pulse_filter.h"
#include "ride_state.h"
#include "scheduler.h"
#include "speed_calculator.h"
#include "trip_computer.h"
#include "wheel_sensor.h"

namespace bike {

class AppController {
 public:
  AppController();
  void begin();
  void loop();

 private:
  static void pulseTask(void* context, uint32_t now_ms);
  static void stateTask(void* context, uint32_t now_ms);
  static void displayTask(void* context, uint32_t now_ms);
  static void batteryTask(void* context, uint32_t now_ms);

  void processPulses(uint32_t now_ms);
  void updateState(uint32_t now_ms);
  void updateDisplay();
  void updateBattery(uint32_t now_ms);
  void applyRideUpdate(const RideUpdate& update);

  DeviceConfig config_;
  WheelSensor wheel_sensor_;
  PulseFilter pulse_filter_;
  SpeedCalculator speed_calculator_;
  TripComputer trip_computer_;
  RideStateMachine ride_state_;
  BatteryManager battery_;
  DisplayManager display_;
  ScheduledTask tasks_[4];
  Scheduler scheduler_;
};

}  // namespace bike
