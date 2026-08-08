#include "storage_manager.h"

#include <string.h>

#include "config_codec.h"
#include "config_validator.h"
#include "crc32.h"
#include "storage_migration.h"

namespace bike {
namespace {

void writeU16(uint8_t* output, uint16_t value) {
  output[0] = static_cast<uint8_t>(value);
  output[1] = static_cast<uint8_t>(value >> 8u);
}

void writeU32(uint8_t* output, uint32_t value) {
  for (uint8_t i = 0; i < 4; ++i) {
    output[i] = static_cast<uint8_t>(value >> (8u * i));
  }
}

void writeU64(uint8_t* output, uint64_t value) {
  for (uint8_t i = 0; i < 8; ++i) {
    output[i] = static_cast<uint8_t>(value >> (8u * i));
  }
}

uint16_t readU16(const uint8_t* input) {
  return static_cast<uint16_t>(input[0]) |
         static_cast<uint16_t>(input[1]) << 8u;
}

uint32_t readU32(const uint8_t* input) {
  uint32_t value = 0;
  for (uint8_t i = 0; i < 4; ++i) {
    value |= static_cast<uint32_t>(input[i]) << (8u * i);
  }
  return value;
}

uint64_t readU64(const uint8_t* input) {
  uint64_t value = 0;
  for (uint8_t i = 0; i < 8; ++i) {
    value |= static_cast<uint64_t>(input[i]) << (8u * i);
  }
  return value;
}

bool isNewer(uint32_t candidate, uint32_t reference) {
  const uint32_t difference = candidate - reference;
  return difference != 0 && difference < 0x80000000u;
}

}  // namespace

size_t encodeRecord(const uint8_t* payload,
                    size_t payload_length,
                    uint16_t version,
                    uint32_t sequence,
                    uint8_t* output,
                    size_t output_capacity) {
  if (payload == nullptr || output == nullptr || payload_length > UINT16_MAX ||
      output_capacity < kRecordHeaderSize + payload_length) {
    return 0;
  }
  writeU32(output, kRecordMagic);
  writeU16(output + 4, version);
  writeU16(output + 6, static_cast<uint16_t>(payload_length));
  writeU32(output + 8, sequence);
  writeU32(output + 12, crc32(payload, payload_length));
  memcpy(output + kRecordHeaderSize, payload, payload_length);
  return kRecordHeaderSize + payload_length;
}

bool inspectRecord(const uint8_t* record,
                   size_t record_length,
                   DecodedRecord& decoded) {
  if (record == nullptr || record_length < kRecordHeaderSize) return false;
  RecordHeader header;
  header.magic = readU32(record);
  header.version = readU16(record + 4);
  header.payload_len = readU16(record + 6);
  header.sequence = readU32(record + 8);
  header.crc32 = readU32(record + 12);
  if (header.magic != kRecordMagic) return false;
  if (record_length != kRecordHeaderSize + header.payload_len) return false;
  const uint8_t* payload = record + kRecordHeaderSize;
  if (crc32(payload, header.payload_len) != header.crc32) return false;
  decoded.header = header;
  decoded.payload = payload;
  return true;
}

bool decodeRecord(const uint8_t* record,
                  size_t record_length,
                  uint16_t expected_version,
                  size_t expected_payload_length,
                  DecodedRecord& decoded) {
  if (!inspectRecord(record, record_length, decoded)) return false;
  return decoded.header.version == expected_version &&
         decoded.header.payload_len == expected_payload_length;
}

void encodeOdometer(const OdometerData& odometer,
                    uint8_t output[kOdometerPayloadSize]) {
  writeU64(output, odometer.odometer_mm);
  writeU64(output + 8, odometer.total_revolutions);
}

bool decodeOdometer(const uint8_t* input,
                    size_t length,
                    OdometerData& odometer) {
  if (input == nullptr || length != kOdometerPayloadSize) return false;
  odometer.odometer_mm = readU64(input);
  odometer.total_revolutions = readU64(input + 8);
  return true;
}

struct StorageManager::Slot {
  bool present = false;
  bool valid = false;
  bool needs_migration = false;
  uint16_t version = 0;
  uint32_t sequence = 0;
  uint8_t payload[kDeviceConfigPayloadSize] = {};
};

StorageManager::StorageManager(StorageBackend& backend,
                               const StoragePaths& paths)
    : backend_(backend), paths_(paths) {}

bool StorageManager::begin() {
  mounted_ = backend_.begin();
  if (!mounted_) ++counters_.read_errors;
  return mounted_;
}

void StorageManager::readConfigSlot(const char* path, Slot& slot) {
  slot = Slot{};
  uint8_t record[kMaximumRecordSize];
  size_t record_length = 0;
  const StorageIoResult result =
      backend_.read(path, record, sizeof(record), record_length);
  if (result == StorageIoResult::kNotFound) return;
  slot.present = true;
  if (result != StorageIoResult::kOk) {
    ++counters_.read_errors;
    return;
  }

  DecodedRecord decoded;
  if (!inspectRecord(record, record_length, decoded)) {
    ++counters_.read_errors;
    return;
  }
  if (decoded.header.version > kConfigRecordVersion ||
      decoded.header.payload_len !=
          configPayloadLengthForVersion(decoded.header.version)) {
    ++counters_.read_errors;
    return;
  }

  DeviceConfig config;
  if (!migrateConfigToCurrent(decoded.header.version, decoded.payload,
                              decoded.header.payload_len, config)) {
    ++counters_.read_errors;
    return;
  }
  if (ConfigValidator::validate(config) != ConfigValidationError::kNone) {
    ++counters_.read_errors;
    return;
  }
  encodeDeviceConfig(config, slot.payload);
  slot.sequence = decoded.header.sequence;
  slot.version = decoded.header.version;
  slot.needs_migration = decoded.header.version < kConfigRecordVersion;
  slot.valid = true;
}

void StorageManager::readOdometerSlot(const char* path, Slot& slot) {
  slot = Slot{};
  uint8_t record[kMaximumRecordSize];
  size_t record_length = 0;
  const StorageIoResult result =
      backend_.read(path, record, sizeof(record), record_length);
  if (result == StorageIoResult::kNotFound) return;
  slot.present = true;
  if (result != StorageIoResult::kOk) {
    ++counters_.read_errors;
    return;
  }

  DecodedRecord decoded;
  if (!inspectRecord(record, record_length, decoded)) {
    ++counters_.read_errors;
    return;
  }
  if (decoded.header.version > kOdometerRecordVersion ||
      decoded.header.payload_len !=
          odometerPayloadLengthForVersion(decoded.header.version)) {
    ++counters_.read_errors;
    return;
  }

  OdometerData odometer;
  if (!migrateOdometerToCurrent(decoded.header.version, decoded.payload,
                                decoded.header.payload_len, odometer)) {
    ++counters_.read_errors;
    return;
  }
  encodeOdometer(odometer, slot.payload);
  slot.sequence = decoded.header.sequence;
  slot.version = decoded.header.version;
  slot.needs_migration = decoded.header.version < kOdometerRecordVersion;
  slot.valid = true;
}

bool StorageManager::writeSlot(const char* path,
                               const uint8_t* payload,
                               size_t payload_length,
                               uint16_t version,
                               uint32_t sequence) {
  uint8_t record[kMaximumRecordSize];
  const size_t record_length = encodeRecord(payload, payload_length, version,
                                            sequence, record, sizeof(record));
  if (record_length == 0 || !backend_.write(path, record, record_length)) {
    ++counters_.write_errors;
    return false;
  }
  ++counters_.writes;
  return true;
}

bool StorageManager::savePayload(const char* path_a,
                                 const char* path_b,
                                 const uint8_t* payload,
                                 size_t payload_length,
                                 uint16_t version,
                                 bool config_payload,
                                 bool force_write) {
  if (!mounted_) return false;
  Slot a;
  Slot b;
  if (config_payload) {
    readConfigSlot(path_a, a);
    readConfigSlot(path_b, b);
  } else {
    readOdometerSlot(path_a, a);
    readOdometerSlot(path_b, b);
  }

  const Slot* newest = nullptr;
  if (a.valid && b.valid) {
    newest = isNewer(b.sequence, a.sequence) ? &b : &a;
  } else if (a.valid) {
    newest = &a;
  } else if (b.valid) {
    newest = &b;
  }
  if (!force_write && newest != nullptr &&
      memcmp(newest->payload, payload, payload_length) == 0 &&
      !newest->needs_migration) {
    ++counters_.skipped_writes;
    return true;
  }

  const char* target = path_a;
  uint32_t sequence = 1;
  if (newest != nullptr) sequence = newest->sequence + 1u;
  if (a.valid && !b.valid) {
    target = path_b;
  } else if (!a.valid && b.valid) {
    target = path_a;
  } else if (a.valid && b.valid) {
    target = isNewer(b.sequence, a.sequence) ? path_a : path_b;
  }
  return writeSlot(target, payload, payload_length, version, sequence);
}

bool StorageManager::loadConfig(DeviceConfig& config, StorageLoadInfo& info) {
  info = StorageLoadInfo{};
  if (!mounted_) return false;
  Slot a;
  Slot b;
  readConfigSlot(paths_.config_a, a);
  readConfigSlot(paths_.config_b, b);
  const Slot* selected = nullptr;
  if (a.valid && b.valid) {
    selected = isNewer(b.sequence, a.sequence) ? &b : &a;
  } else if (a.valid) {
    selected = &a;
  } else if (b.valid) {
    selected = &b;
  }
  if (selected != nullptr) {
    if (!decodeDeviceConfig(selected->payload, kDeviceConfigPayloadSize,
                            config)) {
      return false;
    }
    info.source = selected == &a ? StorageSource::kSlotA : StorageSource::kSlotB;
    info.sequence = selected->sequence;
    info.from_version = selected->version;
    info.recovered = selected == &a ? (b.present && !b.valid)
                                    : (a.present && !a.valid);
    if (info.recovered) ++counters_.config_slot_recoveries;
    if (selected->needs_migration) {
      ++counters_.config_migrations;
      info.migrated = true;
      info.migration_written =
          savePayload(paths_.config_a, paths_.config_b, selected->payload,
                      kDeviceConfigPayloadSize, kConfigRecordVersion, true,
                      true);
      if (info.migration_written) {
        info.sequence = selected->sequence + 1u;
        info.from_version = selected->version;
      }
    }
    return true;
  }

  config = DeviceConfig{};
  info.source = StorageSource::kDefaults;
  info.recovered = a.present || b.present;
  ++counters_.config_defaults_restored;
  uint8_t payload[kDeviceConfigPayloadSize];
  encodeDeviceConfig(config, payload);
  info.defaults_written = writeSlot(paths_.config_a, payload,
                                    sizeof(payload), kConfigRecordVersion, 1);
  info.sequence = info.defaults_written ? 1 : 0;
  info.from_version = kConfigRecordVersion;
  return info.defaults_written;
}

bool StorageManager::saveConfig(const DeviceConfig& config) {
  if (ConfigValidator::validate(config) != ConfigValidationError::kNone) {
    return false;
  }
  uint8_t payload[kDeviceConfigPayloadSize];
  encodeDeviceConfig(config, payload);
  return savePayload(paths_.config_a, paths_.config_b, payload,
                     sizeof(payload), kConfigRecordVersion, true, false);
}

bool StorageManager::loadOdometer(OdometerData& odometer,
                                  StorageLoadInfo& info) {
  info = StorageLoadInfo{};
  if (!mounted_) return false;
  Slot a;
  Slot b;
  readOdometerSlot(paths_.odometer_a, a);
  readOdometerSlot(paths_.odometer_b, b);
  const Slot* selected = nullptr;
  if (a.valid && b.valid) {
    selected = isNewer(b.sequence, a.sequence) ? &b : &a;
  } else if (a.valid) {
    selected = &a;
  } else if (b.valid) {
    selected = &b;
  }
  if (selected != nullptr) {
    if (!decodeOdometer(selected->payload, kOdometerPayloadSize, odometer)) {
      return false;
    }
    info.source = selected == &a ? StorageSource::kSlotA : StorageSource::kSlotB;
    info.sequence = selected->sequence;
    info.from_version = selected->version;
    last_odometer_sequence_ = selected->sequence;
    info.recovered = selected == &a ? (b.present && !b.valid)
                                    : (a.present && !a.valid);
    if (info.recovered) ++counters_.odometer_slot_recoveries;
    if (selected->needs_migration) {
      ++counters_.odometer_migrations;
      info.migrated = true;
      info.migration_written =
          savePayload(paths_.odometer_a, paths_.odometer_b, selected->payload,
                      kOdometerPayloadSize, kOdometerRecordVersion, false,
                      true);
      if (info.migration_written) {
        info.sequence = selected->sequence + 1u;
        last_odometer_sequence_ = info.sequence;
      }
    }
    return true;
  }

  odometer = OdometerData{};
  info.source = StorageSource::kDefaults;
  info.recovered = a.present || b.present;
  ++counters_.odometer_defaults_restored;
  uint8_t payload[kOdometerPayloadSize];
  encodeOdometer(odometer, payload);
  info.defaults_written = writeSlot(paths_.odometer_a, payload,
                                    sizeof(payload), kOdometerRecordVersion, 1);
  info.sequence = info.defaults_written ? 1 : 0;
  info.from_version = kOdometerRecordVersion;
  if (info.defaults_written) last_odometer_sequence_ = 1;
  return info.defaults_written;
}

bool StorageManager::saveOdometer(const OdometerData& odometer) {
  const uint32_t writes_before = counters_.writes;
  const uint32_t skipped_before = counters_.skipped_writes;
  uint8_t payload[kOdometerPayloadSize];
  encodeOdometer(odometer, payload);
  const bool ok = savePayload(paths_.odometer_a, paths_.odometer_b, payload,
                              sizeof(payload), kOdometerRecordVersion, false,
                              false);
  if (!ok) return false;
  if (counters_.writes > writes_before) {
    last_odometer_sequence_ += 1u;
    if (last_odometer_sequence_ == 0u) last_odometer_sequence_ = 1u;
  } else if (counters_.skipped_writes == skipped_before) {
    // No skip and no write should not happen on success, but keep sequence.
  }
  return true;
}

const char* storageSourceName(StorageSource source) {
  switch (source) {
    case StorageSource::kSlotA:
      return "A";
    case StorageSource::kSlotB:
      return "B";
    default:
      return "defaults";
  }
}

}  // namespace bike
