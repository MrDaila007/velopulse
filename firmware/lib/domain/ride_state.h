#pragma once

#include <stdint.h>

#include "types.h"

namespace bike {

struct RideUpdate {
  RideState state = RideState::kIdle;
  uint32_t moving_delta_ms = 0;
  bool state_changed = false;
};

class RideStateMachine {
 public:
  explicit RideStateMachine(uint32_t stop_timeout_ms = 3000);

  RideUpdate onPulse(uint32_t now_ms);
  RideUpdate update(uint32_t now_ms);
  void reset(uint32_t now_ms = 0);

  RideState state() const { return state_; }
  bool hasPulse() const { return had_pulse_; }
  uint32_t lastPulseMs() const { return last_pulse_ms_; }

 private:
  RideUpdate advance(uint32_t now_ms);
  uint32_t stop_timeout_ms_;
  RideState state_ = RideState::kIdle;
  uint32_t last_pulse_ms_ = 0;
  uint32_t last_tick_ms_ = 0;
  bool clock_started_ = false;
  bool had_pulse_ = false;
};

}  // namespace bike
