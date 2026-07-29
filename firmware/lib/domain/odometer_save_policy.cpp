#include "odometer_save_policy.h"

namespace bike {

void OdometerSavePolicy::configure(uint16_t interval_m) {
  if (interval_m < 100) interval_m = 100;
  if (interval_m > 5000) interval_m = 5000;
  interval_mm_ = static_cast<uint32_t>(interval_m) * 1000u;
}

void OdometerSavePolicy::markSaved(uint64_t odometer_mm) {
  saved_odometer_mm_ = odometer_mm;
}

void OdometerSavePolicy::noteRideState(RideState state, uint32_t now_ms) {
  if (state == ride_state_) {
    if (pause_save_armed_ && !pause_save_pending_ &&
        state == RideState::kPaused &&
        static_cast<uint32_t>(now_ms - paused_since_ms_) >=
            kOdometerPauseSaveDelayMs) {
      pause_save_pending_ = true;
      pause_save_armed_ = false;
    }
    return;
  }

  if (ride_state_ == RideState::kMoving && state == RideState::kPaused) {
    paused_since_ms_ = now_ms;
    pause_save_armed_ = true;
    pause_save_pending_ = false;
  } else if (state == RideState::kMoving || state == RideState::kIdle) {
    pause_save_armed_ = false;
    pause_save_pending_ = false;
  }
  ride_state_ = state;
}

void OdometerSavePolicy::noteDisplayPower(DisplayPowerState state) {
  if (state == display_state_) return;
  if (state == DisplayPowerState::kOff) display_off_pending_ = true;
  display_state_ = state;
}

void OdometerSavePolicy::noteBatteryPercent(uint8_t percent, bool valid) {
  if (!valid) return;
  if (percent <= kCriticalBatterySavePercent) {
    if (!critical_latched_) {
      critical_pending_ = true;
      critical_latched_ = true;
    }
  } else {
    critical_latched_ = false;
  }
}

void OdometerSavePolicy::noteUsbPresent(bool usb_present) {
  if (!usb_known_) {
    usb_present_ = usb_present;
    usb_known_ = true;
    return;
  }
  if (usb_present_ && !usb_present) usb_disconnect_pending_ = true;
  usb_present_ = usb_present;
}

void OdometerSavePolicy::requestForceSave() { force_pending_ = true; }

void OdometerSavePolicy::requestDeepSleepSave() { deep_sleep_pending_ = true; }

void OdometerSavePolicy::requestRebootSave() { reboot_pending_ = true; }

OdometerSaveTrigger OdometerSavePolicy::evaluate(uint64_t odometer_mm,
                                                 uint32_t now_ms) const {
  if (force_pending_) return OdometerSaveTrigger::kForceSave;
  if (reboot_pending_) return OdometerSaveTrigger::kReboot;
  if (deep_sleep_pending_) return OdometerSaveTrigger::kDeepSleep;
  if (critical_pending_) return OdometerSaveTrigger::kCriticalBattery;
  if (usb_disconnect_pending_) return OdometerSaveTrigger::kUsbDisconnect;
  if (display_off_pending_) return OdometerSaveTrigger::kDisplayOff;

  if (pause_save_pending_ ||
      (pause_save_armed_ && ride_state_ == RideState::kPaused &&
       static_cast<uint32_t>(now_ms - paused_since_ms_) >=
           kOdometerPauseSaveDelayMs)) {
    return OdometerSaveTrigger::kPausedSettle;
  }

  if (odometer_mm >= saved_odometer_mm_ &&
      (odometer_mm - saved_odometer_mm_) >= interval_mm_) {
    return OdometerSaveTrigger::kDistance;
  }
  return OdometerSaveTrigger::kNone;
}

void OdometerSavePolicy::acknowledge(OdometerSaveTrigger trigger) {
  switch (trigger) {
    case OdometerSaveTrigger::kForceSave:
      force_pending_ = false;
      break;
    case OdometerSaveTrigger::kReboot:
      reboot_pending_ = false;
      break;
    case OdometerSaveTrigger::kDeepSleep:
      deep_sleep_pending_ = false;
      break;
    case OdometerSaveTrigger::kCriticalBattery:
      critical_pending_ = false;
      break;
    case OdometerSaveTrigger::kUsbDisconnect:
      usb_disconnect_pending_ = false;
      break;
    case OdometerSaveTrigger::kDisplayOff:
      display_off_pending_ = false;
      break;
    case OdometerSaveTrigger::kPausedSettle:
      pause_save_pending_ = false;
      pause_save_armed_ = false;
      break;
    case OdometerSaveTrigger::kDistance:
    case OdometerSaveTrigger::kNone:
      break;
  }
}

const char* odometerSaveTriggerName(OdometerSaveTrigger trigger) {
  switch (trigger) {
    case OdometerSaveTrigger::kDistance:
      return "distance";
    case OdometerSaveTrigger::kPausedSettle:
      return "paused_settle";
    case OdometerSaveTrigger::kDisplayOff:
      return "display_off";
    case OdometerSaveTrigger::kDeepSleep:
      return "deep_sleep";
    case OdometerSaveTrigger::kForceSave:
      return "force_save";
    case OdometerSaveTrigger::kCriticalBattery:
      return "critical_battery";
    case OdometerSaveTrigger::kUsbDisconnect:
      return "usb_disconnect";
    case OdometerSaveTrigger::kReboot:
      return "reboot";
    default:
      return "none";
  }
}

}  // namespace bike
