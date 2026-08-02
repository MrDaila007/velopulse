#pragma once

#include <stddef.h>
#include <stdint.h>

#include "display_power.h"
#include "power_manager.h"
#include "types.h"

namespace bike {

constexpr size_t kUsbTestLineMax = 64;
constexpr size_t kUsbTestResponseMax = 96;

enum class UsbTestStatus : uint8_t {
  kOk = 0,
  kFail,
  kError,
};

struct UsbTestSnapshot {
  uint16_t speed_x100 = 0;
  uint32_t revolutions = 0;
  RideState ride_state = RideState::kIdle;
  uint32_t accepted_pulses = 0;
  uint32_t rejected_debounce = 0;
  uint32_t rejected_overspeed = 0;
  SystemPowerMode power_mode = SystemPowerMode::kNormal;
  bool deep_sleep_armed = false;
};

struct UsbTestHooks {
  void (*reset)(void* context, uint32_t now_ms);
  bool (*inject_pulse)(void* context,
                       uint32_t interval_us,
                       uint32_t now_ms,
                       char* detail,
                       size_t detail_len);
  void (*set_smoothing)(void* context, bool enabled);
  void (*snapshot)(void* context, UsbTestSnapshot* out);
  bool (*set_power_fixture)(void* context,
                            DisplayPowerState display_power,
                            uint32_t now_ms);
  void (*update_power)(void* context, uint32_t now_ms);
  void (*set_power_save)(void* context, bool enabled);
  void* context = nullptr;
};

struct UsbTestResult {
  UsbTestStatus status = UsbTestStatus::kError;
  char message[kUsbTestResponseMax] = {};
};

bool usbTestParseUint32(const char* text, uint32_t& out);
bool usbTestReadField(const UsbTestSnapshot& snapshot,
                      const char* field,
                      uint32_t& out);
void formatUsbTestResult(const UsbTestResult& result, char* out, size_t out_len);

UsbTestResult handleUsbTestLine(const char* line, const UsbTestHooks& hooks,
                                uint32_t now_ms);

}  // namespace bike
