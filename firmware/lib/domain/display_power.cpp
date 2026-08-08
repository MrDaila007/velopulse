#include "display_power.h"

namespace bike {

void DisplayPower::configure(uint16_t timeout_s, uint32_t now_ms) {
  timeout_ms_ = static_cast<uint32_t>(timeout_s) * 1000u;
  last_activity_ms_ = now_ms;
  state_ = DisplayPowerState::kBright;
}

bool DisplayPower::noteActivity(uint32_t now_ms) {
  last_activity_ms_ = now_ms;
  if (state_ == DisplayPowerState::kBright) return false;
  state_ = DisplayPowerState::kBright;
  return true;
}

bool DisplayPower::forceOff(uint32_t now_ms) {
  last_activity_ms_ = now_ms - timeout_ms_;
  if (state_ == DisplayPowerState::kOff) return false;
  state_ = DisplayPowerState::kOff;
  return true;
}

bool DisplayPower::update(uint32_t now_ms) {
  if (timeout_ms_ == 0) return false;
  const uint32_t elapsed = now_ms - last_activity_ms_;
  DisplayPowerState next = DisplayPowerState::kBright;
  if (elapsed >= timeout_ms_) {
    next = DisplayPowerState::kOff;
  } else if (elapsed >= timeout_ms_ / 2u) {
    next = DisplayPowerState::kDim;
  }
  if (next == state_) return false;
  state_ = next;
  return true;
}

}  // namespace bike
