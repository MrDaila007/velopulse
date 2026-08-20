#pragma once

#include <stdint.h>

#include "config.h"
#include "types.h"

namespace bike {

constexpr uint32_t kOdometerPauseSaveDelayMs = 180000u;  // 3 min after autopause
constexpr uint8_t kCriticalBatterySavePercent = 5u;

enum class OdometerSaveTrigger : uint8_t {
  kNone = 0,
  kDistance,       // legacy; no longer emitted
  kPausedSettle,
  kDisplayOff,     // legacy; no longer emitted
  kDeepSleep,
  kForceSave,
  kCriticalBattery,
  kUsbDisconnect,
  kReboot,
};

class OdometerSavePolicy {
 public:
  void configure(uint16_t interval_m);
  void markSaved(uint64_t odometer_mm);

  void noteRideState(RideState state, uint32_t now_ms);
  void noteBatteryPercent(uint8_t percent, bool valid);
  void noteUsbPresent(bool usb_present);

  void requestForceSave();
  void requestDeepSleepSave();
  void requestRebootSave();

  OdometerSaveTrigger evaluate(uint64_t odometer_mm, uint32_t now_ms) const;
  void acknowledge(OdometerSaveTrigger trigger);

  uint64_t savedOdometerMm() const { return saved_odometer_mm_; }
  uint32_t intervalMm() const { return interval_mm_; }

 private:
  uint32_t interval_mm_ = static_cast<uint32_t>(kDefaultOdometerSaveIntervalM) * 1000u;
  uint64_t saved_odometer_mm_ = 0;

  RideState ride_state_ = RideState::kIdle;
  uint32_t paused_since_ms_ = 0;
  bool pause_save_armed_ = false;
  bool pause_save_pending_ = false;

  bool critical_pending_ = false;
  bool critical_latched_ = false;

  bool usb_present_ = false;
  bool usb_known_ = false;
  bool usb_disconnect_pending_ = false;

  bool force_pending_ = false;
  bool deep_sleep_pending_ = false;
  bool reboot_pending_ = false;
};

const char* odometerSaveTriggerName(OdometerSaveTrigger trigger);

}  // namespace bike
