#include <unity.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "ambient_calibration_save_policy.h"
#include "ambient_light_calibrator.h"
#include "ambient_light_model.h"
#include "battery_model.h"
#include "ble_advertising.h"
#include "ble_command.h"
#include "ble_config_write.h"
#include "ble_device_info.h"
#include "ble_identity.h"
#include "ble_protocol.h"
#include "build_version.h"
#include "ble_telemetry.h"
#include "boot_counter.h"
#include "companion_snapshot.h"
#include "config_codec.h"
#include "config_validator.h"
#include "crc32.h"
#include "csc_measurement.h"
#include "display_burn_in.h"
#include "diagnostics.h"
#include "error_log.h"
#include "display_formatter.h"
#include "display_power.h"
#include "idle_delay.h"
#include "odometer_save_policy.h"
#include "page_carousel.h"
#include "power_manager.h"
#include "protocol_codec.h"
#include "pulse_filter.h"
#include "ride_state.h"
#include "scheduler.h"
#include "serial_console.h"
#include "serial_usb_test.h"
#include "speed_calculator.h"
#include "speed_interval_guard.h"
#include "storage_manager.h"
#include "storage_migration.h"
#include "trip_computer.h"
#include "watchdog_config.h"

using namespace bike;

void setUp() {}
void tearDown() {}

