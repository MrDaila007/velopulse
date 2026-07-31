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
      {"selftest", 8, SerialCommand::kSelftest},
      {"ambient-raw", 11, SerialCommand::kAmbientRaw},
      {"ambient-stop", 12, SerialCommand::kAmbientStop},
      {"display-state", 13, SerialCommand::kDisplayState},
      {"wake-display", 12, SerialCommand::kWakeDisplay},
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
