#include "ble_device_info.h"

namespace bike {

ResetReason mapNrfResetReason(uint32_t resetreas) {
  if (resetreas == 0u) return ResetReason::kPowerOn;
  if ((resetreas & kNrfResetReasonDog) != 0u) return ResetReason::kWatchdog;
  if ((resetreas & kNrfResetReasonLockup) != 0u) return ResetReason::kLockup;
  if ((resetreas & kNrfResetReasonSreq) != 0u) return ResetReason::kSoftReset;
  if ((resetreas & kNrfResetReasonPin) != 0u) return ResetReason::kPinReset;
  if ((resetreas & (kNrfResetReasonOff | kNrfResetReasonLpcomp |
                    kNrfResetReasonDif | kNrfResetReasonNfc |
                    kNrfResetReasonVbus)) != 0u) {
    return ResetReason::kWakeFromSleep;
  }
  return ResetReason::kUnknown;
}

DeepSleepWakeSource classifyDeepSleepWakeSource(uint32_t resetreas) {
  if ((resetreas & kNrfResetReasonOff) != 0u) return DeepSleepWakeSource::kHall;
  if ((resetreas & kNrfResetReasonVbus) != 0u) return DeepSleepWakeSource::kUsb;
  return DeepSleepWakeSource::kNone;
}

bool isPairingWindowOpen(uint32_t boot_ms,
                         uint32_t now_ms,
                         uint32_t window_ms,
                         bool open_pairing_always) {
  if (open_pairing_always) return true;
  const uint32_t elapsed = static_cast<uint32_t>(now_ms - boot_ms);
  return elapsed < window_ms;
}

bool shouldRejectPairingRequest(uint32_t pairing_window_started_ms,
                                uint32_t now_ms,
                                uint32_t pairing_window_ms,
                                bool open_pairing_always,
                                bool connection_bonded) {
  return !connection_bonded &&
         !isPairingWindowOpen(pairing_window_started_ms, now_ms,
                              pairing_window_ms, open_pairing_always);
}

uint8_t buildDeviceInfoFlags(bool config_valid,
                             bool display_ok,
                             bool fs_ok,
                             bool bonded,
                             bool pairing_window_open,
                             bool usb_connected,
                             bool deep_sleep_supported) {
  uint8_t flags = 0;
  if (config_valid) flags |= kDeviceInfoFlagConfigValid;
  if (display_ok) flags |= kDeviceInfoFlagDisplayOk;
  if (fs_ok) flags |= kDeviceInfoFlagFsOk;
  if (bonded) flags |= kDeviceInfoFlagBonded;
  if (pairing_window_open) flags |= kDeviceInfoFlagPairingWindowOpen;
  if (usb_connected) flags |= kDeviceInfoFlagUsbConnected;
  if (deep_sleep_supported) flags |= kDeviceInfoFlagDeepSleepSupported;
  return flags;
}

void refreshDeviceInfoLiveFields(DeviceInfoPacket& info,
                                 uint32_t boot_ms,
                                 uint32_t now_ms,
                                 uint32_t pairing_window_started_ms,
                                 uint32_t pairing_window_ms,
                                 bool open_pairing_always,
                                 bool bonded,
                                 bool usb_connected,
                                 bool config_valid,
                                 bool display_ok,
                                 bool fs_ok,
                                 bool deep_sleep_supported) {
  info.uptime_s = static_cast<uint32_t>(now_ms - boot_ms) / 1000u;
  const bool pairing_open = isPairingWindowOpen(
      pairing_window_started_ms, now_ms, pairing_window_ms, open_pairing_always);
  info.flags = buildDeviceInfoFlags(config_valid, display_ok, fs_ok, bonded,
                                    pairing_open, usb_connected,
                                    deep_sleep_supported);
}

}  // namespace bike
