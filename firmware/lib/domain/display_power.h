#pragma once

#include <stdint.h>

namespace bike {

enum class DisplayPowerState : uint8_t {
  kBright = 0,
  kDim = 1,
  kOff = 2,
};

class DisplayPower {
 public:
  void configure(uint16_t timeout_s, uint32_t now_ms);
  bool noteActivity(uint32_t now_ms);
  bool update(uint32_t now_ms);

  DisplayPowerState state() const { return state_; }

 private:
  uint32_t timeout_ms_ = 0;
  uint32_t last_activity_ms_ = 0;
  DisplayPowerState state_ = DisplayPowerState::kBright;
};

}  // namespace bike
