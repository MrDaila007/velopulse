#include "littlefs_backend.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/storage/flash_map.h>

namespace bike {
namespace {

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(bikecomp_storage);
static struct fs_mount_t bikecomp_mount = {
    .type = FS_LITTLEFS,
    .mnt_point = "/bikecomp",
    .fs_data = &bikecomp_storage,
    .storage_dev = (void*)FIXED_PARTITION_ID(bikecomp_partition),
};

constexpr char kMountPoint[] = "/bikecomp";

void buildMountedPath(const char* path, char* out, size_t out_len) {
  if (path[0] == '/') {
    snprintf(out, out_len, "%s%s", kMountPoint, path);
  } else {
    snprintf(out, out_len, "%s/%s", kMountPoint, path);
  }
}

}  // namespace

bool LittleFsBackend::begin() {
  if (mounted_) return true;
  struct fs_dirent entry;
  mounted_ = (fs_stat(kMountPoint, &entry) == 0);
  if (!mounted_) {
    const int rc = fs_mount(&bikecomp_mount);
    mounted_ = (rc == 0);
  }
  return mounted_;
}

StorageIoResult LittleFsBackend::read(const char* path,
                                      uint8_t* output,
                                      size_t capacity,
                                      size_t& length) {
  length = 0;
  if (!mounted_) return StorageIoResult::kError;

  char mounted_path[48];
  buildMountedPath(path, mounted_path, sizeof(mounted_path));

  struct fs_file_t file;
  fs_file_t_init(&file);
  if (fs_open(&file, mounted_path, FS_O_READ) != 0) {
    return StorageIoResult::kNotFound;
  }

  struct fs_dirent entry;
  if (fs_stat(mounted_path, &entry) != 0 || entry.type != FS_DIR_ENTRY_FILE) {
    fs_close(&file);
    return StorageIoResult::kError;
  }
  if (static_cast<size_t>(entry.size) > capacity) {
    fs_close(&file);
    return StorageIoResult::kError;
  }

  const ssize_t bytes_read = fs_read(&file, output, entry.size);
  fs_close(&file);
  if (bytes_read < 0 || static_cast<size_t>(bytes_read) != static_cast<size_t>(entry.size)) {
    return StorageIoResult::kError;
  }
  length = static_cast<size_t>(bytes_read);
  return StorageIoResult::kOk;
}

bool LittleFsBackend::beginWrite(const char* path, const uint8_t* data,
                                 size_t length) {
  if (!mounted_ || write_state_ != AsyncWriteState::kIdle) return false;
  if (length > sizeof(pending_data_)) return false;
  memcpy(pending_data_, data, length);
  pending_length_ = length;
  pending_path_ = path;
  write_state_ = AsyncWriteState::kPendingRemove;
  return true;
}

AsyncWriteStatus LittleFsBackend::pollWrite() {
  switch (write_state_) {
    case AsyncWriteState::kIdle:
      return AsyncWriteStatus::kIdle;
    case AsyncWriteState::kPendingRemove: {
      char mounted_path[48];
      buildMountedPath(pending_path_, mounted_path, sizeof(mounted_path));
      fs_unlink(mounted_path);
      write_state_ = AsyncWriteState::kPendingWrite;
      return AsyncWriteStatus::kInProgress;
    }
    case AsyncWriteState::kPendingWrite: {
      write_state_ = AsyncWriteState::kIdle;
      char mounted_path[48];
      buildMountedPath(pending_path_, mounted_path, sizeof(mounted_path));
      struct fs_file_t file;
      fs_file_t_init(&file);
      if (fs_open(&file, mounted_path, FS_O_CREATE | FS_O_WRITE) != 0) {
        return AsyncWriteStatus::kError;
      }
      const ssize_t written = fs_write(&file, pending_data_, pending_length_);
      fs_sync(&file);
      fs_close(&file);
      return (written >= 0 && static_cast<size_t>(written) == pending_length_)
                 ? AsyncWriteStatus::kOk
                 : AsyncWriteStatus::kError;
    }
  }
  return AsyncWriteStatus::kError;  // unreachable
}

}  // namespace bike
