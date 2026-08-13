#pragma once

#include <stddef.h>
#include <stdint.h>

#include "config.h"

namespace bike {

constexpr uint32_t kRecordMagic = 0x50434B42u;
constexpr uint16_t kConfigRecordVersion = 2;
constexpr uint16_t kOdometerRecordVersion = 2;
constexpr uint16_t kAmbientCalibrationRecordVersion = 1;
constexpr size_t kRecordHeaderSize = 16;
constexpr size_t kOdometerPayloadSize = 16;
constexpr size_t kAmbientCalibrationPayloadSize = 5;
constexpr uint16_t kStorageCountersRecordVersion = 1;
constexpr size_t kStorageCountersPayloadSize = 48;
constexpr size_t kMaximumRecordSize = 64;

struct RecordHeader {
  uint32_t magic = kRecordMagic;
  uint16_t version = 0;
  uint16_t payload_len = 0;
  uint32_t sequence = 0;
  uint32_t crc32 = 0;
};

struct DecodedRecord {
  RecordHeader header;
  const uint8_t* payload = nullptr;
};

size_t encodeRecord(const uint8_t* payload,
                    size_t payload_length,
                    uint16_t version,
                    uint32_t sequence,
                    uint8_t* output,
                    size_t output_capacity);
bool decodeRecord(const uint8_t* record,
                  size_t record_length,
                  uint16_t expected_version,
                  size_t expected_payload_length,
                  DecodedRecord& decoded);
bool inspectRecord(const uint8_t* record,
                   size_t record_length,
                   DecodedRecord& decoded);

struct OdometerData {
  uint64_t odometer_mm = 0;
  uint64_t total_revolutions = 0;
};

void encodeOdometer(const OdometerData& odometer,
                    uint8_t output[kOdometerPayloadSize]);
bool decodeOdometer(const uint8_t* input,
                    size_t length,
                    OdometerData& odometer);

struct AmbientCalibrationData {
  uint16_t raw_dark = 0;
  uint16_t raw_bright = 0;
  uint8_t quality = 0;
};

void encodeAmbientCalibration(const AmbientCalibrationData& calibration,
                              uint8_t output[kAmbientCalibrationPayloadSize]);
bool decodeAmbientCalibration(const uint8_t* input,
                              size_t length,
                              AmbientCalibrationData& calibration);

enum class StorageIoResult : uint8_t {
  kOk = 0,
  kNotFound,
  kError,
};

enum class AsyncWriteStatus : uint8_t {
  kIdle,
  kInProgress,
  kOk,
  kError,
};

enum class StorageAsyncStatus : uint8_t {
  kIdle,
  kInProgress,
  kOk,
  kError,
};

class StorageBackend {
 public:
  virtual ~StorageBackend() = default;
  virtual bool begin() = 0;
  virtual StorageIoResult read(const char* path,
                               uint8_t* output,
                               size_t capacity,
                               size_t& length) = 0;
  // Starts an async write. Returns false only on caller error (a write is
  // already in progress on this backend, or length exceeds the backend's
  // internal buffer). The implementation must copy
  // `data` internally -- it will not remain valid past this call. Call
  // pollWrite() repeatedly (e.g. once per scheduler tick) until it returns
  // something other than kInProgress to observe the outcome.
  virtual bool beginWrite(const char* path,
                         const uint8_t* data,
                         size_t length) = 0;
  virtual AsyncWriteStatus pollWrite() = 0;
};

struct StoragePaths {
  const char* config_a = "/cfg_a";
  const char* config_b = "/cfg_b";
  const char* odometer_a = "/odo_a";
  const char* odometer_b = "/odo_b";
  const char* ambient_calibration_a = "/alc_a";
  const char* ambient_calibration_b = "/alc_b";
  const char* counters_a = "/cnt_a";
  const char* counters_b = "/cnt_b";
};

enum class StorageSource : uint8_t {
  kDefaults = 0,
  kSlotA,
  kSlotB,
};

struct StorageLoadInfo {
  StorageSource source = StorageSource::kDefaults;
  uint32_t sequence = 0;
  uint16_t from_version = 0;
  bool recovered = false;
  bool defaults_written = false;
  bool migrated = false;
  bool migration_written = false;
};

struct StorageCounters {
  uint32_t writes = 0;
  uint32_t skipped_writes = 0;
  uint32_t read_errors = 0;
  uint32_t write_errors = 0;
  uint32_t config_slot_recoveries = 0;
  uint32_t odometer_slot_recoveries = 0;
  uint32_t ambient_calibration_slot_recoveries = 0;
  uint32_t config_defaults_restored = 0;
  uint32_t odometer_defaults_restored = 0;
  uint32_t ambient_calibration_defaults_restored = 0;
  uint32_t config_migrations = 0;
  uint32_t odometer_migrations = 0;
};

void encodeStorageCounters(const StorageCounters& counters,
                           uint8_t output[kStorageCountersPayloadSize]);
bool decodeStorageCounters(const uint8_t* input, size_t length,
                           StorageCounters& counters);

class StorageManager {
 public:
  explicit StorageManager(StorageBackend& backend,
                          const StoragePaths& paths = {});

