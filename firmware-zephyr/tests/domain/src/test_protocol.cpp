#include <zephyr/ztest.h>

#include <cstring>
#include <vector>

#include "ble_protocol.h"
#include "config_codec.h"
#include "fixture_loader.h"
#include "protocol_codec.h"
#include "protocol_samples.h"

using namespace bike;
using bike::test_support::loadFixtureHex;
using bike::test_support::makeMovingTelemetry;
using bike::test_support::makeNominalDeviceInfo;
using bike::test_support::makePausedTelemetry;

ZTEST(protocol, test_ble_protocol_sizes_match_contract) {
  zassert_equal(sizeof(DeviceInfoPacket), kDeviceInfoSize, "device info size");
  zassert_equal(sizeof(TelemetryPacket), kTelemetrySize, "telemetry size");
  zassert_equal(sizeof(ConfigurationPacket), kConfigurationSize, "config size");
}

ZTEST(protocol, test_protocol_fixture_device_info_v1_nominal) {
  std::vector<uint8_t> hex;
  zassert_true(loadFixtureHex("device_info_v1_nominal", hex), "load fixture");
  zassert_equal(kDeviceInfoSize, hex.size(), "fixture size");

  uint8_t encoded[kDeviceInfoSize];
  encodeDeviceInfo(makeNominalDeviceInfo(), encoded);
  zassert_mem_equal(hex.data(), encoded, kDeviceInfoSize, "device info encode");

  DeviceInfoPacket decoded = {};
  zassert_true(decodeDeviceInfo(hex.data(), hex.size(), decoded), "decode");
  zassert_equal(decoded.uptime_s, 3600u, "uptime");
  zassert_equal(decoded.boot_count, 42u, "boot count");
  zassert_equal(0, strncmp(decoded.model, "BIKECOMP-XIAO", 13), "model");
}

ZTEST(protocol, test_protocol_fixture_telemetry_v1_moving_and_paused) {
  std::vector<uint8_t> moving_hex;
  std::vector<uint8_t> paused_hex;
  zassert_true(loadFixtureHex("telemetry_v1_moving", moving_hex), "moving hex");
  zassert_true(loadFixtureHex("telemetry_v1_paused", paused_hex), "paused hex");

  uint8_t encoded[kTelemetrySize];
  encodeTelemetry(makeMovingTelemetry(), encoded);
  zassert_mem_equal(moving_hex.data(), encoded, kTelemetrySize, "moving encode");
  encodeTelemetry(makePausedTelemetry(), encoded);
  zassert_mem_equal(paused_hex.data(), encoded, kTelemetrySize, "paused encode");

  TelemetryPacket decoded = {};
  zassert_true(decodeTelemetry(moving_hex.data(), moving_hex.size(), decoded),
               "moving decode");
  zassert_equal(decoded.speed_x100, 2550u, "moving speed");
  zassert_equal(decoded.seq, 42u, "moving seq");

  zassert_true(decodeTelemetry(paused_hex.data(), paused_hex.size(), decoded),
               "paused decode");
  zassert_equal(decoded.speed_x100, 0u, "paused speed");
  zassert_equal(decoded.ride_state, 2u, "paused state");
}

ZTEST(protocol, test_protocol_fixture_config_v1_defaults_and_imperial) {
  std::vector<uint8_t> defaults_hex;
  std::vector<uint8_t> imperial_hex;
  zassert_true(loadFixtureHex("config_v1_defaults", defaults_hex), "defaults");
  zassert_true(loadFixtureHex("config_v1_imperial", imperial_hex), "imperial");

  DeviceConfig defaults;
  uint8_t encoded[kConfigurationSize];
  encodeConfiguration(defaults, encoded);
  zassert_mem_equal(defaults_hex.data(), encoded, kConfigurationSize,
                    "defaults encode");

  DeviceConfig imperial = defaults;
  imperial.units_imperial = true;
  encodeConfiguration(imperial, encoded);
  zassert_mem_equal(imperial_hex.data(), encoded, kConfigurationSize,
                    "imperial encode");

  DeviceConfig decoded;
  zassert_true(
      decodeConfiguration(defaults_hex.data(), defaults_hex.size(), decoded),
      "defaults decode");
  zassert_true(deviceConfigsEqual(defaults, decoded), "defaults round-trip");
  zassert_true(
      decodeConfiguration(imperial_hex.data(), imperial_hex.size(), decoded),
      "imperial decode");
  zassert_true(decoded.units_imperial, "imperial flag");
}

ZTEST(protocol, test_protocol_codec_rejects_bad_length_and_version) {
  uint8_t buffer[kDeviceInfoSize] = {};
  DeviceInfoPacket decoded = {};
  zassert_false(decodeDeviceInfo(buffer, kDeviceInfoSize - 1, decoded),
                "short buffer");
  buffer[0] = 0xFF;
  zassert_false(decodeDeviceInfo(buffer, kDeviceInfoSize, decoded),
                "bad version");
}

ZTEST_SUITE(protocol, NULL, NULL, NULL, NULL, NULL);
