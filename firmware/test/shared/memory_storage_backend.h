#pragma once

#include <map>
#include <string>
#include <vector>

#include "storage_manager.h"

namespace bike {
namespace test_support {

class MemoryStorageBackend final : public StorageBackend {
 public:
  bool begin() override { return begin_ok; }

  StorageIoResult read(const char* path,
                       uint8_t* output,
                       size_t capacity,
                       size_t& length) override;

  bool write(const char* path, const uint8_t* data, size_t length) override;

  void corrupt(const char* path, size_t offset);

  bool begin_ok = true;
  bool write_ok = true;
  std::map<std::string, std::vector<uint8_t>> files;
};

void putConfigRecord(MemoryStorageBackend& backend,
                     const char* path,
                     const DeviceConfig& config,
                     uint32_t sequence,
                     uint16_t version = kConfigRecordVersion);

}  // namespace test_support
}  // namespace bike
