#pragma once

#include <stdint.h>

#include "display_power.h"
#include "types.h"

namespace bike {

enum class SystemPowerMode : uint8_t {
  kNormal = 0,
  kLowPowerIdle = 1,
};

enum class PowerSleepBlockReason : uint8_t {
  kNone = 0,
  kBleConnected,
  kCharging,
  kSensorTest,
  kDisplayTest,
  kDisplayOn,
};

struct PowerManagerConfig {
  bool power_save_mode = false;
  bool deep_sleep_enabled = false;
  uint16_t deep_sleep_timeout_s = 900;
};

struct PowerManagerInput {
  RideState ride_state = RideState::kIdle;
  DisplayPowerState display_power = DisplayPowerState::kBright;
  bool ble_connected = false;
  bool charging = false;
  bool sensor_test_active = false;
  bool display_test_active = false;
  uint32_t now_ms = 0;
};

struct PowerManagerUpdateResult {
  bool mode_changed = false;
  bool deep_sleep_armed_changed = false;
  bool request_deep_sleep_save = false;
  bool request_enter_deep_sleep = false;
};

struct SchedulerPeriods {
  uint32_t pulses_ms = 0;
  uint32_t state_ms = 100;
  uint32_t ambient_ms = 10;
  uint32_t battery_ms = 1000;
  uint32_t display_ms = 50;
  uint32_t ble_ms = 100;
};

constexpr SchedulerPeriods kNormalSchedulerPeriods{};
constexpr SchedulerPeriods kLowPowerSchedulerPeriods{
    10, 200, 2000, 5000, 1000, 1000,
};

class PowerManager {
 public:
  void configure(const PowerManagerConfig& config);
  PowerManagerUpdateResult update(const PowerManagerInput& input);
  void noteActivity(uint32_t now_ms);

  SystemPowerMode systemMode() const { return mode_; }
  bool deepSleepArmed() const { return deep_sleep_armed_; }
  bool sleepBlocked() const { return block_reason_ != PowerSleepBlockReason::kNone; }
  PowerSleepBlockReason blockReason() const { return block_reason_; }
  uint32_t displayOffSinceMs() const { return display_off_since_ms_; }
  uint32_t lowPowerIdleEntryCount() const { return low_power_entry_count_; }

  SchedulerPeriods schedulerPeriods() const {
    return mode_ == SystemPowerMode::kLowPowerIdle ? kLowPowerSchedulerPeriods
                                                    : kNormalSchedulerPeriods;
  }

  bool aggressiveBlePowerSave() const {
    return config_.power_save_mode ||
           (mode_ == SystemPowerMode::kLowPowerIdle && !ble_always_advertise_);
  }

  void setBleAlwaysAdvertise(bool enabled) { ble_always_advertise_ = enabled; }

 private:
  PowerSleepBlockReason evaluateBlockReason(const PowerManagerInput& input) const;
  bool shouldEnterLowPowerIdle(const PowerManagerInput& input) const;

  PowerManagerConfig config_;
  bool ble_always_advertise_ = true;
  SystemPowerMode mode_ = SystemPowerMode::kNormal;
  bool deep_sleep_armed_ = false;
  PowerSleepBlockReason block_reason_ = PowerSleepBlockReason::kNone;
  uint32_t display_off_since_ms_ = 0;
  bool display_off_tracked_ = false;
  uint32_t low_power_entry_count_ = 0;
  bool deep_sleep_save_requested_ = false;
};

const char* systemPowerModeName(SystemPowerMode mode);
const char* powerSleepBlockReasonName(PowerSleepBlockReason reason);

}  // namespace bike
