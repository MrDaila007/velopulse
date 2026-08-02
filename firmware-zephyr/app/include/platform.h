#pragma once

#include <stdint.h>

#include <zephyr/kernel.h>

namespace bike {
namespace platform {

uint32_t millis();
uint32_t micros();
void yield();

uint32_t resetReasonRaw();
void clearResetReason(uint32_t raw);
int freeHeapBytes();
bool usbPresent();
void systemReset();

}  // namespace platform

inline uint32_t millis() { return platform::millis(); }
inline uint32_t micros() { return platform::micros(); }
inline void yield() { platform::yield(); }

}  // namespace bike

// Arduino-compatible interrupt edge constants used by WheelSensor.
#ifndef RISING
#define RISING 1
#endif
#ifndef FALLING
#define FALLING 2
#endif
#ifndef CHANGE
#define CHANGE 3
#endif
#ifndef HEX
#define HEX 16
#endif

// Minimal Serial shim over Zephyr USB CDC / printk.
class SerialConsole {
 public:
  void begin(unsigned long baud);
  explicit operator bool() const { return ready_; }
  int available();
  int read();
  void print(const char* text);
  void print(char value);
  void print(unsigned value);
  void print(int value);
  void print(uint16_t value);
  void print(uint64_t value);
  void print(unsigned value, int base);
  void println();
  void println(const char* text);
  void println(int value);
  void println(unsigned value);
  void println(uint16_t value);
  void println(int16_t value);
  void println(char value);
  void println(bool value);
  void println(unsigned value, int base);

 private:
  bool ready_ = false;
};

extern SerialConsole Serial;
