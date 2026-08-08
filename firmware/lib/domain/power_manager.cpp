#include "power_manager.h"

namespace bike {

void PowerManager::configure(const PowerManagerConfig& config) {
  config_ = config;
}

void PowerManager::noteActivity(uint32_t now_ms) {
  if (mode_ != SystemPowerMode::kNormal) {
    mode_ = SystemPowerMode::kNormal;
    deep_sleep_save_requested_ = false;
  }
  deep_sleep_armed_ = false;
  display_off_tracked_ = false;
  display_off_since_ms_ = now_ms;
}

PowerSleepBlockReason PowerManager::evaluateBlockReason(
    const PowerManagerInput& input) const {
  if (input.ble_connected) return PowerSleepBlockReason::kBleConnected;
  if (input.charging) return PowerSleepBlockReason::kCharging;
  if (input.sensor_test_active) return PowerSleepBlockReason::kSensorTest;
  if (input.display_test_active) return PowerSleepBlockReason::kDisplayTest;
  if (input.display_power != DisplayPowerState::kOff) {
    return PowerSleepBlockReason::kDisplayOn;
  }
  return PowerSleepBlockReason::kNone;
}

bool PowerManager::shouldEnterLowPowerIdle(
    const PowerManagerInput& input) const {
  if (evaluateBlockReason(input) != PowerSleepBlockReason::kNone) return false;
  if (config_.power_save_mode) return true;
  if (config_.deep_sleep_timeout_s == 0) return false;
  if (!display_off_tracked_) return false;
  const uint32_t elapsed_ms =
      input.now_ms - display_off_since_ms_;
  const uint32_t timeout_ms =
      static_cast<uint32_t>(config_.deep_sleep_timeout_s) * 1000u;
  return elapsed_ms >= timeout_ms;
}

PowerManagerUpdateResult PowerManager::update(const PowerManagerInput& input) {
  PowerManagerUpdateResult result;
  const SystemPowerMode previous_mode = mode_;
  const bool previous_armed = deep_sleep_armed_;

  block_reason_ = evaluateBlockReason(input);

  if (input.display_power == DisplayPowerState::kOff) {
    if (!display_off_tracked_) {
      display_off_tracked_ = true;
      display_off_since_ms_ = input.now_ms;
    }
  } else {
    display_off_tracked_ = false;
    deep_sleep_armed_ = false;
    deep_sleep_save_requested_ = false;
  }

  if (block_reason_ != PowerSleepBlockReason::kNone) {
    mode_ = SystemPowerMode::kNormal;
  } else if (shouldEnterLowPowerIdle(input)) {
    if (mode_ != SystemPowerMode::kLowPowerIdle) {
      ++low_power_entry_count_;
    }
    mode_ = SystemPowerMode::kLowPowerIdle;

    if (config_.deep_sleep_timeout_s != 0 && display_off_tracked_) {
      const uint32_t elapsed_ms =
          input.now_ms - display_off_since_ms_;
      const uint32_t timeout_ms =
          static_cast<uint32_t>(config_.deep_sleep_timeout_s) * 1000u;
      deep_sleep_armed_ = elapsed_ms >= timeout_ms;
    } else {
      deep_sleep_armed_ = false;
    }

    if (deep_sleep_armed_ && config_.deep_sleep_enabled &&
        !deep_sleep_save_requested_) {
      result.request_deep_sleep_save = true;
      deep_sleep_save_requested_ = true;
    }
#if defined(BIKECOMP_FEATURE_DEEP_SLEEP) && BIKECOMP_FEATURE_DEEP_SLEEP
    if (deep_sleep_armed_ && config_.deep_sleep_enabled) {
      result.request_enter_deep_sleep = true;
    }
#endif
  } else {
    mode_ = SystemPowerMode::kNormal;
    deep_sleep_armed_ = false;
    deep_sleep_save_requested_ = false;
  }

  result.mode_changed = previous_mode != mode_;
  result.deep_sleep_armed_changed = previous_armed != deep_sleep_armed_;
  return result;
}

const char* systemPowerModeName(SystemPowerMode mode) {
  switch (mode) {
    case SystemPowerMode::kLowPowerIdle:
      return "low_power_idle";
    case SystemPowerMode::kNormal:
    default:
      return "normal";
  }
}

const char* powerSleepBlockReasonName(PowerSleepBlockReason reason) {
  switch (reason) {
    case PowerSleepBlockReason::kBleConnected:
      return "ble_connected";
    case PowerSleepBlockReason::kCharging:
      return "charging";
    case PowerSleepBlockReason::kSensorTest:
      return "sensor_test";
    case PowerSleepBlockReason::kDisplayTest:
      return "display_test";
    case PowerSleepBlockReason::kDisplayOn:
      return "display_on";
    case PowerSleepBlockReason::kNone:
    default:
      return "none";
  }
}

}  // namespace bike
