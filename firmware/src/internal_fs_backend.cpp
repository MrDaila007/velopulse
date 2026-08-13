#include "internal_fs_backend.h"

#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>

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
  if (write_pending_) return false;
  // Task 1 keeps this fully synchronous, matching the old write() exactly --
  // the real two-poll remove/write split lands in Task 3, touching only
  // this method's body, not this file's interface shape.
  if (InternalFS.exists(path) && !InternalFS.remove(path)) {
    pending_result_ = AsyncWriteStatus::kError;
    write_pending_ = true;
    return true;
  }
  File file(path, FILE_O_WRITE, InternalFS);
  if (!file) {
    pending_result_ = AsyncWriteStatus::kError;
    write_pending_ = true;
    return true;
  }
  const size_t bytes_written = file.write(data, length);
  file.flush();
  const bool complete = bytes_written == length && file.size() == length;
  file.close();
  pending_result_ = complete ? AsyncWriteStatus::kOk : AsyncWriteStatus::kError;
  write_pending_ = true;
  return true;
}

AsyncWriteStatus InternalFsBackend::pollWrite() {
  if (!write_pending_) return AsyncWriteStatus::kIdle;
  write_pending_ = false;
  return pending_result_;
}

}  // namespace bike
