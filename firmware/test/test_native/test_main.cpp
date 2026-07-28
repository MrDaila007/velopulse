#include <unity.h>

#include <stdint.h>

#include "battery_model.h"
#include "crc32.h"
#include "display_formatter.h"
#include "page_carousel.h"
#include "pulse_filter.h"
#include "ride_state.h"
#include "scheduler.h"
#include "speed_calculator.h"
#include "trip_computer.h"

using namespace bike;

void setUp() {}
void tearDown() {}

void test_crc32_standard_vector() {
  const uint8_t input[] = "123456789";
  TEST_ASSERT_EQUAL_HEX32(0xCBF43926u, crc32(input, 9));
  TEST_ASSERT_EQUAL_HEX32(0u, crc32(input, 0));
}

void test_pulse_filter_first_debounce_and_overspeed() {
  PulseFilter filter;
  PulseDecision first = filter.process(100000u);
  TEST_ASSERT_TRUE(first.accepted);
  TEST_ASSERT_TRUE(first.first_pulse);

  TEST_ASSERT_EQUAL(PulseRejection::kDebounce, filter.process(101000u).rejection);
  TEST_ASSERT_EQUAL(PulseRejection::kOverspeed, filter.process(110000u).rejection);
  PulseDecision valid = filter.process(175600u);
  TEST_ASSERT_TRUE(valid.accepted);
  TEST_ASSERT_EQUAL_UINT32(75600u, valid.interval_us);
  TEST_ASSERT_EQUAL_UINT32(2u, filter.counters().accepted);
}

void test_pulse_filter_stuck_and_micros_wrap() {
  PulseFilter filter;
  TEST_ASSERT_TRUE(filter.process(0xFFFFFF00u).accepted);
  PulseDecision wrapped = filter.process(0x00013000u);
  TEST_ASSERT_TRUE(wrapped.accepted);
  TEST_ASSERT_EQUAL_UINT32(78080u, wrapped.interval_us);

  PulseFilter stuck;
  TEST_ASSERT_TRUE(stuck.process(100000u).accepted);
  TEST_ASSERT_EQUAL(PulseRejection::kStuck,
                    stuck.process(700000u, false, 500000u).rejection);
}

void test_speed_fixed_point_smoothing_and_timeout() {
  SpeedCalculator speed;
  TEST_ASSERT_EQUAL_UINT16(2520u, speed.onInterval(2100, 300000, 300000, false, 3));
  TEST_ASSERT_EQUAL_UINT16(2520u, speed.onInterval(2100, 300000, 600000, true, 3));
  TEST_ASSERT_EQUAL_UINT16(2160u, speed.onInterval(2100, 450000, 1050000, true, 3));
  TEST_ASSERT_EQUAL_UINT16(2160u, speed.updateForTimeout(3500000u, 3000000u));
  TEST_ASSERT_EQUAL_UINT16(0u, speed.updateForTimeout(4050000u, 3000000u));
}

void test_speed_smoothing_windows_two_and_five() {
  SpeedCalculator two;
  TEST_ASSERT_EQUAL_UINT16(2520u, two.onInterval(2100, 300000, 300000, true, 2));
  TEST_ASSERT_EQUAL_UINT16(2160u, two.onInterval(2100, 400000, 700000, true, 2));
  SpeedCalculator five;
  for (uint32_t i = 1; i <= 5; ++i) {
    five.onInterval(2100, i * 100000u, i * 100000u, true, 5);
  }
  TEST_ASSERT_EQUAL_UINT16(2520u, five.speedX100());
}

void test_speed_boundary_circumferences() {
  SpeedCalculator speed;
  TEST_ASSERT_EQUAL_UINT16(1800u, speed.onInterval(500, 100000, 100000, false, 2));
  speed.reset();
  TEST_ASSERT_EQUAL_UINT16(10800u, speed.onInterval(3000, 100000, 100000, false, 5));
}

