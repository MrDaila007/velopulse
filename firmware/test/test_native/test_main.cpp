#include <unity.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "ambient_light_model.h"
#include "battery_model.h"
#include "ble_advertising.h"
#include "ble_command.h"
#include "ble_config_write.h"
#include "ble_device_info.h"
#include "ble_identity.h"
#include "ble_protocol.h"
#include "ble_telemetry.h"
#include "boot_counter.h"
#include "config_codec.h"
#include "config_validator.h"
#include "crc32.h"
#include "display_burn_in.h"
#include "diagnostics.h"
#include "error_log.h"
#include "display_formatter.h"
#include "display_power.h"
#include "odometer_save_policy.h"
#include "page_carousel.h"
#include "protocol_codec.h"
#include "pulse_filter.h"
#include "ride_state.h"
#include "scheduler.h"
#include "serial_console.h"
#include "speed_calculator.h"
#include "storage_manager.h"
#include "storage_migration.h"
#include "trip_computer.h"

using namespace bike;

void setUp() {}
void tearDown() {}

void test_serial_console_parses_supported_commands_and_crlf() {
  SerialCommandParser parser;
  const char* commands =
      "open-pairing\r\ndump-config\nreset-odo\rselftest\n";
  const SerialCommand expected[] = {
      SerialCommand::kOpenPairing,
      SerialCommand::kDumpConfig,
      SerialCommand::kResetOdometer,
      SerialCommand::kSelftest,
  };
  size_t found = 0;
  for (size_t i = 0; commands[i] != '\0'; ++i) {
    const SerialCommand command = parser.feed(commands[i]);
    if (command != SerialCommand::kNone) {
      TEST_ASSERT_LESS_THAN(sizeof(expected) / sizeof(expected[0]), found);
      TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(expected[found]),
                              static_cast<uint8_t>(command));
      ++found;
    }
  }
  TEST_ASSERT_EQUAL(sizeof(expected) / sizeof(expected[0]), found);
}

void test_serial_console_trims_rejects_and_recovers_after_overflow() {
  SerialCommandParser parser;
  const char* padded = " \tselftest \t\n";
  SerialCommand result = SerialCommand::kNone;
  for (size_t i = 0; padded[i] != '\0'; ++i) {
    result = parser.feed(padded[i]);
  }
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SerialCommand::kSelftest),
                          static_cast<uint8_t>(result));

  const char* overflow = "this-command-is-far-too-long\n";
  for (size_t i = 0; overflow[i] != '\0'; ++i) {
    result = parser.feed(overflow[i]);
  }
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SerialCommand::kUnknown),
                          static_cast<uint8_t>(result));

  const char* recovered = "dump-config\n";
  for (size_t i = 0; recovered[i] != '\0'; ++i) {
    result = parser.feed(recovered[i]);
  }
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SerialCommand::kDumpConfig),
                          static_cast<uint8_t>(result));
}

void test_serial_console_parses_ambient_commands() {
  SerialCommandParser parser;
  const char* commands = "ambient-raw\nambient-stop\n";
  const SerialCommand expected[] = {
      SerialCommand::kAmbientRaw,
      SerialCommand::kAmbientStop,
  };
  size_t found = 0;
  for (size_t i = 0; commands[i] != '\0'; ++i) {
    const SerialCommand command = parser.feed(commands[i]);
    if (command != SerialCommand::kNone) {
      TEST_ASSERT_LESS_THAN(sizeof(expected) / sizeof(expected[0]), found);
      TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(expected[found]),
                              static_cast<uint8_t>(command));
      ++found;
    }
  }
  TEST_ASSERT_EQUAL(sizeof(expected) / sizeof(expected[0]), found);
}

void test_serial_console_parses_display_commands() {
  SerialCommandParser parser;
  const char* commands = "display-state\nwake-display\n";
  const SerialCommand expected[] = {
      SerialCommand::kDisplayState,
      SerialCommand::kWakeDisplay,
  };
  size_t found = 0;
  for (size_t i = 0; commands[i] != '\0'; ++i) {
    const SerialCommand command = parser.feed(commands[i]);
    if (command != SerialCommand::kNone) {
      TEST_ASSERT_LESS_THAN(sizeof(expected) / sizeof(expected[0]), found);
      TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(expected[found]),
                              static_cast<uint8_t>(command));
      ++found;
    }
  }
  TEST_ASSERT_EQUAL(sizeof(expected) / sizeof(expected[0]), found);
}

void test_error_log_keeps_16_and_snapshots_newest_four_in_order() {
  ErrorLogBuffer log;
  ErrorLogPacket packet = {};
  TEST_ASSERT_FALSE(log.snapshot(packet));

  for (uint32_t i = 0; i < 18; ++i) {
    log.append(i, ErrorLogCode::kFlashError,
               i % 2 == 0 ? ErrorLogSeverity::kWarn
                          : ErrorLogSeverity::kError,
               static_cast<uint16_t>(100u + i));
  }

  TEST_ASSERT_EQUAL_UINT32(16u, log.size());
  TEST_ASSERT_TRUE(log.snapshot(packet));
  TEST_ASSERT_EQUAL_UINT8(kBleStructVersion, packet.struct_version);
  TEST_ASSERT_EQUAL_UINT8(4u, packet.entry_count);
  for (uint32_t i = 0; i < 4; ++i) {
    TEST_ASSERT_EQUAL_UINT32(14u + i, packet.entries[i].uptime_s);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ErrorLogCode::kFlashError),
                            packet.entries[i].code);
    TEST_ASSERT_EQUAL_UINT16(114u + i, packet.entries[i].detail);
  }

  log.clear();
  TEST_ASSERT_FALSE(log.snapshot(packet));
}

void test_ble_advertising_policy_timeout_and_movement_restart() {
  TEST_ASSERT_EQUAL_UINT16(30u, kAdvertisingFastIntervalMs);
  TEST_ASSERT_EQUAL_UINT16(1000u, kAdvertisingSlowIntervalMs);
  TEST_ASSERT_EQUAL_UINT16(30u, kAdvertisingFastTimeoutS);
  TEST_ASSERT_EQUAL_UINT16(300u, advertisingTimeoutS(false));
  TEST_ASSERT_EQUAL_UINT16(0u, advertisingTimeoutS(true));

  TEST_ASSERT_TRUE(shouldRestartAdvertisingOnMovement(false, false));
  TEST_ASSERT_FALSE(shouldRestartAdvertisingOnMovement(true, false));
  TEST_ASSERT_FALSE(shouldRestartAdvertisingOnMovement(false, true));
  TEST_ASSERT_FALSE(shouldRestartAdvertisingOnMovement(true, true));
}

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
                     uint32_t sequence,
                     uint16_t version = kConfigRecordVersion) {
  uint8_t payload[kDeviceConfigPayloadSize];
  uint8_t record[kMaximumRecordSize];
  encodeDeviceConfig(config, payload);
  const size_t length = encodeRecord(payload, sizeof(payload), version,
                                     sequence, record, sizeof(record));
  backend.write(path, record, length);
}

void putOdometerRecord(MemoryStorageBackend& backend,
                       const char* path,
                       const OdometerData& odometer,
                       uint32_t sequence,
                       uint16_t version = kOdometerRecordVersion) {
  uint8_t payload[kOdometerPayloadSize];
  uint8_t record[kMaximumRecordSize];
  encodeOdometer(odometer, payload);
  const size_t length = encodeRecord(payload, sizeof(payload), version,
                                     sequence, record, sizeof(record));
  backend.write(path, record, length);
}

#ifndef PROTOCOL_FIXTURES_DIR
#define PROTOCOL_FIXTURES_DIR "../protocol/fixtures"
#endif

std::string fixturePath(const char* name, const char* ext) {
  std::ostringstream path;
  path << PROTOCOL_FIXTURES_DIR << "/" << name << "." << ext;
  return path.str();
}

bool loadFixtureText(const char* name, const char* ext, std::string& text) {
  std::ifstream input(fixturePath(name, ext).c_str());
  if (!input) {
    return false;
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  text = buffer.str();
  return true;
}

int hexNibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

bool loadFixtureHex(const char* name, std::vector<uint8_t>& bytes) {
  std::string text;
  if (!loadFixtureText(name, "hex", text)) {
    return false;
  }
  bytes.clear();
  int high = -1;
  for (char c : text) {
    if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
      continue;
    }
    const int nibble = hexNibble(c);
    if (nibble < 0) {
      return false;
    }
    if (high < 0) {
      high = nibble;
    } else {
      bytes.push_back(static_cast<uint8_t>((high << 4) | nibble));
      high = -1;
    }
  }
  return high < 0 && !bytes.empty();
}

bool jsonHasNumber(const std::string& json, const char* key, long long expected) {
  const std::string needle = std::string("\"") + key + "\": ";
  const size_t pos = json.find(needle);
  if (pos == std::string::npos) {
    return false;
  }
  size_t i = pos + needle.size();
  while (i < json.size() && (json[i] == ' ' || json[i] == '\t')) {
    ++i;
  }
  char* end = nullptr;
  const long long value = strtoll(json.c_str() + i, &end, 10);
  return end != json.c_str() + i && value == expected;
}

bool jsonHasString(const std::string& json, const char* key, const char* expected) {
  const std::string needle =
      std::string("\"") + key + "\": \"" + expected + "\"";
  return json.find(needle) != std::string::npos;
}

