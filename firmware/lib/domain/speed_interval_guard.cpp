#include "speed_interval_guard.h"

namespace bike {

namespace {

constexpr uint32_t kLongGapNumerator = 3u;
constexpr uint32_t kLongGapDenominator = 2u;
constexpr uint32_t kShortGapNumerator = 3u;
constexpr uint32_t kShortGapDenominator = 4u;

}  // namespace

uint32_t SpeedIntervalGuard::sanitize(uint32_t interval_us) {
  if (interval_us == 0) return 0;
  if (!has_last_) {
    last_good_us_ = interval_us;
    has_last_ = true;
    return interval_us;
  }

  uint32_t effective = interval_us;
  if (interval_us > (last_good_us_ * kLongGapNumerator) / kLongGapDenominator) {
    const uint32_t estimated =
        (interval_us + last_good_us_ / 2u) / last_good_us_;
    if (estimated > 1u) {
      effective = interval_us / estimated;
      ++corrected_count_;
    }
  } else if (interval_us < (last_good_us_ * kShortGapNumerator) /
                                kShortGapDenominator) {
    // Collapse bounce spikes up to ~1.33x previous speed. Intervals shorter
    // than one quarter of the last rhythm are usually post-pause cadence
    // recovery, not duplicate pulses.
    const uint32_t quarter = last_good_us_ / 4u;
    if (interval_us >= quarter) {
      effective = last_good_us_;
      ++corrected_count_;
    }
  }

  if (effective == 0) effective = interval_us;
  last_good_us_ = effective;
  return effective;
}

void SpeedIntervalGuard::reset() {
  last_good_us_ = 0;
  corrected_count_ = 0;
  has_last_ = false;
}

}  // namespace bike
