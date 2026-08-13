#include "boot_counter.h"

namespace bike {
namespace {

uint16_t readBootCount(StorageBackend& backend, const char* path) {
  uint8_t buf[2] = {};
  size_t length = 0;
  if (backend.read(path, buf, sizeof(buf), length) != StorageIoResult::kOk) {
    return 0;
  }
  if (length < 2u) return 0;
  return static_cast<uint16_t>(buf[0] | (static_cast<uint16_t>(buf[1]) << 8));
}

bool writeBootCount(StorageBackend& backend, const char* path, uint16_t count) {
  const uint8_t buf[2] = {static_cast<uint8_t>(count & 0xFFu),
                          static_cast<uint8_t>((count >> 8) & 0xFFu)};
  if (!backend.beginWrite(path, buf, sizeof(buf))) return false;
  AsyncWriteStatus status;
  do {
    status = backend.pollWrite();
  } while (status == AsyncWriteStatus::kInProgress);
  return status == AsyncWriteStatus::kOk;
}

}  // namespace

uint16_t loadAndIncrementBootCount(StorageBackend& backend, const char* path) {
  uint16_t count = readBootCount(backend, path);
  if (count < 0xFFFFu) ++count;
  (void)writeBootCount(backend, path, count);
  return count;
}

}  // namespace bike
