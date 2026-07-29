#include "storage_manager.h"

#include <string.h>

#include "config_codec.h"
#include "config_validator.h"
#include "crc32.h"

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

bool decodeRecord(const uint8_t* record,
                  size_t record_length,
                  uint16_t expected_version,
                  size_t expected_payload_length,
                  DecodedRecord& decoded) {
  if (record == nullptr || record_length < kRecordHeaderSize) return false;
  RecordHeader header;
  header.magic = readU32(record);
  header.version = readU16(record + 4);
  header.payload_len = readU16(record + 6);
  header.sequence = readU32(record + 8);
  header.crc32 = readU32(record + 12);
  if (header.magic != kRecordMagic || header.version != expected_version ||
      header.payload_len != expected_payload_length ||
      record_length != kRecordHeaderSize + expected_payload_length) {
    return false;
  }
  const uint8_t* payload = record + kRecordHeaderSize;
  if (crc32(payload, expected_payload_length) != header.crc32) return false;
  decoded.header = header;
  decoded.payload = payload;
  return true;
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

void StorageManager::readSlot(const char* path,
                              uint16_t version,
                              size_t payload_length,
                              bool config_payload,
                              Slot& slot) {
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
  if (!decodeRecord(record, record_length, version, payload_length, decoded)) {
    ++counters_.read_errors;
    return;
  }
  if (config_payload) {
    DeviceConfig config;
    if (!decodeDeviceConfig(decoded.payload, payload_length, config)) {
      ++counters_.read_errors;
      return;
    }
  }
  memcpy(slot.payload, decoded.payload, payload_length);
  slot.sequence = decoded.header.sequence;
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
                                 bool config_payload) {
  if (!mounted_) return false;
  Slot a;
  Slot b;
  readSlot(path_a, version, payload_length, config_payload, a);
  readSlot(path_b, version, payload_length, config_payload, b);

  const Slot* newest = nullptr;
  if (a.valid && b.valid) {
    newest = isNewer(b.sequence, a.sequence) ? &b : &a;
  } else if (a.valid) {
    newest = &a;
  } else if (b.valid) {
    newest = &b;
  }
  if (newest != nullptr &&
      memcmp(newest->payload, payload, payload_length) == 0) {
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
  readSlot(paths_.config_a, kConfigRecordVersion,
           kDeviceConfigPayloadSize, true, a);
  readSlot(paths_.config_b, kConfigRecordVersion,
           kDeviceConfigPayloadSize, true, b);
  const Slot* selected = nullptr;
  if (a.valid && b.valid) {
    selected = isNewer(b.sequence, a.sequence) ? &b : &a;
  } else if (a.valid) {
    selected = &a;
  } else if (b.valid) {
    selected = &b;
  }
  if (selected != nullptr) {
    if (!decodeDeviceConfig(selected->payload, kDeviceConfigPayloadSize, config)) {
      return false;
    }
    info.source = selected == &a ? StorageSource::kSlotA : StorageSource::kSlotB;
    info.sequence = selected->sequence;
    info.recovered = selected == &a ? (b.present && !b.valid)
                                    : (a.present && !a.valid);
    if (info.recovered) ++counters_.config_slot_recoveries;
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
  return info.defaults_written;
}

bool StorageManager::saveConfig(const DeviceConfig& config) {
  if (ConfigValidator::validate(config) != ConfigValidationError::kNone) {
    return false;
  }
  uint8_t payload[kDeviceConfigPayloadSize];
  encodeDeviceConfig(config, payload);
  return savePayload(paths_.config_a, paths_.config_b, payload,
                     sizeof(payload), kConfigRecordVersion, true);
}

bool StorageManager::loadOdometer(OdometerData& odometer,
                                  StorageLoadInfo& info) {
  info = StorageLoadInfo{};
  if (!mounted_) return false;
  Slot a;
  Slot b;
  readSlot(paths_.odometer_a, kOdometerRecordVersion,
           kOdometerPayloadSize, false, a);
  readSlot(paths_.odometer_b, kOdometerRecordVersion,
           kOdometerPayloadSize, false, b);
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
    info.recovered = selected == &a ? (b.present && !b.valid)
                                    : (a.present && !a.valid);
    if (info.recovered) ++counters_.odometer_slot_recoveries;
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
  return info.defaults_written;
}

bool StorageManager::saveOdometer(const OdometerData& odometer) {
  uint8_t payload[kOdometerPayloadSize];
  encodeOdometer(odometer, payload);
  return savePayload(paths_.odometer_a, paths_.odometer_b, payload,
                     sizeof(payload), kOdometerRecordVersion, false);
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
