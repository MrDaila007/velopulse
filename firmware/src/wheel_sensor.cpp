#include "wheel_sensor.h"

#include "board_pins.h"

#ifndef BIKECOMP_HALL_PULLUP
#define BIKECOMP_HALL_PULLUP 1
#endif

#ifndef BIKECOMP_HALL_ANALOG
#define BIKECOMP_HALL_ANALOG 0
#endif

#ifndef BIKECOMP_HALL_ADC_OPEN_MIN
#define BIKECOMP_HALL_ADC_OPEN_MIN 2500
#endif

#ifndef BIKECOMP_HALL_ADC_CLOSED_MAX
#define BIKECOMP_HALL_ADC_CLOSED_MAX 900
#endif

#ifndef BIKECOMP_HALL_TWO_WIRE
#define BIKECOMP_HALL_TWO_WIRE 0
#endif

namespace bike {
namespace {

void configureSingleWireHallPin(uint8_t pin) {
#if BIKECOMP_HALL_PULLUP
  pinMode(pin, INPUT_PULLUP);
#else
  pinMode(pin, INPUT);
#endif
}

bool edgeMatches(int edge, bool rising, bool falling) {
  if (edge == CHANGE) return true;
  if (edge == RISING) return rising;
  return falling;
}

#if BIKECOMP_HALL_ANALOG
uint16_t readHallAdc() {
  return static_cast<uint16_t>(analogRead(kHallAdcPin));
}

bool analogLevelIsOpen(uint16_t raw, bool previous_open) {
  if (raw >= BIKECOMP_HALL_ADC_OPEN_MIN) return true;
  if (raw <= BIKECOMP_HALL_ADC_CLOSED_MAX) return false;
  return previous_open;
}
#endif

}  // namespace

void configureHallPins(uint8_t sense_pin) {
#if BIKECOMP_HALL_TWO_WIRE
  pinMode(kHallDrivePin, OUTPUT);
  digitalWrite(kHallDrivePin, LOW);
  pinMode(sense_pin, INPUT_PULLUP);
#else
  configureSingleWireHallPin(sense_pin);
#endif
}

void WheelSensor::begin(uint8_t pin, int edge) {
  pin_ = pin;
  edge_ = edge;
  configured_ = true;
  polling_enabled_ = true;
  configureHallPins(pin_);
#if BIKECOMP_HALL_ANALOG
  analogReadResolution(12);
  last_analog_raw_ = readHallAdc();
  analog_open_ = analogLevelIsOpen(last_analog_raw_, true);
  last_polled_high_ = analog_open_;
#else
  last_polled_high_ = digitalRead(pin_) == HIGH;
#endif
  saw_passive_ = last_polled_high_;
}

void WheelSensor::suspendInterrupt() {
  if (!configured_) return;
  polling_enabled_ = false;
}

void WheelSensor::resumeInterrupt(int edge) {
  if (!configured_) return;
  edge_ = edge;
  configureHallPins(pin_);
#if BIKECOMP_HALL_ANALOG
  last_analog_raw_ = readHallAdc();
  analog_open_ = analogLevelIsOpen(last_analog_raw_, analog_open_);
  last_polled_high_ = analog_open_;
#else
  last_polled_high_ = digitalRead(pin_) == HIGH;
#endif
  saw_passive_ = last_polled_high_;
  polling_enabled_ = true;
}

void WheelSensor::pollPin() {
  if (!configured_ || !polling_enabled_) return;

#if BIKECOMP_HALL_ANALOG
  last_analog_raw_ = readHallAdc();
  const bool high = analogLevelIsOpen(last_analog_raw_, analog_open_);
  analog_open_ = high;
#else
  const bool high = digitalRead(pin_) == HIGH;
#endif

  if (high) saw_passive_ = true;
  if (high == last_polled_high_) return;

  const bool rising = high && !last_polled_high_;
  const bool falling = !high && last_polled_high_;
  last_polled_high_ = high;

  if (!edgeMatches(edge_, rising, falling)) return;

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