DeviceInfoPacket makeNominalDeviceInfo() {
  DeviceInfoPacket info = {};
  info.struct_version = kBleStructVersion;
  info.proto_major = kBleProtoMajor;
  info.proto_minor = kBleProtoMinor;
  info.hw_revision = 1;
  memcpy(info.model, "BIKECOMP-XIAO", 13);
  memcpy(info.fw_version, "1.0.0", 5);
  const uint8_t serial[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  memcpy(info.serial, serial, 8);
  info.uptime_s = 3600;
  info.reset_reason = static_cast<uint8_t>(ResetReason::kPowerOn);
  info.boot_count = 42;
  info.flags = kDeviceInfoFlagConfigValid | kDeviceInfoFlagDisplayOk |
               kDeviceInfoFlagFsOk;
  return info;
}

TelemetryPacket makeMovingTelemetry() {
  TelemetryPacket telemetry = {};
  telemetry.struct_version = kBleStructVersion;
  telemetry.flags = kTelemetryFlagMoving | kTelemetryFlagDisplayOn |
                    kTelemetryFlagSmoothingEnabled;
  telemetry.speed_x100 = 2550;
  telemetry.avg_speed_x100 = 2200;
  telemetry.max_speed_x100 = 3500;
  telemetry.trip_distance_cm = 125000;
  telemetry.moving_time_s = 1800;
  telemetry.odometer_m = 123456;
  telemetry.battery_mv = 3900;
  telemetry.battery_pct = 75;
  telemetry.ride_state = static_cast<uint8_t>(RideState::kMoving);
  telemetry.revolutions = 500;
  telemetry.last_pulse_age_ms = 250;
  telemetry.seq = 42;
  telemetry.sensor_state = static_cast<uint8_t>(SensorState::kOk);
  telemetry.power_state = static_cast<uint8_t>(PowerState::kActive);
  return telemetry;
}

TelemetryPacket makePausedTelemetry() {
  TelemetryPacket telemetry = makeMovingTelemetry();
  telemetry.flags = kTelemetryFlagDisplayOn | kTelemetryFlagSmoothingEnabled;
  telemetry.speed_x100 = 0;
  telemetry.ride_state = static_cast<uint8_t>(RideState::kPaused);
  telemetry.last_pulse_age_ms = 5000;
  telemetry.seq = 43;
  telemetry.sensor_state = static_cast<uint8_t>(SensorState::kIdle);
  telemetry.power_state = static_cast<uint8_t>(PowerState::kShortStop);
  return telemetry;
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

void test_migrate_config_v1_to_v2_preserves_fields() {
  DeviceConfig original;
  original.brightness_pct = 55;
  original.wheel_circumference_mm = 2155;
  original.odometer_save_interval_m = 250;
  memcpy(original.device_name, "BikeComp-V1", 12);

  uint8_t payload_v1[kDeviceConfigPayloadSize];
  encodeDeviceConfig(original, payload_v1);

  DeviceConfig migrated;
  TEST_ASSERT_TRUE(migrateConfigV1ToV2(payload_v1, sizeof(payload_v1), migrated));
  TEST_ASSERT_TRUE(deviceConfigsEqual(original, migrated));
  TEST_ASSERT_TRUE(migrateConfigToCurrent(kConfigRecordVersionV1, payload_v1,
                                          sizeof(payload_v1), migrated));
  TEST_ASSERT_FALSE(migrateConfigToCurrent(99, payload_v1, sizeof(payload_v1),
                                           migrated));
  TEST_ASSERT_EQUAL_UINT32(kDeviceConfigPayloadSize,
                           configPayloadLengthForVersion(kConfigRecordVersionV1));
  TEST_ASSERT_EQUAL_UINT32(0u, configPayloadLengthForVersion(99));
}

void test_migrate_odometer_v1_to_v2_preserves_totals() {
  OdometerData original{9876543210ull, 123456789ull};
  uint8_t payload_v1[kOdometerPayloadSize];
  encodeOdometer(original, payload_v1);

  OdometerData migrated;
  TEST_ASSERT_TRUE(
      migrateOdometerV1ToV2(payload_v1, sizeof(payload_v1), migrated));
  TEST_ASSERT_EQUAL_UINT64(original.odometer_mm, migrated.odometer_mm);
  TEST_ASSERT_EQUAL_UINT64(original.total_revolutions,
                           migrated.total_revolutions);
  TEST_ASSERT_TRUE(migrateOdometerToCurrent(kOdometerRecordVersionV1, payload_v1,
                                            sizeof(payload_v1), migrated));
  TEST_ASSERT_FALSE(migrateOdometerToCurrent(99, payload_v1, sizeof(payload_v1),
                                             migrated));
}

void test_storage_migrates_config_v1_fixture_and_rewrites_v2() {
  MemoryStorageBackend backend;
  DeviceConfig v1_config;
  v1_config.brightness_pct = 44;
  v1_config.low_battery_pct = 15;
  putConfigRecord(backend, "/cfg_a", v1_config, 7, kConfigRecordVersionV1);

  StorageManager storage(backend);
  TEST_ASSERT_TRUE(storage.begin());
  DeviceConfig loaded;
  StorageLoadInfo info;
  TEST_ASSERT_TRUE(storage.loadConfig(loaded, info));
  TEST_ASSERT_EQUAL(StorageSource::kSlotA, info.source);
  TEST_ASSERT_TRUE(info.migrated);
  TEST_ASSERT_TRUE(info.migration_written);
  TEST_ASSERT_EQUAL_UINT16(kConfigRecordVersionV1, info.from_version);
  TEST_ASSERT_EQUAL_UINT32(8u, info.sequence);
  TEST_ASSERT_EQUAL_UINT8(44u, loaded.brightness_pct);
  TEST_ASSERT_EQUAL_UINT8(15u, loaded.low_battery_pct);
  TEST_ASSERT_EQUAL_UINT32(1u, storage.counters().config_migrations);

  uint8_t record[kMaximumRecordSize];
  size_t length = 0;
  TEST_ASSERT_EQUAL(StorageIoResult::kOk,
                    backend.read("/cfg_b", record, sizeof(record), length));
  DecodedRecord decoded;
  TEST_ASSERT_TRUE(inspectRecord(record, length, decoded));
  TEST_ASSERT_EQUAL_UINT16(kConfigRecordVersion, decoded.header.version);
  TEST_ASSERT_EQUAL_UINT32(8u, decoded.header.sequence);

  StorageManager next_boot(backend);
  TEST_ASSERT_TRUE(next_boot.begin());
  TEST_ASSERT_TRUE(next_boot.loadConfig(loaded, info));
  TEST_ASSERT_FALSE(info.migrated);
  TEST_ASSERT_EQUAL_UINT16(kConfigRecordVersion, info.from_version);
  TEST_ASSERT_EQUAL_UINT32(8u, info.sequence);
  TEST_ASSERT_EQUAL_UINT32(0u, next_boot.counters().config_migrations);
}

void test_storage_migrates_odometer_v1_and_rejects_future_version() {
  MemoryStorageBackend backend;
  OdometerData v1_odo{555000u, 42u};
  putOdometerRecord(backend, "/odo_a", v1_odo, 3, kOdometerRecordVersionV1);

  StorageManager storage(backend);
  TEST_ASSERT_TRUE(storage.begin());
  OdometerData loaded;
  StorageLoadInfo info;
  TEST_ASSERT_TRUE(storage.loadOdometer(loaded, info));
  TEST_ASSERT_TRUE(info.migrated);
  TEST_ASSERT_TRUE(info.migration_written);
  TEST_ASSERT_EQUAL_UINT32(4u, info.sequence);
  TEST_ASSERT_EQUAL_UINT64(555000u, loaded.odometer_mm);
  TEST_ASSERT_EQUAL_UINT64(42u, loaded.total_revolutions);
  TEST_ASSERT_EQUAL_UINT32(1u, storage.counters().odometer_migrations);
  TEST_ASSERT_EQUAL_UINT32(4u, storage.lastOdometerSequence());

  // Future record version must be ignored; fall back to migrated v2 slot.
  OdometerData future{999u, 1u};
  putOdometerRecord(backend, "/odo_a", future, 99, 3);
  StorageManager reload(backend);
  TEST_ASSERT_TRUE(reload.begin());
  TEST_ASSERT_TRUE(reload.loadOdometer(loaded, info));
  TEST_ASSERT_EQUAL(StorageSource::kSlotB, info.source);
  TEST_ASSERT_TRUE(info.recovered);
  TEST_ASSERT_EQUAL_UINT64(555000u, loaded.odometer_mm);
  TEST_ASSERT_FALSE(info.migrated);
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
  power.configure(60, 1000);
  TEST_ASSERT_TRUE(power.forceOff(5000));
  TEST_ASSERT_EQUAL(DisplayPowerState::kOff, power.state());
  TEST_ASSERT_FALSE(power.update(5001));
  TEST_ASSERT_TRUE(power.noteActivity(5002));

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

void test_display_burn_in_guard_cycles_catches_up_and_wraps() {
  DisplayBurnInGuard guard;
  guard.configure(1000u);
  TEST_ASSERT_EQUAL_UINT8(0u, guard.phase());
  TEST_ASSERT_EQUAL_INT8(0, guard.xOffset());
  TEST_ASSERT_EQUAL_INT8(0, guard.yOffset());
  TEST_ASSERT_FALSE(guard.update(60999u));

  TEST_ASSERT_TRUE(guard.update(61000u));
  TEST_ASSERT_EQUAL_UINT8(1u, guard.phase());
  TEST_ASSERT_EQUAL_INT8(1, guard.xOffset());
  TEST_ASSERT_EQUAL_INT8(0, guard.yOffset());

  TEST_ASSERT_TRUE(guard.update(121000u));
  TEST_ASSERT_EQUAL_UINT8(2u, guard.phase());
  TEST_ASSERT_EQUAL_INT8(1, guard.xOffset());
  TEST_ASSERT_EQUAL_INT8(1, guard.yOffset());

  TEST_ASSERT_TRUE(guard.update(181000u));
  TEST_ASSERT_EQUAL_UINT8(3u, guard.phase());
  TEST_ASSERT_EQUAL_INT8(0, guard.xOffset());
  TEST_ASSERT_EQUAL_INT8(1, guard.yOffset());

  TEST_ASSERT_TRUE(guard.update(241000u));
  TEST_ASSERT_EQUAL_UINT8(0u, guard.phase());
  TEST_ASSERT_EQUAL_INT8(0, guard.xOffset());
  TEST_ASSERT_EQUAL_INT8(0, guard.yOffset());

  guard.configure(1000u);
  TEST_ASSERT_TRUE(guard.update(181000u));
  TEST_ASSERT_EQUAL_UINT8(3u, guard.phase());

  guard.configure(0xFFFFFF00u);
  TEST_ASSERT_TRUE(guard.update(59744u));
  TEST_ASSERT_EQUAL_UINT8(1u, guard.phase());
}

void test_ambient_light_model_normalizes_levels_caps_and_contrast() {
  TEST_ASSERT_EQUAL_UINT16(0u, AmbientLightModel::normalize(50u, 100u, 3900u));
  TEST_ASSERT_EQUAL_UINT16(0u, AmbientLightModel::normalize(100u, 100u, 3900u));
  TEST_ASSERT_EQUAL_UINT16(500u,
                           AmbientLightModel::normalize(2000u, 100u, 3900u));
  TEST_ASSERT_EQUAL_UINT16(1000u,
                           AmbientLightModel::normalize(3900u, 100u, 3900u));
  TEST_ASSERT_EQUAL_UINT16(0u, AmbientLightModel::normalize(100u, 500u, 500u));

  struct LevelCase {
    uint16_t raw;
    uint8_t brightness;
  };
  const LevelCase cases[] = {
      {100u, 5u}, {860u, 15u}, {1810u, 35u},
      {2760u, 65u}, {3520u, 100u},
  };
  for (const LevelCase& test : cases) {
    AmbientLightModel model;
    model.configure(100u, 3900u, 0u);
    TEST_ASSERT_TRUE(model.addSample(test.raw, 0u));
    TEST_ASSERT_TRUE(model.snapshot().valid);
    TEST_ASSERT_EQUAL_UINT8(test.brightness, model.snapshot().brightness_pct);
  }

  TEST_ASSERT_EQUAL_UINT8(10u,
                          AmbientLightModel::contrastForBrightness(1u));
  TEST_ASSERT_EQUAL_UINT8(156u,
                          AmbientLightModel::contrastForBrightness(60u));
  TEST_ASSERT_EQUAL_UINT8(255u,
                          AmbientLightModel::contrastForBrightness(100u));

  AmbientLightModel capped;
  capped.configure(100u, 3900u, 0u);
  TEST_ASSERT_EQUAL_UINT8(60u, capped.cappedBrightness(60u));
  TEST_ASSERT_TRUE(capped.addSample(3520u, 0u));
  TEST_ASSERT_EQUAL_UINT8(60u, capped.cappedBrightness(60u));
  TEST_ASSERT_FALSE(capped.addSample(100u, 1u));
  TEST_ASSERT_EQUAL_UINT8(60u, capped.cappedBrightness(60u));
}

void test_ambient_light_model_ema_hysteresis_dwell_and_invalid_fallback() {
  AmbientLightModel model;
  model.configure(100u, 3900u, 0u);
  TEST_ASSERT_TRUE(model.addSample(100u, 0u));
  TEST_ASSERT_EQUAL_UINT8(5u, model.snapshot().brightness_pct);

  for (uint8_t i = 1u; i <= 7u; ++i) {
    model.addSample(3900u, static_cast<uint32_t>(i) * 250u);
  }
  TEST_ASSERT_EQUAL_UINT8(5u, model.snapshot().brightness_pct);
  TEST_ASSERT_TRUE(model.addSample(3900u, 2000u));
  TEST_ASSERT_EQUAL_UINT8(65u, model.snapshot().brightness_pct);

  for (uint8_t i = 1u; i <= 7u; ++i) {
    model.addSample(100u, 2000u + static_cast<uint32_t>(i) * 250u);
  }
  TEST_ASSERT_EQUAL_UINT8(65u, model.snapshot().brightness_pct);
  TEST_ASSERT_TRUE(model.addSample(100u, 4000u));
  TEST_ASSERT_EQUAL_UINT8(15u, model.snapshot().brightness_pct);

  TEST_ASSERT_TRUE(model.addSample(0u, 4250u));
  TEST_ASSERT_FALSE(model.snapshot().valid);
  TEST_ASSERT_EQUAL_UINT8(42u, model.cappedBrightness(42u));
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

void test_odometer_save_flash_error_keeps_oneshot_pending() {
  // Mirrors AppController: acknowledge only after Flash success, otherwise
  // one-shot triggers (display-off / critical) must remain pending for retry.
  OdometerSavePolicy policy;
  policy.configure(500);
  policy.markSaved(0);

  policy.noteDisplayPower(DisplayPowerState::kBright);
  policy.noteDisplayPower(DisplayPowerState::kOff);
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kDisplayOff, policy.evaluate(0, 0));
  // Flash failed → no acknowledge.
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kDisplayOff, policy.evaluate(0, 0));
  policy.acknowledge(OdometerSaveTrigger::kDisplayOff);
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kNone, policy.evaluate(0, 0));

  policy.noteBatteryPercent(5, true);
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kCriticalBattery, policy.evaluate(0, 0));
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kCriticalBattery, policy.evaluate(0, 0));
  policy.acknowledge(OdometerSaveTrigger::kCriticalBattery);
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kNone, policy.evaluate(0, 0));
}

