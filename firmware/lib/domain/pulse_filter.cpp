#include "pulse_filter.h"

namespace bike {

PulseFilter::PulseFilter(const PulseFilterConfig& config) : config_(config) {}

void PulseFilter::configure(const PulseFilterConfig& config) { config_ = config; }

void PulseFilter::reset() {
  counters_ = {};
  has_raw_pulse_ = false;
  has_accepted_pulse_ = false;
  last_raw_us_ = 0;
  last_accepted_us_ = 0;
}

uint32_t PulseFilter::minimumIntervalUs() const {
  uint8_t max_speed_kmh = config_.max_speed_kmh;
  if (max_speed_kmh < 20u) max_speed_kmh = 100u;
  return static_cast<uint32_t>(config_.wheel_circumference_mm) * 360000u /
         (static_cast<uint32_t>(max_speed_kmh) * 100u);
}

PulseDecision PulseFilter::process(uint32_t timestamp_us,
                                   bool returned_passive,
                                   uint32_t active_duration_us) {
  PulseDecision result;
  const uint32_t raw_interval_us = timestamp_us - last_raw_us_;
  if (has_raw_pulse_ && raw_interval_us < static_cast<uint32_t>(config_.debounce_ms) * 1000u) {
    ++counters_.rejected_debounce;
    result.rejection = PulseRejection::kDebounce;
    last_raw_us_ = timestamp_us;
    return result;
  }
  has_raw_pulse_ = true;
  last_raw_us_ = timestamp_us;

  if (has_accepted_pulse_ && !returned_passive &&
      active_duration_us >= static_cast<uint32_t>(config_.stuck_timeout_ms) * 1000u) {
    ++counters_.rejected_stuck;
    result.rejection = PulseRejection::kStuck;
    return result;
  }

  if (has_accepted_pulse_) {
    const uint32_t interval_us = timestamp_us - last_accepted_us_;
    if (interval_us == 0) {
      ++counters_.rejected_overspeed;
      result.rejection = PulseRejection::kOverspeed;
      return result;
    }
    if (interval_us < minimumIntervalUs()) {
      ++counters_.rejected_overspeed;
      result.rejection = PulseRejection::kOverspeed;
      return result;
    }
    result.interval_us = interval_us;
  } else {
    result.first_pulse = true;
  }

  result.accepted = true;
  last_accepted_us_ = timestamp_us;
  has_accepted_pulse_ = true;
  ++counters_.accepted;
  return result;
}

}  // namespace bike
