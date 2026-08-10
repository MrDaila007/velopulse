#pragma once

#include <stdbool.h>
#include <stdint.h>

namespace bike {

// Returns true when the previous reset was a System OFF wake (GPIO or USB).
// Takes the RESETREAS value the caller already read, since RESETREAS is
// write-1-to-clear: re-reading NRF_POWER->RESETREAS after the caller has
// cleared it (as AppController::begin() does for its own reset_reason
// decode) would always observe 0 and never report a wake source.
bool deepSleepWakeFromSleep(uint32_t resetreas);

// Configures GPIO sense on the hall line (and USB reserve), then enters System OFF.
// Does not return on success.
bool deepSleepPrepareAndEnter(uint32_t hall_nrf_gpio, bool sense_low);

const char* deepSleepWakeSourceName();

}  // namespace bike