void test_ble_identity_resolves_placeholder_name() {
  TEST_ASSERT_TRUE(isDeviceNamePlaceholder(nullptr));
  TEST_ASSERT_TRUE(isDeviceNamePlaceholder(""));
  TEST_ASSERT_TRUE(isDeviceNamePlaceholder(kDeviceNamePlaceholder));
  TEST_ASSERT_FALSE(isDeviceNamePlaceholder("BikeComp-V1"));

  char name[16];
  formatDeviceNameWithSerial(0x0A3F, name, sizeof(name));
  TEST_ASSERT_EQUAL_STRING("BikeComp-0A3F", name);

  resolveDeviceLocalName(kDeviceNamePlaceholder, 0xBEEF, name, sizeof(name));
  TEST_ASSERT_EQUAL_STRING("BikeComp-BEEF", name);
  resolveDeviceLocalName("MyBike-01", 0xBEEF, name, sizeof(name));
  TEST_ASSERT_EQUAL_STRING("MyBike-01", name);
}

void test_map_nrf_reset_reason_priority() {
  TEST_ASSERT_EQUAL(ResetReason::kPowerOn, mapNrfResetReason(0u));
  TEST_ASSERT_EQUAL(ResetReason::kWatchdog, mapNrfResetReason(kNrfResetReasonDog));
  TEST_ASSERT_EQUAL(ResetReason::kLockup, mapNrfResetReason(kNrfResetReasonLockup));
  TEST_ASSERT_EQUAL(ResetReason::kSoftReset, mapNrfResetReason(kNrfResetReasonSreq));
  TEST_ASSERT_EQUAL(ResetReason::kPinReset, mapNrfResetReason(kNrfResetReasonPin));
  TEST_ASSERT_EQUAL(ResetReason::kWakeFromSleep,
                    mapNrfResetReason(kNrfResetReasonOff));
  TEST_ASSERT_EQUAL(ResetReason::kWakeFromSleep,
                    mapNrfResetReason(kNrfResetReasonVbus));
  // Watchdog wins over soft+pin when several bits stick.
  TEST_ASSERT_EQUAL(ResetReason::kWatchdog,
                    mapNrfResetReason(kNrfResetReasonDog | kNrfResetReasonSreq |
                                      kNrfResetReasonPin));
  TEST_ASSERT_EQUAL(ResetReason::kUnknown, mapNrfResetReason(1u << 31));
}

void test_pairing_window_and_device_info_flags() {
  TEST_ASSERT_TRUE(isPairingWindowOpen(0, 1000, kDefaultPairingWindowMs, false));
  TEST_ASSERT_FALSE(
      isPairingWindowOpen(0, kDefaultPairingWindowMs, kDefaultPairingWindowMs, false));
  TEST_ASSERT_TRUE(
      isPairingWindowOpen(0, kDefaultPairingWindowMs + 1u, kDefaultPairingWindowMs, true));

  TEST_ASSERT_FALSE(shouldRejectPairingRequest(
      0, 1000, kDefaultPairingWindowMs, false, false));
  TEST_ASSERT_FALSE(shouldRejectPairingRequest(
      0, kDefaultPairingWindowMs, kDefaultPairingWindowMs, true, false));
  TEST_ASSERT_FALSE(shouldRejectPairingRequest(
      0, kDefaultPairingWindowMs, kDefaultPairingWindowMs, false, true));
  TEST_ASSERT_TRUE(shouldRejectPairingRequest(
      0, kDefaultPairingWindowMs, kDefaultPairingWindowMs, false, false));

  const uint8_t flags = buildDeviceInfoFlags(
      true, true, true, true, true, true, false);
  TEST_ASSERT_EQUAL_UINT8(
      kDeviceInfoFlagConfigValid | kDeviceInfoFlagDisplayOk | kDeviceInfoFlagFsOk |
          kDeviceInfoFlagBonded | kDeviceInfoFlagPairingWindowOpen |
          kDeviceInfoFlagUsbConnected,
      flags);

  DeviceInfoPacket info = {};
  info.boot_count = 7;
  info.reset_reason = static_cast<uint8_t>(ResetReason::kSoftReset);
  refreshDeviceInfoLiveFields(info, /*boot_ms=*/1000, /*now_ms=*/65000,
                              /*pairing_started_ms=*/1000,
                              kDefaultPairingWindowMs, /*open_pairing_always=*/false,
                              /*bonded=*/true, /*usb=*/false, /*config=*/true,
                              /*display=*/true, /*fs=*/true, /*deep_sleep=*/false);
  TEST_ASSERT_EQUAL_UINT32(64u, info.uptime_s);
  TEST_ASSERT_TRUE((info.flags & kDeviceInfoFlagBonded) != 0);
  TEST_ASSERT_TRUE((info.flags & kDeviceInfoFlagPairingWindowOpen) != 0);
  TEST_ASSERT_TRUE((info.flags & kDeviceInfoFlagUsbConnected) == 0);

  refreshDeviceInfoLiveFields(info, /*boot_ms=*/1000, /*now_ms=*/400000,
                              /*pairing_started_ms=*/399000,
                              kDefaultPairingWindowMs, /*open_pairing_always=*/false,
                              /*bonded=*/true, /*usb=*/false, /*config=*/true,
                              /*display=*/true, /*fs=*/true, /*deep_sleep=*/false);
  TEST_ASSERT_EQUAL_UINT32(399u, info.uptime_s);
  TEST_ASSERT_TRUE((info.flags & kDeviceInfoFlagPairingWindowOpen) != 0);
}

