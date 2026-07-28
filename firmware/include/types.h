#pragma once

#include <stdint.h>

namespace bike {

enum class RideState : uint8_t {
  kIdle = 0,
  kMoving = 1,
  kPaused = 2,
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

}  // namespace bike
