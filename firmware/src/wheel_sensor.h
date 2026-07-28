#pragma once

#include <Arduino.h>

#include "config.h"

namespace bike {

struct PulseEvent {
  uint32_t timestamp_us = 0;
  bool returned_passive = true;
};

class WheelSensor {
 public:
  void begin(uint8_t pin, int edge = FALLING);
  void pollPin();
  bool pop(PulseEvent& event);

  uint32_t rawPulseCount() const { return raw_pulse_count_; }
  uint32_t overflowCount() const { return overflow_count_; }

 private:
  static void isrThunk();
  void onInterrupt();

  static WheelSensor* instance_;
  uint8_t pin_ = 0;
  volatile uint32_t timestamps_[kPulseBufferSize] = {};
  volatile bool passive_before_[kPulseBufferSize] = {};
  volatile uint8_t head_ = 0;
  volatile uint8_t tail_ = 0;
  volatile uint32_t raw_pulse_count_ = 0;
  volatile uint32_t overflow_count_ = 0;
  volatile bool saw_passive_ = true;
};

}  // namespace bike
