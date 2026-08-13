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

void encodeAmbientCalibration(const AmbientCalibrationData& calibration,
                              uint8_t output[kAmbientCalibrationPayloadSize]) {
  writeU16(output, calibration.raw_dark);
  writeU16(output + 2, calibration.raw_bright);
  output[4] = calibration.quality;
}

bool decodeAmbientCalibration(const uint8_t* input,
                              size_t length,
                              AmbientCalibrationData& calibration) {
  if (input == nullptr || length != kAmbientCalibrationPayloadSize) return false;
  calibration.raw_dark = readU16(input);
  calibration.raw_bright = readU16(input + 2);
  calibration.quality = input[4];
  return true;
}

void encodeStorageCounters(const StorageCounters& counters,
                           uint8_t output[kStorageCountersPayloadSize]) {
  writeU32(output + 0, counters.writes);
  writeU32(output + 4, counters.skipped_writes);
  writeU32(output + 8, counters.read_errors);
  writeU32(output + 12, counters.write_errors);
  writeU32(output + 16, counters.config_slot_recoveries);
  writeU32(output + 20, counters.odometer_slot_recoveries);
  writeU32(output + 24, counters.ambient_calibration_slot_recoveries);
  writeU32(output + 28, counters.config_defaults_restored);
  writeU32(output + 32, counters.odometer_defaults_restored);
  writeU32(output + 36, counters.ambient_calibration_defaults_restored);
  writeU32(output + 40, counters.config_migrations);
  writeU32(output + 44, counters.odometer_migrations);
}

