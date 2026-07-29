#include <unity.h>

#include <stdint.h>
#include <string.h>

#include <map>
#include <string>
#include <vector>

#include "battery_model.h"
#include "config_codec.h"
#include "config_validator.h"
#include "crc32.h"
#include "display_formatter.h"
#include "display_power.h"
#include "odometer_save_policy.h"
#include "page_carousel.h"
#include "pulse_filter.h"
#include "ride_state.h"
#include "scheduler.h"
#include "speed_calculator.h"
#include "storage_manager.h"
#include "trip_computer.h"

using namespace bike;

void setUp() {}
void tearDown() {}

namespace {

class MemoryStorageBackend final : public StorageBackend {
 public:
  bool begin() override { return begin_ok; }

  StorageIoResult read(const char* path,
                       uint8_t* output,
                       size_t capacity,
                       size_t& length) override {
    const auto found = files.find(path);
    if (found == files.end()) return StorageIoResult::kNotFound;
    if (found->second.size() > capacity) return StorageIoResult::kError;
    memcpy(output, found->second.data(), found->second.size());
    length = found->second.size();
    return StorageIoResult::kOk;
  }

  bool write(const char* path, const uint8_t* data, size_t length) override {
    if (!write_ok) return false;
    files[path] = std::vector<uint8_t>(data, data + length);
    return true;
  }

  void corrupt(const char* path, size_t offset) { files[path][offset] ^= 0x80u; }

  bool begin_ok = true;
  bool write_ok = true;
  std::map<std::string, std::vector<uint8_t>> files;
};

void putConfigRecord(MemoryStorageBackend& backend,
                     const char* path,
                     const DeviceConfig& config,
                     uint32_t sequence) {
  uint8_t payload[kDeviceConfigPayloadSize];
  uint8_t record[kMaximumRecordSize];
  encodeDeviceConfig(config, payload);
  const size_t length = encodeRecord(payload, sizeof(payload),
                                     kConfigRecordVersion, sequence,
                                     record, sizeof(record));
  backend.write(path, record, length);
}

}  // namespace

void test_crc32_standard_vector() {
  const uint8_t input[] = "123456789";
  TEST_ASSERT_EQUAL_HEX32(0xCBF43926u, crc32(input, 9));
  TEST_ASSERT_EQUAL_HEX32(0u, crc32(input, 0));
}

void test_config_codec_exact_48_byte_round_trip() {
  const uint8_t expected[kDeviceConfigPayloadSize] = {
      0x01, 0x0F, 0x34, 0x08, 0x64, 0x03, 0x3C, 0x00,
      0x84, 0x03, 0x3C, 0x04, 0x1F, 0x14, 0xF4, 0x01,
      0x03, 0x03, 0x00, 0x00, 0xE8, 0x03, 0x00, 0x00,
      0x00, 0x01, 0x02, 0x03, 0x04, 0x00, 'B',  'i',
      'k',  'e',  'C',  'o',  'm',  'p',  '-',  'X',
      'X',  'X',  'X',  0x00, 0x00, 0x00, 0x00, 0x00};
  DeviceConfig defaults;
  uint8_t encoded[kDeviceConfigPayloadSize];
  encodeDeviceConfig(defaults, encoded);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, encoded, kDeviceConfigPayloadSize);

  DeviceConfig decoded;
  TEST_ASSERT_TRUE(decodeDeviceConfig(encoded, sizeof(encoded), decoded));
  TEST_ASSERT_TRUE(deviceConfigsEqual(defaults, decoded));
}

