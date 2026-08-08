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
constexpr const char* kTestOdoA = "/test_odo_a";
constexpr const char* kTestOdoB = "/test_odo_b";
bike::InternalFsBackend backend;

void removeTestFiles() {
  if (InternalFS.exists(kTestSlotA)) InternalFS.remove(kTestSlotA);
  if (InternalFS.exists(kTestSlotB)) InternalFS.remove(kTestSlotB);
  if (InternalFS.exists(kTestOdoA)) InternalFS.remove(kTestOdoA);
  if (InternalFS.exists(kTestOdoB)) InternalFS.remove(kTestOdoB);
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
  const bike::StoragePaths paths{kTestSlotA, kTestSlotB, kTestOdoA, kTestOdoB};
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

void test_odometer_ab_write_read_and_corrupt_fallback() {
  // Config slots unused; exercise /test_odo_a/b like production /odo_a/b.
  const bike::StoragePaths paths{kTestSlotA, kTestSlotB, kTestOdoA, kTestOdoB};
  bike::StorageManager storage(backend, paths);
  TEST_ASSERT_TRUE(storage.begin());
  bike::OdometerData odometer;
  bike::StorageLoadInfo info;
  TEST_ASSERT_TRUE(storage.loadOdometer(odometer, info));
  TEST_ASSERT_EQUAL(bike::StorageSource::kDefaults, info.source);
  TEST_ASSERT_TRUE(InternalFS.exists(kTestOdoA));

  odometer.odometer_mm = 123456789012ull;
  odometer.total_revolutions = 9876543210ull;
  TEST_ASSERT_TRUE(storage.saveOdometer(odometer));
  TEST_ASSERT_TRUE(InternalFS.exists(kTestOdoB));

  bike::StorageManager reloaded(backend, paths);
  TEST_ASSERT_TRUE(reloaded.begin());
  bike::OdometerData restored;
  TEST_ASSERT_TRUE(reloaded.loadOdometer(restored, info));
  TEST_ASSERT_EQUAL(bike::StorageSource::kSlotB, info.source);
  TEST_ASSERT_TRUE(restored.odometer_mm == 123456789012ull);
  TEST_ASSERT_TRUE(restored.total_revolutions == 9876543210ull);

  uint8_t record[bike::kMaximumRecordSize];
  size_t length = 0;
  TEST_ASSERT_EQUAL(bike::StorageIoResult::kOk,
                    backend.read(kTestOdoB, record, sizeof(record), length));
  record[bike::kRecordHeaderSize] ^= 0x80u;
  TEST_ASSERT_TRUE(backend.write(kTestOdoB, record, length));

  bike::StorageManager fallback(backend, paths);
  TEST_ASSERT_TRUE(fallback.begin());
  TEST_ASSERT_TRUE(fallback.loadOdometer(restored, info));
  TEST_ASSERT_EQUAL(bike::StorageSource::kSlotA, info.source);
  TEST_ASSERT_TRUE(restored.odometer_mm == 0ull);
  TEST_ASSERT_TRUE(restored.total_revolutions == 0ull);
  TEST_ASSERT_TRUE(info.recovered);
}

void setup() {
  // USB CDC re-enumerates after upload; wait long enough that UNITY_BEGIN and
  // the first RUN_TEST are not lost (previously PlatformIO reported 2/3).
  delay(2500);
  Serial.begin(115200);
  const uint32_t started = millis();
  while (!Serial && static_cast<uint32_t>(millis() - started) < 2000u) {
    delay(10);
  }
  delay(200);

  backend.begin();
  UNITY_BEGIN();
  RUN_TEST(test_display_is_present);
  RUN_TEST(test_internal_fs_ab_write_read_and_corrupt_fallback);
  RUN_TEST(test_odometer_ab_write_read_and_corrupt_fallback);
  UNITY_END();
}

void loop() {}
