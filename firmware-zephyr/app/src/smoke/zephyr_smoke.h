#pragma once

#include <stdint.h>

namespace bike {

struct ZephyrSmokeContext {
  bool fs_mounted = false;
  bool adc_valid = false;
  bool hall_configured = false;
  uint8_t selftest_mask = 0;
};

void runZephyrSmokeChecks(const ZephyrSmokeContext& context);

}  // namespace bike
