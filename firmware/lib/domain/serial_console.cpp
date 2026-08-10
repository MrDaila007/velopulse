#include "serial_console.h"

#include <string.h>

namespace bike {
namespace {

SerialCommand parseCommand(const char* value, size_t length) {
  while (length != 0 && (*value == ' ' || *value == '\t')) {
    ++value;
    --length;
  }
  while (length != 0 &&
         (value[length - 1] == ' ' || value[length - 1] == '\t')) {
    --length;
  }

  struct CommandName {
    const char* name;
    size_t length;
    SerialCommand command;
  };
  constexpr CommandName kCommands[] = {
      {"open-pairing", 12, SerialCommand::kOpenPairing},
      {"dump-config", 11, SerialCommand::kDumpConfig},
      {"reset-odo", 9, SerialCommand::kResetOdometer},
      {"reboot", 6, SerialCommand::kReboot},
      {"selftest", 8, SerialCommand::kSelftest},
      {"ambient-raw", 11, SerialCommand::kAmbientRaw},
      {"ambient-stop", 12, SerialCommand::kAmbientStop},
      {"display-state", 13, SerialCommand::kDisplayState},
      {"wake-display", 12, SerialCommand::kWakeDisplay},
      {"hall-status", 11, SerialCommand::kHallStatus},
      {"hall-watch", 10, SerialCommand::kHallWatch},
      {"hall-stop", 9, SerialCommand::kHallStop},
      {"hall-rising", 11, SerialCommand::kHallRising},
      {"hall-falling", 12, SerialCommand::kHallFalling},
      {"hall-change", 11, SerialCommand::kHallChange},
      {"hall-analog", 11, SerialCommand::kHallAnalog},
      {"hall-analog-stop", 16, SerialCommand::kHallAnalogStop},
      {"gpio-probe", 10, SerialCommand::kGpioProbe},
      {"gpio-watch", 10, SerialCommand::kGpioWatch},
      {"gpio-stop", 9, SerialCommand::kGpioStop},
      {"power-status", 12, SerialCommand::kPowerStatus},
      {"status", 6, SerialCommand::kStatus},
      {"sched", 5, SerialCommand::kSchedStats},
      {"wdt-hang", 8, SerialCommand::kWdtHang},
      {"test-on", 7, SerialCommand::kTestOn},
      {"test-off", 8, SerialCommand::kTestOff},
  };

  for (const CommandName& candidate : kCommands) {
    if (length == candidate.length &&
        memcmp(value, candidate.name, length) == 0) {
      return candidate.command;
    }
  }
  return length == 0 ? SerialCommand::kNone : SerialCommand::kUnknown;
}

}  // namespace

SerialCommand SerialCommandParser::feed(char value) {
  if (value != '\r' && value != '\n') {
    if (length_ < kSerialCommandMaxLength) {
      buffer_[length_++] = value;
      buffer_[length_] = '\0';
    } else {
      overflow_ = true;
    }
    return SerialCommand::kNone;
  }

  const SerialCommand result =
      overflow_ ? SerialCommand::kUnknown : parseCommand(buffer_, length_);
  reset();
  return result;
}

void SerialCommandParser::reset() {
  length_ = 0;
  overflow_ = false;
  buffer_[0] = '\0';
}

}  // namespace bike