void test_trip_accumulation_average_and_reset() {
  TripComputer trip;
  for (uint32_t i = 0; i < 1000000u; ++i) trip.onRevolution(2100, 2520);
  TEST_ASSERT_EQUAL_UINT32(2100000000u, trip.snapshot().trip_distance_mm);
  TEST_ASSERT_EQUAL_UINT64(2100000000ull, trip.snapshot().odometer_mm);
  TEST_ASSERT_EQUAL_UINT16(0u, trip.snapshot().average_speed_x100);

  trip.addMovingTime(300000000u);
  TEST_ASSERT_EQUAL_UINT16(2520u, trip.snapshot().average_speed_x100);
  TEST_ASSERT_EQUAL_UINT16(2520u, trip.snapshot().max_speed_x100);
  trip.resetTrip();
  TEST_ASSERT_EQUAL_UINT32(0u, trip.snapshot().trip_distance_mm);
  TEST_ASSERT_EQUAL_UINT32(0u, trip.snapshot().revolutions);
  TEST_ASSERT_EQUAL_UINT64(2100000000ull, trip.snapshot().odometer_mm);
}

void test_ride_state_transitions_and_paused_time() {
  RideStateMachine ride(3000);
  ride.reset(1000);
  RideUpdate started = ride.onPulse(1100);
  TEST_ASSERT_EQUAL(RideState::kMoving, started.state);
  RideUpdate moving = ride.update(2100);
  TEST_ASSERT_EQUAL_UINT32(1000u, moving.moving_delta_ms);
  RideUpdate paused = ride.update(4200);
  TEST_ASSERT_EQUAL(RideState::kPaused, paused.state);
  TEST_ASSERT_EQUAL_UINT32(2000u, paused.moving_delta_ms);
  TEST_ASSERT_EQUAL_UINT32(0u, ride.update(5200).moving_delta_ms);
  TEST_ASSERT_EQUAL(RideState::kMoving, ride.onPulse(5300).state);
}

namespace {
uint32_t callback_count = 0;
void countTask(void*, uint32_t) { ++callback_count; }
}

void test_scheduler_period_and_wrap() {
  callback_count = 0;
  ScheduledTask task{"test", 100, 0xFFFFFFF0u, countTask, nullptr, 0};
  Scheduler scheduler(&task, 1);
  scheduler.run(0xFFFFFFE0u);
  TEST_ASSERT_EQUAL_UINT32(0u, callback_count);
  scheduler.run(0xFFFFFFF0u);
  TEST_ASSERT_EQUAL_UINT32(1u, callback_count);
  scheduler.run(0x00000060u);
  TEST_ASSERT_EQUAL_UINT32(2u, callback_count);
}


void test_page_carousel_default_period_and_wrap() {
  DeviceConfig config;
  PageCarousel carousel;
  carousel.configure(config, 0xFFFFFF00u);

  TEST_ASSERT_EQUAL(DisplayPage::kTrip, carousel.currentPage());
  TEST_ASSERT_EQUAL_UINT8(5u, carousel.pageCount());
  TEST_ASSERT_FALSE(carousel.update(0x00000E9Fu));
  TEST_ASSERT_TRUE(carousel.update(0x00000EA0u));
  TEST_ASSERT_EQUAL(DisplayPage::kAverage, carousel.currentPage());
  TEST_ASSERT_TRUE(carousel.update(0x00002DE0u));
  TEST_ASSERT_EQUAL(DisplayPage::kMovingTime, carousel.currentPage());
}

void test_page_carousel_mask_order_and_fallback() {
  DeviceConfig config;
  config.enabled_pages_mask = 0x15u;
  config.page_order[0] = 4;
  config.page_order[1] = 2;
  config.page_order[2] = 0;
  config.page_order[3] = 4;
  config.page_order[4] = 9;

  PageCarousel carousel;
  carousel.configure(config, 0);
  TEST_ASSERT_EQUAL_UINT8(3u, carousel.pageCount());
  TEST_ASSERT_EQUAL(DisplayPage::kOdometer, carousel.currentPage());
  TEST_ASSERT_TRUE(carousel.update(4000));
  TEST_ASSERT_EQUAL(DisplayPage::kMaximum, carousel.currentPage());

  config.enabled_pages_mask = 0;
  carousel.configure(config, 5000);
  TEST_ASSERT_EQUAL_UINT8(1u, carousel.pageCount());
  TEST_ASSERT_EQUAL(DisplayPage::kTrip, carousel.currentPage());
}

