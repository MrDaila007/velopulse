#include "wheel_sensor.h"

namespace bike {

WheelSensor* WheelSensor::instance_ = nullptr;

void WheelSensor::begin(uint8_t pin, int edge) {
  pin_ = pin;
  instance_ = this;
  pinMode(pin_, INPUT_PULLUP);
  saw_passive_ = digitalRead(pin_) == HIGH;
  attachInterrupt(digitalPinToInterrupt(pin_), isrThunk, edge);
}

void WheelSensor::pollPin() {
  if (digitalRead(pin_) == HIGH) saw_passive_ = true;
}

void WheelSensor::isrThunk() {
  if (instance_ != nullptr) instance_->onInterrupt();
}

void WheelSensor::onInterrupt() {
  const uint32_t now = micros();
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