void test_boot_count_increments_and_persists() {
  MemoryStorageBackend backend;
  TEST_ASSERT_EQUAL_UINT16(1u, loadAndIncrementBootCount(backend));
  TEST_ASSERT_EQUAL_UINT16(2u, loadAndIncrementBootCount(backend));
  TEST_ASSERT_EQUAL_UINT16(3u, loadAndIncrementBootCount(backend));

  const auto found = backend.files.find(kBootCountPath);
  TEST_ASSERT_TRUE(found != backend.files.end());
  TEST_ASSERT_EQUAL_UINT32(2u, found->second.size());
  TEST_ASSERT_EQUAL_UINT8(3u, found->second[0]);
  TEST_ASSERT_EQUAL_UINT8(0u, found->second[1]);

  // Corrupt / short payload restarts from 1.
  backend.files[kBootCountPath] = {0x01};
  TEST_ASSERT_EQUAL_UINT16(1u, loadAndIncrementBootCount(backend));

  // Saturate at UINT16_MAX.
  backend.files[kBootCountPath] = {0xFF, 0xFF};
  TEST_ASSERT_EQUAL_UINT16(0xFFFFu, loadAndIncrementBootCount(backend));
}

void test_telemetry_publish_mode_and_intervals() {
  TEST_ASSERT_EQUAL_UINT32(kTelemetryIntervalUnsubscribedMs,
                           telemetryPublishIntervalMs(TelemetryPublishMode::kUnsubscribed));
  TEST_ASSERT_EQUAL_UINT32(kTelemetryIntervalSubscribedMs,
                           telemetryPublishIntervalMs(TelemetryPublishMode::kSubscribed));
  TEST_ASSERT_EQUAL_UINT32(kTelemetryIntervalSensorTestMs,
                           telemetryPublishIntervalMs(TelemetryPublishMode::kSensorTest));

  TEST_ASSERT_EQUAL(TelemetryPublishMode::kUnsubscribed,
                    selectTelemetryPublishMode(false, false));
  TEST_ASSERT_EQUAL(TelemetryPublishMode::kSubscribed,
                    selectTelemetryPublishMode(true, false));
  TEST_ASSERT_EQUAL(TelemetryPublishMode::kSensorTest,
                    selectTelemetryPublishMode(true, true));
  TEST_ASSERT_EQUAL(TelemetryPublishMode::kSensorTest,
                    selectTelemetryPublishMode(false, true));

  TEST_ASSERT_TRUE(telemetryDue(0, 0, TelemetryPublishMode::kSubscribed, false));
  TEST_ASSERT_FALSE(
      telemetryDue(1000, 1999, TelemetryPublishMode::kSubscribed, true));
  TEST_ASSERT_TRUE(
      telemetryDue(1000, 2000, TelemetryPublishMode::kSubscribed, true));
  TEST_ASSERT_FALSE(
      telemetryDue(0, 4999, TelemetryPublishMode::kUnsubscribed, true));
  TEST_ASSERT_TRUE(
      telemetryDue(0, 5000, TelemetryPublishMode::kUnsubscribed, true));
  TEST_ASSERT_TRUE(
      telemetryDue(0, 200, TelemetryPublishMode::kSensorTest, true));
  TEST_ASSERT_EQUAL_UINT16(1u, nextTelemetrySeq(0));
  TEST_ASSERT_EQUAL_UINT16(0u, nextTelemetrySeq(0xFFFF));
}

void test_fill_telemetry_packet_moving_and_idle() {
  TelemetryBuildInput input;
  input.trip.speed_x100 = 2550;
  input.trip.average_speed_x100 = 2200;
  input.trip.max_speed_x100 = 3500;
  input.trip.trip_distance_mm = 1250000;
  input.trip.moving_time_ms = 1800000;
  input.trip.odometer_mm = 123456000ull;
  input.trip.revolutions = 500;
  input.trip.ride_state = RideState::kMoving;
  input.battery.millivolts = 3900;
  input.battery.percent = 75;
  input.battery.usb_present = true;
  input.battery.charge_status = ChargeStatus::kNotCharging;
  input.display_on = true;
  input.smoothing_enabled = true;
  input.had_pulse = true;
  input.last_pulse_ms = 1000;
  input.now_ms = 1250;

  TelemetryPacket packet = {};
  fillTelemetryPacket(packet, input);
  packet.seq = 42;
  TEST_ASSERT_EQUAL_UINT8(kBleStructVersion, packet.struct_version);
  TEST_ASSERT_EQUAL_UINT8(
      kTelemetryFlagMoving | kTelemetryFlagDisplayOn | kTelemetryFlagUsbConnected |
          kTelemetryFlagSmoothingEnabled,
      packet.flags);
  TEST_ASSERT_EQUAL_UINT16(2550u, packet.speed_x100);
  TEST_ASSERT_EQUAL_UINT32(125000u, packet.trip_distance_cm);
  TEST_ASSERT_EQUAL_UINT32(1800u, packet.moving_time_s);
  TEST_ASSERT_EQUAL_UINT32(123456u, packet.odometer_m);
  TEST_ASSERT_EQUAL_UINT16(3900u, packet.battery_mv);
  TEST_ASSERT_EQUAL_UINT8(75u, packet.battery_pct);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(RideState::kMoving),
                          packet.ride_state);
  TEST_ASSERT_EQUAL_UINT32(500u, packet.revolutions);
  TEST_ASSERT_EQUAL_UINT32(250u, packet.last_pulse_age_ms);
  TEST_ASSERT_EQUAL_UINT16(42u, packet.seq);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SensorState::kOk),
                          packet.sensor_state);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PowerState::kActive),
                          packet.power_state);

  input.trip.ride_state = RideState::kIdle;
  input.had_pulse = false;
  input.display_on = false;
  input.battery.usb_present = false;
  input.battery.charge_status = ChargeStatus::kUnknown;
  fillTelemetryPacket(packet, input);
  TEST_ASSERT_TRUE((packet.flags & kTelemetryFlagMoving) == 0);
  TEST_ASSERT_TRUE((packet.flags & kTelemetryFlagDisplayOn) == 0);
  TEST_ASSERT_TRUE((packet.flags & kTelemetryFlagChargeStatusUnknown) != 0);
  TEST_ASSERT_EQUAL_UINT32(kTelemetryNoPulseAgeMs, packet.last_pulse_age_ms);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SensorState::kNoSignal),
                          packet.sensor_state);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PowerState::kIdleDisplayOff),
                          packet.power_state);
}

void test_ride_state_tracks_last_pulse() {
  RideStateMachine ride(3000);
  TEST_ASSERT_FALSE(ride.hasPulse());
  ride.onPulse(1000);
  TEST_ASSERT_TRUE(ride.hasPulse());
  TEST_ASSERT_EQUAL_UINT32(1000u, ride.lastPulseMs());
  ride.reset(5000);
  TEST_ASSERT_FALSE(ride.hasPulse());
  TEST_ASSERT_EQUAL(RideState::kIdle, ride.state());
  ride.setStopTimeoutMs(5000);
}

void test_config_write_parse_valid_and_range_error() {
  std::vector<uint8_t> defaults_hex;
  TEST_ASSERT_TRUE(loadFixtureHex("config_v1_defaults", defaults_hex));

  ConfigWriteParseResult ok =
      parseConfigWritePayload(defaults_hex.data(), defaults_hex.size());
  TEST_ASSERT_TRUE(ok.ok);
  TEST_ASSERT_EQUAL(CommandStatus::kOk, ok.status);
  TEST_ASSERT_EQUAL_UINT16(kDefaultWheelCircumferenceMm,
                           ok.config.wheel_circumference_mm);

  defaults_hex[2] = 0xF3;
  defaults_hex[3] = 0x01;  // 499 mm, below minimum
  ConfigWriteParseResult bad =
      parseConfigWritePayload(defaults_hex.data(), defaults_hex.size());
  TEST_ASSERT_FALSE(bad.ok);
  TEST_ASSERT_EQUAL(CommandStatus::kErrRange, bad.status);
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(ConfigFieldId::kWheelCircumferenceMm), bad.field_id);
}

void test_config_write_pending_queue_single_slot() {
  std::vector<uint8_t> defaults_hex;
  TEST_ASSERT_TRUE(loadFixtureHex("config_v1_defaults", defaults_hex));

  PendingConfigWrite queue = {};
  ConfigWriteRejectReason reject = ConfigWriteRejectReason::kNone;
  TEST_ASSERT_TRUE(configWriteQueueStage(queue, defaults_hex.data(),
                                         defaults_hex.size(), reject));
  TEST_ASSERT_EQUAL(ConfigWriteQueueState::kPending, queue.state);

  reject = ConfigWriteRejectReason::kNone;
  TEST_ASSERT_FALSE(configWriteQueueStage(queue, defaults_hex.data(),
                                          defaults_hex.size(), reject));
  TEST_ASSERT_EQUAL(ConfigWriteRejectReason::kBusy, reject);

  uint8_t taken[kConfigurationSize] = {};
  TEST_ASSERT_TRUE(configWriteQueueDequeue(queue, taken));
  TEST_ASSERT_EQUAL_UINT8_ARRAY(defaults_hex.data(), taken, defaults_hex.size());
  configWriteQueueFinish(queue);
  TEST_ASSERT_EQUAL(ConfigWriteQueueState::kIdle, queue.state);
}

void test_config_validation_error_maps_to_field_id() {
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(ConfigFieldId::kWheelCircumferenceMm),
      configValidationErrorToFieldId(ConfigValidationError::kWheelCircumference));
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(ConfigFieldId::kDeviceName),
      configValidationErrorToFieldId(ConfigValidationError::kDeviceName));
}

