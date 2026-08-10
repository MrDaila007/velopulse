#pragma once

#include <stdint.h>

namespace bike {

constexpr uint32_t kAmbientCalibrationMinSaveIntervalMs = 300000u;

enum class AmbientCalibrationSaveTrigger : uint8_t {
  kNone = 0,
  kThrottledChange,
  kDeepSleep,
  kReboot,
  kUsbDisconnect,
};

class AmbientCalibrationSavePolicy {
 public:
  void noteUsbPresent(bool usb_present);
  void requestDeepSleepSave();
  void requestRebootSave();

  AmbientCalibrationSaveTrigger evaluate(bool calibration_changed,
                                         uint32_t now_ms) const;
  void acknowledge(AmbientCalibrationSaveTrigger trigger, uint32_t now_ms);

 private:
  uint32_t last_saved_ms_ = 0;
  bool has_saved_ = false;
  bool usb_present_ = false;
  bool usb_known_ = false;
  bool usb_disconnect_pending_ = false;
  bool deep_sleep_pending_ = false;
  bool reboot_pending_ = false;
};

const char* ambientCalibrationSaveTriggerName(AmbientCalibrationSaveTrigger trigger);

}  // namespace bike