  bool begin();
  bool loadConfig(DeviceConfig& config, StorageLoadInfo& info);
  bool saveConfig(const DeviceConfig& config);
  bool loadOdometer(OdometerData& odometer, StorageLoadInfo& info);
  bool saveOdometer(const OdometerData& odometer);
  bool loadAmbientCalibration(AmbientCalibrationData& calibration,
                              StorageLoadInfo& info);
  bool saveAmbientCalibration(const AmbientCalibrationData& calibration);
  bool saveStorageCounters();

  bool beginSaveOdometer(const OdometerData& odometer);
  bool beginSaveAmbientCalibration(const AmbientCalibrationData& calibration);
  bool beginSaveStorageCounters();
  StorageAsyncStatus pollSave();
  StorageAsyncStatus drainPendingSave();
  bool saveInProgress() const { return save_in_progress_; }

  bool mounted() const { return mounted_; }
  const StorageCounters& counters() const { return counters_; }
  uint32_t lastOdometerSequence() const { return last_odometer_sequence_; }

 private:
  struct Slot;
  enum class PayloadKind : uint8_t { kConfig, kOdometer, kAmbientCalibration, kStorageCounters };

  void readConfigSlot(const char* path, Slot& slot);
  void readOdometerSlot(const char* path, Slot& slot);
  void readAmbientCalibrationSlot(const char* path, Slot& slot);
  void readStorageCountersSlot(const char* path, Slot& slot);
  void loadStorageCounters();
  bool writeSlot(const char* path,
                 const uint8_t* payload,
                 size_t payload_length,
                 uint16_t version,
                 uint32_t sequence);
  bool savePayload(const char* path_a,
                   const char* path_b,
                   const uint8_t* payload,
                   size_t payload_length,
                   uint16_t version,
                   PayloadKind kind,
                   bool force_write);
  bool beginWriteSlotAsync(const char* path,
                           const uint8_t* payload,
                           size_t payload_length,
                           uint16_t version,
                           uint32_t sequence);
  bool beginSavePayloadAsync(const char* path_a,
                             const char* path_b,
                             const uint8_t* payload,
                             size_t payload_length,
                             uint16_t version,
                             PayloadKind kind,
                             bool force_write);

  StorageBackend& backend_;
  StoragePaths paths_;
  bool mounted_ = false;
  StorageCounters counters_;
  uint32_t last_odometer_sequence_ = 0;
  bool save_in_progress_ = false;
  bool pending_is_odometer_ = false;
  uint32_t pending_odometer_sequence_ = 0;
};

const char* storageSourceName(StorageSource source);

}  // namespace bike
