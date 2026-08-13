#pragma once

#include <stddef.h>
#include <stdint.h>

#include "storage_manager.h"

namespace bike {

class InternalFsBackend final : public StorageBackend {
 public:
  bool begin() override;
  StorageIoResult read(const char* path,
                       uint8_t* output,
                       size_t capacity,
                       size_t& length) override;
  bool beginWrite(const char* path, const uint8_t* data,
                  size_t length) override;
  AsyncWriteStatus pollWrite() override;

 private:
  bool mounted_ = false;
  bool write_pending_ = false;
  AsyncWriteStatus pending_result_ = AsyncWriteStatus::kIdle;
};

}  // namespace bike