bool decodeStorageCounters(const uint8_t* input, size_t length,
                           StorageCounters& counters) {
  if (input == nullptr || length != kStorageCountersPayloadSize) return false;
  counters.writes = readU32(input + 0);
  counters.skipped_writes = readU32(input + 4);
  counters.read_errors = readU32(input + 8);
  counters.write_errors = readU32(input + 12);
  counters.config_slot_recoveries = readU32(input + 16);
  counters.odometer_slot_recoveries = readU32(input + 20);
  counters.ambient_calibration_slot_recoveries = readU32(input + 24);
  counters.config_defaults_restored = readU32(input + 28);
  counters.odometer_defaults_restored = readU32(input + 32);
  counters.ambient_calibration_defaults_restored = readU32(input + 36);
  counters.config_migrations = readU32(input + 40);
  counters.odometer_migrations = readU32(input + 44);
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

static_assert(kStorageCountersPayloadSize <= kDeviceConfigPayloadSize,
             "StorageCounters payload must fit the shared Slot buffer");
static_assert(kRecordHeaderSize + kStorageCountersPayloadSize <= kMaximumRecordSize,
             "StorageCounters record must fit within kMaximumRecordSize");

StorageManager::StorageManager(StorageBackend& backend,
                               const StoragePaths& paths)
    : backend_(backend), paths_(paths) {}

bool StorageManager::begin() {
  mounted_ = backend_.begin();
  if (!mounted_) {
    ++counters_.read_errors;
    return false;
  }
  loadStorageCounters();
  return true;
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

void StorageManager::readAmbientCalibrationSlot(const char* path, Slot& slot) {
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
  if (!decodeRecord(record, record_length, kAmbientCalibrationRecordVersion,
                    kAmbientCalibrationPayloadSize, decoded)) {
    ++counters_.read_errors;
    return;
  }

  AmbientCalibrationData calibration;
  if (!decodeAmbientCalibration(decoded.payload, decoded.header.payload_len,
                                calibration)) {
    ++counters_.read_errors;
    return;
  }
  encodeAmbientCalibration(calibration, slot.payload);
  slot.sequence = decoded.header.sequence;
  slot.version = decoded.header.version;
  slot.needs_migration = false;
  slot.valid = true;
}

void StorageManager::readStorageCountersSlot(const char* path, Slot& slot) {
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
  if (!decodeRecord(record, record_length, kStorageCountersRecordVersion,
                    kStorageCountersPayloadSize, decoded)) {
    ++counters_.read_errors;
    return;
  }

  StorageCounters restored;
  if (!decodeStorageCounters(decoded.payload, decoded.header.payload_len,
                             restored)) {
    ++counters_.read_errors;
    return;
  }
  encodeStorageCounters(restored, slot.payload);
  slot.sequence = decoded.header.sequence;
  slot.version = decoded.header.version;
  slot.needs_migration = false;
  slot.valid = true;
}

void StorageManager::loadStorageCounters() {
  const uint32_t read_errors_before = counters_.read_errors;
  Slot a;
  Slot b;
  readStorageCountersSlot(paths_.counters_a, a);
  readStorageCountersSlot(paths_.counters_b, b);
  const uint32_t read_errors_during = counters_.read_errors - read_errors_before;

  const Slot* selected = nullptr;
  if (a.valid && b.valid) {
    selected = isNewer(b.sequence, a.sequence) ? &b : &a;
  } else if (a.valid) {
    selected = &a;
  } else if (b.valid) {
    selected = &b;
  }
  if (selected == nullptr) return;

  StorageCounters restored;
  if (!decodeStorageCounters(selected->payload, kStorageCountersPayloadSize,
                             restored)) {
    return;
  }
  // Preserve this boot's own read-error bump instead of discarding it under
  // the wholesale overwrite below.
  restored.read_errors += read_errors_during;
  counters_ = restored;
}

bool StorageManager::beginWriteSlotAsync(const char* path,
                                         const uint8_t* payload,
                                         size_t payload_length,
                                         uint16_t version,
                                         uint32_t sequence) {
  pending_is_odometer_ = false;  // reset first -- beginSavePayloadAsync re-sets
                                 // this after a successful call below, only
                                 // for an odometer-kind write.
  uint8_t record[kMaximumRecordSize];
  const size_t record_length = encodeRecord(payload, payload_length, version,
                                            sequence, record, sizeof(record));
  if (record_length == 0 || !backend_.beginWrite(path, record, record_length)) {
    ++counters_.write_errors;
    return false;
  }
  save_in_progress_ = true;
  return true;
}

bool StorageManager::writeSlot(const char* path,
                               const uint8_t* payload,
                               size_t payload_length,
                               uint16_t version,
                               uint32_t sequence) {
  if (!beginWriteSlotAsync(path, payload, payload_length, version, sequence)) {
    return false;
  }
  return drainPendingSave() == StorageAsyncStatus::kOk;
}

StorageAsyncStatus StorageManager::pollSave() {
  if (!save_in_progress_) return StorageAsyncStatus::kIdle;
  const AsyncWriteStatus status = backend_.pollWrite();
  if (status == AsyncWriteStatus::kInProgress) return StorageAsyncStatus::kInProgress;
  save_in_progress_ = false;
  if (status == AsyncWriteStatus::kOk) {
    ++counters_.writes;
    if (pending_is_odometer_) last_odometer_sequence_ = pending_odometer_sequence_;
    return StorageAsyncStatus::kOk;
  }
  ++counters_.write_errors;
  return StorageAsyncStatus::kError;
}

StorageAsyncStatus StorageManager::drainPendingSave() {
  constexpr int kMaxDrainPolls = 8;  // the real state machine completes in <=2
  for (int i = 0; i < kMaxDrainPolls && save_in_progress_; ++i) {
    const StorageAsyncStatus status = pollSave();
    if (status != StorageAsyncStatus::kInProgress) return status;
  }
  if (save_in_progress_) {
    // Backend never resolved within the safety cap -- force-clear so the
    // manager doesn't wedge permanently; count it as a write error.
    save_in_progress_ = false;
    ++counters_.write_errors;
    return StorageAsyncStatus::kError;
  }
  return StorageAsyncStatus::kIdle;
}

bool StorageManager::beginSavePayloadAsync(const char* path_a,
                                           const char* path_b,
                                           const uint8_t* payload,
                                           size_t payload_length,
                                           uint16_t version,
                                           PayloadKind kind,
                                           bool force_write) {
  if (!mounted_ || save_in_progress_) return false;
  Slot a;
  Slot b;
  switch (kind) {
    case PayloadKind::kConfig:
      readConfigSlot(path_a, a);
      readConfigSlot(path_b, b);
      break;
    case PayloadKind::kOdometer:
      readOdometerSlot(path_a, a);
      readOdometerSlot(path_b, b);
      break;
    case PayloadKind::kAmbientCalibration:
      readAmbientCalibrationSlot(path_a, a);
      readAmbientCalibrationSlot(path_b, b);
      break;
    case PayloadKind::kStorageCounters:
      readStorageCountersSlot(path_a, a);
      readStorageCountersSlot(path_b, b);
      break;
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
  const bool started = beginWriteSlotAsync(target, payload, payload_length, version, sequence);
  if (started && kind == PayloadKind::kOdometer) {
    pending_is_odometer_ = true;
    pending_odometer_sequence_ = sequence;
  }
  return started;
}

bool StorageManager::savePayload(const char* path_a,
                                 const char* path_b,
                                 const uint8_t* payload,
                                 size_t payload_length,
                                 uint16_t version,
                                 PayloadKind kind,
                                 bool force_write) {
  if (!beginSavePayloadAsync(path_a, path_b, payload, payload_length, version,
                             kind, force_write)) {
    return false;
  }
  if (!save_in_progress_) return true;  // dedup-skip completed synchronously
  return drainPendingSave() == StorageAsyncStatus::kOk;
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
                      kDeviceConfigPayloadSize, kConfigRecordVersion,
                      PayloadKind::kConfig, true);
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
                     sizeof(payload), kConfigRecordVersion,
                     PayloadKind::kConfig, false);
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
                      kOdometerPayloadSize, kOdometerRecordVersion,
                      PayloadKind::kOdometer, true);
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

bool StorageManager::beginSaveOdometer(const OdometerData& odometer) {
  uint8_t payload[kOdometerPayloadSize];
  encodeOdometer(odometer, payload);
  return beginSavePayloadAsync(paths_.odometer_a, paths_.odometer_b, payload,
                               sizeof(payload), kOdometerRecordVersion,
                               PayloadKind::kOdometer, false);
}

bool StorageManager::saveOdometer(const OdometerData& odometer) {
  if (!beginSaveOdometer(odometer)) return false;
  if (!save_in_progress_) return true;
  return drainPendingSave() == StorageAsyncStatus::kOk;
}

bool StorageManager::loadAmbientCalibration(AmbientCalibrationData& calibration,
                                            StorageLoadInfo& info) {
  info = StorageLoadInfo{};
  if (!mounted_) return false;
  Slot a;
  Slot b;
  readAmbientCalibrationSlot(paths_.ambient_calibration_a, a);
  readAmbientCalibrationSlot(paths_.ambient_calibration_b, b);
  const Slot* selected = nullptr;
  if (a.valid && b.valid) {
    selected = isNewer(b.sequence, a.sequence) ? &b : &a;
  } else if (a.valid) {
    selected = &a;
  } else if (b.valid) {
    selected = &b;
  }
  if (selected != nullptr) {
    if (!decodeAmbientCalibration(selected->payload,
                                  kAmbientCalibrationPayloadSize, calibration)) {
      return false;
    }
    info.source = selected == &a ? StorageSource::kSlotA : StorageSource::kSlotB;
    info.sequence = selected->sequence;
    info.from_version = selected->version;
    info.recovered = selected == &a ? (b.present && !b.valid)
                                    : (a.present && !a.valid);
    if (info.recovered) ++counters_.ambient_calibration_slot_recoveries;
    return true;
  }

  // Unlike config/odometer, do not eagerly write a default record here --
  // the caller (AppController) decides the real bootstrap raw_dark/raw_bright
  // and persists explicitly once the save policy decides to.
  calibration = AmbientCalibrationData{};
  info.source = StorageSource::kDefaults;
  info.recovered = a.present || b.present;
  ++counters_.ambient_calibration_defaults_restored;
  info.sequence = 0;
  info.from_version = kAmbientCalibrationRecordVersion;
  return true;
}

bool StorageManager::beginSaveAmbientCalibration(
    const AmbientCalibrationData& calibration) {
  uint8_t payload[kAmbientCalibrationPayloadSize];
  encodeAmbientCalibration(calibration, payload);
  return beginSavePayloadAsync(paths_.ambient_calibration_a,
                               paths_.ambient_calibration_b, payload,
                               sizeof(payload), kAmbientCalibrationRecordVersion,
                               PayloadKind::kAmbientCalibration, false);
}

bool StorageManager::saveAmbientCalibration(
    const AmbientCalibrationData& calibration) {
  if (!beginSaveAmbientCalibration(calibration)) return false;
  if (!save_in_progress_) return true;
  return drainPendingSave() == StorageAsyncStatus::kOk;
}

bool StorageManager::beginSaveStorageCounters() {
  // Snapshot before encoding: the eventual pollSave() completion will bump
  // counters_.writes as a side effect of the save itself, which must not
  // leak into this payload (it belongs to the *next* save).
  uint8_t payload[kStorageCountersPayloadSize];
  encodeStorageCounters(counters_, payload);
  return beginSavePayloadAsync(paths_.counters_a, paths_.counters_b, payload,
                               sizeof(payload), kStorageCountersRecordVersion,
                               PayloadKind::kStorageCounters, false);
}

bool StorageManager::saveStorageCounters() {
  if (!beginSaveStorageCounters()) return false;
  if (!save_in_progress_) return true;
  return drainPendingSave() == StorageAsyncStatus::kOk;
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
