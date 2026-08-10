#pragma once

#include <stdint.h>

namespace bike {

// The nRF52 hardware WDT counts down from CRV (Counter Reload Value) at the
// 32.768 kHz LFCLK; feeding resets the counter to CRV. Per the nRF52840
// Product Specification, CRV must be >= 0xF, and the register is a full
// 32-bit field, so this is also the representable range for the computed
// reload value.
constexpr uint32_t kWatchdogClockHz = 32768u;
constexpr uint32_t kWatchdogMinCrv = 0xFu;
constexpr uint32_t kWatchdogMaxCrv = 0xFFFFFFFFu;

#ifndef BIKECOMP_WDT_TIMEOUT_MS
#define BIKECOMP_WDT_TIMEOUT_MS 8000
#endif

// Converts a millisecond timeout to the nRF52 WDT CRV reload value, clamped
// to the register's valid range. The multiply runs in uint64_t so timeouts
// beyond ~131s (2^32 / 32768 ms), which would overflow a uint32 intermediate,
// still clamp to kWatchdogMaxCrv instead of silently wrapping.
constexpr uint32_t watchdogTimeoutMsToCrv(uint32_t timeout_ms) {
  const uint64_t ticks =
      (static_cast<uint64_t>(timeout_ms) * kWatchdogClockHz) / 1000u;
  const uint64_t crv = ticks == 0u ? 0u : ticks - 1u;
  if (crv < kWatchdogMinCrv) return kWatchdogMinCrv;
  if (crv > kWatchdogMaxCrv) return kWatchdogMaxCrv;
  return static_cast<uint32_t>(crv);
}

}  // namespace bike