void test_serial_console_parses_supported_commands_and_crlf() {
  SerialCommandParser parser;
  const char* commands =
      "open-pairing\r\ndump-config\nreset-odo\rreboot\nselftest\nsched\n"
      "wdt-hang\n";
  const SerialCommand expected[] = {
      SerialCommand::kOpenPairing,
      SerialCommand::kDumpConfig,
      SerialCommand::kResetOdometer,
      SerialCommand::kReboot,
      SerialCommand::kSelftest,
      SerialCommand::kSchedStats,
      SerialCommand::kWdtHang,
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

void test_serial_console_parses_status_command() {
  SerialCommandParser parser;
  SerialCommand result = SerialCommand::kNone;
  const char* command = "status\n";
  for (size_t i = 0; command[i] != '\0'; ++i) {
    result = parser.feed(command[i]);
  }
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SerialCommand::kStatus),
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

void test_serial_console_parses_hall_commands() {
  SerialCommandParser parser;
  const char* commands = "hall-status\nhall-watch\nhall-stop\n";
  const SerialCommand expected[] = {
      SerialCommand::kHallStatus,
      SerialCommand::kHallWatch,
      SerialCommand::kHallStop,
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

void test_serial_console_parses_hall_analog_commands() {
  SerialCommandParser parser;
  const char* commands = "hall-analog\nhall-analog-stop\n";
  const SerialCommand expected[] = {
      SerialCommand::kHallAnalog,
      SerialCommand::kHallAnalogStop,
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

void test_serial_console_parses_gpio_commands() {
  SerialCommandParser parser;
  const char* commands = "gpio-probe\ngpio-watch\ngpio-stop\n";
  const SerialCommand expected[] = {
      SerialCommand::kGpioProbe,
      SerialCommand::kGpioWatch,
      SerialCommand::kGpioStop,
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

void test_serial_console_parses_csc_commands() {
  SerialCommandParser parser;
  const char* commands = "csc-status\ncsc-pair\ncsc-forget\n";
  const SerialCommand expected[] = {
      SerialCommand::kCscStatus,
      SerialCommand::kCscPair,
      SerialCommand::kCscForget,
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

void test_power_manager_ble_always_advertise_overrides_power_save() {
  PowerManager manager;
  PowerManagerConfig config;
  config.power_save_mode = true;
  manager.configure(config);
  manager.setBleAlwaysAdvertise(true);
  TEST_ASSERT_FALSE(manager.aggressiveBlePowerSave());

  manager.setBleAlwaysAdvertise(false);
  TEST_ASSERT_TRUE(manager.aggressiveBlePowerSave());
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

  bool beginWrite(const char* path, const uint8_t* data,
                  size_t length) override {
    if (write_pending_) return false;
    pending_path_ = path;
    pending_data_.assign(data, data + length);
    polls_remaining_ = polls_to_complete;
    write_pending_ = true;
    return true;
  }

  AsyncWriteStatus pollWrite() override {
    if (!write_pending_) return AsyncWriteStatus::kIdle;
    if (--polls_remaining_ > 0) return AsyncWriteStatus::kInProgress;
    write_pending_ = false;
    if (!write_ok) return AsyncWriteStatus::kError;
    files[pending_path_] = pending_data_;
    return AsyncWriteStatus::kOk;
  }

  // Test-only synchronous seeding helper -- bypasses beginWrite/pollWrite
  // entirely. Used by putXRecord()/corrupt() fixtures below to set up raw
  // slot contents directly.
  bool write(const char* path, const uint8_t* data, size_t length) {
    if (!write_ok) return false;
    files[path] = std::vector<uint8_t>(data, data + length);
    return true;
  }

  void corrupt(const char* path, size_t offset) { files[path][offset] ^= 0x80u; }

  bool begin_ok = true;
  bool write_ok = true;
  size_t polls_to_complete = 1;
  std::map<std::string, std::vector<uint8_t>> files;

 private:
  bool write_pending_ = false;
  std::string pending_path_;
  std::vector<uint8_t> pending_data_;
  size_t polls_remaining_ = 0;
};

void test_memory_backend_async_write_completes_after_configured_polls() {
  MemoryStorageBackend backend;
  backend.polls_to_complete = 3;
  const uint8_t data[] = {1, 2, 3, 4};
  TEST_ASSERT_TRUE(backend.beginWrite("/test_async", data, sizeof(data)));
  TEST_ASSERT_EQUAL(AsyncWriteStatus::kInProgress, backend.pollWrite());
  TEST_ASSERT_EQUAL(AsyncWriteStatus::kInProgress, backend.pollWrite());
  TEST_ASSERT_EQUAL(AsyncWriteStatus::kOk, backend.pollWrite());
  TEST_ASSERT_EQUAL_UINT32(1u, backend.files.count("/test_async"));
  const auto& stored = backend.files.at("/test_async");
  TEST_ASSERT_EQUAL_UINT32(sizeof(data), stored.size());
  TEST_ASSERT_EQUAL_UINT8_ARRAY(data, stored.data(), sizeof(data));
  TEST_ASSERT_EQUAL(AsyncWriteStatus::kIdle, backend.pollWrite());

  // A second beginWrite while one is already pending is rejected.
  backend.polls_to_complete = 1;
  TEST_ASSERT_TRUE(backend.beginWrite("/test_async2", data, sizeof(data)));
  TEST_ASSERT_FALSE(backend.beginWrite("/test_async3", data, sizeof(data)));
}

void test_storage_async_save_completes_over_multiple_polls() {
  MemoryStorageBackend backend;
  StorageManager storage(backend);
  TEST_ASSERT_TRUE(storage.begin());

  backend.polls_to_complete = 2;
  OdometerData data{1234u, 5u};
  TEST_ASSERT_TRUE(storage.beginSaveOdometer(data));
  TEST_ASSERT_TRUE(storage.saveInProgress());
  TEST_ASSERT_EQUAL(StorageAsyncStatus::kInProgress, storage.pollSave());
  TEST_ASSERT_TRUE(storage.saveInProgress());
  TEST_ASSERT_EQUAL(StorageAsyncStatus::kOk, storage.pollSave());
  TEST_ASSERT_FALSE(storage.saveInProgress());
  TEST_ASSERT_EQUAL_UINT32(1u, storage.counters().writes);
  TEST_ASSERT_EQUAL_UINT32(1u, storage.lastOdometerSequence());
}

void test_storage_async_save_serializes_concurrent_begin() {
  MemoryStorageBackend backend;
  StorageManager storage(backend);
  TEST_ASSERT_TRUE(storage.begin());

  backend.polls_to_complete = 2;
  OdometerData odometer{1000u, 1u};
  TEST_ASSERT_TRUE(storage.beginSaveOdometer(odometer));
  TEST_ASSERT_TRUE(storage.saveInProgress());

  AmbientCalibrationData ambient{266u, 1126u, 1u};
  TEST_ASSERT_FALSE(storage.beginSaveAmbientCalibration(ambient));

  TEST_ASSERT_EQUAL(StorageAsyncStatus::kOk, storage.drainPendingSave());
  TEST_ASSERT_TRUE(storage.beginSaveAmbientCalibration(ambient));
}

void test_storage_async_drain_pending_save_blocks_to_completion() {
  MemoryStorageBackend backend;
  StorageManager storage(backend);
  TEST_ASSERT_TRUE(storage.begin());

  backend.polls_to_complete = 2;
  OdometerData odometer{1234u, 5u};
  TEST_ASSERT_TRUE(storage.beginSaveOdometer(odometer));
  TEST_ASSERT_EQUAL(StorageAsyncStatus::kOk, storage.drainPendingSave());

  TEST_ASSERT_TRUE(storage.beginSaveStorageCounters());
  TEST_ASSERT_TRUE(storage.saveInProgress());
  TEST_ASSERT_EQUAL(StorageAsyncStatus::kOk, storage.drainPendingSave());
  TEST_ASSERT_FALSE(storage.saveInProgress());
  TEST_ASSERT_EQUAL_UINT32(1u, backend.files.count("/cnt_a"));
}

void test_storage_async_dedup_skip_completes_synchronously() {
  MemoryStorageBackend backend;
  StorageManager storage(backend);
  TEST_ASSERT_TRUE(storage.begin());

  backend.polls_to_complete = 2;
  AmbientCalibrationData data{266u, 1126u, 1u};
  TEST_ASSERT_TRUE(storage.beginSaveAmbientCalibration(data));
  TEST_ASSERT_EQUAL(StorageAsyncStatus::kOk, storage.drainPendingSave());

  const uint32_t writes_before = storage.counters().writes;
  const uint32_t skipped_before = storage.counters().skipped_writes;
  TEST_ASSERT_TRUE(storage.beginSaveAmbientCalibration(data));
  TEST_ASSERT_FALSE(storage.saveInProgress());
  TEST_ASSERT_EQUAL_UINT32(writes_before, storage.counters().writes);
  TEST_ASSERT_EQUAL_UINT32(skipped_before + 1u, storage.counters().skipped_writes);
}

void test_storage_async_write_error_propagates_through_poll() {
  MemoryStorageBackend backend;
  StorageManager storage(backend);
  TEST_ASSERT_TRUE(storage.begin());

  backend.polls_to_complete = 2;
  OdometerData odometer{1234u, 5u};
  TEST_ASSERT_TRUE(storage.beginSaveOdometer(odometer));
  TEST_ASSERT_EQUAL(StorageAsyncStatus::kOk, storage.drainPendingSave());

  backend.write_ok = false;
  TEST_ASSERT_TRUE(storage.beginSaveStorageCounters());
  TEST_ASSERT_EQUAL(StorageAsyncStatus::kError, storage.drainPendingSave());
  TEST_ASSERT_EQUAL_UINT32(1u, storage.counters().write_errors);
}

void test_storage_async_failed_odometer_save_does_not_leak_sequence_into_unrelated_write() {
  MemoryStorageBackend backend;
  StorageManager storage(backend);
  TEST_ASSERT_TRUE(storage.begin());

  // A failed odometer save leaves a stale pending_odometer_sequence_ behind
  // -- it must not survive to taint a later, unrelated write's completion.
  backend.polls_to_complete = 1;
  backend.write_ok = false;
  OdometerData odometer{5000u, 10u};
  TEST_ASSERT_TRUE(storage.beginSaveOdometer(odometer));
  TEST_ASSERT_EQUAL(StorageAsyncStatus::kError, storage.drainPendingSave());
  TEST_ASSERT_EQUAL_UINT32(0u, storage.lastOdometerSequence());

  // loadConfig()'s boot-time defaults-write goes through the raw writeSlot()
  // path (not beginSavePayloadAsync), which is exactly the path that never
  // touched pending_is_odometer_ before the fix. It must succeed without
  // stamping the stale odometer sequence.
  backend.write_ok = true;
  DeviceConfig config;
  StorageLoadInfo info;
  TEST_ASSERT_TRUE(storage.loadConfig(config, info));
  TEST_ASSERT_TRUE(info.defaults_written);
  TEST_ASSERT_EQUAL_UINT32(0u, storage.lastOdometerSequence());
}

void test_storage_async_drain_gives_up_on_a_backend_that_never_completes() {
  MemoryStorageBackend backend;
  StorageManager storage(backend);
  TEST_ASSERT_TRUE(storage.begin());

  backend.polls_to_complete = 2;
  OdometerData odometer{1234u, 5u};
  TEST_ASSERT_TRUE(storage.beginSaveOdometer(odometer));
  TEST_ASSERT_EQUAL(StorageAsyncStatus::kOk, storage.drainPendingSave());

  backend.polls_to_complete = 100;  // exceeds the internal drain cap
  TEST_ASSERT_TRUE(storage.beginSaveStorageCounters());
  TEST_ASSERT_EQUAL(StorageAsyncStatus::kError, storage.drainPendingSave());
  TEST_ASSERT_FALSE(storage.saveInProgress());
  TEST_ASSERT_EQUAL_UINT32(1u, storage.counters().write_errors);
}

void test_storage_async_sync_save_rejected_while_another_save_in_progress() {
  MemoryStorageBackend backend;
  StorageManager storage(backend);
  TEST_ASSERT_TRUE(storage.begin());

  backend.polls_to_complete = 2;
  OdometerData odometer{1000u, 1u};
  TEST_ASSERT_TRUE(storage.beginSaveOdometer(odometer));
  TEST_ASSERT_TRUE(storage.saveInProgress());

  AmbientCalibrationData ambient{266u, 1126u, 1u};
  TEST_ASSERT_FALSE(storage.saveAmbientCalibration(ambient));
}

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

void putStorageCountersRecord(MemoryStorageBackend& backend,
                              const char* path,
                              const StorageCounters& counters,
                              uint32_t sequence,
                              uint16_t version = kStorageCountersRecordVersion) {
  uint8_t payload[kStorageCountersPayloadSize];
  uint8_t record[kMaximumRecordSize];
  encodeStorageCounters(counters, payload);
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
  info.hw_revision = BIKECOMP_HW_REVISION;
  memcpy(info.model, "BIKECOMP-XIAO", 13);
  memcpy(info.fw_version, BIKECOMP_FW_VERSION, strlen(BIKECOMP_FW_VERSION) + 1);
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
  maximum.enabled_pages_mask = 0xFF;
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
  config.enabled_pages_mask = 0x80;
  TEST_ASSERT_EQUAL(ConfigValidationError::kNone,
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
  config.pinned_page = 8;
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

void test_ambient_calibration_encode_decode_round_trip() {
  AmbientCalibrationData original{123u, 3800u, 1u};
  uint8_t payload[kAmbientCalibrationPayloadSize];
  encodeAmbientCalibration(original, payload);

  AmbientCalibrationData decoded;
  TEST_ASSERT_TRUE(decodeAmbientCalibration(payload, sizeof(payload), decoded));
  TEST_ASSERT_EQUAL_UINT16(original.raw_dark, decoded.raw_dark);
  TEST_ASSERT_EQUAL_UINT16(original.raw_bright, decoded.raw_bright);
  TEST_ASSERT_EQUAL_UINT8(original.quality, decoded.quality);

  TEST_ASSERT_FALSE(decodeAmbientCalibration(payload, sizeof(payload) - 1, decoded));
}

void test_storage_counters_encode_decode_round_trip() {
  StorageCounters original;
  original.writes = 42u;
  original.skipped_writes = 7u;
  original.read_errors = 3u;
  original.write_errors = 1u;
  original.config_slot_recoveries = 2u;
  original.odometer_slot_recoveries = 5u;
  original.ambient_calibration_slot_recoveries = 4u;
  original.config_defaults_restored = 1u;
  original.odometer_defaults_restored = 1u;
  original.ambient_calibration_defaults_restored = 1u;
  original.config_migrations = 6u;
  original.odometer_migrations = 8u;

  uint8_t payload[kStorageCountersPayloadSize];
  encodeStorageCounters(original, payload);

  StorageCounters decoded;
  TEST_ASSERT_TRUE(decodeStorageCounters(payload, sizeof(payload), decoded));
  TEST_ASSERT_EQUAL_UINT32(original.writes, decoded.writes);
  TEST_ASSERT_EQUAL_UINT32(original.skipped_writes, decoded.skipped_writes);
  TEST_ASSERT_EQUAL_UINT32(original.read_errors, decoded.read_errors);
  TEST_ASSERT_EQUAL_UINT32(original.write_errors, decoded.write_errors);
  TEST_ASSERT_EQUAL_UINT32(original.config_slot_recoveries,
                           decoded.config_slot_recoveries);
  TEST_ASSERT_EQUAL_UINT32(original.odometer_slot_recoveries,
                           decoded.odometer_slot_recoveries);
  TEST_ASSERT_EQUAL_UINT32(original.ambient_calibration_slot_recoveries,
                           decoded.ambient_calibration_slot_recoveries);
  TEST_ASSERT_EQUAL_UINT32(original.config_defaults_restored,
                           decoded.config_defaults_restored);
  TEST_ASSERT_EQUAL_UINT32(original.odometer_defaults_restored,
                           decoded.odometer_defaults_restored);
  TEST_ASSERT_EQUAL_UINT32(original.ambient_calibration_defaults_restored,
                           decoded.ambient_calibration_defaults_restored);
  TEST_ASSERT_EQUAL_UINT32(original.config_migrations, decoded.config_migrations);
  TEST_ASSERT_EQUAL_UINT32(original.odometer_migrations, decoded.odometer_migrations);

  TEST_ASSERT_FALSE(decodeStorageCounters(payload, sizeof(payload) - 1, decoded));
}

void test_storage_counters_absent_record_defaults_without_flash_write() {
  MemoryStorageBackend backend;
  StorageManager storage(backend);
  TEST_ASSERT_TRUE(storage.begin());
  TEST_ASSERT_EQUAL_UINT32(0u, storage.counters().writes);
  TEST_ASSERT_EQUAL_UINT32(0u, backend.files.count("/cnt_a"));
  TEST_ASSERT_EQUAL_UINT32(0u, backend.files.count("/cnt_b"));
}

void test_storage_counters_skip_persist_when_only_skipped_writes_changed() {
  MemoryStorageBackend backend;
  StorageManager storage(backend);
  TEST_ASSERT_TRUE(storage.begin());

  OdometerData odometer{1234u, 5u};
  TEST_ASSERT_TRUE(storage.saveOdometer(odometer));
  TEST_ASSERT_EQUAL_UINT32(1u, storage.counters().writes);
  storage.markStorageCountersPersisted();

  TEST_ASSERT_TRUE(storage.saveOdometer(odometer));
  TEST_ASSERT_EQUAL_UINT32(1u, storage.counters().writes);
  TEST_ASSERT_EQUAL_UINT32(1u, storage.counters().skipped_writes);
  TEST_ASSERT_FALSE(storage.storageCountersNeedPersist());

  TEST_ASSERT_TRUE(storage.beginSaveStorageCounters());
  TEST_ASSERT_FALSE(storage.saveInProgress());
  TEST_ASSERT_EQUAL_UINT32(0u, backend.files.count("/cnt_a"));
}

void test_storage_counters_persist_across_reboot() {
  MemoryStorageBackend backend;
  StorageManager storage(backend);
  TEST_ASSERT_TRUE(storage.begin());

  OdometerData odometer{1234u, 5u};
  TEST_ASSERT_TRUE(storage.saveOdometer(odometer));
  odometer.odometer_mm = 2000u;
  TEST_ASSERT_TRUE(storage.saveOdometer(odometer));

  const uint32_t writes_before_save = storage.counters().writes;
  TEST_ASSERT_TRUE(writes_before_save >= 2u);
  TEST_ASSERT_TRUE(storage.saveStorageCounters());
  TEST_ASSERT_EQUAL_UINT32(1u, backend.files.count("/cnt_a"));

  StorageManager reloaded(backend);
  TEST_ASSERT_TRUE(reloaded.begin());
  TEST_ASSERT_EQUAL_UINT32(writes_before_save, reloaded.counters().writes);
}

void test_storage_counters_recovers_from_corrupt_slot_and_keeps_read_error() {
  MemoryStorageBackend backend;
  StorageCounters slot_a;
  slot_a.writes = 10u;
  putStorageCountersRecord(backend, "/cnt_a", slot_a, 3u);

  StorageCounters slot_b;
  slot_b.writes = 9u;
  putStorageCountersRecord(backend, "/cnt_b", slot_b, 2u);
  backend.corrupt("/cnt_a", kRecordHeaderSize);

  StorageManager storage(backend);
  TEST_ASSERT_TRUE(storage.begin());
  // Slot A (sequence 3, newest) is corrupt; falls back to slot B (sequence 2)
  // and keeps the read_errors bump this boot's failed slot A read caused.
  TEST_ASSERT_EQUAL_UINT32(9u, storage.counters().writes);
  TEST_ASSERT_EQUAL_UINT32(1u, storage.counters().read_errors);
}

void test_ambient_calibration_defaults_without_flash_write_then_alternates_slots() {
  MemoryStorageBackend backend;
  StorageManager storage(backend);
  TEST_ASSERT_TRUE(storage.begin());

  AmbientCalibrationData loaded;
  StorageLoadInfo info;
  TEST_ASSERT_TRUE(storage.loadAmbientCalibration(loaded, info));
  TEST_ASSERT_EQUAL(StorageSource::kDefaults, info.source);
  TEST_ASSERT_EQUAL_UINT16(0u, loaded.raw_dark);
  // Unlike config/odometer, an absent calibration record must NOT be
  // eagerly written -- the caller decides the real bootstrap values.
  TEST_ASSERT_EQUAL_UINT32(0u, backend.files.count("/alc_a"));
  TEST_ASSERT_EQUAL_UINT32(0u, backend.files.count("/alc_b"));
  TEST_ASSERT_EQUAL_UINT32(0u, storage.counters().writes);

  AmbientCalibrationData saved{266u, 1126u,
                               static_cast<uint8_t>(AmbientCalibrationQuality::kOk)};
  TEST_ASSERT_TRUE(storage.saveAmbientCalibration(saved));
  TEST_ASSERT_EQUAL_UINT32(1u, backend.files.count("/alc_a"));

  StorageManager reloaded(backend);
  TEST_ASSERT_TRUE(reloaded.begin());
  AmbientCalibrationData from_flash;
  TEST_ASSERT_TRUE(reloaded.loadAmbientCalibration(from_flash, info));
  TEST_ASSERT_EQUAL(StorageSource::kSlotA, info.source);
  TEST_ASSERT_EQUAL_UINT16(266u, from_flash.raw_dark);
  TEST_ASSERT_EQUAL_UINT16(1126u, from_flash.raw_bright);

  from_flash.raw_dark = 200u;
  TEST_ASSERT_TRUE(reloaded.saveAmbientCalibration(from_flash));
  TEST_ASSERT_EQUAL_UINT32(1u, backend.files.count("/alc_b"));

  // Saving the same value again is a no-op (dedup by content).
  const uint32_t writes_before = reloaded.counters().writes;
  TEST_ASSERT_TRUE(reloaded.saveAmbientCalibration(from_flash));
  TEST_ASSERT_EQUAL_UINT32(writes_before, reloaded.counters().writes);
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
    five.onInterval(2100, 300000, i * 300000u, true, 5);
  }
  TEST_ASSERT_EQUAL_UINT16(2520u, five.speedX100());
}

void test_speed_boundary_circumferences() {
  SpeedCalculator speed;
  TEST_ASSERT_EQUAL_UINT16(1800u, speed.onInterval(500, 100000, 100000, false, 2));
  speed.reset();
  TEST_ASSERT_EQUAL_UINT16(10800u, speed.onInterval(3000, 100000, 100000, false, 5));
}

void test_speed_gap_long_interval_corrected() {
  SpeedCalculator speed;
  TEST_ASSERT_EQUAL_UINT16(2520u, speed.onInterval(2100, 300000, 300000, false, 3));
  TEST_ASSERT_EQUAL_UINT16(2520u, speed.onInterval(2100, 900000, 1200000, false, 3));
  TEST_ASSERT_EQUAL_UINT32(1u, speed.speedIntervalCorrectedCount());
}

void test_speed_gap_short_spike_corrected() {
  SpeedCalculator speed;
  TEST_ASSERT_EQUAL_UINT16(2520u, speed.onInterval(2100, 300000, 300000, false, 3));
  TEST_ASSERT_EQUAL_UINT16(2520u, speed.onInterval(2100, 100000, 400000, false, 3));
  TEST_ASSERT_EQUAL_UINT32(1u, speed.speedIntervalCorrectedCount());
}

void test_speed_gap_pause_recovery_uses_new_cadence() {
  SpeedCalculator speed;
  TEST_ASSERT_EQUAL_UINT16(126u, speed.onInterval(2100, 6000000, 6000000, false, 3));
  TEST_ASSERT_EQUAL_UINT16(8400u, speed.onInterval(2100, 90000, 6090000, false, 3));
}

void test_speed_gap_revolutions_unaffected() {
  TripComputer trip;
  trip.onRevolution(2100, 0);
  trip.onRevolution(2100, 2520);
  TEST_ASSERT_EQUAL_UINT32(2u, trip.snapshot().revolutions);
}

void test_speed_gap_reset_clears_guard() {
  SpeedCalculator speed;
  speed.onInterval(2100, 300000, 300000, false, 3);
  speed.onInterval(2100, 900000, 1200000, false, 3);
  TEST_ASSERT_EQUAL_UINT32(1u, speed.speedIntervalCorrectedCount());
  speed.reset();
  TEST_ASSERT_EQUAL_UINT32(0u, speed.speedIntervalCorrectedCount());
}

void test_speed_gap_mid_ride_spike_corrected() {
  SpeedCalculator speed;
  TEST_ASSERT_EQUAL_UINT16(2520u, speed.onInterval(2100, 300000, 300000, false, 3));
  TEST_ASSERT_EQUAL_UINT16(2520u, speed.onInterval(2100, 176470, 476470, false, 3));
  TEST_ASSERT_EQUAL_UINT32(1u, speed.speedIntervalCorrectedCount());
}

void test_speed_gap_exact_half_interval_corrected() {
  SpeedCalculator speed;
  TEST_ASSERT_EQUAL_UINT16(3000u, speed.onInterval(2055, 246600, 246600, false, 3));
  TEST_ASSERT_EQUAL_UINT16(3000u, speed.onInterval(2055, 123300, 369900, false, 3));
  TEST_ASSERT_EQUAL_UINT32(1u, speed.speedIntervalCorrectedCount());
}

void test_pulse_to_speed_pipeline_sets_max_speed() {
  PulseFilter filter;
  SpeedCalculator speed;
  TripComputer trip;

  uint32_t ts = 1000000u;
  for (uint8_t i = 0; i < 8u; ++i) {
    const PulseDecision decision = filter.process(ts);
    if (decision.accepted) {
      uint16_t speed_x100 = 0;
      if (decision.interval_us > 0) {
        speed_x100 = speed.onInterval(2100, decision.interval_us, ts, true, 3);
      }
      trip.onRevolution(2100, speed_x100);
    }
    ts += 300000u;
  }

  TEST_ASSERT_GREATER_THAN(1u, trip.snapshot().revolutions);
  TEST_ASSERT_GREATER_THAN(0u, trip.snapshot().max_speed_x100);
  TEST_ASSERT_GREATER_THAN(0u, trip.snapshot().speed_x100);
  trip.addMovingTime(2100u * 360u);
  TEST_ASSERT_GREATER_THAN(0u, trip.snapshot().average_speed_x100);
}

void test_pulse_filter_rejects_zero_interval() {
  PulseFilterConfig cfg;
  cfg.debounce_ms = 0;
  PulseFilter filter(cfg);
  TEST_ASSERT_TRUE(filter.process(1000000u).accepted);
  const PulseDecision second = filter.process(1000000u);
  TEST_ASSERT_FALSE(second.accepted);
  TEST_ASSERT_EQUAL(PulseRejection::kOverspeed, second.rejection);
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

void test_trip_test_revolution_skips_persistent_totals() {
  TripComputer trip;
  trip.restorePersistentTotals(50000ull, 100ull);
  trip.onRevolutionForTest(2100, 2520);
  trip.onRevolutionForTest(2100, 3000);
  TEST_ASSERT_EQUAL_UINT32(2u, trip.snapshot().revolutions);
  TEST_ASSERT_EQUAL_UINT32(4200u, trip.snapshot().trip_distance_mm);
  TEST_ASSERT_EQUAL_UINT16(3000u, trip.snapshot().max_speed_x100);
  TEST_ASSERT_EQUAL_UINT64(50000ull, trip.snapshot().odometer_mm);
  TEST_ASSERT_EQUAL_UINT64(100ull, trip.totalRevolutions());
}

void test_trip_restore_snapshot_reverts_session() {
  TripComputer trip;
  trip.restorePersistentTotals(1000ull, 10ull);
  const TripSnapshot backup = trip.snapshot();
  trip.onRevolutionForTest(2100, 1500);
  trip.restoreSnapshot(backup, 10ull);
  TEST_ASSERT_EQUAL_UINT32(0u, trip.snapshot().revolutions);
  TEST_ASSERT_EQUAL_UINT64(1000ull, trip.snapshot().odometer_mm);
  TEST_ASSERT_EQUAL_UINT64(10ull, trip.totalRevolutions());
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

void test_scheduler_next_due_ms_wrap_safe_picks_soonest_task() {
  // "soon" is due in 5ms, "far" is due in ~2^31 ms (the theoretical max
  // distance isDue() treats as "not due yet"). The old implementation
  // compared raw next_due_ms values and would return "far" here because
  // 0x7FFFFFF0 < 0xFFFFFFF5, understating urgency by roughly 2^31 ms.
  ScheduledTask tasks[] = {
      {"soon", 1000, 0xFFFFFFF5u, countTask, nullptr, 0},
      {"far", 1000, 0x7FFFFFF0u, countTask, nullptr, 0},
  };
  Scheduler scheduler(tasks, 2);
  TEST_ASSERT_EQUAL_UINT32(0xFFFFFFF5u, scheduler.nextDueMs(0xFFFFFFF0u));
}

namespace {
void busyTask(void*, uint32_t) {}
}  // namespace

namespace {
uint32_t stepping_micros_value = 0;
uint32_t stepping_micros_step = 0;
uint32_t steppingMicros() {
  const uint32_t value = stepping_micros_value;
  stepping_micros_value += stepping_micros_step;
  return value;
}
}  // namespace

void test_scheduler_max_duration_holds_peak_across_runs() {
  ScheduledTask task{"work", 10, 0, busyTask, nullptr, 0};
  Scheduler scheduler(&task, 1, steppingMicros);

  stepping_micros_value = 0;
  stepping_micros_step = 200;  // each run() call measures 200us
  scheduler.run(0);
  TEST_ASSERT_EQUAL_UINT32(200u, task.last_duration_us);
  TEST_ASSERT_EQUAL_UINT32(200u, task.max_duration_us);

  stepping_micros_step = 50;  // a faster run should not lower the peak
  scheduler.run(10);
  TEST_ASSERT_EQUAL_UINT32(50u, task.last_duration_us);
  TEST_ASSERT_EQUAL_UINT32(200u, task.max_duration_us);

  stepping_micros_step = 900;  // a new peak should replace the old one
  scheduler.run(20);
  TEST_ASSERT_EQUAL_UINT32(900u, task.last_duration_us);
  TEST_ASSERT_EQUAL_UINT32(900u, task.max_duration_us);
}

void test_scheduler_overrun_only_when_budget_exceeded() {
  ScheduledTask task{"work", 10, 0, busyTask, nullptr, 0};
  task.budget_us = 500;
  Scheduler scheduler(&task, 1, steppingMicros);

  stepping_micros_value = 0;
  stepping_micros_step = 400;  // under budget
  scheduler.run(0);
  TEST_ASSERT_EQUAL_UINT32(0u, task.overrun_count);

  stepping_micros_step = 500;  // exactly at budget: not an overrun
  scheduler.run(10);
  TEST_ASSERT_EQUAL_UINT32(0u, task.overrun_count);

  stepping_micros_step = 501;  // over budget
  scheduler.run(20);
  TEST_ASSERT_EQUAL_UINT32(1u, task.overrun_count);

  stepping_micros_step = 900;  // over budget again
  scheduler.run(30);
  TEST_ASSERT_EQUAL_UINT32(2u, task.overrun_count);
}

void test_scheduler_budget_zero_never_overruns() {
  ScheduledTask task{"work", 10, 0, busyTask, nullptr, 0};
  task.budget_us = 0;  // no budget enforced
  Scheduler scheduler(&task, 1, steppingMicros);

  stepping_micros_value = 0;
  stepping_micros_step = 1000000;  // absurdly slow, but no budget to exceed
  scheduler.run(0);
  TEST_ASSERT_EQUAL_UINT32(0u, task.overrun_count);
}

void test_scheduler_no_micros_fn_leaves_duration_fields_zero() {
  ScheduledTask task{"work", 10, 0, busyTask, nullptr, 0};
  task.budget_us = 1;  // would overrun immediately if duration were measured
  Scheduler scheduler(&task, 1);  // no MicrosFn supplied

  scheduler.run(0);
  TEST_ASSERT_EQUAL_UINT32(1u, task.run_count);
  TEST_ASSERT_EQUAL_UINT32(0u, task.last_duration_us);
  TEST_ASSERT_EQUAL_UINT32(0u, task.max_duration_us);
  TEST_ASSERT_EQUAL_UINT32(0u, task.overrun_count);
}

void test_scheduler_duration_wraps_safely_across_micros_rollover() {
  ScheduledTask task{"work", 10, 0, busyTask, nullptr, 0};
  Scheduler scheduler(&task, 1, steppingMicros);

  // Start just before a uint32 micros() rollover; the callback "takes" 100us
  // and the clock wraps from 0xFFFFFFF0 past 0 to 0x00000054.
  stepping_micros_value = 0xFFFFFFF0u;
  stepping_micros_step = 100;
  scheduler.run(0);
  TEST_ASSERT_EQUAL_UINT32(100u, task.last_duration_us);
  TEST_ASSERT_EQUAL_UINT32(100u, task.max_duration_us);
}

void test_clamp_idle_delay_bounds_to_max_chunk() {
  TEST_ASSERT_EQUAL_UINT32(50u, clampIdleDelayMs(1000u, 50u));
  TEST_ASSERT_EQUAL_UINT32(30u, clampIdleDelayMs(30u, 50u));
  TEST_ASSERT_EQUAL_UINT32(50u, clampIdleDelayMs(50u, 50u));
  TEST_ASSERT_EQUAL_UINT32(0u, clampIdleDelayMs(0u, 50u));
  TEST_ASSERT_EQUAL_UINT32(1000u, clampIdleDelayMs(1000u, 0u));
  TEST_ASSERT_EQUAL_UINT32(50u, clampIdleDelayMs(UINT32_MAX, 50u));
}

void test_watchdog_timeout_ms_to_crv_matches_lfclk_formula() {
  // 8000ms default: 8000 * 32768 / 1000 - 1 = 262143.
  TEST_ASSERT_EQUAL_UINT32(262143u, watchdogTimeoutMsToCrv(8000u));
  TEST_ASSERT_EQUAL_UINT32(262143u,
                           watchdogTimeoutMsToCrv(BIKECOMP_WDT_TIMEOUT_MS));
}

void test_watchdog_timeout_ms_to_crv_clamps_below_minimum() {
  // 0ms has zero ticks, which clamps up to the register's documented floor.
  TEST_ASSERT_EQUAL_UINT32(kWatchdogMinCrv, watchdogTimeoutMsToCrv(0u));
}

void test_watchdog_timeout_ms_to_crv_clamps_above_maximum() {
  TEST_ASSERT_EQUAL_UINT32(kWatchdogMaxCrv,
                           watchdogTimeoutMsToCrv(UINT32_MAX));
}

void test_watchdog_timeout_ms_to_crv_does_not_overflow_in_32_bits() {
  // 200000ms * 32768 overflows a uint32 intermediate (wraps to a much
  // smaller value); the 64-bit intermediate must still produce the exact
  // result: 200000 * 32768 / 1000 - 1 = 6553599.
  TEST_ASSERT_EQUAL_UINT32(6553599u, watchdogTimeoutMsToCrv(200000u));
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

void test_page_carousel_weather_pages() {
  DeviceConfig config;
  config.enabled_pages_mask = 0x60u;  // weather clock + rain only
  PageCarousel carousel;
  carousel.configure(config, 0);
  TEST_ASSERT_EQUAL_UINT8(2u, carousel.pageCount());
  TEST_ASSERT_EQUAL(DisplayPage::kWeatherClock, carousel.currentPage());
  TEST_ASSERT_TRUE(carousel.update(4000));
  TEST_ASSERT_EQUAL(DisplayPage::kWeatherRain, carousel.currentPage());
}

void test_page_carousel_cadence_page() {
  DeviceConfig config;
  config.enabled_pages_mask = 0x80u;
  PageCarousel carousel;
  carousel.configure(config, 0);
  TEST_ASSERT_EQUAL_UINT8(1u, carousel.pageCount());
  TEST_ASSERT_EQUAL(DisplayPage::kCadence, carousel.currentPage());
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

void test_ambient_light_calibrator_expands_bounds_and_tracks_changed() {
  AmbientLightCalibrator calibrator;
  calibrator.configure(266, 1126);
  TEST_ASSERT_EQUAL_UINT16(266u, calibrator.rawDark());
  TEST_ASSERT_EQUAL_UINT16(1126u, calibrator.rawBright());
  TEST_ASSERT_FALSE(calibrator.changed());

  calibrator.addSample(500);  // inside current bounds, no change
  TEST_ASSERT_EQUAL_UINT16(266u, calibrator.rawDark());
  TEST_ASSERT_EQUAL_UINT16(1126u, calibrator.rawBright());
  TEST_ASSERT_FALSE(calibrator.changed());

  calibrator.addSample(150);  // new low
  TEST_ASSERT_EQUAL_UINT16(150u, calibrator.rawDark());
  TEST_ASSERT_TRUE(calibrator.changed());

  calibrator.addSample(2000);  // new high
  TEST_ASSERT_EQUAL_UINT16(2000u, calibrator.rawBright());

  calibrator.markPersisted();
  TEST_ASSERT_FALSE(calibrator.changed());

  calibrator.addSample(100);  // lower again after persisting
  TEST_ASSERT_EQUAL_UINT16(100u, calibrator.rawDark());
  TEST_ASSERT_TRUE(calibrator.changed());
}

void test_ambient_light_calibrator_rejects_rail_samples() {
  AmbientLightCalibrator calibrator;
  calibrator.configure(266, 1126);

  calibrator.addSample(0);      // presence-check-failure-style sentinel
  calibrator.addSample(4);      // at the rail margin, still rejected
  calibrator.addSample(4095);   // pinned bright rail
  calibrator.addSample(4091);   // at the rail margin, still rejected
  TEST_ASSERT_EQUAL_UINT16(266u, calibrator.rawDark());
  TEST_ASSERT_EQUAL_UINT16(1126u, calibrator.rawBright());
  TEST_ASSERT_FALSE(calibrator.changed());

  calibrator.addSample(5);      // just past the margin, admitted
  calibrator.addSample(4090);   // just past the margin, admitted
  TEST_ASSERT_EQUAL_UINT16(5u, calibrator.rawDark());
  TEST_ASSERT_EQUAL_UINT16(4090u, calibrator.rawBright());
}

void test_ambient_light_calibrator_quality_checks_width_and_absolute_bounds() {
  AmbientLightCalibrator calibrator;

  calibrator.configure(266, 1126);  // real factory default must read as kOk
  TEST_ASSERT_EQUAL(AmbientCalibrationQuality::kOk, calibrator.quality());

  calibrator.configure(1700, 2400);  // width 700 ok, but dark too high
  TEST_ASSERT_EQUAL(AmbientCalibrationQuality::kNarrow, calibrator.quality());

  calibrator.configure(200, 700);  // dark ok, but bright too low
  TEST_ASSERT_EQUAL(AmbientCalibrationQuality::kNarrow, calibrator.quality());

  calibrator.configure(500, 700);  // width 200, below minimum
  TEST_ASSERT_EQUAL(AmbientCalibrationQuality::kNarrow, calibrator.quality());

  calibrator.configure(200, 1200);  // width 1000, dark 200, bright 1200: all pass
  TEST_ASSERT_EQUAL(AmbientCalibrationQuality::kOk, calibrator.quality());

  calibrator.configure(700, 700);  // degenerate zero-width range
  TEST_ASSERT_EQUAL(AmbientCalibrationQuality::kNarrow, calibrator.quality());
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
  snapshot.battery.millivolts = 3900;

  DisplayFrame frame = DisplayFormatter::format(snapshot, DisplayPage::kTrip);
  TEST_ASSERT_EQUAL_STRING("24.8", frame.speed);
  TEST_ASSERT_EQUAL_STRING("km/h", frame.units);
  TEST_ASSERT_EQUAL_STRING("MOV TRIP 18.42 km", frame.lower);
  TEST_ASSERT_EQUAL_STRING("3.9V", frame.battery_voltage);
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

  snapshot.companion_header_valid = true;
  snprintf(snapshot.companion_header, sizeof(snapshot.companion_header),
           "14:30");
  frame = DisplayFormatter::format(snapshot, DisplayPage::kWeatherClock);
  TEST_ASSERT_EQUAL_STRING("WX 14:30", frame.lower);

  snapshot.companion_weather_valid = true;
  snprintf(snapshot.companion_weather_temp,
           sizeof(snapshot.companion_weather_temp), "+18.5C");
  snprintf(snapshot.companion_weather_rain,
           sizeof(snapshot.companion_weather_rain), "R40%%");
  frame = DisplayFormatter::format(snapshot, DisplayPage::kWeatherRain);
  TEST_ASSERT_EQUAL_STRING("+18.5C R40%", frame.lower);

  snapshot.cadence_valid = true;
  snapshot.cadence_x10 = 870;
  frame = DisplayFormatter::format(snapshot, DisplayPage::kCadence);
  TEST_ASSERT_EQUAL_STRING("MOV CAD 87.0 rpm", frame.lower);
  snapshot.cadence_valid = false;
  snapshot.csc_connected = true;
  frame = DisplayFormatter::format(snapshot, DisplayPage::kCadence);
  TEST_ASSERT_EQUAL_STRING("MOV CAD --", frame.lower);
}

void test_display_formatter_battery_and_value_limits() {
  DisplaySnapshot snapshot;
  snapshot.trip.ride_state = RideState::kPaused;
  snapshot.trip.odometer_mm = 100000000000ull;

  DisplayFrame frame = DisplayFormatter::format(snapshot, DisplayPage::kOdometer);
  TEST_ASSERT_EQUAL_STRING("PAUSE ODO 99999+ km", frame.lower);
  TEST_ASSERT_EQUAL_STRING("--V", frame.battery_voltage);
  TEST_ASSERT_EQUAL_STRING("--%", frame.battery_percent);
  TEST_ASSERT_EQUAL_UINT8(0u, frame.battery_fill_width);

  snapshot.battery.valid = true;
  snapshot.battery.percent = 255;
  snapshot.battery.millivolts = 4350;
  frame = DisplayFormatter::format(snapshot, DisplayPage::kTrip);
  TEST_ASSERT_EQUAL_STRING("4.3V", frame.battery_voltage);
  TEST_ASSERT_EQUAL_STRING("100%", frame.battery_percent);
  TEST_ASSERT_EQUAL_UINT8(8u, frame.battery_fill_width);
}

void test_display_formatter_low_battery_warning() {
  DisplaySnapshot snapshot;
  snapshot.battery.valid = true;
  snapshot.battery.percent = 18;
  snapshot.battery.millivolts = 3420;
  snapshot.battery.low_battery = true;
  DisplayFrame frame =
      DisplayFormatter::format(snapshot, DisplayPage::kTrip, true);
  TEST_ASSERT_EQUAL_STRING("3.4V", frame.battery_voltage);
  TEST_ASSERT_EQUAL_STRING("18%", frame.battery_percent);
  TEST_ASSERT_EQUAL_STRING("LOW BATT", frame.lower);
  TEST_ASSERT_TRUE(frame.low_battery_warning);
}

void test_display_formatter_ble_indicator() {
  DisplaySnapshot snapshot;
  snapshot.trip.ride_state = RideState::kIdle;

  DisplayFrame disconnected = DisplayFormatter::format(snapshot, DisplayPage::kTrip);
  TEST_ASSERT_FALSE(disconnected.ble_connected);

  snapshot.ble_connected = true;
  DisplayFrame connected = DisplayFormatter::format(snapshot, DisplayPage::kTrip);
  TEST_ASSERT_TRUE(connected.ble_connected);
}

void test_odometer_save_distance_no_longer_triggers_while_moving() {
  OdometerSavePolicy policy;
  policy.configure(500);
  policy.markSaved(0);
  policy.noteRideState(RideState::kMoving, 1000);
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kNone, policy.evaluate(500000u, 2000));
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kNone, policy.evaluate(5000000ull, 3000));
}

void test_odometer_save_paused_settle_delay_and_cancel() {
  OdometerSavePolicy policy;
  policy.configure(500);
  policy.markSaved(0);
  policy.noteRideState(RideState::kMoving, 1000);
  policy.noteRideState(RideState::kPaused, 2000);
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kNone, policy.evaluate(0, 181999));
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kPausedSettle,
                    policy.evaluate(0, 182000));

  policy.acknowledge(OdometerSaveTrigger::kPausedSettle);
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kNone, policy.evaluate(0, 40000));

  policy.noteRideState(RideState::kMoving, 41000);
  policy.noteRideState(RideState::kPaused, 42000);
  policy.noteRideState(RideState::kMoving, 43000);
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kNone, policy.evaluate(0, 80000));
}

void test_odometer_save_display_off_no_longer_triggers() {
  OdometerSavePolicy policy;
  policy.configure(500);
  policy.markSaved(1000);
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

void test_odometer_save_flash_error_keeps_paused_settle_retry() {
  MemoryStorageBackend backend;
  StorageManager storage(backend);
  TEST_ASSERT_TRUE(storage.begin());
  OdometerData loaded;
  StorageLoadInfo info;
  TEST_ASSERT_TRUE(storage.loadOdometer(loaded, info));

  OdometerSavePolicy policy;
  policy.configure(500);
  policy.markSaved(0);
  policy.noteRideState(RideState::kMoving, 1000);
  policy.noteRideState(RideState::kPaused, 2000);
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kPausedSettle,
                    policy.evaluate(500000u, 182000));

  backend.write_ok = false;
  OdometerData data{500000u, 100u};
  TEST_ASSERT_FALSE(storage.saveOdometer(data));
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kPausedSettle,
                    policy.evaluate(500000u, 182000));

  backend.write_ok = true;
  TEST_ASSERT_TRUE(storage.saveOdometer(data));
  policy.markSaved(500000u);
  policy.acknowledge(OdometerSaveTrigger::kPausedSettle);
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kNone, policy.evaluate(500000u, 182000));
}

void test_odometer_save_flash_error_keeps_oneshot_pending() {
  // Mirrors AppController: acknowledge only after Flash success, otherwise
  // one-shot triggers (critical battery) must remain pending for retry.
  OdometerSavePolicy policy;
  policy.configure(500);
  policy.markSaved(0);

  policy.noteBatteryPercent(5, true);
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kCriticalBattery, policy.evaluate(0, 0));
  // Flash failed → no acknowledge.
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kCriticalBattery, policy.evaluate(0, 0));
  policy.acknowledge(OdometerSaveTrigger::kCriticalBattery);
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kNone, policy.evaluate(0, 0));

  policy.requestDeepSleepSave();
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kDeepSleep, policy.evaluate(0, 0));
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kDeepSleep, policy.evaluate(0, 0));
  policy.acknowledge(OdometerSaveTrigger::kDeepSleep);
  TEST_ASSERT_EQUAL(OdometerSaveTrigger::kNone, policy.evaluate(0, 0));
}

void test_ambient_calibration_save_throttles_changes_and_prioritizes_one_shots() {
  AmbientCalibrationSavePolicy policy;

  // No change yet: nothing to save.
  TEST_ASSERT_EQUAL(AmbientCalibrationSaveTrigger::kNone, policy.evaluate(false, 0));

  // First observed change saves immediately (no prior save to throttle against).
  TEST_ASSERT_EQUAL(AmbientCalibrationSaveTrigger::kThrottledChange,
                    policy.evaluate(true, 0));
  policy.acknowledge(AmbientCalibrationSaveTrigger::kThrottledChange, 1000);

  // Too soon after the last save: throttled even though it changed again.
  TEST_ASSERT_EQUAL(AmbientCalibrationSaveTrigger::kNone,
                    policy.evaluate(true, 1000 + 299999u));
  // Exactly at the interval: allowed.
  TEST_ASSERT_EQUAL(AmbientCalibrationSaveTrigger::kThrottledChange,
                    policy.evaluate(true, 1000 + 300000u));

  // No change at all: never saves regardless of elapsed time.
  TEST_ASSERT_EQUAL(AmbientCalibrationSaveTrigger::kNone,
                    policy.evaluate(false, 1000 + 999999u));
}

void test_ambient_calibration_save_one_shot_triggers_and_usb_disconnect() {
  AmbientCalibrationSavePolicy policy;

  policy.requestDeepSleepSave();
  TEST_ASSERT_EQUAL(AmbientCalibrationSaveTrigger::kDeepSleep,
                    policy.evaluate(false, 0));
  policy.requestRebootSave();
  // Reboot outranks a still-pending deep sleep request.
  TEST_ASSERT_EQUAL(AmbientCalibrationSaveTrigger::kReboot, policy.evaluate(false, 0));
  policy.acknowledge(AmbientCalibrationSaveTrigger::kReboot, 0);
  // Deep sleep request is still pending after acknowledging reboot only.
  TEST_ASSERT_EQUAL(AmbientCalibrationSaveTrigger::kDeepSleep,
                    policy.evaluate(false, 0));
  policy.acknowledge(AmbientCalibrationSaveTrigger::kDeepSleep, 0);
  TEST_ASSERT_EQUAL(AmbientCalibrationSaveTrigger::kNone, policy.evaluate(false, 0));

  // First noteUsbPresent call only establishes the baseline, no trigger.
  policy.noteUsbPresent(true);
  TEST_ASSERT_EQUAL(AmbientCalibrationSaveTrigger::kNone, policy.evaluate(false, 0));
  policy.noteUsbPresent(false);
  TEST_ASSERT_EQUAL(AmbientCalibrationSaveTrigger::kUsbDisconnect,
                    policy.evaluate(false, 0));

  // Deep sleep outranks a still-pending usb disconnect request.
  policy.requestDeepSleepSave();
  TEST_ASSERT_EQUAL(AmbientCalibrationSaveTrigger::kDeepSleep,
                    policy.evaluate(false, 0));
  policy.acknowledge(AmbientCalibrationSaveTrigger::kDeepSleep, 0);
  // Usb disconnect request is still pending after acknowledging deep sleep only.
  TEST_ASSERT_EQUAL(AmbientCalibrationSaveTrigger::kUsbDisconnect,
                    policy.evaluate(false, 0));

  // Usb disconnect outranks a throttled change, even when calibration_changed
  // is true.
  TEST_ASSERT_EQUAL(AmbientCalibrationSaveTrigger::kUsbDisconnect,
                    policy.evaluate(true, 0));

  policy.acknowledge(AmbientCalibrationSaveTrigger::kUsbDisconnect, 0);
  TEST_ASSERT_EQUAL(AmbientCalibrationSaveTrigger::kNone, policy.evaluate(false, 0));
}

void test_ambient_calibration_save_usb_absent_baseline_first_call() {
  AmbientCalibrationSavePolicy policy;

  // First-ever noteUsbPresent call establishes "absent" as the baseline;
  // it must not be treated as a disconnect transition.
  policy.noteUsbPresent(false);
  TEST_ASSERT_EQUAL(AmbientCalibrationSaveTrigger::kNone, policy.evaluate(false, 0));

  // A genuine later disconnect still triggers normally.
  policy.noteUsbPresent(true);
  policy.noteUsbPresent(false);
  TEST_ASSERT_EQUAL(AmbientCalibrationSaveTrigger::kUsbDisconnect,
                    policy.evaluate(false, 0));
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

void test_classify_deep_sleep_wake_source() {
  TEST_ASSERT_EQUAL(DeepSleepWakeSource::kHall,
                    classifyDeepSleepWakeSource(kNrfResetReasonOff));
  TEST_ASSERT_EQUAL(DeepSleepWakeSource::kUsb,
                    classifyDeepSleepWakeSource(kNrfResetReasonVbus));
  // OFF takes priority when both are latched (matches the old direct-register
  // check's OFF-then-VBUS order).
  TEST_ASSERT_EQUAL(
      DeepSleepWakeSource::kHall,
      classifyDeepSleepWakeSource(kNrfResetReasonOff | kNrfResetReasonVbus));
  TEST_ASSERT_EQUAL(DeepSleepWakeSource::kNone,
                    classifyDeepSleepWakeSource(kNrfResetReasonPin));
  TEST_ASSERT_EQUAL(DeepSleepWakeSource::kNone, classifyDeepSleepWakeSource(0u));
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
  TEST_ASSERT_EQUAL_UINT8(kTelemetryStructVersion, packet.struct_version);
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

void test_map_telemetry_power_state_extended() {
  TelemetryBuildInput input;
  input.battery.charge_status = ChargeStatus::kCharging;
  TEST_ASSERT_EQUAL(PowerState::kCharging, mapTelemetryPowerState(input));

  input.battery.charge_status = ChargeStatus::kNotCharging;
  input.ble_connected = true;
  TEST_ASSERT_EQUAL(PowerState::kBleConfig, mapTelemetryPowerState(input));

  input.ble_connected = false;
  input.deep_sleep_pending = true;
  TEST_ASSERT_EQUAL(PowerState::kDeepSleepPending,
                    mapTelemetryPowerState(input));

  input.deep_sleep_pending = false;
  input.display_on = false;
  TEST_ASSERT_EQUAL(PowerState::kIdleDisplayOff,
                    mapTelemetryPowerState(input));

  input.display_on = true;
  input.trip.ride_state = RideState::kPaused;
  TEST_ASSERT_EQUAL(PowerState::kShortStop, mapTelemetryPowerState(input));

  input.trip.ride_state = RideState::kMoving;
  TEST_ASSERT_EQUAL(PowerState::kActive, mapTelemetryPowerState(input));
}

void test_power_manager_power_save_and_timeout() {
  PowerManager manager;
  PowerManagerConfig config;
  config.power_save_mode = true;
  config.deep_sleep_enabled = false;
  config.deep_sleep_timeout_s = 900;
  manager.configure(config);

  PowerManagerInput input;
  input.display_power = DisplayPowerState::kOff;
  input.now_ms = 1000;
  PowerManagerUpdateResult result = manager.update(input);
  TEST_ASSERT_TRUE(result.mode_changed);
  TEST_ASSERT_EQUAL(SystemPowerMode::kLowPowerIdle, manager.systemMode());
  TEST_ASSERT_FALSE(manager.deepSleepArmed());

  manager.noteActivity(2000);
  TEST_ASSERT_EQUAL(SystemPowerMode::kNormal, manager.systemMode());

  config.power_save_mode = false;
  config.deep_sleep_timeout_s = 60;
  manager.configure(config);
  input.now_ms = 0;
  manager.update(input);
  TEST_ASSERT_EQUAL(SystemPowerMode::kNormal, manager.systemMode());

  input.now_ms = 59000;
  manager.update(input);
  TEST_ASSERT_EQUAL(SystemPowerMode::kNormal, manager.systemMode());

  input.now_ms = 60000;
  result = manager.update(input);
  TEST_ASSERT_EQUAL(SystemPowerMode::kLowPowerIdle, manager.systemMode());
  TEST_ASSERT_TRUE(manager.deepSleepArmed());
}

void test_power_manager_deep_sleep_timeout_zero_and_ble_block() {
  PowerManager manager;
  PowerManagerConfig config;
  config.deep_sleep_timeout_s = 0;
  manager.configure(config);

  PowerManagerInput input;
  input.display_power = DisplayPowerState::kOff;
  input.now_ms = 100000;
  manager.update(input);
  TEST_ASSERT_EQUAL(SystemPowerMode::kNormal, manager.systemMode());

  config.deep_sleep_timeout_s = 60;
  manager.configure(config);
  input.ble_connected = true;
  manager.update(input);
  TEST_ASSERT_EQUAL(SystemPowerMode::kNormal, manager.systemMode());
  TEST_ASSERT_EQUAL(PowerSleepBlockReason::kBleConnected,
                    manager.blockReason());
}

void test_power_manager_deep_sleep_save_request() {
  PowerManager manager;
  PowerManagerConfig config;
  config.deep_sleep_enabled = true;
  config.deep_sleep_timeout_s = 10;
  manager.configure(config);

  PowerManagerInput input;
  input.display_power = DisplayPowerState::kOff;
  input.now_ms = 0;
  manager.update(input);
  PowerManagerUpdateResult result = manager.update(input);
  TEST_ASSERT_FALSE(result.request_deep_sleep_save);

  input.now_ms = 10000;
  result = manager.update(input);
  TEST_ASSERT_TRUE(result.request_deep_sleep_save);
  TEST_ASSERT_TRUE(manager.deepSleepArmed());
}

struct MockUsbTestState {
  uint32_t speed_x100 = 0;
  uint32_t revolutions = 0;
  uint32_t accepted = 0;
  bool smooth = false;
  bool power_save = false;
  DisplayPowerState display = DisplayPowerState::kBright;
};

void mockUsbReset(void* context, uint32_t now_ms) {
  auto* state = static_cast<MockUsbTestState*>(context);
  (void)now_ms;
  state->speed_x100 = 0;
  state->revolutions = 0;
  state->accepted = 0;
}

bool mockUsbInject(void* context,
                   uint32_t interval_us,
                   uint32_t now_ms,
                   char* detail,
                   size_t detail_len) {
  auto* state = static_cast<MockUsbTestState*>(context);
  (void)interval_us;
  (void)now_ms;
  ++state->accepted;
  ++state->revolutions;
  if (state->accepted > 1) state->speed_x100 = 3600;
  snprintf(detail, detail_len, "interval=%lu", static_cast<unsigned long>(interval_us));
  return true;
}

void mockUsbSmooth(void* context, bool enabled) {
  static_cast<MockUsbTestState*>(context)->smooth = enabled;
}

void mockUsbSnapshot(void* context, UsbTestSnapshot* out) {
  const auto* state = static_cast<const MockUsbTestState*>(context);
  out->speed_x100 = static_cast<uint16_t>(state->speed_x100);
  out->revolutions = state->revolutions;
  out->ride_state = RideState::kMoving;
  out->accepted_pulses = state->accepted;
}

bool mockUsbPowerFixture(void* context,
                         DisplayPowerState display_power,
                         uint32_t now_ms) {
  auto* state = static_cast<MockUsbTestState*>(context);
  (void)now_ms;
  state->display = display_power;
  return true;
}

void mockUsbUpdatePower(void* context, uint32_t now_ms) {
  auto* state = static_cast<MockUsbTestState*>(context);
  (void)now_ms;
  if (state->power_save && state->display == DisplayPowerState::kOff) {
    state->speed_x100 = 0;
  }
}

void mockUsbSetPowerSave(void* context, bool enabled) {
  static_cast<MockUsbTestState*>(context)->power_save = enabled;
}

void test_serial_usb_test_speed_and_expect() {
  MockUsbTestState state;
  UsbTestHooks hooks = {};
  hooks.context = &state;
  hooks.reset = mockUsbReset;
  hooks.inject_pulse = mockUsbInject;
  hooks.set_smoothing = mockUsbSmooth;
  hooks.snapshot = mockUsbSnapshot;
  hooks.set_power_fixture = mockUsbPowerFixture;
  hooks.update_power = mockUsbUpdatePower;
  hooks.set_power_save = mockUsbSetPowerSave;

  UsbTestResult reset = handleUsbTestLine("reset", hooks, 0);
  TEST_ASSERT_EQUAL(UsbTestStatus::kOk, reset.status);

  UsbTestResult pulse = handleUsbTestLine("pulse 210000", hooks, 0);
  TEST_ASSERT_EQUAL(UsbTestStatus::kOk, pulse.status);

  UsbTestResult expect_ok = handleUsbTestLine("expect revolutions 1", hooks, 0);
  TEST_ASSERT_EQUAL(UsbTestStatus::kOk, expect_ok.status);

  UsbTestResult expect_bad = handleUsbTestLine("expect speed_x100 9999", hooks, 0);
  TEST_ASSERT_EQUAL(UsbTestStatus::kFail, expect_bad.status);
}

void test_scheduler_next_due_and_set_period() {
  int calls = 0;
  auto callback = [](void* context, uint32_t) {
    ++(*static_cast<int*>(context));
  };
  ScheduledTask tasks[] = {{"task", 100, 0, callback, &calls, 0}};
  Scheduler scheduler(tasks, 1);
  scheduler.run(0);
  TEST_ASSERT_EQUAL(1, calls);
  TEST_ASSERT_EQUAL_UINT32(100u, scheduler.nextDueMs(0));
  scheduler.setTaskPeriod(0, 200, 50);
  TEST_ASSERT_EQUAL_UINT32(50u, scheduler.nextDueMs(50));
  scheduler.run(50);
  TEST_ASSERT_EQUAL_UINT32(250u, scheduler.nextDueMs(50));
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
  TEST_ASSERT_EQUAL_UINT32(36u, kTelemetryV1Size);
  TEST_ASSERT_EQUAL_UINT32(44u, kTelemetrySize);
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
  TEST_ASSERT_EQUAL_STRING_LEN(BIKECOMP_FW_VERSION, decoded.fw_version,
                               strlen(BIKECOMP_FW_VERSION));
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
  TEST_ASSERT_EQUAL_UINT8_ARRAY(moving_hex.data(), encoded, moving_hex.size());
  encodeTelemetry(makePausedTelemetry(), encoded);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(paused_hex.data(), encoded, paused_hex.size());

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

void test_companion_snapshot_fixture_roundtrip() {
  std::vector<uint8_t> fixture_hex;
  TEST_ASSERT_TRUE(loadFixtureHex("companion_v1_nominal", fixture_hex));
  CompanionSnapshotPacket decoded = {};
  TEST_ASSERT_TRUE(
      decodeCompanionSnapshot(fixture_hex.data(), fixture_hex.size(), decoded));
  TEST_ASSERT_EQUAL_UINT8(1u, decoded.struct_version);
  TEST_ASSERT_EQUAL_UINT32(1704067200u, decoded.unix_time);
  TEST_ASSERT_EQUAL_INT16(180, decoded.tz_offset_min);
  TEST_ASSERT_EQUAL_INT16(185, decoded.temp_c_x10);
  TEST_ASSERT_EQUAL_UINT8(40u, decoded.pop_pct);
  TEST_ASSERT_EQUAL_UINT8(0x03u, decoded.flags);
  TEST_ASSERT_EQUAL_UINT32(1704070800u, decoded.valid_until);

  uint8_t encoded[kCompanionSnapshotSize] = {};
  encodeCompanionSnapshot(decoded, encoded);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(fixture_hex.data(), encoded, kCompanionSnapshotSize);
}

void test_companion_clock_and_header() {
  CompanionSnapshotPacket packet = {};
  packet.struct_version = 1;
  packet.unix_time = 1704067200u;
  packet.tz_offset_min = 180;
  packet.temp_c_x10 = 185;
  packet.pop_pct = 40;
  packet.flags = kCompanionFlagTimeValid | kCompanionFlagWeatherValid;
  packet.valid_until = 1704070800u;

  CompanionState state;
  state.apply(packet, 1000u);
  const CompanionHeaderView header = state.header(1000u);
  TEST_ASSERT_TRUE(header.valid);
  TEST_ASSERT_FALSE(header.stale);
  TEST_ASSERT_EQUAL_STRING("03:00", header.text);

  const CompanionWeatherView weather = state.weather(1000u);
  TEST_ASSERT_TRUE(weather.valid);
  TEST_ASSERT_FALSE(weather.stale);
  TEST_ASSERT_EQUAL_STRING("+18.5C", weather.temp);
  TEST_ASSERT_EQUAL_STRING("R40%", weather.rain);

  const CompanionHeaderView stale = state.header(3700000u);
  TEST_ASSERT_TRUE(stale.stale);
  const CompanionWeatherView stale_weather = state.weather(3700000u);
  TEST_ASSERT_TRUE(stale_weather.stale);
}

void test_protocol_codec_rejects_bad_length_and_version() {
  uint8_t telemetry[kTelemetrySize] = {};
  encodeTelemetry(makeMovingTelemetry(), telemetry);
  TelemetryPacket decoded_telemetry = {};
  telemetry[0] = 0;
  TEST_ASSERT_FALSE(
      decodeTelemetry(telemetry, sizeof(telemetry), decoded_telemetry));
  telemetry[0] = 1;
  TEST_ASSERT_FALSE(
      decodeTelemetry(telemetry, kTelemetryV1Size - 1, decoded_telemetry));
  TEST_ASSERT_TRUE(
      decodeTelemetry(telemetry, kTelemetryV1Size, decoded_telemetry));
  telemetry[0] = 2;
  TEST_ASSERT_TRUE(
      decodeTelemetry(telemetry, sizeof(telemetry), decoded_telemetry));

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

void test_csc_parses_c3_cadence_and_s3_speed_and_computes_motion() {
  TEST_ASSERT_TRUE(cscNameLooksLikeCycplus("CYCPLUS C3"));
  TEST_ASSERT_TRUE(cscNameLooksLikeCadenceSensor("CYCPLUS C3"));
  TEST_ASSERT_TRUE(cscNameLooksLikeSpeedSensor("CYCPLUS S3"));
  TEST_ASSERT_FALSE(cscNameLooksLikeSpeedSensor("CYCPLUS C3"));

  const uint8_t cadence_pkt[] = {0x02, 0x0A, 0x00, 0x00, 0x04};
  CscMeasurement meas = {};
  TEST_ASSERT_TRUE(parseCscMeasurement(cadence_pkt, sizeof(cadence_pkt), meas));
  TEST_ASSERT_FALSE(meas.wheel_present);
  TEST_ASSERT_TRUE(meas.crank_present);
  TEST_ASSERT_EQUAL_UINT16(10u, meas.cumulative_crank_revolutions);
  TEST_ASSERT_EQUAL_UINT16(1024u, meas.last_crank_event_time);

  CscMotionTracker tracker;
  tracker.ingest(meas, 1000);
  meas.cumulative_crank_revolutions = 11;
  meas.last_crank_event_time = 1024 + 1024;
  tracker.ingest(meas, 2000);
  TEST_ASSERT_TRUE(tracker.cadenceValid());
  TEST_ASSERT_EQUAL_UINT16(600u, tracker.cadenceX10());  // 60.0 rpm
  tracker.poll(2000);
  TEST_ASSERT_TRUE(tracker.cadenceValid());
  tracker.poll(2000 + kCscStaleMs);
  TEST_ASSERT_FALSE(tracker.cadenceValid());

  const uint8_t speed_pkt[] = {0x01, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00};
  TEST_ASSERT_TRUE(parseCscMeasurement(speed_pkt, sizeof(speed_pkt), meas));
  TEST_ASSERT_TRUE(meas.wheel_present);
  TEST_ASSERT_FALSE(meas.crank_present);
  TEST_ASSERT_EQUAL_UINT32(100u, meas.cumulative_wheel_revolutions);

  tracker.reset();
  tracker.ingest(meas, 1000);
  CscWheelDelta delta = tracker.takeWheelDelta();
  TEST_ASSERT_EQUAL_UINT8(0u, delta.revolutions);
  meas.cumulative_wheel_revolutions = 102;
  meas.last_wheel_event_time = 1024;
  tracker.ingest(meas, 1500);
  delta = tracker.takeWheelDelta();
  TEST_ASSERT_EQUAL_UINT8(2u, delta.revolutions);
  TEST_ASSERT_EQUAL_UINT32(500000u, delta.mean_interval_us);
  TEST_ASSERT_TRUE(tracker.speedSourceActive(2000));
  TEST_ASSERT_FALSE(tracker.speedSourceActive(1500 + kCscStaleMs));
  TEST_ASSERT_EQUAL_UINT32(500000u, cscMeanIntervalUs(1024, 2));

  tracker.ingest(meas, 1600);
  tracker.onDisconnect();
  TEST_ASSERT_TRUE(tracker.speedSourceActive(2000));
  meas.cumulative_wheel_revolutions = 200;
  meas.last_wheel_event_time = 3000;
  tracker.ingest(meas, 2500);
  delta = tracker.takeWheelDelta();
  TEST_ASSERT_EQUAL_UINT8(0u, delta.revolutions);
  meas.cumulative_wheel_revolutions = 201;
  meas.last_wheel_event_time = 3000 + 1024;
  tracker.ingest(meas, 2600);
  delta = tracker.takeWheelDelta();
  TEST_ASSERT_EQUAL_UINT8(1u, delta.revolutions);
}

void test_csc_bond_round_trip_and_telemetry_v2_fields() {
  CscBondData bond = {};
  bond.address[0] = 0x11;
  bond.address[5] = 0x66;
  bond.address_type = 1;
  bond.flags = kCscBondFlagValid;
  memcpy(bond.name, "CYCPLUS S3", 10);
  uint8_t payload[kCscBondPayloadSize];
  encodeCscBond(bond, payload);
  CscBondData decoded = {};
  TEST_ASSERT_TRUE(decodeCscBond(payload, sizeof(payload), decoded));
  TEST_ASSERT_TRUE(cscBondIsValid(decoded));
  TEST_ASSERT_EQUAL_UINT8(0x11, decoded.address[0]);
  TEST_ASSERT_EQUAL_UINT8(0x66, decoded.address[5]);
  TEST_ASSERT_EQUAL_STRING("CYCPLUS S3", decoded.name);

  TelemetryBuildInput input;
  input.cadence_x10 = 870;
  input.csc_flags = kTelemetryCscFlagConnected | kTelemetryCscFlagCrankPresent |
                    kTelemetryCscFlagCadenceValid;
  input.last_crank_event_age_ms = 500;
  TelemetryPacket packet = {};
  fillTelemetryPacket(packet, input);
  uint8_t encoded[kTelemetrySize];
  encodeTelemetry(packet, encoded);
  TelemetryPacket roundtrip = {};
  TEST_ASSERT_TRUE(decodeTelemetry(encoded, sizeof(encoded), roundtrip));
  TEST_ASSERT_EQUAL_UINT8(kTelemetryStructVersion, roundtrip.struct_version);
  TEST_ASSERT_EQUAL_UINT16(870u, roundtrip.cadence_x10);
  TEST_ASSERT_EQUAL_UINT8(input.csc_flags, roundtrip.csc_flags);
  TEST_ASSERT_EQUAL_UINT32(500u, roundtrip.last_crank_event_age_ms);
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
  RUN_TEST(test_ambient_calibration_encode_decode_round_trip);
  RUN_TEST(test_memory_backend_async_write_completes_after_configured_polls);
  RUN_TEST(test_storage_async_save_completes_over_multiple_polls);
  RUN_TEST(test_storage_async_save_serializes_concurrent_begin);
  RUN_TEST(test_storage_async_drain_pending_save_blocks_to_completion);
  RUN_TEST(test_storage_async_dedup_skip_completes_synchronously);
  RUN_TEST(test_storage_async_write_error_propagates_through_poll);
  RUN_TEST(test_storage_async_failed_odometer_save_does_not_leak_sequence_into_unrelated_write);
  RUN_TEST(test_storage_async_drain_gives_up_on_a_backend_that_never_completes);
  RUN_TEST(test_storage_async_sync_save_rejected_while_another_save_in_progress);
  RUN_TEST(test_storage_counters_encode_decode_round_trip);
  RUN_TEST(test_ambient_calibration_defaults_without_flash_write_then_alternates_slots);
  RUN_TEST(test_storage_counters_absent_record_defaults_without_flash_write);
  RUN_TEST(test_storage_counters_skip_persist_when_only_skipped_writes_changed);
  RUN_TEST(test_storage_counters_persist_across_reboot);
  RUN_TEST(test_storage_counters_recovers_from_corrupt_slot_and_keeps_read_error);
  RUN_TEST(test_migrate_config_v1_to_v2_preserves_fields);
  RUN_TEST(test_migrate_odometer_v1_to_v2_preserves_totals);
  RUN_TEST(test_storage_migrates_config_v1_fixture_and_rewrites_v2);
  RUN_TEST(test_storage_migrates_odometer_v1_and_rejects_future_version);
  RUN_TEST(test_pulse_filter_first_debounce_and_overspeed);
  RUN_TEST(test_pulse_filter_stuck_and_micros_wrap);
  RUN_TEST(test_speed_fixed_point_smoothing_and_timeout);
  RUN_TEST(test_speed_smoothing_windows_two_and_five);
  RUN_TEST(test_speed_boundary_circumferences);
  RUN_TEST(test_speed_gap_long_interval_corrected);
  RUN_TEST(test_speed_gap_short_spike_corrected);
  RUN_TEST(test_speed_gap_pause_recovery_uses_new_cadence);
  RUN_TEST(test_speed_gap_revolutions_unaffected);
  RUN_TEST(test_speed_gap_reset_clears_guard);
  RUN_TEST(test_speed_gap_mid_ride_spike_corrected);
  RUN_TEST(test_speed_gap_exact_half_interval_corrected);
  RUN_TEST(test_pulse_to_speed_pipeline_sets_max_speed);
  RUN_TEST(test_pulse_filter_rejects_zero_interval);
  RUN_TEST(test_trip_accumulation_average_and_reset);
  RUN_TEST(test_trip_restores_only_persistent_totals);
  RUN_TEST(test_trip_test_revolution_skips_persistent_totals);
  RUN_TEST(test_trip_restore_snapshot_reverts_session);
  RUN_TEST(test_ride_state_transitions_and_paused_time);
  RUN_TEST(test_scheduler_period_and_wrap);
  RUN_TEST(test_scheduler_next_due_ms_wrap_safe_picks_soonest_task);
  RUN_TEST(test_scheduler_max_duration_holds_peak_across_runs);
  RUN_TEST(test_scheduler_overrun_only_when_budget_exceeded);
  RUN_TEST(test_scheduler_budget_zero_never_overruns);
  RUN_TEST(test_scheduler_no_micros_fn_leaves_duration_fields_zero);
  RUN_TEST(test_scheduler_duration_wraps_safely_across_micros_rollover);
  RUN_TEST(test_clamp_idle_delay_bounds_to_max_chunk);
  RUN_TEST(test_watchdog_timeout_ms_to_crv_matches_lfclk_formula);
  RUN_TEST(test_watchdog_timeout_ms_to_crv_clamps_below_minimum);
  RUN_TEST(test_watchdog_timeout_ms_to_crv_clamps_above_maximum);
  RUN_TEST(test_watchdog_timeout_ms_to_crv_does_not_overflow_in_32_bits);
  RUN_TEST(test_serial_console_parses_supported_commands_and_crlf);
  RUN_TEST(test_serial_console_trims_rejects_and_recovers_after_overflow);
  RUN_TEST(test_serial_console_parses_status_command);
  RUN_TEST(test_serial_console_parses_ambient_commands);
  RUN_TEST(test_serial_console_parses_display_commands);
  RUN_TEST(test_serial_console_parses_hall_commands);
  RUN_TEST(test_serial_console_parses_hall_analog_commands);
  RUN_TEST(test_serial_console_parses_gpio_commands);
  RUN_TEST(test_serial_console_parses_csc_commands);
  RUN_TEST(test_error_log_keeps_16_and_snapshots_newest_four_in_order);
  RUN_TEST(test_ble_advertising_policy_timeout_and_movement_restart);
  RUN_TEST(test_power_manager_ble_always_advertise_overrides_power_save);
  RUN_TEST(test_page_carousel_default_period_and_wrap);
  RUN_TEST(test_page_carousel_mask_order_and_fallback);
  RUN_TEST(test_page_carousel_pinned_page);
  RUN_TEST(test_page_carousel_weather_pages);
  RUN_TEST(test_page_carousel_cadence_page);
  RUN_TEST(test_display_power_dim_off_wake_disable_and_wrap);
  RUN_TEST(test_display_burn_in_guard_cycles_catches_up_and_wraps);
  RUN_TEST(test_battery_raw_conversion_and_calibration);
  RUN_TEST(test_ambient_light_model_normalizes_levels_caps_and_contrast);
  RUN_TEST(test_ambient_light_model_ema_hysteresis_dwell_and_invalid_fallback);
  RUN_TEST(test_ambient_light_calibrator_expands_bounds_and_tracks_changed);
  RUN_TEST(test_ambient_light_calibrator_rejects_rail_samples);
  RUN_TEST(test_ambient_light_calibrator_quality_checks_width_and_absolute_bounds);
  RUN_TEST(test_battery_soc_table_and_interpolation);
  RUN_TEST(test_battery_ema_monotonicity_and_usb_growth);
  RUN_TEST(test_battery_low_threshold_hysteresis);
  RUN_TEST(test_display_formatter_all_pages_and_battery);
  RUN_TEST(test_display_formatter_battery_and_value_limits);
  RUN_TEST(test_display_formatter_low_battery_warning);
  RUN_TEST(test_display_formatter_ble_indicator);
  RUN_TEST(test_odometer_save_distance_no_longer_triggers_while_moving);
  RUN_TEST(test_odometer_save_paused_settle_delay_and_cancel);
  RUN_TEST(test_odometer_save_display_off_no_longer_triggers);
  RUN_TEST(test_odometer_save_critical_battery_once_and_usb_reboot);
  RUN_TEST(test_odometer_save_unchanged_skips_sequence_growth);
  RUN_TEST(test_odometer_save_flash_error_keeps_paused_settle_retry);
  RUN_TEST(test_odometer_save_flash_error_keeps_oneshot_pending);
  RUN_TEST(test_ambient_calibration_save_throttles_changes_and_prioritizes_one_shots);
  RUN_TEST(test_ambient_calibration_save_one_shot_triggers_and_usb_disconnect);
  RUN_TEST(test_ambient_calibration_save_usb_absent_baseline_first_call);
  RUN_TEST(test_ble_identity_resolves_placeholder_name);
  RUN_TEST(test_map_nrf_reset_reason_priority);
  RUN_TEST(test_classify_deep_sleep_wake_source);
  RUN_TEST(test_pairing_window_and_device_info_flags);
  RUN_TEST(test_boot_count_increments_and_persists);
  RUN_TEST(test_telemetry_publish_mode_and_intervals);
  RUN_TEST(test_fill_telemetry_packet_moving_and_idle);
  RUN_TEST(test_map_telemetry_power_state_extended);
  RUN_TEST(test_power_manager_power_save_and_timeout);
  RUN_TEST(test_power_manager_deep_sleep_timeout_zero_and_ble_block);
  RUN_TEST(test_power_manager_deep_sleep_save_request);
  RUN_TEST(test_serial_usb_test_speed_and_expect);
  RUN_TEST(test_scheduler_next_due_and_set_period);
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
  RUN_TEST(test_companion_snapshot_fixture_roundtrip);
  RUN_TEST(test_companion_clock_and_header);
  RUN_TEST(test_protocol_codec_rejects_bad_length_and_version);
  RUN_TEST(test_csc_parses_c3_cadence_and_s3_speed_and_computes_motion);
  RUN_TEST(test_csc_bond_round_trip_and_telemetry_v2_fields);
  return UNITY_END();
}
