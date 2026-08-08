#pragma once

#include <stdint.h>

namespace bike {

class SpeedIntervalGuard {
 public:
  uint32_t sanitize(uint32_t interval_us);
  uint32_t correctedCount() const { return corrected_count_; }
  void reset();

 private:
  uint32_t last_good_us_ = 0;
  uint32_t corrected_count_ = 0;
  bool has_last_ = false;
};

}  // namespace bike
