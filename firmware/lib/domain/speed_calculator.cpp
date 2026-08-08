#include "speed_calculator.h"

namespace bike {

uint16_t SpeedCalculator::onInterval(uint16_t circumference_mm,
                                     uint32_t interval_us,
                                     uint32_t pulse_timestamp_us,
                                     bool smoothing_enabled,
                                     uint8_t smoothing_window) {
  if (interval_us == 0) return speed_x100_;
  if (smoothing_window < 2) smoothing_window = 2;
  if (smoothing_window > kMaxWindow) smoothing_window = kMaxWindow;

  const uint32_t effective_us = interval_guard_.sanitize(interval_us);
  if (effective_us == 0) return speed_x100_;

  intervals_[next_] = effective_us;
  next_ = static_cast<uint8_t>((next_ + 1u) % smoothing_window);
  if (count_ < smoothing_window) ++count_;

  uint64_t sum = effective_us;
  uint8_t samples = 1;
  if (smoothing_enabled) {
    sum = 0;
    samples = count_;
    for (uint8_t i = 0; i < count_; ++i) sum += intervals_[i];
  }

  const uint64_t numerator =
      static_cast<uint64_t>(circumference_mm) * 360000u * samples;
  const uint64_t calculated = numerator / sum;
  speed_x100_ = calculated > UINT16_MAX ? UINT16_MAX : static_cast<uint16_t>(calculated);
  last_pulse_us_ = pulse_timestamp_us;
  has_pulse_ = true;
  return speed_x100_;
}

uint16_t SpeedCalculator::updateForTimeout(uint32_t now_us, uint32_t zero_timeout_us) {
  if (has_pulse_ && static_cast<uint32_t>(now_us - last_pulse_us_) >= zero_timeout_us) {
    speed_x100_ = 0;
  }
  return speed_x100_;
}

void SpeedCalculator::reset() {
  for (auto& interval : intervals_) interval = 0;
  count_ = 0;
  next_ = 0;
  speed_x100_ = 0;
  last_pulse_us_ = 0;
  has_pulse_ = false;
  interval_guard_.reset();
}

}  // namespace bike