void test_config_validator_accepts_all_boundaries() {
  DeviceConfig minimum;
  minimum.wheel_circumference_mm = 500;
  minimum.max_speed_kmh = 20;
  minimum.stop_timeout_s = 1;
  minimum.display_timeout_s = 0;
  minimum.deep_sleep_timeout_s = 0;
  minimum.brightness_pct = 1;
  minimum.page_switch_period_s = 1;
  minimum.enabled_pages_mask = 1;
  minimum.low_battery_pct = 5;
  minimum.odometer_save_interval_m = 100;
  minimum.smoothing_window = 2;
  minimum.debounce_ms = 0;
  minimum.active_edge = 0;
  minimum.pinned_page = 0;
  minimum.batt_cal_scale_permille = 800;
  minimum.batt_cal_offset_mv = -500;
  memset(minimum.device_name, 0, sizeof(minimum.device_name));
  memcpy(minimum.device_name, "Ab3", 3);
  TEST_ASSERT_EQUAL(ConfigValidationError::kNone,
                    ConfigValidator::validate(minimum));

  DeviceConfig maximum;
  maximum.wheel_circumference_mm = 3000;
  maximum.max_speed_kmh = 200;
  maximum.stop_timeout_s = 30;
  maximum.display_timeout_s = 600;
  maximum.deep_sleep_timeout_s = 3600;
  maximum.brightness_pct = 100;
  maximum.page_switch_period_s = 60;
  maximum.enabled_pages_mask = 0x1F;
  maximum.low_battery_pct = 50;
  maximum.odometer_save_interval_m = 5000;
  maximum.smoothing_window = 5;
  maximum.debounce_ms = 50;
  maximum.active_edge = 2;
  maximum.pinned_page = 4;
  maximum.batt_cal_scale_permille = 1200;
  maximum.batt_cal_offset_mv = 500;
  memcpy(maximum.device_name, "123456789012345", 16);
  TEST_ASSERT_EQUAL(ConfigValidationError::kNone,
                    ConfigValidator::validate(maximum));
}

void test_config_validator_rejects_ranges_mask_order_and_name() {
  DeviceConfig config;
  config.wheel_circumference_mm = 499;
  TEST_ASSERT_EQUAL(ConfigValidationError::kWheelCircumference,
                    ConfigValidator::validate(config));
  config = {};
  config.max_speed_kmh = 201;
  TEST_ASSERT_EQUAL(ConfigValidationError::kMaxSpeed,
                    ConfigValidator::validate(config));
  config = {};
  config.stop_timeout_s = 0;
  TEST_ASSERT_EQUAL(ConfigValidationError::kStopTimeout,
                    ConfigValidator::validate(config));
  config = {};
  config.display_timeout_s = 9;
  TEST_ASSERT_EQUAL(ConfigValidationError::kDisplayTimeout,
                    ConfigValidator::validate(config));
  config = {};
  config.deep_sleep_timeout_s = 59;
  TEST_ASSERT_EQUAL(ConfigValidationError::kDeepSleepTimeout,
                    ConfigValidator::validate(config));
  config = {};
  config.brightness_pct = 0;
  TEST_ASSERT_EQUAL(ConfigValidationError::kBrightness,
                    ConfigValidator::validate(config));
  config = {};
  config.page_switch_period_s = 61;
  TEST_ASSERT_EQUAL(ConfigValidationError::kPageSwitchPeriod,
                    ConfigValidator::validate(config));
  config = {};
  config.enabled_pages_mask = 0x20;
  TEST_ASSERT_EQUAL(ConfigValidationError::kEnabledPagesMask,
                    ConfigValidator::validate(config));
  config = {};
  config.low_battery_pct = 4;
  TEST_ASSERT_EQUAL(ConfigValidationError::kLowBattery,
                    ConfigValidator::validate(config));
  config = {};
  config.odometer_save_interval_m = 99;
  TEST_ASSERT_EQUAL(ConfigValidationError::kOdometerSaveInterval,
                    ConfigValidator::validate(config));
  config = {};
  config.smoothing_window = 1;
  TEST_ASSERT_EQUAL(ConfigValidationError::kSmoothingWindow,
                    ConfigValidator::validate(config));
  config = {};
  config.debounce_ms = 51;
  TEST_ASSERT_EQUAL(ConfigValidationError::kDebounce,
                    ConfigValidator::validate(config));
  config = {};
  config.active_edge = 3;
  TEST_ASSERT_EQUAL(ConfigValidationError::kActiveEdge,
                    ConfigValidator::validate(config));
  config = {};
  config.pinned_page = 5;
  TEST_ASSERT_EQUAL(ConfigValidationError::kPinnedPage,
                    ConfigValidator::validate(config));
  config = {};
  config.batt_cal_scale_permille = 799;
  TEST_ASSERT_EQUAL(ConfigValidationError::kBatteryScale,
                    ConfigValidator::validate(config));
  config = {};
  config.batt_cal_offset_mv = 501;
  TEST_ASSERT_EQUAL(ConfigValidationError::kBatteryOffset,
                    ConfigValidator::validate(config));
  config = {};
  config.page_order[4] = 3;
  TEST_ASSERT_EQUAL(ConfigValidationError::kPageOrder,
                    ConfigValidator::validate(config));
  config = {};
  memcpy(config.device_name, "Bad!", 5);
  TEST_ASSERT_EQUAL(ConfigValidationError::kDeviceName,
                    ConfigValidator::validate(config));
}