void test_diagnostic_snapshot_maps_storage_and_pulse_counters() {
  StorageCounters storage{};
  storage.writes = 42u;
  storage.skipped_writes = 7u;
  PulseFilterCounters pulses{};
  pulses.rejected_debounce = 3u;
  pulses.rejected_overspeed = 5u;
  pulses.accepted = 100u;

  const DiagnosticSources sources = makeDiagnosticSources(
      storage, pulses, /*raw_pulse_count=*/1234u, /*isr_overflow=*/9u,
      /*free_heap_bytes=*/1600u, /*i2c_error_count=*/2u,
      static_cast<uint8_t>(kSelftestDisplayOk | kSelftestFsOk |
                           kSelftestConfigValid));
  const DiagnosticSnapshot snap = buildDiagnosticSnapshot(sources);

  TEST_ASSERT_EQUAL_UINT32(1234u, snap.raw_pulse_count);
  TEST_ASSERT_EQUAL_UINT16(3u, snap.rejected_debounce);
  TEST_ASSERT_EQUAL_UINT16(5u, snap.rejected_overspeed);
  TEST_ASSERT_EQUAL_UINT16(9u, snap.isr_overflow);
  TEST_ASSERT_EQUAL_UINT16(42u, snap.flash_write_count);
  TEST_ASSERT_EQUAL_UINT16(100u, snap.free_heap_units);
  TEST_ASSERT_EQUAL_UINT8(2u, snap.i2c_error_count);
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(kSelftestDisplayOk | kSelftestFsOk |
                           kSelftestConfigValid),
      snap.selftest_mask);
}

void test_diagnostic_payload_little_endian_layout() {
  DiagnosticSnapshot snap;
  snap.raw_pulse_count = 0x01020304u;
  snap.rejected_debounce = 0x0506u;
  snap.rejected_overspeed = 0x0708u;
  snap.isr_overflow = 0x090Au;
  snap.flash_write_count = 0x0B0Cu;
  snap.free_heap_units = 0x0D0Eu;
  snap.i2c_error_count = 0x0Fu;
  snap.selftest_mask = 0x1Fu;

  uint8_t payload[kDiagnosticPayloadSize];
  encodeDiagnosticPayload(snap, payload);

  const uint8_t expected[kDiagnosticPayloadSize] = {
      0x04, 0x03, 0x02, 0x01, 0x06, 0x05, 0x08, 0x07,
      0x0A, 0x09, 0x0C, 0x0B, 0x0E, 0x0D, 0x0F, 0x1F};
  TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, payload, kDiagnosticPayloadSize);
  TEST_ASSERT_EQUAL(16u, kDiagnosticPayloadSize);
}

void test_diagnostic_saturates_narrow_fields() {
  DiagnosticSources sources;
  sources.raw_pulse_count = 0xFFFFFFFFu;
  sources.rejected_debounce = 0x10000u;
  sources.rejected_overspeed = 0x12345u;
  sources.isr_overflow = 0xFFFFFFFFu;
  sources.flash_write_count = 0x10001u;
  sources.free_heap_bytes = 0xFFFFFFF0u;
  sources.i2c_error_count = 0xFFu;
  sources.selftest_mask = 0xAAu;

  const DiagnosticSnapshot snap = buildDiagnosticSnapshot(sources);
  TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFFu, snap.raw_pulse_count);
  TEST_ASSERT_EQUAL_UINT16(0xFFFFu, snap.rejected_debounce);
  TEST_ASSERT_EQUAL_UINT16(0xFFFFu, snap.rejected_overspeed);
  TEST_ASSERT_EQUAL_UINT16(0xFFFFu, snap.isr_overflow);
  TEST_ASSERT_EQUAL_UINT16(0xFFFFu, snap.flash_write_count);
  TEST_ASSERT_EQUAL_UINT16(0xFFFFu, snap.free_heap_units);
  TEST_ASSERT_EQUAL_UINT8(0xFFu, snap.i2c_error_count);
}

void test_ble_protocol_sizes_match_contract() {
  TEST_ASSERT_EQUAL_UINT32(48u, kDeviceInfoSize);
  TEST_ASSERT_EQUAL_UINT32(36u, kTelemetrySize);
  TEST_ASSERT_EQUAL_UINT32(48u, kConfigurationSize);
  TEST_ASSERT_EQUAL_UINT32(kDeviceConfigPayloadSize, kConfigurationSize);
  TEST_ASSERT_EQUAL_UINT32(20u, kCommandMaxSize);
  TEST_ASSERT_EQUAL_UINT32(25u, kCommandResultMaxSize);
  TEST_ASSERT_EQUAL_UINT32(sizeof(DeviceInfoPacket), kDeviceInfoSize);
  TEST_ASSERT_EQUAL_UINT32(sizeof(TelemetryPacket), kTelemetrySize);
  TEST_ASSERT_EQUAL_UINT32(sizeof(ConfigurationPacket), kConfigurationSize);
}

void test_protocol_fixture_device_info_v1_nominal() {
  std::vector<uint8_t> hex;
  std::string json;
  TEST_ASSERT_TRUE(loadFixtureHex("device_info_v1_nominal", hex));
  TEST_ASSERT_TRUE(loadFixtureText("device_info_v1_nominal", "json", json));
  TEST_ASSERT_EQUAL_UINT32(kDeviceInfoSize, hex.size());

  const DeviceInfoPacket expected = makeNominalDeviceInfo();
  uint8_t encoded[kDeviceInfoSize];
  encodeDeviceInfo(expected, encoded);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(hex.data(), encoded, kDeviceInfoSize);

  DeviceInfoPacket decoded = {};
  TEST_ASSERT_TRUE(decodeDeviceInfo(hex.data(), hex.size(), decoded));
  TEST_ASSERT_EQUAL_UINT8(expected.struct_version, decoded.struct_version);
  TEST_ASSERT_EQUAL_UINT8(expected.proto_major, decoded.proto_major);
  TEST_ASSERT_EQUAL_UINT8(expected.proto_minor, decoded.proto_minor);
  TEST_ASSERT_EQUAL_UINT8(expected.hw_revision, decoded.hw_revision);
  TEST_ASSERT_EQUAL_STRING_LEN("BIKECOMP-XIAO", decoded.model, 13);
  TEST_ASSERT_EQUAL_STRING_LEN("1.0.0", decoded.fw_version, 5);
  TEST_ASSERT_EQUAL_UINT32(3600u, decoded.uptime_s);
  TEST_ASSERT_EQUAL_UINT8(1u, decoded.reset_reason);
  TEST_ASSERT_EQUAL_UINT16(42u, decoded.boot_count);
  TEST_ASSERT_EQUAL_UINT8(7u, decoded.flags);
  TEST_ASSERT_TRUE(jsonHasNumber(json, "uptime_s", 3600));
  TEST_ASSERT_TRUE(jsonHasNumber(json, "boot_count", 42));
  TEST_ASSERT_TRUE(jsonHasString(json, "model", "BIKECOMP-XIAO"));
}

void test_protocol_fixture_telemetry_v1_moving_and_paused() {
  std::vector<uint8_t> moving_hex;
  std::vector<uint8_t> paused_hex;
  std::string moving_json;
  std::string paused_json;
  TEST_ASSERT_TRUE(loadFixtureHex("telemetry_v1_moving", moving_hex));
  TEST_ASSERT_TRUE(loadFixtureHex("telemetry_v1_paused", paused_hex));
  TEST_ASSERT_TRUE(loadFixtureText("telemetry_v1_moving", "json", moving_json));
  TEST_ASSERT_TRUE(loadFixtureText("telemetry_v1_paused", "json", paused_json));

  uint8_t encoded[kTelemetrySize];
  encodeTelemetry(makeMovingTelemetry(), encoded);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(moving_hex.data(), encoded, kTelemetrySize);
  encodeTelemetry(makePausedTelemetry(), encoded);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(paused_hex.data(), encoded, kTelemetrySize);

  TelemetryPacket decoded = {};
  TEST_ASSERT_TRUE(
      decodeTelemetry(moving_hex.data(), moving_hex.size(), decoded));
  TEST_ASSERT_EQUAL_UINT16(2550u, decoded.speed_x100);
  TEST_ASSERT_EQUAL_UINT8(1u, decoded.ride_state);
  TEST_ASSERT_EQUAL_UINT16(42u, decoded.seq);
  TEST_ASSERT_TRUE(jsonHasNumber(moving_json, "speed_x100", 2550));
  TEST_ASSERT_TRUE(jsonHasNumber(moving_json, "seq", 42));

  TEST_ASSERT_TRUE(
      decodeTelemetry(paused_hex.data(), paused_hex.size(), decoded));
  TEST_ASSERT_EQUAL_UINT16(0u, decoded.speed_x100);
  TEST_ASSERT_EQUAL_UINT8(2u, decoded.ride_state);
  TEST_ASSERT_EQUAL_UINT16(43u, decoded.seq);
  TEST_ASSERT_TRUE(jsonHasNumber(paused_json, "ride_state", 2));
  TEST_ASSERT_TRUE(jsonHasNumber(paused_json, "last_pulse_age_ms", 5000));
}

void test_protocol_fixture_config_v1_defaults_and_imperial() {
  std::vector<uint8_t> defaults_hex;
  std::vector<uint8_t> imperial_hex;
  std::string defaults_json;
  std::string imperial_json;
  TEST_ASSERT_TRUE(loadFixtureHex("config_v1_defaults", defaults_hex));
  TEST_ASSERT_TRUE(loadFixtureHex("config_v1_imperial", imperial_hex));
  TEST_ASSERT_TRUE(loadFixtureText("config_v1_defaults", "json", defaults_json));
  TEST_ASSERT_TRUE(loadFixtureText("config_v1_imperial", "json", imperial_json));

  DeviceConfig defaults;
  uint8_t encoded[kConfigurationSize];
  encodeConfiguration(defaults, encoded);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(defaults_hex.data(), encoded, kConfigurationSize);

  DeviceConfig imperial = defaults;
  imperial.units_imperial = true;
  encodeConfiguration(imperial, encoded);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(imperial_hex.data(), encoded, kConfigurationSize);

  DeviceConfig decoded;
  TEST_ASSERT_TRUE(
      decodeConfiguration(defaults_hex.data(), defaults_hex.size(), decoded));
  TEST_ASSERT_TRUE(deviceConfigsEqual(defaults, decoded));
  TEST_ASSERT_TRUE(
      decodeConfiguration(imperial_hex.data(), imperial_hex.size(), decoded));
  TEST_ASSERT_TRUE(decoded.units_imperial);
  TEST_ASSERT_TRUE(jsonHasNumber(defaults_json, "flags", 15));
  TEST_ASSERT_TRUE(jsonHasNumber(imperial_json, "flags", 31));
  TEST_ASSERT_TRUE(jsonHasString(defaults_json, "device_name", "BikeComp-XXXX"));
}

