#include "ride_state.h"

namespace bike {

RideStateMachine::RideStateMachine(uint32_t stop_timeout_ms)
    : stop_timeout_ms_(stop_timeout_ms) {}

RideUpdate RideStateMachine::advance(uint32_t now_ms) {
  RideUpdate result{state_, 0, false};
  if (!clock_started_) {
    last_tick_ms_ = now_ms;
    clock_started_ = true;
    return result;
  }

  const uint32_t elapsed = now_ms - last_tick_ms_;
  if (state_ == RideState::kMoving) {
    const uint32_t since_pulse = now_ms - last_pulse_ms_;
    if (since_pulse >= stop_timeout_ms_) {
      const uint32_t pause_at = last_pulse_ms_ + stop_timeout_ms_;
      result.moving_delta_ms = pause_at - last_tick_ms_;
      state_ = RideState::kPaused;
      result.state_changed = true;
    } else {
      result.moving_delta_ms = elapsed;
    }
  }
  last_tick_ms_ = now_ms;
  result.state = state_;
  return result;
}

RideUpdate RideStateMachine::onPulse(uint32_t now_ms) {
  RideUpdate result = advance(now_ms);
  const RideState previous = state_;
  last_pulse_ms_ = now_ms;
  had_pulse_ = true;
  state_ = RideState::kMoving;
  result.state = state_;
  result.state_changed = result.state_changed || previous != state_;
  return result;
}

RideUpdate RideStateMachine::update(uint32_t now_ms) { return advance(now_ms); }

void RideStateMachine::reset(uint32_t now_ms) {
  state_ = RideState::kIdle;
  last_pulse_ms_ = now_ms;
  last_tick_ms_ = now_ms;
  clock_started_ = true;
  had_pulse_ = false;
}

}  // namespace bike
