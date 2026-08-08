#pragma once

#include <Arduino.h>

#include "config.h"

namespace bike {

#ifndef BIKECOMP_HALL_ANALOG
#define BIKECOMP_HALL_ANALOG 0
#endif

#ifndef BIKECOMP_HALL_TWO_WIRE
#define BIKECOMP_HALL_TWO_WIRE 0
#endif

void configureHallPins(uint8_t sense_pin);

struct PulseEvent {
  uint32_t timestamp_us = 0;
  bool returned_passive = true;
};

class WheelSensor {
 public:
  void begin(uint8_t pin, int edge = FALLING);
  void suspendInterrupt();
  void resumeInterrupt(int edge);
  void pollPin();
  bool pop(PulseEvent& event);

  uint32_t rawPulseCount() const { return raw_pulse_count_; }
  uint32_t overflowCount() const { return overflow_count_; }
  bool pinIsHigh() const;
  uint16_t lastAnalogRaw() const { return last_analog_raw_; }
  uint8_t pin() const { return pin_; }

 private:
#if !BIKECOMP_HALL_ANALOG
  static void isrThunk();
  void onInterrupt();
  static WheelSensor* instance_;
  void attachSenseInterrupt();
  void detachSenseInterrupt();
#endif
  void pushPulse(uint32_t timestamp_us);

  uint8_t pin_ = 0xFF;
  volatile uint32_t timestamps_[kPulseBufferSize] = {};
  volatile bool passive_before_[kPulseBufferSize] = {};
  volatile uint8_t head_ = 0;
  volatile uint8_t tail_ = 0;
  volatile uint32_t raw_pulse_count_ = 0;
  volatile uint32_t overflow_count_ = 0;
  volatile bool saw_passive_ = true;
  int edge_ = FALLING;
  bool configured_ = false;
  bool polling_enabled_ = true;
  bool interrupt_attached_ = false;
  bool last_polled_high_ = true;
  uint16_t last_analog_raw_ = 0;
  bool analog_open_ = true;
};

inline bool WheelSensor::pinIsHigh() const {
#if BIKECOMP_HALL_ANALOG
  return analog_open_;
#else
  return digitalRead(pin_) == HIGH;
#endif
}

}  // namespace bike