void test_protocol_fixture_commands_and_results() {
  std::vector<uint8_t> reset_hex;
  std::vector<uint8_t> odo_hex;
  std::vector<uint8_t> ok_hex;
  std::vector<uint8_t> err_hex;
  std::string reset_json;
  std::string odo_json;
  std::string ok_json;
  std::string err_json;
  TEST_ASSERT_TRUE(loadFixtureHex("command_reset_trip", reset_hex));
  TEST_ASSERT_TRUE(loadFixtureHex("command_reset_odo_with_token", odo_hex));
  TEST_ASSERT_TRUE(loadFixtureHex("result_ok", ok_hex));
  TEST_ASSERT_TRUE(loadFixtureHex("result_err_range_wheel", err_hex));
  TEST_ASSERT_TRUE(loadFixtureText("command_reset_trip", "json", reset_json));
  TEST_ASSERT_TRUE(
      loadFixtureText("command_reset_odo_with_token", "json", odo_json));
  TEST_ASSERT_TRUE(loadFixtureText("result_ok", "json", ok_json));
  TEST_ASSERT_TRUE(loadFixtureText("result_err_range_wheel", "json", err_json));

  CommandPacket reset_cmd = {};
  reset_cmd.struct_version = kBleStructVersion;
  reset_cmd.command_id = static_cast<uint8_t>(CommandId::kResetTrip);
  uint8_t encoded_cmd[kCommandMaxSize];
  TEST_ASSERT_EQUAL_UINT32(4u, encodeCommand(reset_cmd, encoded_cmd,
                                             sizeof(encoded_cmd)));
  TEST_ASSERT_EQUAL_UINT8_ARRAY(reset_hex.data(), encoded_cmd, 4);

  CommandPacket odo_cmd = {};
  odo_cmd.struct_version = kBleStructVersion;
  odo_cmd.command_id = static_cast<uint8_t>(CommandId::kResetOdometer);
  odo_cmd.flags = kCommandFlagHasToken;
  odo_cmd.payload_len = 4;
  odo_cmd.payload[0] = 0x78;
  odo_cmd.payload[1] = 0x56;
  odo_cmd.payload[2] = 0x34;
  odo_cmd.payload[3] = 0x12;
  TEST_ASSERT_EQUAL_UINT32(8u, encodeCommand(odo_cmd, encoded_cmd,
                                             sizeof(encoded_cmd)));
  TEST_ASSERT_EQUAL_UINT8_ARRAY(odo_hex.data(), encoded_cmd, 8);

  CommandPacket decoded_cmd = {};
  TEST_ASSERT_TRUE(decodeCommand(odo_hex.data(), odo_hex.size(), decoded_cmd));
  TEST_ASSERT_EQUAL_UINT8(0x20u, decoded_cmd.command_id);
  TEST_ASSERT_EQUAL_UINT8(4u, decoded_cmd.payload_len);
  TEST_ASSERT_TRUE(jsonHasNumber(reset_json, "command_id", 1));
  TEST_ASSERT_TRUE(jsonHasNumber(odo_json, "command_id", 32));
  TEST_ASSERT_TRUE(jsonHasNumber(odo_json, "token", 305419896));

  CommandResultPacket ok_result = {};
  ok_result.struct_version = kBleStructVersion;
  ok_result.command_id = static_cast<uint8_t>(CommandId::kResetTrip);
  ok_result.status = static_cast<uint8_t>(CommandStatus::kOk);
  uint8_t encoded_result[kCommandResultMaxSize];
  TEST_ASSERT_EQUAL_UINT32(
      9u, encodeCommandResult(ok_result, encoded_result, sizeof(encoded_result)));
  TEST_ASSERT_EQUAL_UINT8_ARRAY(ok_hex.data(), encoded_result, 9);

  CommandResultPacket err_result = {};
  err_result.struct_version = kBleStructVersion;
  err_result.command_id = kCommandResultConfigWriteId;
  err_result.status = static_cast<uint8_t>(CommandStatus::kErrRange);
  err_result.detail =
      static_cast<uint8_t>(ConfigFieldId::kWheelCircumferenceMm);
  TEST_ASSERT_EQUAL_UINT32(
      9u,
      encodeCommandResult(err_result, encoded_result, sizeof(encoded_result)));
  TEST_ASSERT_EQUAL_UINT8_ARRAY(err_hex.data(), encoded_result, 9);

  CommandResultPacket decoded_result = {};
  TEST_ASSERT_TRUE(
      decodeCommandResult(err_hex.data(), err_hex.size(), decoded_result));
  TEST_ASSERT_EQUAL_UINT8(0xF0u, decoded_result.command_id);
  TEST_ASSERT_EQUAL_UINT8(4u, decoded_result.status);
  TEST_ASSERT_EQUAL_UINT8(2u, decoded_result.detail);
  TEST_ASSERT_TRUE(jsonHasNumber(ok_json, "status", 0));
  TEST_ASSERT_TRUE(jsonHasNumber(err_json, "detail", 2));
  TEST_ASSERT_TRUE(jsonHasString(err_json, "field", "wheel_circumference_mm"));
}

void test_ble_command_is_safe_command_id_range() {
  TEST_ASSERT_TRUE(isSafeCommandId(static_cast<uint8_t>(CommandId::kResetTrip)));
  TEST_ASSERT_TRUE(isSafeCommandId(static_cast<uint8_t>(CommandId::kGetDiagnostic)));
  TEST_ASSERT_FALSE(isSafeCommandId(0x00u));
  TEST_ASSERT_FALSE(isSafeCommandId(0x0Cu));
  TEST_ASSERT_FALSE(isSafeCommandId(static_cast<uint8_t>(CommandId::kResetOdometer)));
}

void test_ble_command_reset_trip_fixture() {
  std::vector<uint8_t> reset_hex;
  TEST_ASSERT_TRUE(loadFixtureHex("command_reset_trip", reset_hex));

  SafeCommandParseResult result =
      parseSafeBleCommand(reset_hex.data(), reset_hex.size());
  TEST_ASSERT_TRUE(result.ok);
  TEST_ASSERT_EQUAL(CommandStatus::kOk, result.status);
  TEST_ASSERT_EQUAL(CommandId::kResetTrip, result.command_id);
  TEST_ASSERT_EQUAL_UINT8(0u, result.field_id);
}

void test_ble_command_safe_no_payload_commands() {
  const CommandId ids[] = {
      CommandId::kResetTrip,      CommandId::kResetMaxSpeed,
      CommandId::kForceSave,      CommandId::kDisplayOn,
      CommandId::kDisplayOff,     CommandId::kSensorTestStop,
      CommandId::kBatteryTest,    CommandId::kStartDiagnostic,
      CommandId::kGetDiagnostic,
  };

  for (CommandId id : ids) {
    CommandPacket packet = {};
    packet.struct_version = kBleStructVersion;
    packet.command_id = static_cast<uint8_t>(id);
    uint8_t encoded[kCommandMaxSize];
    const size_t encoded_len = encodeCommand(packet, encoded, sizeof(encoded));
    TEST_ASSERT_EQUAL_UINT32(4u, encoded_len);

    SafeCommandParseResult result =
        parseSafeBleCommand(encoded, encoded_len);
    TEST_ASSERT_TRUE(result.ok);
    TEST_ASSERT_EQUAL(CommandStatus::kOk, result.status);
    TEST_ASSERT_EQUAL(id, result.command_id);
  }
}

void test_ble_command_display_test_fixture_and_pattern_range() {
  std::vector<uint8_t> fixture_hex;
  TEST_ASSERT_TRUE(loadFixtureHex("command_display_test", fixture_hex));

  SafeCommandParseResult ok =
      parseSafeBleCommand(fixture_hex.data(), fixture_hex.size());
  TEST_ASSERT_TRUE(ok.ok);
  TEST_ASSERT_EQUAL(CommandId::kDisplayTest, ok.command_id);
  TEST_ASSERT_EQUAL_UINT8(1u, ok.params.display_test_pattern);

  uint8_t bad_pattern[] = {1, 6, 0, 1, 3};
  SafeCommandParseResult range =
      parseSafeBleCommand(bad_pattern, sizeof(bad_pattern));
  TEST_ASSERT_FALSE(range.ok);
  TEST_ASSERT_EQUAL(CommandStatus::kErrRange, range.status);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CommandFieldId::kPayload),
                          range.field_id);
}

void test_ble_command_sensor_test_start_fixture_and_duration_range() {
  std::vector<uint8_t> fixture_hex;
  TEST_ASSERT_TRUE(loadFixtureHex("command_sensor_test_start", fixture_hex));

  SafeCommandParseResult ok =
      parseSafeBleCommand(fixture_hex.data(), fixture_hex.size());
  TEST_ASSERT_TRUE(ok.ok);
  TEST_ASSERT_EQUAL(CommandId::kSensorTestStart, ok.command_id);
  TEST_ASSERT_EQUAL_UINT16(60u, ok.params.sensor_test_duration_s);

  uint8_t too_short[] = {1, 7, 0, 2, 0, 0};
  SafeCommandParseResult range =
      parseSafeBleCommand(too_short, sizeof(too_short));
  TEST_ASSERT_FALSE(range.ok);
  TEST_ASSERT_EQUAL(CommandStatus::kErrRange, range.status);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CommandFieldId::kPayload),
                          range.field_id);

  uint8_t too_long[] = {1, 7, 0, 2, 121, 0};
  SafeCommandParseResult range_high =
      parseSafeBleCommand(too_long, sizeof(too_long));
  TEST_ASSERT_FALSE(range_high.ok);
  TEST_ASSERT_EQUAL(CommandStatus::kErrRange, range_high.status);
}

