#include <zephyr/ztest.h>

#include <vector>

#include "companion_snapshot.h"
#include "fixture_loader.h"

using namespace bike;
using bike::test_support::loadFixtureHex;

ZTEST(companion_snapshot, test_fixture_roundtrip) {
  std::vector<uint8_t> fixture_hex;
  zassert_true(loadFixtureHex("companion_v1_nominal", fixture_hex), "load fixture");
  CompanionSnapshotPacket decoded = {};
  zassert_true(
      decodeCompanionSnapshot(fixture_hex.data(), fixture_hex.size(), decoded),
      "decode");
  zassert_equal(1, decoded.struct_version, "struct_version");
  zassert_equal(1704067200u, decoded.unix_time, "unix_time");
  zassert_equal(180, decoded.tz_offset_min, "tz_offset_min");
  zassert_equal(185, decoded.temp_c_x10, "temp_c_x10");
  zassert_equal(40, decoded.pop_pct, "pop_pct");
  zassert_equal(0x03, decoded.flags, "flags");
  zassert_equal(1704070800u, decoded.valid_until, "valid_until");

  uint8_t encoded[kCompanionSnapshotSize] = {};
  encodeCompanionSnapshot(decoded, encoded);
  zassert_mem_equal(fixture_hex.data(), encoded, kCompanionSnapshotSize,
                    "round-trip encode");
}

ZTEST(companion_snapshot, test_clock_and_header_stale) {
  CompanionSnapshotPacket packet = {};
  packet.struct_version = 1;
  packet.unix_time = 1704067200u;
  packet.tz_offset_min = 180;
  packet.temp_c_x10 = 185;
  packet.pop_pct = 40;
  packet.flags = kCompanionFlagTimeValid | kCompanionFlagWeatherValid;
  packet.valid_until = 1704070800u;

  CompanionState state;
  state.apply(packet, 1000u);
  const CompanionHeaderView header = state.header(1000u);
  zassert_true(header.valid, "header valid");
  zassert_false(header.stale, "header not stale immediately");
  zassert_str_equal("03:00", header.text, "formatted local time");

  const CompanionWeatherView weather = state.weather(1000u);
  zassert_true(weather.valid, "weather valid");
  zassert_str_equal("+18.5C", weather.temp, "formatted temp");
  zassert_str_equal("R40%", weather.rain, "formatted rain");

  const CompanionHeaderView stale = state.header(3700000u);
  zassert_true(stale.stale, "header stale after 3700s");
  const CompanionWeatherView stale_weather = state.weather(3700000u);
  zassert_true(stale_weather.stale, "weather stale after 3700s");
}

ZTEST_SUITE(companion_snapshot, NULL, NULL, NULL, NULL, NULL);
