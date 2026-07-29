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
  bool write(const char* path,
             const uint8_t* data,
             size_t length) override;

 private:
  bool mounted_ = false;
};

}  // namespace bike
