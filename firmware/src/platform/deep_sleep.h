#pragma once

#include <stdbool.h>
#include <stdint.h>

namespace bike {

// Returns true when the previous reset was a System OFF wake (GPIO or USB).
bool deepSleepWakeFromSleep();

// Configures GPIO sense on the hall line (and USB reserve), then enters System OFF.
// Does not return on success.
bool deepSleepPrepareAndEnter(uint32_t hall_nrf_gpio, bool sense_low);

const char* deepSleepWakeSourceName();

}  // namespace bike
