#include "wheel_sensor.h"

#include "board_pins.h"

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define HALL_SENSE_NODE DT_ALIAS(hall_sensor)
#define HALL_SENSE_PIN_NODE DT_CHILD(HALL_SENSE_NODE, hall_pin)
#define HALL_DRIVE_NODE DT_ALIAS(hall_drive)
#define HALL_DRIVE_PIN_NODE DT_CHILD(HALL_DRIVE_NODE, hall_drive_pin)

namespace bike {

WheelSensor* WheelSensor::instance_ = nullptr;

static const struct gpio_dt_spec kHallSenseGpio =
    GPIO_DT_SPEC_GET(HALL_SENSE_PIN_NODE, gpios);
static const struct gpio_dt_spec kHallDriveGpio =
    GPIO_DT_SPEC_GET(HALL_DRIVE_PIN_NODE, gpios);

namespace {

bool edgeMatches(int edge, bool rising, bool falling) {
  if (edge == CHANGE) return true;
  if (edge == RISING) return rising;
  return falling;
}

gpio_flags_t interruptFlags(int edge) {
  if (edge == RISING) return GPIO_INT_EDGE_RISING;
  if (edge == CHANGE) return GPIO_INT_EDGE_BOTH;
  return GPIO_INT_EDGE_FALLING;
}

}  // namespace

void configureHallPins() {
  if (!gpio_is_ready_dt(&kHallDriveGpio) ||
      !gpio_is_ready_dt(&kHallSenseGpio)) {
    return;
  }
  gpio_pin_configure_dt(&kHallDriveGpio, GPIO_OUTPUT_INACTIVE);
  gpio_pin_set_dt(&kHallDriveGpio, 0);
  gpio_pin_configure_dt(&kHallSenseGpio, GPIO_INPUT | GPIO_PULL_UP);
}

void WheelSensor::configureSenseInterrupt(int edge) {
  if (sense_gpio_ == nullptr || !gpio_is_ready_dt(sense_gpio_)) return;

  gpio_flags_t flags = interruptFlags(edge);
  gpio_init_callback(&callback_, isrThunk, BIT(sense_gpio_->pin));
  gpio_add_callback(sense_gpio_->port, &callback_);
  gpio_pin_interrupt_configure_dt(sense_gpio_, flags);
  interrupt_enabled_ = true;
}

void WheelSensor::removeSenseInterrupt() {
  if (!interrupt_enabled_ || sense_gpio_ == nullptr) return;
  gpio_pin_interrupt_configure_dt(sense_gpio_, GPIO_INT_DISABLE);
  gpio_remove_callback(sense_gpio_->port, &callback_);
  interrupt_enabled_ = false;
}

void WheelSensor::begin(uint8_t pin, int edge) {
  (void)pin;
  instance_ = this;
  sense_gpio_ = &kHallSenseGpio;
  drive_gpio_ = &kHallDriveGpio;
  sense_pin_ = kHallSensePin;
  edge_ = edge;
  configured_ = true;
  polling_enabled_ = true;

  if (!gpio_is_ready_dt(sense_gpio_) || !gpio_is_ready_dt(drive_gpio_)) {
    return;
  }

  configureHallPins();
  last_polled_high_ = gpio_pin_get_dt(sense_gpio_) != 0;
  saw_passive_ = last_polled_high_;

  removeSenseInterrupt();
  configureSenseInterrupt(edge);
}

void WheelSensor::suspendInterrupt() {
  if (!configured_) return;
  removeSenseInterrupt();
  polling_enabled_ = false;
}

void WheelSensor::resumeInterrupt(int edge) {
  if (!configured_) return;
  edge_ = edge;
  configureHallPins();
  last_polled_high_ = gpio_pin_get_dt(sense_gpio_) != 0;
  saw_passive_ = last_polled_high_;
  configureSenseInterrupt(edge);
  polling_enabled_ = true;
}

void WheelSensor::pollPin() {
  if (!configured_ || sense_gpio_ == nullptr) return;

  const bool high = gpio_pin_get_dt(sense_gpio_) != 0;
  if (high) saw_passive_ = true;
  if (!polling_enabled_ || interrupt_enabled_) return;

  if (high == last_polled_high_) return;

  const bool rising = high && !last_polled_high_;
  const bool falling = !high && last_polled_high_;
  last_polled_high_ = high;

  if (!edgeMatches(edge_, rising, falling)) return;

  onInterrupt();
}

bool WheelSensor::pinIsHigh() const {
  if (sense_gpio_ == nullptr || !gpio_is_ready_dt(sense_gpio_)) return true;
  return gpio_pin_get_dt(sense_gpio_) != 0;
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
  const uint8_t next =
      static_cast<uint8_t>((head_ + 1u) & (kPulseBufferSize - 1u));
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
