#pragma once

#include <stdbool.h>
#include <stdint.h>

namespace bike {

// Configures the WDT behaviour and reload value for the given timeout. Must
// run before watchdogStart() — CONFIG/RREN/CRV are write-locked by hardware
// once the WDT is running and cannot be changed or stopped short of reset.
void watchdogConfigure(uint32_t timeout_ms);

// Starts the WDT counting down from the configured reload value.
void watchdogStart();

// Feeds (reloads) the WDT. No-op if the WDT hasn't been started, so callers
// don't need to guard every call site with watchdogStarted().
void watchdogFeed();

// True once watchdogStart() has run; mirrors the hardware RUNSTATUS bit.
bool watchdogStarted();

}  // namespace bike