void test_config_codec_rejects_version_length_reserved_and_invalid_payload() {
  uint8_t encoded[kDeviceConfigPayloadSize];
  encodeDeviceConfig(DeviceConfig{}, encoded);
  DeviceConfig decoded;
  TEST_ASSERT_FALSE(decodeDeviceConfig(encoded, sizeof(encoded) - 1, decoded));
  encoded[0] = 2;
  TEST_ASSERT_FALSE(decodeDeviceConfig(encoded, sizeof(encoded), decoded));
  encodeDeviceConfig(DeviceConfig{}, encoded);
  encoded[29] = 1;
  TEST_ASSERT_FALSE(decodeDeviceConfig(encoded, sizeof(encoded), decoded));
  encodeDeviceConfig(DeviceConfig{}, encoded);
  encoded[12] = 0;
  TEST_ASSERT_FALSE(decodeDeviceConfig(encoded, sizeof(encoded), decoded));
}

void test_record_header_crc_and_metadata_validation() {
  const uint8_t payload[] = {1, 2, 3, 4};
  uint8_t record[32];
  const size_t length =
      encodeRecord(payload, sizeof(payload), 7, 42, record, sizeof(record));
  TEST_ASSERT_EQUAL_UINT32(20u, length);
  TEST_ASSERT_EQUAL_HEX8('B', record[0]);
  TEST_ASSERT_EQUAL_HEX8('K', record[1]);
  TEST_ASSERT_EQUAL_HEX8('C', record[2]);
  TEST_ASSERT_EQUAL_HEX8('P', record[3]);

  DecodedRecord decoded;
  TEST_ASSERT_TRUE(decodeRecord(record, length, 7, sizeof(payload), decoded));
  TEST_ASSERT_EQUAL_UINT32(42u, decoded.header.sequence);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(payload, decoded.payload, sizeof(payload));

  uint8_t damaged[sizeof(record)];
  memcpy(damaged, record, length);
  damaged[0] ^= 1;
  TEST_ASSERT_FALSE(decodeRecord(damaged, length, 7, sizeof(payload), decoded));
  memcpy(damaged, record, length);
  damaged[4] = 8;
  TEST_ASSERT_FALSE(decodeRecord(damaged, length, 7, sizeof(payload), decoded));
  memcpy(damaged, record, length);
  damaged[6] = 5;
  TEST_ASSERT_FALSE(decodeRecord(damaged, length, 7, sizeof(payload), decoded));
  memcpy(damaged, record, length);
  damaged[kRecordHeaderSize] ^= 1;
  TEST_ASSERT_FALSE(decodeRecord(damaged, length, 7, sizeof(payload), decoded));
}

void test_storage_selects_newest_slot_alternates_and_skips_unchanged() {
  MemoryStorageBackend backend;
  StorageManager storage(backend);
  TEST_ASSERT_TRUE(storage.begin());

  DeviceConfig loaded;
  StorageLoadInfo info;
  TEST_ASSERT_TRUE(storage.loadConfig(loaded, info));
  TEST_ASSERT_EQUAL(StorageSource::kDefaults, info.source);
  TEST_ASSERT_TRUE(info.defaults_written);
  TEST_ASSERT_EQUAL_UINT32(1u, info.sequence);
  TEST_ASSERT_EQUAL_UINT32(1u, storage.counters().writes);

  TEST_ASSERT_TRUE(storage.saveConfig(loaded));
  TEST_ASSERT_EQUAL_UINT32(1u, storage.counters().writes);
  TEST_ASSERT_EQUAL_UINT32(1u, storage.counters().skipped_writes);

  loaded.brightness_pct = 61;
  TEST_ASSERT_TRUE(storage.saveConfig(loaded));
  TEST_ASSERT_TRUE(backend.files.count("/cfg_b") != 0);

  StorageManager reloaded(backend);
  TEST_ASSERT_TRUE(reloaded.begin());
  DeviceConfig from_flash;
  TEST_ASSERT_TRUE(reloaded.loadConfig(from_flash, info));
  TEST_ASSERT_EQUAL(StorageSource::kSlotB, info.source);
  TEST_ASSERT_EQUAL_UINT32(2u, info.sequence);
  TEST_ASSERT_EQUAL_UINT8(61u, from_flash.brightness_pct);

  from_flash.brightness_pct = 62;
  TEST_ASSERT_TRUE(reloaded.saveConfig(from_flash));
  StorageManager third_boot(backend);
  TEST_ASSERT_TRUE(third_boot.begin());
  TEST_ASSERT_TRUE(third_boot.loadConfig(from_flash, info));
  TEST_ASSERT_EQUAL(StorageSource::kSlotA, info.source);
  TEST_ASSERT_EQUAL_UINT32(3u, info.sequence);
  TEST_ASSERT_EQUAL_UINT8(62u, from_flash.brightness_pct);
}

