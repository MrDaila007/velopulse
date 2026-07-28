#pragma once

#include <stdint.h>

namespace bike {

enum class PulseRejection : uint8_t {
  kNone = 0,
  kDebounce,
  kOverspeed,
  kStuck,
};

struct PulseFilterConfig {
  uint16_t wheel_circumference_mm = 2100;
  uint8_t max_speed_kmh = 100;
  uint8_t debounce_ms = 3;
  uint16_t stuck_timeout_ms = 500;
};

struct PulseFilterCounters {
  uint32_t accepted = 0;
  uint32_t rejected_debounce = 0;
  uint32_t rejected_overspeed = 0;
  uint32_t rejected_stuck = 0;
};

struct PulseDecision {
  bool accepted = false;
  bool first_pulse = false;
  uint32_t interval_us = 0;
  PulseRejection rejection = PulseRejection::kNone;
};

class PulseFilter {
 public:
  explicit PulseFilter(const PulseFilterConfig& config = {});

  PulseDecision process(uint32_t timestamp_us,
                        bool returned_passive = true,
                        uint32_t active_duration_us = 0);
  void configure(const PulseFilterConfig& config);
  void reset();

  const PulseFilterCounters& counters() const { return counters_; }
  uint32_t minimumIntervalUs() const;

 private:
  PulseFilterConfig config_;
  PulseFilterCounters counters_;
  bool has_raw_pulse_ = false;
  bool has_accepted_pulse_ = false;
  uint32_t last_raw_us_ = 0;
  uint32_t last_accepted_us_ = 0;
};

}  // namespace bike