void test_ble_command_rejects_has_token_and_unknown_id() {
  uint8_t with_token[] = {1, 1, 1, 0};
  SafeCommandParseResult token =
      parseSafeBleCommand(with_token, sizeof(with_token));
  TEST_ASSERT_FALSE(token.ok);
  TEST_ASSERT_EQUAL(CommandStatus::kErrRange, token.status);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CommandFieldId::kFlags),
                          token.field_id);

  uint8_t dangerous[] = {1, 0x20, 0, 0};
  SafeCommandParseResult unknown =
      parseSafeBleCommand(dangerous, sizeof(dangerous));
  uint8_t reserved_flag[] = {1, 1, 2, 0};
  SafeCommandParseResult reserved =
      parseSafeBleCommand(reserved_flag, sizeof(reserved_flag));
  TEST_ASSERT_FALSE(reserved.ok);
  TEST_ASSERT_EQUAL(CommandStatus::kErrRange, reserved.status);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CommandFieldId::kFlags),
                          reserved.field_id);

  TEST_ASSERT_FALSE(unknown.ok);
  TEST_ASSERT_EQUAL(CommandStatus::kErrUnknownCommand, unknown.status);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CommandFieldId::kCommandId),
                          unknown.field_id);
}

void test_ble_command_rejects_bad_wire() {
  uint8_t bad_version[] = {2, 1, 0, 0};
  SafeCommandParseResult version =
      parseSafeBleCommand(bad_version, sizeof(bad_version));
  TEST_ASSERT_FALSE(version.ok);
  TEST_ASSERT_EQUAL(CommandStatus::kErrStructVersion, version.status);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CommandFieldId::kStructVersion),
                          version.field_id);

  uint8_t bad_length[] = {1, 1, 0, 1};
  SafeCommandParseResult length =
      parseSafeBleCommand(bad_length, sizeof(bad_length));
  TEST_ASSERT_FALSE(length.ok);
  TEST_ASSERT_EQUAL(CommandStatus::kErrLength, length.status);

  SafeCommandParseResult null_ptr = parseSafeBleCommand(nullptr, 4);
  TEST_ASSERT_FALSE(null_ptr.ok);
  TEST_ASSERT_EQUAL(CommandStatus::kErrLength, null_ptr.status);
}

void test_ble_command_pending_queue_is_single_slot() {
  uint8_t reset_trip[] = {1, 1, 0, 0};
  const SafeCommandParseResult parsed =
      parseSafeBleCommand(reset_trip, sizeof(reset_trip));
  PendingSafeCommand queue;

  TEST_ASSERT_TRUE(safeCommandQueueStage(queue, parsed));
  TEST_ASSERT_FALSE(safeCommandQueueStage(queue, parsed));

  SafeCommandParseResult taken;
  TEST_ASSERT_TRUE(safeCommandQueueDequeue(queue, taken));
  TEST_ASSERT_EQUAL(CommandId::kResetTrip, taken.command_id);
  TEST_ASSERT_FALSE(safeCommandQueueStage(queue, parsed));

  safeCommandQueueFinish(queue);
  TEST_ASSERT_FALSE(safeCommandQueueDequeue(queue, taken));
  TEST_ASSERT_TRUE(safeCommandQueueStage(queue, parsed));
}

void test_ble_dangerous_command_reset_odometer_fixture() {
  std::vector<uint8_t> request_bytes;
  TEST_ASSERT_TRUE(loadFixtureHex("command_reset_odo_request", request_bytes));
  const DangerousCommandParseResult request =
      parseDangerousBleCommand(request_bytes.data(), request_bytes.size());
  TEST_ASSERT_TRUE(request.ok);
  TEST_ASSERT_FALSE(request.has_token);
  TEST_ASSERT_EQUAL(CommandId::kResetOdometer, request.command_id);

  std::vector<uint8_t> fixture_hex;
  TEST_ASSERT_TRUE(loadFixtureHex("command_reset_odo_with_token", fixture_hex));
  const DangerousCommandParseResult confirmation =
      parseDangerousBleCommand(fixture_hex.data(), fixture_hex.size());
  TEST_ASSERT_TRUE(confirmation.ok);
  TEST_ASSERT_TRUE(confirmation.has_token);
  TEST_ASSERT_EQUAL_UINT32(0x12345678u, confirmation.token);
}

void test_ble_dangerous_command_needs_confirm_result_fixture() {
  std::vector<uint8_t> fixture_hex;
  TEST_ASSERT_TRUE(
      loadFixtureHex("result_needs_confirm_reset_odo", fixture_hex));

  CommandResultPacket packet = {};
  packet.struct_version = kBleStructVersion;
  packet.command_id = static_cast<uint8_t>(CommandId::kResetOdometer);
  packet.status = static_cast<uint8_t>(CommandStatus::kNeedsConfirm);
  packet.token = 0x12345678u;
  uint8_t encoded[kCommandResultMaxSize];
  TEST_ASSERT_EQUAL_UINT32(
      9u, encodeCommandResult(packet, encoded, sizeof(encoded)));
  TEST_ASSERT_EQUAL_UINT8_ARRAY(fixture_hex.data(), encoded, 9);

  CommandResultPacket decoded = {};
  TEST_ASSERT_TRUE(
      decodeCommandResult(fixture_hex.data(), fixture_hex.size(), decoded));
  TEST_ASSERT_EQUAL_UINT8(0x20u, decoded.command_id);
  TEST_ASSERT_EQUAL_UINT8(5u, decoded.status);
  TEST_ASSERT_EQUAL_UINT32(0x12345678u, decoded.token);
}

void test_ble_dangerous_command_parameter_layouts_and_ranges() {
  uint8_t battery_request[] = {1, 0x23, 0, 4, 0x4C, 0x04, 0xE7, 0xFF};
  DangerousCommandParseResult battery =
      parseDangerousBleCommand(battery_request, sizeof(battery_request));
  TEST_ASSERT_TRUE(battery.ok);
  TEST_ASSERT_EQUAL_UINT16(1100u, battery.params.battery_scale_permille);
  TEST_ASSERT_EQUAL_INT16(-25, battery.params.battery_offset_mv);

  uint8_t odometer_request[] = {1, 0x30, 0, 4, 0x40, 0xE2, 0x01, 0x00};
  DangerousCommandParseResult odometer =
      parseDangerousBleCommand(odometer_request, sizeof(odometer_request));
  TEST_ASSERT_TRUE(odometer.ok);
  TEST_ASSERT_EQUAL_UINT32(123456u, odometer.params.odometer_m);

  uint8_t pairing_request[] = {1, 0x40, 0, 2, 0x2C, 0x01};
  DangerousCommandParseResult pairing =
      parseDangerousBleCommand(pairing_request, sizeof(pairing_request));
  TEST_ASSERT_TRUE(pairing.ok);
  TEST_ASSERT_EQUAL_UINT16(300u, pairing.params.pairing_window_duration_s);

  uint8_t bad_battery[] = {1, 0x23, 0, 4, 0x1F, 0x03, 0, 0};
  DangerousCommandParseResult range =
      parseDangerousBleCommand(bad_battery, sizeof(bad_battery));
  TEST_ASSERT_FALSE(range.ok);
  TEST_ASSERT_EQUAL(CommandStatus::kErrRange, range.status);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CommandFieldId::kPayload),
                          range.field_id);

  uint8_t bad_pairing[] = {1, 0x40, 0, 2, 0, 0};
  range = parseDangerousBleCommand(bad_pairing, sizeof(bad_pairing));
  TEST_ASSERT_FALSE(range.ok);
  TEST_ASSERT_EQUAL(CommandStatus::kErrRange, range.status);

  uint8_t bad_length[] = {1, 0x30, 1, 4, 1, 2, 3, 4};
  DangerousCommandParseResult length =
      parseDangerousBleCommand(bad_length, sizeof(bad_length));
  TEST_ASSERT_FALSE(length.ok);
  TEST_ASSERT_EQUAL(CommandStatus::kErrLength, length.status);

  uint8_t bad_flags[] = {1, 0x20, 2, 0};
  DangerousCommandParseResult flags =
      parseDangerousBleCommand(bad_flags, sizeof(bad_flags));
  TEST_ASSERT_FALSE(flags.ok);
  TEST_ASSERT_EQUAL(CommandStatus::kErrRange, flags.status);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CommandFieldId::kFlags),
                          flags.field_id);
}

void test_ble_dangerous_handshake_success_expiry_and_binding() {
  uint8_t request_bytes[] = {1, 0x20, 0, 0};
  uint8_t confirm_bytes[] = {1, 0x20, 1, 4, 0x78, 0x56, 0x34, 0x12};
  const DangerousCommandParseResult request =
      parseDangerousBleCommand(request_bytes, sizeof(request_bytes));
  const DangerousCommandParseResult confirmation =
      parseDangerousBleCommand(confirm_bytes, sizeof(confirm_bytes));
  DangerousCommandSession session;

  DangerousCommandHandshakeResult needs = processDangerousCommandHandshake(
      session, request, 7, true, 1000, 0x12345678u);
  TEST_ASSERT_EQUAL(CommandStatus::kNeedsConfirm, needs.status);
  TEST_ASSERT_EQUAL_UINT32(0x12345678u, needs.token);
  TEST_ASSERT_TRUE(session.active);

  DangerousCommandHandshakeResult success = processDangerousCommandHandshake(
      session, confirmation, 7, true, 30999, 0);
  TEST_ASSERT_EQUAL(CommandStatus::kOk, success.status);
  TEST_ASSERT_TRUE(success.execute);
  TEST_ASSERT_FALSE(session.active);

  needs = processDangerousCommandHandshake(session, request, 7, true, 1000,
                                            0x12345678u);
  DangerousCommandHandshakeResult expired = processDangerousCommandHandshake(
      session, confirmation, 7, true, 31000, 0);
  TEST_ASSERT_EQUAL(CommandStatus::kErrTokenExpired, expired.status);
  TEST_ASSERT_FALSE(expired.execute);
  TEST_ASSERT_FALSE(session.active);

  needs = processDangerousCommandHandshake(session, request, 7, true, 1000,
                                            0x12345678u);
  DangerousCommandHandshakeResult wrong_connection =
      processDangerousCommandHandshake(session, confirmation, 8, true, 1001, 0);
  TEST_ASSERT_EQUAL(CommandStatus::kErrTokenInvalid, wrong_connection.status);
  TEST_ASSERT_FALSE(session.active);
}

