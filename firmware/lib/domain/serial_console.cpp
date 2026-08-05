#include "serial_console.h"

#include <string.h>

namespace bike {
namespace {

size_t trimLeading(const char* value, size_t length) {
  size_t start = 0;
  while (start < length &&
         (value[start] == ' ' || value[start] == '\t')) {
    ++start;
  }
  return start;
}

size_t trimTrailing(const char* value, size_t length) {
  while (length != 0 &&
         (value[length - 1] == ' ' || value[length - 1] == '\t')) {
    --length;
  }
  return length;
}

size_t commandNameLength(const char* value, size_t length) {
  size_t end = 0;
  while (end < length && value[end] != ' ' && value[end] != '\t') {
    ++end;
  }
  return end;
}

void copyArgs(const char* value, size_t length, char* out, size_t out_max) {
  const size_t start = trimLeading(value, length);
  length = trimTrailing(value + start, length - start);
  if (length > out_max) {
    out[0] = '\0';
    return;
  }
  if (length != 0) memcpy(out, value + start, length);
  out[length] = '\0';
}

SerialCommand parseCommandName(const char* value, size_t length) {
  struct CommandName {
    const char* name;
    size_t length;
    SerialCommand command;
  };
  constexpr CommandName kCommands[] = {
      {"open-pairing", 12, SerialCommand::kOpenPairing},
      {"dump-config", 11, SerialCommand::kDumpConfig},
      {"load-config", 11, SerialCommand::kLoadConfig},
      {"set-odo-mm", 10, SerialCommand::kSetOdometerMm},
      {"reset-odo", 9, SerialCommand::kResetOdometer},
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

SerialCommand parseLine(const char* value, size_t length, char* args_out,
                        size_t args_max) {
  const size_t start = trimLeading(value, length);
  length = trimTrailing(value + start, length - start);
  const size_t name_length = commandNameLength(value + start, length);
  const SerialCommand command =
      parseCommandName(value + start, name_length);
  if (command == SerialCommand::kNone || command == SerialCommand::kUnknown) {
    args_out[0] = '\0';
    return command;
  }
  const size_t args_offset = start + name_length;
  copyArgs(value + args_offset, length - name_length, args_out, args_max);
  return command;
}

}  // namespace

SerialCommand SerialCommandParser::feed(char value) {
  if (value != '\r' && value != '\n') {
    if (length_ == 0) {
      args_[0] = '\0';
    }
    if (length_ < kSerialCommandMaxLength) {
      buffer_[length_++] = value;
      buffer_[length_] = '\0';
    } else {
      overflow_ = true;
    }
    return SerialCommand::kNone;
  }

  const SerialCommand result =
      overflow_ ? SerialCommand::kUnknown
                : parseLine(buffer_, length_, args_, kSerialCommandArgsMaxLength);
  reset();
  return result;
}

const char* SerialCommandParser::args() const { return args_; }

void SerialCommandParser::reset() {
  length_ = 0;
  overflow_ = false;
  buffer_[0] = '\0';
}

}  // namespace bike
