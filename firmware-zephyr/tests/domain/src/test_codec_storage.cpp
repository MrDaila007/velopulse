#include <zephyr/ztest.h>

#include <cstring>

#include "config_codec.h"
#include "config_validator.h"
#include "crc32.h"
#include "memory_storage_backend.h"
#include "storage_manager.h"
#include "storage_migration.h"

using namespace bike;
using bike::test_support::MemoryStorageBackend;

ZTEST(codec_storage, test_crc32_standard_vector) {
  const uint8_t input[] = "123456789";
  zassert_equal(0xCBF43926u, crc32(input, 9), "crc32 vector");
  zassert_equal(0u, crc32(input, 0), "crc32 empty");
}

ZTEST(codec_storage, test_config_codec_exact_48_byte_round_trip) {
  const uint8_t expected[kDeviceConfigPayloadSize] = {
      0x01, 0x0F, 0x34, 0x08, 0x64, 0x03, 0x3C, 0x00, 0x84, 0x03, 0x3C, 0x04,
      0x1F, 0x14, 0xF4, 0x01, 0x03, 0x03, 0x00, 0x00, 0xE8, 0x03, 0x00, 0x00,
      0x00, 0x01, 0x02, 0x03, 0x04, 0x00, 'B',  'i',  'k',  'e',  'C',  'o',
      'm',  'p',  '-',  'X',  'X',  'X',  'X',  0x00, 0x00, 0x00, 0x00, 0x00};
  DeviceConfig defaults;
  uint8_t encoded[kDeviceConfigPayloadSize];
  encodeDeviceConfig(defaults, encoded);
  zassert_mem_equal(expected, encoded, kDeviceConfigPayloadSize, "config encode");

  DeviceConfig decoded;
  zassert_true(decodeDeviceConfig(encoded, sizeof(encoded), decoded),
               "config decode");
  zassert_true(deviceConfigsEqual(defaults, decoded), "config round-trip");
}

ZTEST(codec_storage, test_config_validator_accepts_boundaries) {
  DeviceConfig minimum;
  minimum.wheel_circumference_mm = 500;
  minimum.max_speed_kmh = 20;
  minimum.stop_timeout_s = 1;
  minimum.display_timeout_s = 0;
  minimum.deep_sleep_timeout_s = 0;
  minimum.brightness_pct = 1;
  minimum.page_switch_period_s = 1;
  minimum.enabled_pages_mask = 1;
  minimum.low_battery_pct = 5;
  minimum.odometer_save_interval_m = 100;
  minimum.smoothing_window = 2;
  minimum.debounce_ms = 0;
  minimum.active_edge = 0;
  minimum.pinned_page = 0;
  minimum.batt_cal_scale_permille = 800;
  minimum.batt_cal_offset_mv = -500;
  memset(minimum.device_name, 0, sizeof(minimum.device_name));
  memcpy(minimum.device_name, "Ab3", 3);
  zassert_equal(static_cast<int>(ConfigValidationError::kNone),
                static_cast<int>(ConfigValidator::validate(minimum)),
                "minimum config");

  DeviceConfig maximum;
  maximum.wheel_circumference_mm = 3000;
  maximum.max_speed_kmh = 200;
  maximum.stop_timeout_s = 30;
  maximum.display_timeout_s = 600;
  maximum.deep_sleep_timeout_s = 3600;
  maximum.brightness_pct = 100;
  maximum.page_switch_period_s = 60;
  maximum.enabled_pages_mask = 0xFF;
  maximum.low_battery_pct = 50;
  maximum.odometer_save_interval_m = 5000;
  maximum.smoothing_window = 5;
  maximum.debounce_ms = 50;
  maximum.active_edge = 2;
  maximum.pinned_page = 4;
  maximum.batt_cal_scale_permille = 1200;
  maximum.batt_cal_offset_mv = 500;
  memcpy(maximum.device_name, "123456789012345", 16);
  zassert_equal(static_cast<int>(ConfigValidationError::kNone),
                static_cast<int>(ConfigValidator::validate(maximum)),
                "maximum config");
}

ZTEST(codec_storage, test_storage_selects_newest_slot) {
  MemoryStorageBackend backend;
  StorageManager storage(backend);
  zassert_true(storage.begin(), "storage begin");

  DeviceConfig loaded;
  StorageLoadInfo info;
  zassert_true(storage.loadConfig(loaded, info), "load defaults");
  zassert_equal(static_cast<int>(StorageSource::kDefaults),
                static_cast<int>(info.source), "defaults source");

  loaded.brightness_pct = 61;
  zassert_true(storage.saveConfig(loaded), "save changed");
  zassert_true(backend.files.count("/cfg_b") != 0, "slot b written");

  StorageManager reloaded(backend);
  zassert_true(reloaded.begin(), "reloaded begin");
  DeviceConfig from_flash;
  zassert_true(reloaded.loadConfig(from_flash, info), "reload config");
  zassert_equal(static_cast<int>(StorageSource::kSlotB),
                static_cast<int>(info.source), "slot b source");
  zassert_equal(61u, from_flash.brightness_pct, "brightness");
}

ZTEST(codec_storage, test_migrate_config_v1_to_v2_preserves_fields) {
  DeviceConfig original;
  original.brightness_pct = 55;
  original.wheel_circumference_mm = 2155;
  original.odometer_save_interval_m = 250;
  memcpy(original.device_name, "BikeComp-V1", 12);

  uint8_t payload_v1[kDeviceConfigPayloadSize];
  encodeDeviceConfig(original, payload_v1);

  DeviceConfig migrated;
  zassert_true(migrateConfigV1ToV2(payload_v1, sizeof(payload_v1), migrated),
               "migrate v1");
  zassert_true(deviceConfigsEqual(original, migrated), "fields preserved");
}

ZTEST_SUITE(codec_storage, NULL, NULL, NULL, NULL, NULL);
