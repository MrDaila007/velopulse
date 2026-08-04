#include <zephyr/ztest.h>

#include "power_manager.h"

using namespace bike;

ZTEST(power_manager, test_power_save_and_timeout) {
  PowerManager manager;
  PowerManagerConfig config;
  config.power_save_mode = true;
  config.deep_sleep_enabled = false;
  config.deep_sleep_timeout_s = 900;
  manager.configure(config);

  PowerManagerInput input;
  input.display_power = DisplayPowerState::kOff;
  input.now_ms = 1000;
  PowerManagerUpdateResult result = manager.update(input);
  zassert_true(result.mode_changed, "mode changed on entry");
  zassert_equal(static_cast<int>(SystemPowerMode::kLowPowerIdle),
                static_cast<int>(manager.systemMode()), "enters low power idle");
  zassert_false(manager.deepSleepArmed(), "deep sleep not armed yet");

  manager.noteActivity(2000);
  zassert_equal(static_cast<int>(SystemPowerMode::kNormal),
                static_cast<int>(manager.systemMode()), "activity exits low power");

  config.power_save_mode = false;
  config.deep_sleep_timeout_s = 60;
  manager.configure(config);
  input.now_ms = 0;
  manager.update(input);
  zassert_equal(static_cast<int>(SystemPowerMode::kNormal),
                static_cast<int>(manager.systemMode()), "normal before timeout");

  input.now_ms = 59000;
  manager.update(input);
  zassert_equal(static_cast<int>(SystemPowerMode::kNormal),
                static_cast<int>(manager.systemMode()), "still normal at 59s");

  input.now_ms = 60000;
  result = manager.update(input);
  zassert_equal(static_cast<int>(SystemPowerMode::kLowPowerIdle),
                static_cast<int>(manager.systemMode()), "idle at 60s timeout");
  zassert_true(manager.deepSleepArmed(), "deep sleep armed at timeout");
}

ZTEST(power_manager, test_deep_sleep_timeout_zero_and_ble_block) {
  PowerManager manager;
  PowerManagerConfig config;
  config.deep_sleep_timeout_s = 0;
  manager.configure(config);

  PowerManagerInput input;
  input.display_power = DisplayPowerState::kOff;
  input.now_ms = 100000;
  manager.update(input);
  zassert_equal(static_cast<int>(SystemPowerMode::kNormal),
                static_cast<int>(manager.systemMode()), "zero timeout never idles");

  config.deep_sleep_timeout_s = 60;
  manager.configure(config);
  input.ble_connected = true;
  manager.update(input);
  zassert_equal(static_cast<int>(SystemPowerMode::kNormal),
                static_cast<int>(manager.systemMode()), "BLE connection blocks idle");
  zassert_equal(static_cast<int>(PowerSleepBlockReason::kBleConnected),
                static_cast<int>(manager.blockReason()), "block reason is BLE");
}

ZTEST(power_manager, test_deep_sleep_save_request) {
  PowerManager manager;
  PowerManagerConfig config;
  config.deep_sleep_enabled = true;
  config.deep_sleep_timeout_s = 10;
  manager.configure(config);

  PowerManagerInput input;
  input.display_power = DisplayPowerState::kOff;
  input.now_ms = 0;
  manager.update(input);
  PowerManagerUpdateResult result = manager.update(input);
  zassert_false(result.request_deep_sleep_save, "no save request before timeout");

  input.now_ms = 10000;
  result = manager.update(input);
  zassert_true(result.request_deep_sleep_save, "save requested at timeout");
  zassert_true(manager.deepSleepArmed(), "armed at timeout");
}

ZTEST_SUITE(power_manager, NULL, NULL, NULL, NULL, NULL);
