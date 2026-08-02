#pragma once

#include <stdint.h>

#include <zephyr/drivers/gpio.h>

#include "config.h"
#include "platform.h"

namespace bike {

struct PulseEvent {
  uint32_t timestamp_us = 0;
  bool returned_passive = true;
};

void configureHallPins();

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
  uint8_t pin() const { return sense_pin_; }

 private:
  static void isrThunk(const struct device* dev, struct gpio_callback* cb,
                       uint32_t pins);
  void onInterrupt();
  void configureSenseInterrupt(int edge);
  void removeSenseInterrupt();

  static WheelSensor* instance_;
  const struct gpio_dt_spec* sense_gpio_ = nullptr;
  const struct gpio_dt_spec* drive_gpio_ = nullptr;
  struct gpio_callback callback_;
  volatile uint32_t timestamps_[kPulseBufferSize] = {};
  volatile bool passive_before_[kPulseBufferSize] = {};
  volatile uint8_t head_ = 0;
  volatile uint8_t tail_ = 0;
  volatile uint32_t raw_pulse_count_ = 0;
  volatile uint32_t overflow_count_ = 0;
  volatile bool saw_passive_ = true;
  uint8_t sense_pin_ = 0;
  int edge_ = FALLING;
  bool configured_ = false;
  bool interrupt_enabled_ = false;
  bool polling_enabled_ = true;
  bool last_polled_high_ = true;
};

}  // namespace bike