void test_storage_falls_back_from_corrupt_or_invalid_newest_slot() {
  MemoryStorageBackend backend;
  DeviceConfig older;
  older.brightness_pct = 40;
  DeviceConfig newer;
  newer.brightness_pct = 70;
  putConfigRecord(backend, "/cfg_a", older, 10);
  putConfigRecord(backend, "/cfg_b", newer, 11);
  backend.corrupt("/cfg_b", kRecordHeaderSize + 10);

  StorageManager storage(backend);
  TEST_ASSERT_TRUE(storage.begin());
  DeviceConfig loaded;
  StorageLoadInfo info;
  TEST_ASSERT_TRUE(storage.loadConfig(loaded, info));
  TEST_ASSERT_EQUAL(StorageSource::kSlotA, info.source);
  TEST_ASSERT_EQUAL_UINT32(10u, info.sequence);
  TEST_ASSERT_EQUAL_UINT8(40u, loaded.brightness_pct);
  TEST_ASSERT_TRUE(info.recovered);
  TEST_ASSERT_EQUAL_UINT32(1u, storage.counters().config_slot_recoveries);

  uint8_t invalid_payload[kDeviceConfigPayloadSize];
  encodeDeviceConfig(newer, invalid_payload);
  invalid_payload[12] = 0;
  uint8_t invalid_record[kMaximumRecordSize];
  const size_t invalid_length = encodeRecord(
      invalid_payload, sizeof(invalid_payload), kConfigRecordVersion, 12,
      invalid_record, sizeof(invalid_record));
  backend.write("/cfg_b", invalid_record, invalid_length);
  StorageManager validation_fallback(backend);
  TEST_ASSERT_TRUE(validation_fallback.begin());
  TEST_ASSERT_TRUE(validation_fallback.loadConfig(loaded, info));
  TEST_ASSERT_EQUAL(StorageSource::kSlotA, info.source);
}

void test_storage_restores_defaults_when_both_slots_are_corrupt() {
  MemoryStorageBackend backend;
  backend.files["/cfg_a"] = {1, 2, 3};
  backend.files["/cfg_b"] = {4, 5, 6};
  StorageManager storage(backend);
  TEST_ASSERT_TRUE(storage.begin());
  DeviceConfig config;
  config.brightness_pct = 99;
  StorageLoadInfo info;
  TEST_ASSERT_TRUE(storage.loadConfig(config, info));
  TEST_ASSERT_EQUAL(StorageSource::kDefaults, info.source);
  TEST_ASSERT_TRUE(info.recovered);
  TEST_ASSERT_TRUE(info.defaults_written);
  TEST_ASSERT_EQUAL_UINT8(kDefaultBrightnessPct, config.brightness_pct);
  TEST_ASSERT_EQUAL_UINT32(1u, storage.counters().config_defaults_restored);

  StorageManager next_boot(backend);
  TEST_ASSERT_TRUE(next_boot.begin());
  TEST_ASSERT_TRUE(next_boot.loadConfig(config, info));
  TEST_ASSERT_EQUAL(StorageSource::kSlotA, info.source);
  TEST_ASSERT_EQUAL_UINT32(1u, info.sequence);
}

