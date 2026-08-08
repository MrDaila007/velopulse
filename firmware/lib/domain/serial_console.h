#pragma once

#include <stddef.h>
#include <stdint.h>

namespace bike {

constexpr size_t kSerialCommandMaxLength = 128;
constexpr size_t kSerialCommandArgsMaxLength = 112;

enum class SerialCommand : uint8_t {
  kNone = 0,
  kOpenPairing,
  kDumpConfig,
  kResetOdometer,
  kReboot,
  kLoadConfig,
  kSetOdometerMm,
  kSelftest,
  kAmbientRaw,
  kAmbientStop,
  kDisplayState,
  kWakeDisplay,
  kHallStatus,
  kHallWatch,
  kHallStop,
  kHallRising,
  kHallFalling,
  kHallChange,
  kHallAnalog,
  kHallAnalogStop,
  kGpioProbe,
  kGpioWatch,
  kGpioStop,
  kPowerStatus,
  kStatus,
  kTestOn,
  kTestOff,
  kUnknown,
};

// Collects one command line without blocking the firmware loop. Both CR and LF
// terminate a line, so terminal CRLF sequences are accepted naturally.
class SerialCommandParser {
 public:
  SerialCommand feed(char value);
  const char* args() const;
  void reset();

 private:
  char buffer_[kSerialCommandMaxLength + 1] = {};
  char args_[kSerialCommandArgsMaxLength + 1] = {};
  size_t length_ = 0;
  bool overflow_ = false;
};

}  // namespace bike
