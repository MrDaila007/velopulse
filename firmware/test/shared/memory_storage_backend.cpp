#include "memory_storage_backend.h"

#include <cstring>

#include "config_codec.h"
#include "storage_manager.h"

namespace bike {
namespace test_support {

StorageIoResult MemoryStorageBackend::read(const char* path,
                                           uint8_t* output,
                                           size_t capacity,
                                           size_t& length) {
  const auto found = files.find(path);
  if (found == files.end()) return StorageIoResult::kNotFound;
  if (found->second.size() > capacity) return StorageIoResult::kError;
  memcpy(output, found->second.data(), found->second.size());
  length = found->second.size();
  return StorageIoResult::kOk;
}

bool MemoryStorageBackend::write(const char* path,
                                 const uint8_t* data,
                                 size_t length) {
  if (!write_ok) return false;
  files[path] = std::vector<uint8_t>(data, data + length);
  return true;
}

void MemoryStorageBackend::corrupt(const char* path, size_t offset) {
  files[path][offset] ^= 0x80u;
}

void putConfigRecord(MemoryStorageBackend& backend,
                     const char* path,
                     const DeviceConfig& config,
                     uint32_t sequence,
                     uint16_t version) {
  uint8_t payload[kDeviceConfigPayloadSize];
  uint8_t record[kMaximumRecordSize];
  encodeDeviceConfig(config, payload);
  const size_t length = encodeRecord(payload, sizeof(payload), version,
                                     sequence, record, sizeof(record));
  backend.write(path, record, length);
}

}  // namespace test_support
}  // namespace bike
