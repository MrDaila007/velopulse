#pragma once

#include <stdint.h>

#include "ble_protocol.h"

namespace bike {

// nRF52 POWER.RESETREAS bit positions (host-testable without Nordic headers).
constexpr uint32_t kNrfResetReasonPin = 1u << 0;
constexpr uint32_t kNrfResetReasonDog = 1u << 1;
constexpr uint32_t kNrfResetReasonSreq = 1u << 2;
constexpr uint32_t kNrfResetReasonLockup = 1u << 3;
constexpr uint32_t kNrfResetReasonOff = 1u << 16;
constexpr uint32_t kNrfResetReasonLpcomp = 1u << 17;
constexpr uint32_t kNrfResetReasonDif = 1u << 18;
constexpr uint32_t kNrfResetReasonNfc = 1u << 19;
constexpr uint32_t kNrfResetReasonVbus = 1u << 20;

// Default pairing window after boot (protocol §17 / architecture §11.5).
constexpr uint32_t kDefaultPairingWindowMs = 5u * 60u * 1000u;

// Maps Nordic RESETREAS bits to protocol ResetReason. Priority when several
// bits are set: watchdog > lockup > soft > pin > wake-from-off > power-on.
ResetReason mapNrfResetReason(uint32_t resetreas);

// True while the post-boot pairing window is open, or when open_pairing_always
// is set (prototype FEATURE_OPEN_PAIRING).
bool isPairingWindowOpen(uint32_t boot_ms,
                         uint32_t now_ms,
                         uint32_t window_ms,
                         bool open_pairing_always);

// Rejects only a new pairing request outside the active window. A connection
// already resolved against a stored bond remains allowed after the window.
bool shouldRejectPairingRequest(uint32_t pairing_window_started_ms,
                                uint32_t now_ms,
                                uint32_t pairing_window_ms,
                                bool open_pairing_always,
                                bool connection_bonded);

// Assembles Device Info flags from live runtime bits.
uint8_t buildDeviceInfoFlags(bool config_valid,
                             bool display_ok,
                             bool fs_ok,
                             bool bonded,
                             bool pairing_window_open,
                             bool usb_connected,
                             bool deep_sleep_supported);

// Fills uptime_s and flags on an existing DeviceInfoPacket (static fields kept).
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
                                 bool deep_sleep_supported);

}  // namespace bike
