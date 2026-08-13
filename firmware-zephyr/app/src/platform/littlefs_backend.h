#pragma once

#include <stddef.h>
#include <stdint.h>

#include "storage_manager.h"

namespace bike {

class LittleFsBackend final : public StorageBackend {
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
  enum class AsyncWriteState : uint8_t { kIdle, kPendingRemove, kPendingWrite };

  bool mounted_ = false;
  AsyncWriteState write_state_ = AsyncWriteState::kIdle;
  const char* pending_path_ = nullptr;
  uint8_t pending_data_[kMaximumRecordSize] = {};
  size_t pending_length_ = 0;
};

}  // namespace bike
