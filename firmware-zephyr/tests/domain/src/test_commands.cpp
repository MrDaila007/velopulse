#include <zephyr/ztest.h>

#include <cstring>
#include <vector>

#include "ble_command.h"
#include "ble_protocol.h"
#include "fixture_loader.h"

using namespace bike;
using bike::test_support::loadFixtureHex;

static std::vector<uint8_t> loadCommandFixture(const char* name) {
  std::vector<uint8_t> hex;
  zassert_true(loadFixtureHex(name, hex), "load fixture");
  return hex;
}

ZTEST(commands, test_command_fixture_reset_trip) {
  const std::vector<uint8_t> hex = loadCommandFixture("command_reset_trip");
  zassert_equal(4u, hex.size(), "reset trip size");

  SafeCommandParseResult parsed =
      parseSafeBleCommand(const_cast<uint8_t*>(hex.data()),
                          static_cast<uint16_t>(hex.size()));
  zassert_true(parsed.ok, "parse reset trip");
  zassert_equal(CommandId::kResetTrip, parsed.command_id, "command id");
}

ZTEST(commands, test_command_fixture_reset_odo_handshake) {
  const std::vector<uint8_t> request =
      loadCommandFixture("command_reset_odo_request");
  const std::vector<uint8_t> confirm =
      loadCommandFixture("command_reset_odo_with_token");

  const DangerousCommandParseResult request_parsed =
      parseDangerousBleCommand(const_cast<uint8_t*>(request.data()),
                               static_cast<uint16_t>(request.size()));
  zassert_true(request_parsed.ok, "parse odo request");
  zassert_equal(CommandId::kResetOdometer, request_parsed.command_id,
                "command id");

  const DangerousCommandParseResult confirm_parsed =
      parseDangerousBleCommand(const_cast<uint8_t*>(confirm.data()),
                               static_cast<uint16_t>(confirm.size()));
  zassert_true(confirm_parsed.ok, "parse odo confirm");
  zassert_true(confirm_parsed.has_token, "confirm has token");

  DangerousCommandSession session = {};
  const DangerousCommandHandshakeResult needs =
      processDangerousCommandHandshake(session, request_parsed, 1, true, 1000,
                                       0x12345678u);
  zassert_equal(CommandStatus::kNeedsConfirm, needs.status, "needs confirm");
  zassert_equal(0x12345678u, needs.token, "token");

  const DangerousCommandHandshakeResult success =
      processDangerousCommandHandshake(session, confirm_parsed, 1, true, 2000,
                                       0);
  zassert_equal(CommandStatus::kOk, success.status, "confirm ok");
  zassert_true(success.execute, "execute");
}

ZTEST(commands, test_command_fixture_sensor_test_start) {
  const std::vector<uint8_t> hex = loadCommandFixture("command_sensor_test_start");
  SafeCommandParseResult parsed =
      parseSafeBleCommand(const_cast<uint8_t*>(hex.data()),
                          static_cast<uint16_t>(hex.size()));
  zassert_true(parsed.ok, "parse sensor test");
  zassert_equal(CommandId::kSensorTestStart, parsed.command_id, "command id");
  zassert_equal(60u, parsed.params.sensor_test_duration_s, "duration");
}

ZTEST_SUITE(commands, nullptr, nullptr, nullptr, nullptr, nullptr);
