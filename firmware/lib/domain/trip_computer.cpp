#include "trip_computer.h"

namespace bike {

void TripComputer::onRevolution(uint16_t circumference_mm, uint16_t speed_x100) {
  ++snapshot_.revolutions;
  ++total_revolutions_;
  snapshot_.trip_distance_mm += circumference_mm;
  snapshot_.odometer_mm += circumference_mm;
  snapshot_.speed_x100 = speed_x100;
  if (speed_x100 > snapshot_.max_speed_x100) snapshot_.max_speed_x100 = speed_x100;
  updateAverage();
}

void TripComputer::onRevolutionForTest(uint16_t circumference_mm,
                                     uint16_t speed_x100) {
  ++snapshot_.revolutions;
  snapshot_.trip_distance_mm += circumference_mm;
  snapshot_.speed_x100 = speed_x100;
  if (speed_x100 > snapshot_.max_speed_x100) snapshot_.max_speed_x100 = speed_x100;
  updateAverage();
}

void TripComputer::addMovingTime(uint32_t elapsed_ms) {
  snapshot_.moving_time_ms += elapsed_ms;
  updateAverage();
}

void TripComputer::updateAverage() {
  if (snapshot_.moving_time_ms == 0) {
    snapshot_.average_speed_x100 = 0;
    return;
  }
  const uint64_t value = snapshot_.trip_distance_mm * 360ull / snapshot_.moving_time_ms;
  snapshot_.average_speed_x100 =
      value > UINT16_MAX ? UINT16_MAX : static_cast<uint16_t>(value);
}

const TripSnapshot& TripComputer::snapshot() {
  updateAverage();
  return snapshot_;
}

void TripComputer::resetTrip() {
  const uint64_t odometer = snapshot_.odometer_mm;
  snapshot_ = {};
  snapshot_.odometer_mm = odometer;
}

void TripComputer::restorePersistentTotals(uint64_t odometer_mm,
                                           uint64_t total_revolutions) {
  snapshot_.odometer_mm = odometer_mm;
  total_revolutions_ = total_revolutions;
}

void TripComputer::restoreSnapshot(const TripSnapshot& snapshot,
                                   uint64_t total_revolutions) {
  snapshot_ = snapshot;
  total_revolutions_ = total_revolutions;
}

}  // namespace bike