void test_page_carousel_pinned_page() {
  DeviceConfig config;
  config.auto_page_switch = false;
  config.enabled_pages_mask = 0x06u;
  config.pinned_page = 2;
  PageCarousel carousel;
  carousel.configure(config, 0);
  TEST_ASSERT_EQUAL(DisplayPage::kMaximum, carousel.currentPage());
  TEST_ASSERT_FALSE(carousel.update(10000));

  config.pinned_page = 4;
  carousel.configure(config, 10000);
  TEST_ASSERT_EQUAL(DisplayPage::kAverage, carousel.currentPage());
}


void test_battery_raw_conversion_and_calibration() {
  TEST_ASSERT_EQUAL_UINT16(0u, BatteryModel::rawToMillivolts(0, 1000, 0));
  TEST_ASSERT_EQUAL_UINT16(4001u, BatteryModel::rawToMillivolts(3413, 1000, 0));
  TEST_ASSERT_EQUAL_UINT16(4051u, BatteryModel::rawToMillivolts(3413, 1010, 10));
  TEST_ASSERT_EQUAL_UINT16(4800u, BatteryModel::rawToMillivolts(5000, 1000, 0));
}

void test_battery_soc_table_and_interpolation() {
  const uint16_t millivolts[] = {3300, 3400, 3500, 3600, 3650, 3700,
                                 3800, 3900, 4000, 4100, 4200};
  const uint8_t percent[] = {0, 4, 10, 20, 28, 38, 52, 68, 82, 92, 100};
  for (uint8_t i = 0; i < 11; ++i) {
    TEST_ASSERT_EQUAL_UINT8(percent[i], BatteryModel::voltageToPercent(millivolts[i]));
  }
  TEST_ASSERT_EQUAL_UINT8(0u, BatteryModel::voltageToPercent(3000));
  TEST_ASSERT_EQUAL_UINT8(15u, BatteryModel::voltageToPercent(3550));
  TEST_ASSERT_EQUAL_UINT8(100u, BatteryModel::voltageToPercent(4300));
}

void test_battery_ema_monotonicity_and_usb_growth() {
  BatteryModel model;
  TEST_ASSERT_FALSE(model.addVoltageSample(2000));
  TEST_ASSERT_FALSE(model.snapshot().valid);
  TEST_ASSERT_TRUE(model.addVoltageSample(4000));
  BatterySnapshot snapshot = model.recalculate(false, 20);
  TEST_ASSERT_EQUAL_UINT16(4000u, snapshot.millivolts);
  TEST_ASSERT_EQUAL_UINT8(82u, snapshot.percent);
  TEST_ASSERT_FALSE(snapshot.usb_present);
  TEST_ASSERT_EQUAL(ChargeStatus::kUnknown, snapshot.charge_status);

  TEST_ASSERT_TRUE(model.addVoltageSample(4200));
  snapshot = model.recalculate(false, 20);
  TEST_ASSERT_EQUAL_UINT16(4025u, snapshot.millivolts);
  TEST_ASSERT_EQUAL_UINT8(82u, snapshot.percent);

  snapshot = model.recalculate(true, 20);
  TEST_ASSERT_EQUAL_UINT8(85u, snapshot.percent);
  TEST_ASSERT_TRUE(snapshot.usb_present);
}

void test_battery_low_threshold_hysteresis() {
  BatteryModel model;
  TEST_ASSERT_TRUE(model.addVoltageSample(3600));
  BatterySnapshot snapshot = model.recalculate(false, 20);
  TEST_ASSERT_EQUAL_UINT8(20u, snapshot.percent);
  TEST_ASSERT_TRUE(snapshot.low_battery);

  for (uint8_t i = 0; i < 8; ++i) {
    TEST_ASSERT_TRUE(model.addVoltageSample(3700));
  }
  snapshot = model.recalculate(true, 20);
  TEST_ASSERT_GREATER_OR_EQUAL_UINT8(23u, snapshot.percent);
  TEST_ASSERT_FALSE(snapshot.low_battery);
}

