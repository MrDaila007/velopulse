#include "display_burn_in.h"

namespace bike {

void DisplayBurnInGuard::configure(uint32_t now_ms) {
  last_shift_ms_ = now_ms;
  phase_ = 0;
}

bool DisplayBurnInGuard::update(uint32_t now_ms) {
  const uint32_t elapsed = now_ms - last_shift_ms_;
  const uint32_t steps = elapsed / kDisplayBurnInShiftIntervalMs;
  if (steps == 0) return false;

  last_shift_ms_ += steps * kDisplayBurnInShiftIntervalMs;
  const uint8_t previous_phase = phase_;
  phase_ = static_cast<uint8_t>((phase_ + steps) % 4u);
  return phase_ != previous_phase;
}

int8_t DisplayBurnInGuard::xOffset() const {
  return phase_ == 1u || phase_ == 2u ? 1 : 0;
}

int8_t DisplayBurnInGuard::yOffset() const {
  return phase_ == 2u || phase_ == 3u ? 1 : 0;
}

}  // namespace bike
