#pragma once

#include <stdint.h>

namespace bike {

class SpeedCalculator {
 public:
  uint16_t onInterval(uint16_t circumference_mm,
                      uint32_t interval_us,
                      uint32_t pulse_timestamp_us,
                      bool smoothing_enabled,
                      uint8_t smoothing_window);
  uint16_t updateForTimeout(uint32_t now_us, uint32_t zero_timeout_us);
  void reset();

  uint16_t speedX100() const { return speed_x100_; }

 private:
  static constexpr uint8_t kMaxWindow = 5;
  uint32_t intervals_[kMaxWindow] = {};
  uint8_t count_ = 0;
  uint8_t next_ = 0;
  uint16_t speed_x100_ = 0;
  uint32_t last_pulse_us_ = 0;
  bool has_pulse_ = false;
};

}  // namespace bike
