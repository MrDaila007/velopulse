#include "littlefs_backend.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "storage_manager.h"

#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/printk.h>

namespace bike {
namespace {

#define BIKECOMP_LFS_NODE DT_NODELABEL(bikecomp_lfs)

#if !DT_NODE_EXISTS(BIKECOMP_LFS_NODE)
#error "bikecomp_lfs fstab node missing from devicetree overlay"
#endif

constexpr char kMountPoint[] = DT_PROP(BIKECOMP_LFS_NODE, mount_point);
constexpr char kProbeRelPath[] = "/.wr_probe";
constexpr char kLayoutMarkerRelPath[] = "/.layout_v3";

}  // namespace

extern "C" {
FS_FSTAB_DECLARE_ENTRY(BIKECOMP_LFS_NODE);
}

namespace {

struct fs_mount_t* fstabMountPoint() {
  return &FS_FSTAB_ENTRY(BIKECOMP_LFS_NODE);
}

void buildMountedPath(const char* path, char* out, size_t out_len) {
  if (path[0] == '/') {
    snprintf(out, out_len, "%s%s", kMountPoint, path);
  } else {
    snprintf(out, out_len, "%s/%s", kMountPoint, path);
  }
}

bool mountPointReady() {
  struct fs_dirent entry;
  return fs_stat(kMountPoint, &entry) == 0;
}

bool unmountFilesystem(struct fs_mount_t* mp) {
  if (!mountPointReady()) {
    return true;
  }
  if (fs_unmount(mp) != 0) {
    return false;
  }
  return !mountPointReady();
}

bool mountFilesystem(struct fs_mount_t* mp) {
  if (mountPointReady()) {
    return true;
  }
  return fs_mount(mp) == 0;
}

bool eraseStoragePartition() {
  if (mountPointReady()) {
    return false;
  }

  const struct flash_area* fa = nullptr;
  if (flash_area_open(FIXED_PARTITION_ID(storage_partition), &fa) != 0) {
    return false;
  }
  const int rc = flash_area_flatten(fa, 0, fa->fa_size);
  flash_area_close(fa);
  return rc == 0;
}

bool writeMountedFile(const char* mounted_path,
                      const uint8_t* data,
                      size_t length) {
  struct fs_file_t file;
  fs_file_t_init(&file);
  const int open_rc =
      fs_open(&file, mounted_path, FS_O_CREATE | FS_O_RDWR | FS_O_TRUNC);
  if (open_rc != 0) {
    printk("Storage: open %s failed err=%d\n", mounted_path, open_rc);
    return false;
  }
  const ssize_t written = fs_write(&file, data, length);
  const int sync_rc = fs_sync(&file);
  fs_close(&file);
  if (written < 0 || static_cast<size_t>(written) != length || sync_rc != 0) {
    printk("Storage: write %s failed written=%d sync=%d\n", mounted_path,
           static_cast<int>(written), sync_rc);
    return false;
  }
  return true;
}

bool layoutMarkerPresent() {
  char mounted_path[48];
  buildMountedPath(kLayoutMarkerRelPath, mounted_path, sizeof(mounted_path));
  struct fs_dirent entry;
  return fs_stat(mounted_path, &entry) == 0 &&
         entry.type == FS_DIR_ENTRY_FILE;
}

bool writeLayoutMarker() {
  char mounted_path[48];
  buildMountedPath(kLayoutMarkerRelPath, mounted_path, sizeof(mounted_path));
  static constexpr uint8_t kMarker[] = {0x03};
  return writeMountedFile(mounted_path, kMarker, sizeof(kMarker));
}

bool formatFilesystem(struct fs_mount_t* mp) {
  if (!unmountFilesystem(mp) || !eraseStoragePartition() || !mountFilesystem(mp)) {
    return false;
  }
  return writeLayoutMarker();
}

bool initializeLayoutV3(struct fs_mount_t* mp) {
  if (layoutMarkerPresent()) {
    return true;
  }

  printk("Storage: formatting layout v3 partition\n");
  return formatFilesystem(mp);
}

bool probeFilesystem() {
  char mounted_path[48];
  buildMountedPath(kProbeRelPath, mounted_path, sizeof(mounted_path));
  uint8_t probe[kMaximumRecordSize];
  memset(probe, 0xA5, sizeof(probe));
  if (!writeMountedFile(mounted_path, probe, sizeof(probe))) {
    return false;
  }

  uint8_t readback[kMaximumRecordSize];
  struct fs_file_t file;
  fs_file_t_init(&file);
  if (fs_open(&file, mounted_path, FS_O_READ) != 0) {
    return false;
  }
  const ssize_t bytes_read = fs_read(&file, readback, sizeof(readback));
  fs_close(&file);
  fs_unlink(mounted_path);
  return bytes_read == static_cast<ssize_t>(sizeof(readback)) &&
         memcmp(probe, readback, sizeof(readback)) == 0;
}

bool ensureHealthyFilesystem(struct fs_mount_t* mp) {
  if (!mountFilesystem(mp)) {
    if (!formatFilesystem(mp)) {
      return false;
    }
  }

  if (!initializeLayoutV3(mp)) {
    return false;
  }

  if (probeFilesystem()) {
    return true;
  }

  printk("Storage: probe failed, reformatting partition\n");
  return formatFilesystem(mp) && probeFilesystem();
}

}  // namespace

bool LittleFsBackend::begin() {
  if (mounted_) return true;
  mounted_ = ensureHealthyFilesystem(fstabMountPoint());
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

bool LittleFsBackend::write(const char* path,
                            const uint8_t* data,
                            size_t length) {
  if (!mounted_) return false;

  char mounted_path[48];
  buildMountedPath(path, mounted_path, sizeof(mounted_path));
  return writeMountedFile(mounted_path, data, length);
}

}  // namespace bike
