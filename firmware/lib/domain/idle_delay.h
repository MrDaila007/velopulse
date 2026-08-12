#pragma once

#include <stdint.h>

namespace bike {

// Bounds a requested idle wait to at most max_chunk_ms, so a caller doing
// `delay(clampIdleDelayMs(requested, cap))` in a loop never blocks longer
// than one chunk at a time while still reaching the same total wait.
// max_chunk_ms == 0 disables clamping (returns requested_delay_ms unchanged).
uint32_t clampIdleDelayMs(uint32_t requested_delay_ms, uint32_t max_chunk_ms);

}  // namespace bike
