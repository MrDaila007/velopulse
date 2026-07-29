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

bool InternalFsBackend::write(const char* path,
                              const uint8_t* data,
                              size_t length) {
  if (InternalFS.exists(path) && !InternalFS.remove(path)) return false;
  File file(path, FILE_O_WRITE, InternalFS);
  if (!file) return false;
  const size_t bytes_written = file.write(data, length);
  file.flush();
  const bool complete = bytes_written == length && file.size() == length;
  file.close();
  return complete;
}

}  // namespace bike