void test_odometer_alternates_and_recovers_older_slot() {
  MemoryStorageBackend backend;
  StorageManager storage(backend);
  TEST_ASSERT_TRUE(storage.begin());
  OdometerData odometer;
  StorageLoadInfo info;
  TEST_ASSERT_TRUE(storage.loadOdometer(odometer, info));
  TEST_ASSERT_EQUAL(StorageSource::kDefaults, info.source);
  odometer.odometer_mm = 123456789012ull;
  odometer.total_revolutions = 9876543210ull;
  TEST_ASSERT_TRUE(storage.saveOdometer(odometer));

  StorageManager reloaded(backend);
  TEST_ASSERT_TRUE(reloaded.begin());
  OdometerData restored;
  TEST_ASSERT_TRUE(reloaded.loadOdometer(restored, info));
  TEST_ASSERT_EQUAL(StorageSource::kSlotB, info.source);
  TEST_ASSERT_EQUAL_UINT64(odometer.odometer_mm, restored.odometer_mm);
  TEST_ASSERT_EQUAL_UINT64(odometer.total_revolutions,
                           restored.total_revolutions);

  backend.corrupt("/odo_b", kRecordHeaderSize);
  StorageManager fallback(backend);
  TEST_ASSERT_TRUE(fallback.begin());
  TEST_ASSERT_TRUE(fallback.loadOdometer(restored, info));
  TEST_ASSERT_EQUAL(StorageSource::kSlotA, info.source);
  TEST_ASSERT_EQUAL_UINT64(0u, restored.odometer_mm);
  TEST_ASSERT_TRUE(info.recovered);
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
  TEST_ASSERT_EQUAL_UINT64(1000000ull, trip.totalRevolutions());
}

void test_trip_restores_only_persistent_totals() {
  TripComputer trip;
  trip.restorePersistentTotals(123456789012ull, 9876543210ull);
  TEST_ASSERT_EQUAL_UINT64(123456789012ull, trip.snapshot().odometer_mm);
  TEST_ASSERT_EQUAL_UINT64(9876543210ull, trip.totalRevolutions());
  TEST_ASSERT_EQUAL_UINT32(0u, trip.snapshot().trip_distance_mm);
  TEST_ASSERT_EQUAL_UINT32(0u, trip.snapshot().revolutions);
  trip.onRevolution(2100, 1000);
  TEST_ASSERT_EQUAL_UINT64(123456791112ull, trip.snapshot().odometer_mm);
  TEST_ASSERT_EQUAL_UINT64(9876543211ull, trip.totalRevolutions());
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


void test_display_power_dim_off_wake_disable_and_wrap() {
  DisplayPower power;
  power.configure(60, 1000);
  TEST_ASSERT_EQUAL(DisplayPowerState::kBright, power.state());
  TEST_ASSERT_FALSE(power.update(30999));
  TEST_ASSERT_TRUE(power.update(31000));
  TEST_ASSERT_EQUAL(DisplayPowerState::kDim, power.state());
  TEST_ASSERT_FALSE(power.update(60999));
  TEST_ASSERT_TRUE(power.update(61000));
  TEST_ASSERT_EQUAL(DisplayPowerState::kOff, power.state());
  TEST_ASSERT_TRUE(power.noteActivity(62000));
  TEST_ASSERT_EQUAL(DisplayPowerState::kBright, power.state());

  TEST_ASSERT_FALSE(power.noteActivity(63000));
  TEST_ASSERT_FALSE(power.update(92999));
  TEST_ASSERT_TRUE(power.update(93000));
  TEST_ASSERT_EQUAL(DisplayPowerState::kDim, power.state());

  power.configure(0, 0);
  TEST_ASSERT_FALSE(power.update(0xFFFFFFFFu));
  TEST_ASSERT_EQUAL(DisplayPowerState::kBright, power.state());

  power.configure(2, 0xFFFFFF00u);
  TEST_ASSERT_FALSE(power.update(0x000002E7u));
  TEST_ASSERT_TRUE(power.update(0x000002E8u));
  TEST_ASSERT_EQUAL(DisplayPowerState::kDim, power.state());
  TEST_ASSERT_TRUE(power.update(0x000006D0u));
  TEST_ASSERT_EQUAL(DisplayPowerState::kOff, power.state());
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

void test_odometer_save_distance_boundaries_and_five_km_count() {
  OdometerSavePolicy policy;
  policy.configure(500);
  policy.markSaved(0);
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kNone, policy.evaluate(499999u, 0));
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kDistance,
                    policy.evaluate(500000u, 0));

  uint32_t saves = 0;
  for (uint32_t step = 1; step <= 10; ++step) {
    const uint64_t odometer_mm = static_cast<uint64_t>(step) * 500000ull;
    const OdometerSaveTrigger trigger = policy.evaluate(odometer_mm, 0);
    TEST_ASSERT_EQUAL(OdometerSaveTrigger::kDistance, trigger);
    policy.markSaved(odometer_mm);
    policy.acknowledge(trigger);
    ++saves;
  }
  TEST_ASSERT_EQUAL_UINT32(10u, saves);
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kNone,
                    policy.evaluate(5000000ull, 0));
}