void test_display_formatter_all_pages_and_battery() {
  DisplaySnapshot snapshot;
  snapshot.trip.speed_x100 = 2489;
  snapshot.trip.trip_distance_mm = 18420000u;
  snapshot.trip.average_speed_x100 = 1975;
  snapshot.trip.max_speed_x100 = 4239;
  snapshot.trip.moving_time_ms = 4356000u;
  snapshot.trip.odometer_mm = 1234500000ull;
  snapshot.trip.ride_state = RideState::kMoving;
  snapshot.battery.valid = true;
  snapshot.battery.percent = 82;

  DisplayFrame frame = DisplayFormatter::format(snapshot, DisplayPage::kTrip);
  TEST_ASSERT_EQUAL_STRING("24.8", frame.speed);
  TEST_ASSERT_EQUAL_STRING("km/h", frame.units);
  TEST_ASSERT_EQUAL_STRING("MOV TRIP 18.42 km", frame.lower);
  TEST_ASSERT_EQUAL_STRING("82%", frame.battery_percent);
  TEST_ASSERT_EQUAL_UINT8(6u, frame.battery_fill_width);

  frame = DisplayFormatter::format(snapshot, DisplayPage::kAverage);
  TEST_ASSERT_EQUAL_STRING("MOV AVG 19.7 km/h", frame.lower);
  frame = DisplayFormatter::format(snapshot, DisplayPage::kMaximum);
  TEST_ASSERT_EQUAL_STRING("MOV MAX 42.3 km/h", frame.lower);
  frame = DisplayFormatter::format(snapshot, DisplayPage::kMovingTime);
  TEST_ASSERT_EQUAL_STRING("MOV TIME 1:12:36", frame.lower);
  frame = DisplayFormatter::format(snapshot, DisplayPage::kOdometer);
  TEST_ASSERT_EQUAL_STRING("MOV ODO 1234.5 km", frame.lower);
}

void test_display_formatter_battery_and_value_limits() {
  DisplaySnapshot snapshot;
  snapshot.trip.ride_state = RideState::kPaused;
  snapshot.trip.odometer_mm = 100000000000ull;

  DisplayFrame frame = DisplayFormatter::format(snapshot, DisplayPage::kOdometer);
  TEST_ASSERT_EQUAL_STRING("PAUSE ODO 99999+ km", frame.lower);
  TEST_ASSERT_EQUAL_STRING("--%", frame.battery_percent);
  TEST_ASSERT_EQUAL_UINT8(0u, frame.battery_fill_width);

  snapshot.battery.valid = true;
  snapshot.battery.percent = 255;
  frame = DisplayFormatter::format(snapshot, DisplayPage::kTrip);
  TEST_ASSERT_EQUAL_STRING("100%", frame.battery_percent);
  TEST_ASSERT_EQUAL_UINT8(8u, frame.battery_fill_width);
}

void test_display_formatter_low_battery_warning() {
  DisplaySnapshot snapshot;
  snapshot.battery.valid = true;
  snapshot.battery.percent = 18;
  snapshot.battery.low_battery = true;
  DisplayFrame frame =
      DisplayFormatter::format(snapshot, DisplayPage::kTrip, true);
  TEST_ASSERT_EQUAL_STRING("18%", frame.battery_percent);
  TEST_ASSERT_EQUAL_STRING("LOW BATT", frame.lower);
  TEST_ASSERT_TRUE(frame.low_battery_warning);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_crc32_standard_vector);
  RUN_TEST(test_pulse_filter_first_debounce_and_overspeed);
  RUN_TEST(test_pulse_filter_stuck_and_micros_wrap);
  RUN_TEST(test_speed_fixed_point_smoothing_and_timeout);
  RUN_TEST(test_speed_smoothing_windows_two_and_five);
  RUN_TEST(test_speed_boundary_circumferences);
  RUN_TEST(test_trip_accumulation_average_and_reset);
  RUN_TEST(test_ride_state_transitions_and_paused_time);
  RUN_TEST(test_scheduler_period_and_wrap);
  RUN_TEST(test_page_carousel_default_period_and_wrap);
  RUN_TEST(test_page_carousel_mask_order_and_fallback);
  RUN_TEST(test_page_carousel_pinned_page);
  RUN_TEST(test_battery_raw_conversion_and_calibration);
  RUN_TEST(test_battery_soc_table_and_interpolation);
  RUN_TEST(test_battery_ema_monotonicity_and_usb_growth);
  RUN_TEST(test_battery_low_threshold_hysteresis);
  RUN_TEST(test_display_formatter_all_pages_and_battery);
  RUN_TEST(test_display_formatter_battery_and_value_limits);
  RUN_TEST(test_display_formatter_low_battery_warning);
  return UNITY_END();
}
