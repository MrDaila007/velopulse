#include "display_formatter.h"

#include <stdio.h>

namespace bike {

namespace {
const char* stateLabel(RideState state) {
  switch (state) {
    case RideState::kMoving:
      return "MOV";
    case RideState::kPaused:
      return "PAUSE";
    case RideState::kIdle:
    default:
      return "IDLE";
  }
}
}  // namespace

DisplayFrame DisplayFormatter::format(const DisplaySnapshot& snapshot,
                                      DisplayPage page,
                                      bool low_battery_warning) {
  DisplayFrame frame;
  const TripSnapshot& trip = snapshot.trip;
  snprintf(frame.speed, sizeof(frame.speed), "%u.%u", trip.speed_x100 / 100u,
           (trip.speed_x100 % 100u) / 10u);
  snprintf(frame.units, sizeof(frame.units), "km/h");

  const char* state = stateLabel(trip.ride_state);
  switch (page) {
    case DisplayPage::kAverage:
      snprintf(frame.lower, sizeof(frame.lower), "%s AVG %u.%u km/h", state,
               trip.average_speed_x100 / 100u,
               (trip.average_speed_x100 % 100u) / 10u);
      break;
    case DisplayPage::kMaximum:
      snprintf(frame.lower, sizeof(frame.lower), "%s MAX %u.%u km/h", state,
               trip.max_speed_x100 / 100u, (trip.max_speed_x100 % 100u) / 10u);
      break;
    case DisplayPage::kMovingTime: {
      const uint32_t total_seconds = trip.moving_time_ms / 1000u;
      const uint32_t hours = total_seconds / 3600u;
      const uint8_t minutes = static_cast<uint8_t>((total_seconds / 60u) % 60u);
      const uint8_t seconds = static_cast<uint8_t>(total_seconds % 60u);
      snprintf(frame.lower, sizeof(frame.lower), "%s TIME %lu:%02u:%02u", state,
               static_cast<unsigned long>(hours), minutes, seconds);
      break;
    }
    case DisplayPage::kOdometer: {
      const uint64_t tenths_km = trip.odometer_mm / 100000u;
      if (tenths_km <= 999999u) {
        snprintf(frame.lower, sizeof(frame.lower), "%s ODO %llu.%llu km", state,
                 static_cast<unsigned long long>(tenths_km / 10u),
                 static_cast<unsigned long long>(tenths_km % 10u));
      } else {
        snprintf(frame.lower, sizeof(frame.lower), "%s ODO 99999+ km", state);
      }
      break;
    }
    case DisplayPage::kTrip:
    default:
      snprintf(frame.lower, sizeof(frame.lower), "%s TRIP %lu.%02lu km", state,
               static_cast<unsigned long>(trip.trip_distance_mm / 1000000u),
               static_cast<unsigned long>((trip.trip_distance_mm / 10000u) % 100u));
      break;
  }

  if (snapshot.battery.valid) {
    const uint8_t percent = snapshot.battery.percent > 100 ? 100 : snapshot.battery.percent;
    snprintf(frame.battery_percent, sizeof(frame.battery_percent), "%u%%", percent);
    frame.battery_fill_width = static_cast<uint8_t>((percent * 8u) / 100u);
  } else {
    snprintf(frame.battery_percent, sizeof(frame.battery_percent), "--%%");
  }
  if (low_battery_warning) {
    snprintf(frame.lower, sizeof(frame.lower), "LOW BATT");
    frame.low_battery_warning = true;
  }
  return frame;
}

}  // namespace bike
