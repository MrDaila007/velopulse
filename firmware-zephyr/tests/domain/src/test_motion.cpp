#include <zephyr/ztest.h>

#include "pulse_filter.h"
#include "ride_state.h"
#include "scheduler.h"
#include "speed_calculator.h"
#include "trip_computer.h"

using namespace bike;

ZTEST(motion, test_pulse_filter_debounce_and_overspeed) {
  PulseFilter filter;
  PulseDecision first = filter.process(100000u);
  zassert_true(first.accepted, "first pulse");
  zassert_true(first.first_pulse, "first flag");

  zassert_equal(static_cast<int>(PulseRejection::kDebounce),
                static_cast<int>(filter.process(101000u).rejection), "debounce");
  zassert_equal(static_cast<int>(PulseRejection::kOverspeed),
                static_cast<int>(filter.process(110000u).rejection), "overspeed");

  PulseDecision valid = filter.process(175600u);
  zassert_true(valid.accepted, "valid pulse");
  zassert_equal(75600u, valid.interval_us, "interval");
  zassert_equal(2u, filter.counters().accepted, "accepted count");
}

ZTEST(motion, test_speed_smoothing_and_timeout) {
  SpeedCalculator speed;
  zassert_equal(2520u, speed.onInterval(2100, 300000, 300000, false, 3),
                "first speed");
  zassert_equal(2520u, speed.onInterval(2100, 300000, 600000, true, 3),
                "smoothed speed");
  zassert_equal(2160u, speed.onInterval(2100, 450000, 1050000, true, 3),
                "window speed");
  zassert_equal(2160u, speed.updateForTimeout(3500000u, 3000000u),
                "timeout hold");
  zassert_equal(0u, speed.updateForTimeout(4050000u, 3000000u), "timeout zero");
}

ZTEST(motion, test_speed_gap_long_interval_corrected) {
  SpeedCalculator speed;
  zassert_equal(2520u, speed.onInterval(2100, 300000, 300000, false, 3),
                "baseline speed");
  zassert_equal(2520u, speed.onInterval(2100, 900000, 1200000, false, 3),
                "gap corrected speed");
  zassert_equal(1u, speed.speedIntervalCorrectedCount(), "correction count");
}

ZTEST(motion, test_speed_gap_short_spike_corrected) {
  SpeedCalculator speed;
  zassert_equal(2520u, speed.onInterval(2100, 300000, 300000, false, 3),
                "baseline speed");
  zassert_equal(2520u, speed.onInterval(2100, 100000, 400000, false, 3),
                "spike corrected speed");
  zassert_equal(1u, speed.speedIntervalCorrectedCount(), "correction count");
}

ZTEST(motion, test_trip_accumulation_and_reset) {
  TripComputer trip;
  for (uint32_t i = 0; i < 1000u; ++i) {
    trip.onRevolution(2100, 2520);
  }
  zassert_equal(2100000u, trip.snapshot().trip_distance_mm, "trip distance");
  zassert_equal(1000u, trip.snapshot().revolutions, "revs");

  trip.resetTrip();
  zassert_equal(0u, trip.snapshot().trip_distance_mm, "reset distance");
  zassert_equal(0u, trip.snapshot().revolutions, "reset revs");
  zassert_equal(1000ull, trip.totalRevolutions(), "persistent revs");
}

ZTEST(motion, test_ride_state_transitions) {
  RideStateMachine ride(3000);
  ride.reset(1000);
  zassert_equal(static_cast<int>(RideState::kMoving),
                static_cast<int>(ride.onPulse(1100).state), "moving");
  zassert_equal(1000u, ride.update(2100).moving_delta_ms, "moving delta");
  zassert_equal(static_cast<int>(RideState::kPaused),
                static_cast<int>(ride.update(4200).state), "paused");
  zassert_equal(static_cast<int>(RideState::kMoving),
                static_cast<int>(ride.onPulse(5300).state), "resume");
}

namespace {
uint32_t callback_count = 0;
void countTask(void*, uint32_t) { ++callback_count; }
}  // namespace

ZTEST(motion, test_scheduler_period_and_wrap) {
  callback_count = 0;
  ScheduledTask task{"test", 100, 0xFFFFFFF0u, countTask, nullptr, 0};
  Scheduler scheduler(&task, 1);
  scheduler.run(0xFFFFFFE0u);
  zassert_equal(0u, callback_count, "before due");
  scheduler.run(0xFFFFFFF0u);
  zassert_equal(1u, callback_count, "first run");
  scheduler.run(0x00000060u);
  zassert_equal(2u, callback_count, "wrapped run");
}

ZTEST_SUITE(motion, NULL, NULL, NULL, NULL, NULL);