void test_odometer_save_paused_settle_delay_and_cancel() {
  OdometerSavePolicy policy;
  policy.configure(500);
  policy.markSaved(0);
  policy.noteRideState(RideState::kMoving, 1000);
  policy.noteRideState(RideState::kPaused, 2000);
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kNone, policy.evaluate(0, 31999));
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kPausedSettle,
                    policy.evaluate(0, 32000));

  policy.acknowledge(OdometerSaveTrigger::kPausedSettle);
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kNone, policy.evaluate(0, 40000));

  policy.noteRideState(RideState::kMoving, 41000);
  policy.noteRideState(RideState::kPaused, 42000);
  policy.noteRideState(RideState::kMoving, 43000);
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kNone, policy.evaluate(0, 80000));
}

void test_odometer_save_display_off_deep_sleep_and_force() {
  OdometerSavePolicy policy;
  policy.configure(500);
  policy.markSaved(1000);
  policy.noteDisplayPower(DisplayPowerState::kBright);
  policy.noteDisplayPower(DisplayPowerState::kOff);
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kDisplayOff, policy.evaluate(1000, 0));
  policy.acknowledge(OdometerSaveTrigger::kDisplayOff);
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kNone, policy.evaluate(1000, 0));

  policy.requestDeepSleepSave();
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kDeepSleep, policy.evaluate(1000, 0));
  policy.acknowledge(OdometerSaveTrigger::kDeepSleep);

  policy.requestForceSave();
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kForceSave, policy.evaluate(1000, 0));
  policy.acknowledge(OdometerSaveTrigger::kForceSave);
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kNone, policy.evaluate(1000, 0));
}

void test_odometer_save_critical_battery_once_and_usb_reboot() {
  OdometerSavePolicy policy;
  policy.configure(500);
  policy.markSaved(0);
  policy.noteBatteryPercent(6, true);
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kNone, policy.evaluate(0, 0));
  policy.noteBatteryPercent(5, true);
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kCriticalBattery,
                    policy.evaluate(0, 0));
  policy.acknowledge(OdometerSaveTrigger::kCriticalBattery);
  policy.noteBatteryPercent(4, true);
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kNone, policy.evaluate(0, 0));
  policy.noteBatteryPercent(10, true);
  policy.noteBatteryPercent(5, true);
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kCriticalBattery,
                    policy.evaluate(0, 0));
  policy.acknowledge(OdometerSaveTrigger::kCriticalBattery);

  policy.noteUsbPresent(true);
  policy.noteUsbPresent(false);
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kUsbDisconnect,
                    policy.evaluate(0, 0));
  policy.acknowledge(OdometerSaveTrigger::kUsbDisconnect);

  policy.requestRebootSave();
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kReboot, policy.evaluate(0, 0));
  policy.acknowledge(OdometerSaveTrigger::kReboot);
}

void test_odometer_save_unchanged_skips_sequence_growth() {
  MemoryStorageBackend backend;
  StorageManager storage(backend);
  TEST_ASSERT_TRUE(storage.begin());
  OdometerData odometer;
  StorageLoadInfo info;
  TEST_ASSERT_TRUE(storage.loadOdometer(odometer, info));
  TEST_ASSERT_EQUAL_UINT32(1u, storage.lastOdometerSequence());
  const uint32_t writes_before = storage.counters().writes;

  odometer.odometer_mm = 0;
  odometer.total_revolutions = 0;
  TEST_ASSERT_TRUE(storage.saveOdometer(odometer));
  TEST_ASSERT_EQUAL_UINT32(writes_before, storage.counters().writes);
  TEST_ASSERT_EQUAL_UINT32(1u, storage.counters().skipped_writes);
  TEST_ASSERT_EQUAL_UINT32(1u, storage.lastOdometerSequence());

  odometer.odometer_mm = 500000;
  odometer.total_revolutions = 238;
  TEST_ASSERT_TRUE(storage.saveOdometer(odometer));
  TEST_ASSERT_EQUAL_UINT32(writes_before + 1u, storage.counters().writes);
  TEST_ASSERT_EQUAL_UINT32(2u, storage.lastOdometerSequence());
}

