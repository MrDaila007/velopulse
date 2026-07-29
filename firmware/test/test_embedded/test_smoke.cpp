#include <Arduino.h>
#include <InternalFileSystem.h>
#include <Wire.h>
#include <unity.h>

#include "board_pins.h"
#include "config.h"
#include "../../src/internal_fs_backend.cpp"

namespace {

constexpr const char* kTestSlotA = "/test_storage_a";
constexpr const char* kTestSlotB = "/test_storage_b";
constexpr const char* kUnusedOdometerA = "/test_odo_a";
constexpr const char* kUnusedOdometerB = "/test_odo_b";
bike::InternalFsBackend backend;

void removeTestFiles() {
  if (InternalFS.exists(kTestSlotA)) InternalFS.remove(kTestSlotA);
  if (InternalFS.exists(kTestSlotB)) InternalFS.remove(kTestSlotB);
  if (InternalFS.exists(kUnusedOdometerA)) InternalFS.remove(kUnusedOdometerA);
  if (InternalFS.exists(kUnusedOdometerB)) InternalFS.remove(kUnusedOdometerB);
}

}  // namespace

void setUp() { removeTestFiles(); }
void tearDown() { removeTestFiles(); }

void test_display_is_present() {
  Wire.begin();
  Wire.beginTransmission(bike::kDisplayI2cAddress);
  TEST_ASSERT_EQUAL_UINT8(0, Wire.endTransmission());
}

void test_internal_fs_ab_write_read_and_corrupt_fallback() {
  const bike::StoragePaths paths{kTestSlotA, kTestSlotB,
                                 kUnusedOdometerA, kUnusedOdometerB};
  bike::StorageManager storage(backend, paths);
  TEST_ASSERT_TRUE(storage.begin());
  bike::DeviceConfig config;
  bike::StorageLoadInfo info;
  TEST_ASSERT_TRUE(storage.loadConfig(config, info));
  TEST_ASSERT_EQUAL(bike::StorageSource::kDefaults, info.source);
  TEST_ASSERT_TRUE(InternalFS.exists(kTestSlotA));

  config.brightness_pct = 61;
  TEST_ASSERT_TRUE(storage.saveConfig(config));
  TEST_ASSERT_TRUE(InternalFS.exists(kTestSlotB));
  config.brightness_pct = 62;
  TEST_ASSERT_TRUE(storage.saveConfig(config));

  bike::StorageManager reload(backend, paths);
  TEST_ASSERT_TRUE(reload.begin());
  bike::DeviceConfig loaded;
  TEST_ASSERT_TRUE(reload.loadConfig(loaded, info));
  TEST_ASSERT_EQUAL(bike::StorageSource::kSlotA, info.source);
  TEST_ASSERT_EQUAL_UINT32(3u, info.sequence);
  TEST_ASSERT_EQUAL_UINT8(62u, loaded.brightness_pct);

  uint8_t record[bike::kMaximumRecordSize];
  size_t length = 0;
  TEST_ASSERT_EQUAL(bike::StorageIoResult::kOk,
                    backend.read(kTestSlotA, record, sizeof(record), length));
  record[bike::kRecordHeaderSize + 10] ^= 0x80u;
  TEST_ASSERT_TRUE(backend.write(kTestSlotA, record, length));

  bike::StorageManager fallback(backend, paths);
  TEST_ASSERT_TRUE(fallback.begin());
  TEST_ASSERT_TRUE(fallback.loadConfig(loaded, info));
  TEST_ASSERT_EQUAL(bike::StorageSource::kSlotB, info.source);
  TEST_ASSERT_EQUAL_UINT32(2u, info.sequence);
  TEST_ASSERT_EQUAL_UINT8(61u, loaded.brightness_pct);
  TEST_ASSERT_TRUE(info.recovered);
}

void setup() {
  delay(1500);
  backend.begin();
  UNITY_BEGIN();
  RUN_TEST(test_display_is_present);
  RUN_TEST(test_internal_fs_ab_write_read_and_corrupt_fallback);
  UNITY_END();
}

void loop() {}
