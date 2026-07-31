#pragma once

#include <stdint.h>

namespace bike {

constexpr uint16_t kAdvertisingFastIntervalMs = 30;
constexpr uint16_t kAdvertisingSlowIntervalMs = 1000;
constexpr uint16_t kAdvertisingFastTimeoutS = 30;
constexpr uint16_t kAdvertisingIdleTimeoutS = 5u * 60u;

uint16_t advertisingTimeoutS(bool always_advertise);

// Evaluated on accepted wheel movement. A disconnected device whose limited
// advertising window expired becomes discoverable again immediately.
bool shouldRestartAdvertisingOnMovement(bool connected,
                                        bool advertising_running);

}  // namespace bike