void test_odometer_save_flash_error_keeps_distance_retry() {
  MemoryStorageBackend backend;
  StorageManager storage(backend);
  TEST_ASSERT_TRUE(storage.begin());
  OdometerData loaded;
  StorageLoadInfo info;
  TEST_ASSERT_TRUE(storage.loadOdometer(loaded, info));

  OdometerSavePolicy policy;
  policy.configure(500);
  policy.markSaved(0);
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kDistance,
                    policy.evaluate(500000u, 0));

  backend.write_ok = false;
  OdometerData data{500000u, 100u};
  TEST_ASSERT_FALSE(storage.saveOdometer(data));
  // Distance baseline stays unsaved so the ride can continue and retry.
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kDistance,
                    policy.evaluate(500000u, 0));

  backend.write_ok = true;
  TEST_ASSERT_TRUE(storage.saveOdometer(data));
  policy.markSaved(500000u);
  policy.acknowledge(OdometerSaveTrigger::kDistance);
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kNone, policy.evaluate(500000u, 0));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_crc32_standard_vector);
  RUN_TEST(test_config_codec_exact_48_byte_round_trip);
  RUN_TEST(test_config_validator_accepts_all_boundaries);
  RUN_TEST(test_config_validator_rejects_ranges_mask_order_and_name);
  RUN_TEST(test_config_codec_rejects_version_length_reserved_and_invalid_payload);
  RUN_TEST(test_record_header_crc_and_metadata_validation);
  RUN_TEST(test_storage_selects_newest_slot_alternates_and_skips_unchanged);
  RUN_TEST(test_storage_falls_back_from_corrupt_or_invalid_newest_slot);
  RUN_TEST(test_storage_restores_defaults_when_both_slots_are_corrupt);
  RUN_TEST(test_odometer_alternates_and_recovers_older_slot);
  RUN_TEST(test_pulse_filter_first_debounce_and_overspeed);
  RUN_TEST(test_pulse_filter_stuck_and_micros_wrap);
  RUN_TEST(test_speed_fixed_point_smoothing_and_timeout);
  RUN_TEST(test_speed_smoothing_windows_two_and_five);
  RUN_TEST(test_speed_boundary_circumferences);
  RUN_TEST(test_trip_accumulation_average_and_reset);
  RUN_TEST(test_trip_restores_only_persistent_totals);
  RUN_TEST(test_ride_state_transitions_and_paused_time);
  RUN_TEST(test_scheduler_period_and_wrap);
  RUN_TEST(test_page_carousel_default_period_and_wrap);
  RUN_TEST(test_page_carousel_mask_order_and_fallback);
  RUN_TEST(test_page_carousel_pinned_page);
  RUN_TEST(test_display_power_dim_off_wake_disable_and_wrap);
  RUN_TEST(test_battery_raw_conversion_and_calibration);
  RUN_TEST(test_battery_soc_table_and_interpolation);
  RUN_TEST(test_battery_ema_monotonicity_and_usb_growth);
  RUN_TEST(test_battery_low_threshold_hysteresis);
  RUN_TEST(test_display_formatter_all_pages_and_battery);
  RUN_TEST(test_display_formatter_battery_and_value_limits);
  RUN_TEST(test_display_formatter_low_battery_warning);
  RUN_TEST(test_odometer_save_distance_boundaries_and_five_km_count);
  RUN_TEST(test_odometer_save_paused_settle_delay_and_cancel);
  RUN_TEST(test_odometer_save_display_off_deep_sleep_and_force);
  RUN_TEST(test_odometer_save_critical_battery_once_and_usb_reboot);
  RUN_TEST(test_odometer_save_unchanged_skips_sequence_growth);
  RUN_TEST(test_odometer_save_flash_error_keeps_distance_retry);
  return UNITY_END();
}
