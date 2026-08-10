#include "ambient_calibration_save_policy.h"

namespace bike {

void AmbientCalibrationSavePolicy::noteUsbPresent(bool usb_present) {
  if (!usb_known_) {
    usb_present_ = usb_present;
    usb_known_ = true;
    return;
  }
  if (usb_present_ && !usb_present) usb_disconnect_pending_ = true;
  usb_present_ = usb_present;
}

void AmbientCalibrationSavePolicy::requestDeepSleepSave() {
  deep_sleep_pending_ = true;
}

void AmbientCalibrationSavePolicy::requestRebootSave() { reboot_pending_ = true; }

AmbientCalibrationSaveTrigger AmbientCalibrationSavePolicy::evaluate(
    bool calibration_changed, uint32_t now_ms) const {
  if (reboot_pending_) return AmbientCalibrationSaveTrigger::kReboot;
  if (deep_sleep_pending_) return AmbientCalibrationSaveTrigger::kDeepSleep;
  if (usb_disconnect_pending_) return AmbientCalibrationSaveTrigger::kUsbDisconnect;
  if (calibration_changed &&
      (!has_saved_ || static_cast<uint32_t>(now_ms - last_saved_ms_) >=
                          kAmbientCalibrationMinSaveIntervalMs)) {
    return AmbientCalibrationSaveTrigger::kThrottledChange;
  }
  return AmbientCalibrationSaveTrigger::kNone;
}

void AmbientCalibrationSavePolicy::acknowledge(
    AmbientCalibrationSaveTrigger trigger, uint32_t now_ms) {
  if (trigger == AmbientCalibrationSaveTrigger::kNone) return;
  switch (trigger) {
    case AmbientCalibrationSaveTrigger::kReboot:
      reboot_pending_ = false;
      break;
    case AmbientCalibrationSaveTrigger::kDeepSleep:
      deep_sleep_pending_ = false;
      break;
    case AmbientCalibrationSaveTrigger::kUsbDisconnect:
      usb_disconnect_pending_ = false;
      break;
    case AmbientCalibrationSaveTrigger::kThrottledChange:
    case AmbientCalibrationSaveTrigger::kNone:
      break;
  }
  last_saved_ms_ = now_ms;
  has_saved_ = true;
}

const char* ambientCalibrationSaveTriggerName(AmbientCalibrationSaveTrigger trigger) {
  switch (trigger) {
    case AmbientCalibrationSaveTrigger::kThrottledChange:
      return "throttled_change";
    case AmbientCalibrationSaveTrigger::kDeepSleep:
      return "deep_sleep";
    case AmbientCalibrationSaveTrigger::kReboot:
      return "reboot";
    case AmbientCalibrationSaveTrigger::kUsbDisconnect:
      return "usb_disconnect";
    default:
      return "none";
  }
}

}  // namespace bike
