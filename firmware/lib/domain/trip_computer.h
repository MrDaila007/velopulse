#pragma once

#include <stdint.h>

#include "types.h"

namespace bike {

class TripComputer {
 public:
  void onRevolution(uint16_t circumference_mm, uint16_t speed_x100);
  void onRevolutionForTest(uint16_t circumference_mm, uint16_t speed_x100);
  void addMovingTime(uint32_t elapsed_ms);
  void setCurrentSpeed(uint16_t speed_x100) { snapshot_.speed_x100 = speed_x100; }
  void setRideState(RideState state) { snapshot_.ride_state = state; }
  void resetTrip();
  void resetMaxSpeed() { snapshot_.max_speed_x100 = 0; }
  void restorePersistentTotals(uint64_t odometer_mm,
                               uint64_t total_revolutions);
  void restoreSnapshot(const TripSnapshot& snapshot,
                       uint64_t total_revolutions);

  const TripSnapshot& snapshot() const;
  uint32_t revolutions() const { return snapshot_.revolutions; }
  uint64_t totalRevolutions() const { return total_revolutions_; }

 private:
  void updateAverage();
  TripSnapshot snapshot_;
  uint64_t total_revolutions_ = 0;
};

}  // namespace bike
