#pragma once

#include <stdint.h>

namespace bike {

constexpr uint32_t kDisplayBurnInShiftIntervalMs = 60000u;

class DisplayBurnInGuard {
 public:
  void configure(uint32_t now_ms);
  bool update(uint32_t now_ms);

  int8_t xOffset() const;
  int8_t yOffset() const;
  uint8_t phase() const { return phase_; }

 private:
  uint32_t last_shift_ms_ = 0;
  uint8_t phase_ = 0;
};

}  // namespace bike
