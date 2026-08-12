#include "idle_delay.h"

namespace bike {

uint32_t clampIdleDelayMs(uint32_t requested_delay_ms, uint32_t max_chunk_ms) {
  if (max_chunk_ms == 0u) return requested_delay_ms;
  return requested_delay_ms < max_chunk_ms ? requested_delay_ms : max_chunk_ms;
}

}  // namespace bike
