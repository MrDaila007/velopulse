#pragma once

#include <stdint.h>

namespace bike {

enum class RideState : uint8_t {
  kIdle = 0,
  kMoving = 1,
  kPaused = 2,
};

enum class DisplayPage : uint8_t {
  kTrip = 0,
  kAverage = 1,
  kMaximum = 2,
  kMovingTime = 3,
  kOdometer = 4,
};

struct TripSnapshot {
  uint16_t speed_x100 = 0;
  uint16_t average_speed_x100 = 0;
  uint16_t max_speed_x100 = 0;
  uint32_t trip_distance_mm = 0;
  uint64_t odometer_mm = 0;
  uint32_t moving_time_ms = 0;
  uint32_t revolutions = 0;
  RideState ride_state = RideState::kIdle;
};

struct BatterySnapshot {
  bool valid = false;
  uint8_t percent = 0;
  uint16_t millivolts = 0;
};

struct DisplaySnapshot {
  TripSnapshot trip;
  BatterySnapshot battery;
};

struct DisplayFrame {
  char speed[16] = {};
  char units[8] = {};
  char lower[32] = {};
  char battery_percent[5] = {};
  uint8_t battery_fill_width = 0;
};

}  // namespace bike
