#pragma once

#include <stdint.h>

#include "types.h"

namespace bike {

class TripComputer {
 public:
  void onRevolution(uint16_t circumference_mm, uint16_t speed_x100);
  void addMovingTime(uint32_t elapsed_ms);
  void setCurrentSpeed(uint16_t speed_x100) { snapshot_.speed_x100 = speed_x100; }
  void setRideState(RideState state) { snapshot_.ride_state = state; }
  void resetTrip();

  const TripSnapshot& snapshot();

 private:
  void updateAverage();
  TripSnapshot snapshot_;
};

}  // namespace bike