void test_ble_dangerous_handshake_pairing_rng_and_parameter_binding() {
  uint8_t request_bytes[] = {1, 0x30, 0, 4, 100, 0, 0, 0};
  uint8_t confirm_bytes[] = {
      1, 0x30, 1, 8, 101, 0, 0, 0, 0x78, 0x56, 0x34, 0x12};
  const DangerousCommandParseResult request =
      parseDangerousBleCommand(request_bytes, sizeof(request_bytes));
  const DangerousCommandParseResult changed_confirmation =
      parseDangerousBleCommand(confirm_bytes, sizeof(confirm_bytes));
  DangerousCommandSession session;

  DangerousCommandHandshakeResult not_paired = processDangerousCommandHandshake(
      session, request, 1, false, 100, 0x12345678u);
  TEST_ASSERT_EQUAL(CommandStatus::kErrNotPaired, not_paired.status);
  TEST_ASSERT_FALSE(session.active);

  DangerousCommandHandshakeResult no_rng = processDangerousCommandHandshake(
      session, request, 1, true, 100, 0);
  TEST_ASSERT_EQUAL(CommandStatus::kErrBusy, no_rng.status);
  TEST_ASSERT_FALSE(session.active);

  processDangerousCommandHandshake(session, request, 1, true, 100,
                                   0x12345678u);
  DangerousCommandHandshakeResult changed = processDangerousCommandHandshake(
      session, changed_confirmation, 1, true, 101, 0);
  TEST_ASSERT_EQUAL(CommandStatus::kErrTokenInvalid, changed.status);
  TEST_ASSERT_FALSE(changed.execute);

  processDangerousCommandHandshake(session, request, 1, true, 100,
                                   0x12345678u);
  invalidateDangerousCommandSession(session);
  TEST_ASSERT_FALSE(session.active);
}

void test_ble_dangerous_command_pending_queue_is_single_slot() {
  uint8_t confirm_bytes[] = {1, 0x20, 1, 4, 0x78, 0x56, 0x34, 0x12};
  const DangerousCommandParseResult confirmation =
      parseDangerousBleCommand(confirm_bytes, sizeof(confirm_bytes));
  PendingDangerousCommand queue;

  TEST_ASSERT_TRUE(dangerousCommandQueueStage(queue, confirmation));
  TEST_ASSERT_FALSE(dangerousCommandQueueStage(queue, confirmation));
  DangerousCommandParseResult taken;
  TEST_ASSERT_TRUE(dangerousCommandQueueDequeue(queue, taken));
  TEST_ASSERT_EQUAL_UINT32(0x12345678u, taken.token);
  dangerousCommandQueueFinish(queue);
  TEST_ASSERT_FALSE(dangerousCommandQueueDequeue(queue, taken));
}

void test_protocol_codec_rejects_bad_length_and_version() {
  uint8_t telemetry[kTelemetrySize] = {};
  encodeTelemetry(makeMovingTelemetry(), telemetry);
  TelemetryPacket decoded_telemetry = {};
  telemetry[0] = 2;
  TEST_ASSERT_FALSE(
      decodeTelemetry(telemetry, sizeof(telemetry), decoded_telemetry));
  telemetry[0] = 1;
  TEST_ASSERT_FALSE(
      decodeTelemetry(telemetry, sizeof(telemetry) - 1, decoded_telemetry));

  uint8_t command[4] = {1, 1, 0, 0};
  CommandPacket decoded_command = {};
  TEST_ASSERT_FALSE(decodeCommand(command, 3, decoded_command));
  command[3] = 1;
  TEST_ASSERT_FALSE(decodeCommand(command, 4, decoded_command));

  ErrorLogPacket log = {};
  log.struct_version = kBleStructVersion;
  log.entry_count = 1;
  log.entries[0].uptime_s = 10;
  log.entries[0].code = static_cast<uint8_t>(ErrorLogCode::kFlashError);
  log.entries[0].severity = static_cast<uint8_t>(ErrorLogSeverity::kError);
  log.entries[0].detail = 7;
  uint8_t encoded_log[kErrorLogMaxSize];
  TEST_ASSERT_EQUAL_UINT32(
      10u, encodeErrorLog(log, encoded_log, sizeof(encoded_log)));
  ErrorLogPacket decoded_log = {};
  TEST_ASSERT_TRUE(decodeErrorLog(encoded_log, 10, decoded_log));
  TEST_ASSERT_EQUAL_UINT8(1u, decoded_log.entry_count);
  TEST_ASSERT_EQUAL_UINT16(7u, decoded_log.entries[0].detail);
  TEST_ASSERT_FALSE(decodeErrorLog(encoded_log, 9, decoded_log));
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
  RUN_TEST(test_migrate_config_v1_to_v2_preserves_fields);
  RUN_TEST(test_migrate_odometer_v1_to_v2_preserves_totals);
  RUN_TEST(test_storage_migrates_config_v1_fixture_and_rewrites_v2);
  RUN_TEST(test_storage_migrates_odometer_v1_and_rejects_future_version);
  RUN_TEST(test_pulse_filter_first_debounce_and_overspeed);
  RUN_TEST(test_pulse_filter_stuck_and_micros_wrap);
  RUN_TEST(test_speed_fixed_point_smoothing_and_timeout);
  RUN_TEST(test_speed_smoothing_windows_two_and_five);
  RUN_TEST(test_speed_boundary_circumferences);
  RUN_TEST(test_trip_accumulation_average_and_reset);
  RUN_TEST(test_trip_restores_only_persistent_totals);
  RUN_TEST(test_ride_state_transitions_and_paused_time);
  RUN_TEST(test_scheduler_period_and_wrap);
  RUN_TEST(test_serial_console_parses_supported_commands_and_crlf);
  RUN_TEST(test_serial_console_trims_rejects_and_recovers_after_overflow);
  RUN_TEST(test_serial_console_parses_ambient_commands);
  RUN_TEST(test_serial_console_parses_display_commands);
  RUN_TEST(test_error_log_keeps_16_and_snapshots_newest_four_in_order);
  RUN_TEST(test_ble_advertising_policy_timeout_and_movement_restart);
  RUN_TEST(test_page_carousel_default_period_and_wrap);
  RUN_TEST(test_page_carousel_mask_order_and_fallback);
  RUN_TEST(test_page_carousel_pinned_page);
  RUN_TEST(test_display_power_dim_off_wake_disable_and_wrap);
  RUN_TEST(test_display_burn_in_guard_cycles_catches_up_and_wraps);
  RUN_TEST(test_battery_raw_conversion_and_calibration);
  RUN_TEST(test_ambient_light_model_normalizes_levels_caps_and_contrast);
  RUN_TEST(test_ambient_light_model_ema_hysteresis_dwell_and_invalid_fallback);
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
  RUN_TEST(test_odometer_save_flash_error_keeps_oneshot_pending);
  RUN_TEST(test_ble_identity_resolves_placeholder_name);
  RUN_TEST(test_map_nrf_reset_reason_priority);
  RUN_TEST(test_pairing_window_and_device_info_flags);
  RUN_TEST(test_boot_count_increments_and_persists);
  RUN_TEST(test_telemetry_publish_mode_and_intervals);
  RUN_TEST(test_fill_telemetry_packet_moving_and_idle);
  RUN_TEST(test_ride_state_tracks_last_pulse);
  RUN_TEST(test_config_write_parse_valid_and_range_error);
  RUN_TEST(test_config_write_pending_queue_single_slot);
  RUN_TEST(test_config_validation_error_maps_to_field_id);
  RUN_TEST(test_ble_command_is_safe_command_id_range);
  RUN_TEST(test_ble_command_reset_trip_fixture);
  RUN_TEST(test_ble_command_safe_no_payload_commands);
  RUN_TEST(test_ble_command_display_test_fixture_and_pattern_range);
  RUN_TEST(test_ble_command_sensor_test_start_fixture_and_duration_range);
  RUN_TEST(test_ble_command_rejects_has_token_and_unknown_id);
  RUN_TEST(test_ble_command_rejects_bad_wire);
  RUN_TEST(test_ble_command_pending_queue_is_single_slot);
  RUN_TEST(test_ble_dangerous_command_reset_odometer_fixture);
  RUN_TEST(test_ble_dangerous_command_needs_confirm_result_fixture);
  RUN_TEST(test_ble_dangerous_command_parameter_layouts_and_ranges);
  RUN_TEST(test_ble_dangerous_handshake_success_expiry_and_binding);
  RUN_TEST(test_ble_dangerous_handshake_pairing_rng_and_parameter_binding);
  RUN_TEST(test_ble_dangerous_command_pending_queue_is_single_slot);
  RUN_TEST(test_diagnostic_snapshot_maps_storage_and_pulse_counters);
  RUN_TEST(test_diagnostic_payload_little_endian_layout);
  RUN_TEST(test_diagnostic_saturates_narrow_fields);
  RUN_TEST(test_ble_protocol_sizes_match_contract);
  RUN_TEST(test_protocol_fixture_device_info_v1_nominal);
  RUN_TEST(test_protocol_fixture_telemetry_v1_moving_and_paused);
  RUN_TEST(test_protocol_fixture_config_v1_defaults_and_imperial);
  RUN_TEST(test_protocol_fixture_commands_and_results);
  RUN_TEST(test_protocol_codec_rejects_bad_length_and_version);
  return UNITY_END();
}
