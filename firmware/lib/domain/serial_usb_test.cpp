#include "serial_usb_test.h"

#include <stdio.h>
#include <string.h>

namespace bike {
namespace {

void setMessage(UsbTestResult& result, UsbTestStatus status, const char* message) {
  result.status = status;
  strncpy(result.message, message, sizeof(result.message) - 1u);
  result.message[sizeof(result.message) - 1u] = '\0';
}

bool startsWith(const char* line, const char* prefix) {
  return strncmp(line, prefix, strlen(prefix)) == 0;
}

bool startsWithWord(const char* line, const char* word) {
  if (!startsWith(line, word)) return false;
  const char next = line[strlen(word)];
  return next == '\0' || next == ' ' || next == '\t';
}

const char* skipSpaces(const char* cursor) {
  while (*cursor == ' ' || *cursor == '\t') ++cursor;
  return cursor;
}

bool parseFieldAndValue(const char* line,
                        const char* command,
                        char* field_out,
                        size_t field_out_len,
                        const char*& value) {
  if (!startsWithWord(line, command)) return false;
  const char* field = skipSpaces(line + strlen(command));
  if (*field == '\0') return false;
  const char* space = strchr(field, ' ');
  if (space == nullptr) return false;
  const size_t field_len = static_cast<size_t>(space - field);
  if (field_len + 1u > field_out_len) return false;
  memcpy(field_out, field, field_len);
  field_out[field_len] = '\0';
  value = skipSpaces(space + 1);
  return *value != '\0';
}

}  // namespace

bool usbTestParseUint32(const char* text, uint32_t& out) {
  if (text == nullptr || *text == '\0') return false;
  uint32_t value = 0;
  for (const char* cursor = text; *cursor != '\0'; ++cursor) {
    if (*cursor < '0' || *cursor > '9') return false;
    value = value * 10u + static_cast<uint32_t>(*cursor - '0');
  }
  out = value;
  return true;
}

bool usbTestReadField(const UsbTestSnapshot& snapshot,
                      const char* field,
                      uint32_t& out) {
  if (strcmp(field, "speed_x100") == 0) {
    out = snapshot.speed_x100;
    return true;
  }
  if (strcmp(field, "revolutions") == 0) {
    out = snapshot.revolutions;
    return true;
  }
  if (strcmp(field, "ride_state") == 0) {
    out = static_cast<uint32_t>(snapshot.ride_state);
    return true;
  }
  if (strcmp(field, "accepted") == 0) {
    out = snapshot.accepted_pulses;
    return true;
  }
  if (strcmp(field, "rejected_debounce") == 0) {
    out = snapshot.rejected_debounce;
    return true;
  }
  if (strcmp(field, "rejected_overspeed") == 0) {
    out = snapshot.rejected_overspeed;
    return true;
  }
  if (strcmp(field, "power_mode") == 0) {
    out = static_cast<uint32_t>(snapshot.power_mode);
    return true;
  }
  if (strcmp(field, "deep_sleep_armed") == 0) {
    out = snapshot.deep_sleep_armed ? 1u : 0u;
    return true;
  }
  return false;
}

void formatUsbTestResult(const UsbTestResult& result, char* out, size_t out_len) {
  const char* prefix = "ERROR";
  if (result.status == UsbTestStatus::kOk) prefix = "OK";
  if (result.status == UsbTestStatus::kFail) prefix = "FAIL";
  snprintf(out, out_len, "%s %s", prefix, result.message);
}

UsbTestResult handleUsbTestLine(const char* line, const UsbTestHooks& hooks,
                                uint32_t now_ms) {
  UsbTestResult result;
  if (line == nullptr || hooks.context == nullptr) {
    setMessage(result, UsbTestStatus::kError, "hooks");
    return result;
  }

  while (*line == ' ' || *line == '\t') ++line;
  if (*line == '\0' || *line == '#') {
    setMessage(result, UsbTestStatus::kOk, "noop");
    return result;
  }

  if (strcmp(line, "reset") == 0) {
    if (hooks.reset == nullptr) {
      setMessage(result, UsbTestStatus::kError, "reset hook");
      return result;
    }
    hooks.reset(hooks.context, now_ms);
    setMessage(result, UsbTestStatus::kOk, "reset");
    return result;
  }

  if (startsWith(line, "pulse")) {
    if (hooks.inject_pulse == nullptr) {
      setMessage(result, UsbTestStatus::kError, "pulse hook");
      return result;
    }
    const char* value = skipSpaces(line + 5);
    uint32_t interval_us = 0;
    if (*value != '\0' && !usbTestParseUint32(value, interval_us)) {
      setMessage(result, UsbTestStatus::kError, "pulse value");
      return result;
    }
    char detail[64] = {};
    if (!hooks.inject_pulse(hooks.context, interval_us, now_ms, detail,
                            sizeof(detail))) {
      snprintf(result.message, sizeof(result.message), "pulse %s", detail);
      result.status = UsbTestStatus::kFail;
      return result;
    }
    snprintf(result.message, sizeof(result.message), "pulse %s", detail);
    result.status = UsbTestStatus::kOk;
    return result;
  }

  if (startsWith(line, "smooth")) {
    if (hooks.set_smoothing == nullptr) {
      setMessage(result, UsbTestStatus::kError, "smooth hook");
      return result;
    }
    const char* value = skipSpaces(line + 6);
    if (strcmp(value, "0") == 0) {
      hooks.set_smoothing(hooks.context, false);
      setMessage(result, UsbTestStatus::kOk, "smooth 0");
      return result;
    }
    if (strcmp(value, "1") == 0) {
      hooks.set_smoothing(hooks.context, true);
      setMessage(result, UsbTestStatus::kOk, "smooth 1");
      return result;
    }
    setMessage(result, UsbTestStatus::kError, "smooth value");
    return result;
  }

  if (startsWith(line, "cfg")) {
    const char* args = skipSpaces(line + 3);
    if (startsWith(args, "power_save")) {
      if (hooks.set_power_save == nullptr) {
        setMessage(result, UsbTestStatus::kError, "cfg hook");
        return result;
      }
      args = skipSpaces(args + 10);
      if (strcmp(args, "0") == 0) {
        hooks.set_power_save(hooks.context, false);
        setMessage(result, UsbTestStatus::kOk, "cfg power_save 0");
        return result;
      }
      if (strcmp(args, "1") == 0) {
        hooks.set_power_save(hooks.context, true);
        setMessage(result, UsbTestStatus::kOk, "cfg power_save 1");
        return result;
      }
      setMessage(result, UsbTestStatus::kError, "cfg power_save value");
      return result;
    }
    setMessage(result, UsbTestStatus::kError, "cfg key");
    return result;
  }

  if (startsWith(line, "power")) {
    if (hooks.set_power_fixture == nullptr || hooks.update_power == nullptr) {
      setMessage(result, UsbTestStatus::kError, "power hook");
      return result;
    }
    const char* args = skipSpaces(line + 5);
    DisplayPowerState display = DisplayPowerState::kBright;
    uint32_t fixture_now_ms = now_ms;
    if (startsWith(args, "bright")) {
      display = DisplayPowerState::kBright;
      args = skipSpaces(args + 6);
    } else if (startsWith(args, "off")) {
      display = DisplayPowerState::kOff;
      args = skipSpaces(args + 3);
    } else {
      setMessage(result, UsbTestStatus::kError, "power state");
      return result;
    }
    if (*args != '\0') {
      if (!usbTestParseUint32(args, fixture_now_ms)) {
        setMessage(result, UsbTestStatus::kError, "power now_ms");
        return result;
      }
    }
    if (!hooks.set_power_fixture(hooks.context, display, fixture_now_ms)) {
      setMessage(result, UsbTestStatus::kFail, "power fixture");
      return result;
    }
    hooks.update_power(hooks.context, fixture_now_ms);
  UsbTestSnapshot snapshot = {};
    hooks.snapshot(hooks.context, &snapshot);
    snprintf(result.message, sizeof(result.message),
             "power mode=%u armed=%u",
             static_cast<unsigned>(snapshot.power_mode),
             snapshot.deep_sleep_armed ? 1u : 0u);
    result.status = UsbTestStatus::kOk;
    return result;
  }

  if (strcmp(line, "snapshot") == 0) {
    if (hooks.snapshot == nullptr) {
      setMessage(result, UsbTestStatus::kError, "snapshot hook");
      return result;
    }
    UsbTestSnapshot snapshot = {};
    hooks.snapshot(hooks.context, &snapshot);
    snprintf(result.message, sizeof(result.message),
             "snapshot speed_x100=%u rev=%lu ride=%u accepted=%lu",
             snapshot.speed_x100,
             static_cast<unsigned long>(snapshot.revolutions),
             static_cast<unsigned>(snapshot.ride_state),
             static_cast<unsigned long>(snapshot.accepted_pulses));
    result.status = UsbTestStatus::kOk;
    return result;
  }

  if (startsWith(line, "read")) {
    const char* field = skipSpaces(line + 4);
    if (*field == '\0') {
      setMessage(result, UsbTestStatus::kError, "read field");
      return result;
    }
    if (hooks.snapshot == nullptr) {
      setMessage(result, UsbTestStatus::kError, "snapshot hook");
      return result;
    }
    UsbTestSnapshot snapshot = {};
    hooks.snapshot(hooks.context, &snapshot);
    uint32_t actual = 0;
    if (!usbTestReadField(snapshot, field, actual)) {
      setMessage(result, UsbTestStatus::kError, "unknown field");
      return result;
    }
    snprintf(result.message, sizeof(result.message), "read %s=%lu", field,
             static_cast<unsigned long>(actual));
    result.status = UsbTestStatus::kOk;
    return result;
  }

  if (startsWith(line, "expect-range")) {
    const char* args = skipSpaces(line + 12);
    char range_field[kUsbTestLineMax] = {};
    const char* rest = strchr(args, ' ');
    if (rest == nullptr) {
      setMessage(result, UsbTestStatus::kError, "expect-range field");
      return result;
    }
    const size_t field_len = static_cast<size_t>(rest - args);
    if (field_len + 1u > sizeof(range_field)) {
      setMessage(result, UsbTestStatus::kError, "expect-range field");
      return result;
    }
    memcpy(range_field, args, field_len);
    range_field[field_len] = '\0';
    rest = skipSpaces(rest + 1);
    const char* max_text = strchr(rest, ' ');
    if (max_text == nullptr) {
      setMessage(result, UsbTestStatus::kError, "expect-range bounds");
      return result;
    }
    char min_buf[16] = {};
    const size_t min_len = static_cast<size_t>(max_text - rest);
    if (min_len >= sizeof(min_buf)) {
      setMessage(result, UsbTestStatus::kError, "expect-range min");
      return result;
    }
    memcpy(min_buf, rest, min_len);
    min_buf[min_len] = '\0';
    max_text = skipSpaces(max_text + 1);
    uint32_t min_value = 0;
    uint32_t max_value = 0;
    if (!usbTestParseUint32(min_buf, min_value) ||
        !usbTestParseUint32(max_text, max_value)) {
      setMessage(result, UsbTestStatus::kError, "expect-range bounds");
      return result;
    }
    if (hooks.snapshot == nullptr) {
      setMessage(result, UsbTestStatus::kError, "snapshot hook");
      return result;
    }
    UsbTestSnapshot snapshot = {};
    hooks.snapshot(hooks.context, &snapshot);
    uint32_t actual = 0;
    if (!usbTestReadField(snapshot, range_field, actual)) {
      setMessage(result, UsbTestStatus::kError, "unknown field");
      return result;
    }
    if (actual >= min_value && actual <= max_value) {
      snprintf(result.message, sizeof(result.message),
               "expect-range %s=%lu in [%lu,%lu]", range_field,
               static_cast<unsigned long>(actual),
               static_cast<unsigned long>(min_value),
               static_cast<unsigned long>(max_value));
      result.status = UsbTestStatus::kOk;
    } else {
      snprintf(result.message, sizeof(result.message),
               "expect-range %s got=%lu want=%lu..%lu", range_field,
               static_cast<unsigned long>(actual),
               static_cast<unsigned long>(min_value),
               static_cast<unsigned long>(max_value));
      result.status = UsbTestStatus::kFail;
    }
    return result;
  }

  char field[kUsbTestLineMax] = {};
  const char* value_text = nullptr;
  if (parseFieldAndValue(line, "expect", field, sizeof(field), value_text)) {
    if (hooks.snapshot == nullptr) {
      setMessage(result, UsbTestStatus::kError, "snapshot hook");
      return result;
    }
    UsbTestSnapshot snapshot = {};
    hooks.snapshot(hooks.context, &snapshot);
    uint32_t actual = 0;
    if (!usbTestReadField(snapshot, field, actual)) {
      setMessage(result, UsbTestStatus::kError, "unknown field");
      return result;
    }
    uint32_t expected = 0;
    if (!usbTestParseUint32(value_text, expected)) {
      setMessage(result, UsbTestStatus::kError, "expect value");
      return result;
    }
    if (actual == expected) {
      snprintf(result.message, sizeof(result.message), "expect %s=%lu", field,
               static_cast<unsigned long>(expected));
      result.status = UsbTestStatus::kOk;
    } else {
      snprintf(result.message, sizeof(result.message),
               "expect %s got=%lu want=%lu", field,
               static_cast<unsigned long>(actual),
               static_cast<unsigned long>(expected));
      result.status = UsbTestStatus::kFail;
    }
    return result;
  }

  setMessage(result, UsbTestStatus::kError, "unknown command");
  return result;
}

}  // namespace bike
