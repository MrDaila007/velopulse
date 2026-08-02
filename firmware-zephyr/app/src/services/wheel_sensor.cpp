#include "wheel_sensor.h"

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define HALL_KEYS_NODE DT_ALIAS(hall_sensor)
#define HALL_PIN_NODE DT_CHILD(HALL_KEYS_NODE, hall_pin)

namespace bike {

WheelSensor* WheelSensor::instance_ = nullptr;

static const struct gpio_dt_spec kHallGpio =
    GPIO_DT_SPEC_GET(HALL_PIN_NODE, gpios);

void WheelSensor::begin(uint8_t pin, int edge) {
  (void)pin;
  instance_ = this;
  hall_gpio_ = &kHallGpio;

  if (!gpio_is_ready_dt(hall_gpio_)) return;

  gpio_pin_configure_dt(hall_gpio_, GPIO_INPUT);
  saw_passive_ = gpio_pin_get_dt(hall_gpio_) != 0;

  gpio_flags_t flags = GPIO_INT_EDGE_FALLING;
  if (edge == RISING) {
    flags = GPIO_INT_EDGE_RISING;
  } else if (edge == CHANGE) {
    flags = GPIO_INT_EDGE_BOTH;
  }

  gpio_init_callback(&callback_, isrThunk, BIT(hall_gpio_->pin));
  gpio_add_callback(hall_gpio_->port, &callback_);
  gpio_pin_interrupt_configure_dt(hall_gpio_, flags);
}

void WheelSensor::pollPin() {
  if (hall_gpio_ == nullptr) return;
  if (gpio_pin_get_dt(hall_gpio_) != 0) saw_passive_ = true;
}

void WheelSensor::isrThunk(const struct device* dev, struct gpio_callback* cb,
                             uint32_t pins) {
  (void)dev;
  (void)cb;
  (void)pins;
  if (instance_ != nullptr) instance_->onInterrupt();
}

void WheelSensor::onInterrupt() {
  const uint32_t now = platform::micros();
  const uint8_t next = static_cast<uint8_t>((head_ + 1u) & (kPulseBufferSize - 1u));
  if (next != tail_) {
    timestamps_[head_] = now;
    passive_before_[head_] = saw_passive_;
    saw_passive_ = false;
    head_ = next;
  } else {
    ++overflow_count_;
  }
  ++raw_pulse_count_;
}

bool WheelSensor::pop(PulseEvent& event) {
  if (tail_ == head_) return false;
  event.timestamp_us = timestamps_[tail_];
  event.returned_passive = passive_before_[tail_];
  tail_ = static_cast<uint8_t>((tail_ + 1u) & (kPulseBufferSize - 1u));
  return true;
}

}  // namespace bike
