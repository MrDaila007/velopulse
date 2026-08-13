#include "internal_fs_backend.h"

#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>

#include <cstring>

using namespace Adafruit_LittleFS_Namespace;

namespace bike {

bool InternalFsBackend::begin() {
  if (mounted_) return true;
  mounted_ = InternalFS.begin();
  return mounted_;
}

StorageIoResult InternalFsBackend::read(const char* path,
                                        uint8_t* output,
                                        size_t capacity,
                                        size_t& length) {
  length = 0;
  if (!InternalFS.exists(path)) return StorageIoResult::kNotFound;
  File file(path, FILE_O_READ, InternalFS);
  if (!file) return StorageIoResult::kError;
  const size_t file_size = file.size();
  if (file_size > capacity) {
    file.close();
    return StorageIoResult::kError;
  }
  const int bytes_read = file.read(output, static_cast<uint16_t>(file_size));
  file.close();
  if (bytes_read < 0 || static_cast<size_t>(bytes_read) != file_size) {
    return StorageIoResult::kError;
  }
  length = file_size;
  return StorageIoResult::kOk;
}

bool InternalFsBackend::beginWrite(const char* path, const uint8_t* data,
                                   size_t length) {
  if (write_state_ != AsyncWriteState::kIdle) return false;
  if (length > sizeof(pending_data_)) return false;
  memcpy(pending_data_, data, length);
  pending_length_ = length;
  pending_path_ = path;
  write_state_ = AsyncWriteState::kPendingRemove;
  return true;
}

AsyncWriteStatus InternalFsBackend::pollWrite() {
  switch (write_state_) {
    case AsyncWriteState::kIdle:
      return AsyncWriteStatus::kIdle;
    case AsyncWriteState::kPendingRemove: {
      if (InternalFS.exists(pending_path_) && !InternalFS.remove(pending_path_)) {
        write_state_ = AsyncWriteState::kIdle;
        return AsyncWriteStatus::kError;
      }
      write_state_ = AsyncWriteState::kPendingWrite;
      return AsyncWriteStatus::kInProgress;
    }
    case AsyncWriteState::kPendingWrite: {
      write_state_ = AsyncWriteState::kIdle;
      File file(pending_path_, FILE_O_WRITE, InternalFS);
      if (!file) return AsyncWriteStatus::kError;
      const size_t bytes_written = file.write(pending_data_, pending_length_);
      file.flush();
      const bool complete =
          bytes_written == pending_length_ && file.size() == pending_length_;
      file.close();
      return complete ? AsyncWriteStatus::kOk : AsyncWriteStatus::kError;
    }
  }
  return AsyncWriteStatus::kError;  // unreachable
}

}  // namespace bike
