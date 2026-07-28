#include "app_controller.h"

#include <Arduino.h>

#include "board_pins.h"

namespace bike {

AppController::AppController()
    : pulse_filter_(PulseFilterConfig{config_.wheel_circumference_mm,
                                      config_.max_speed_kmh,
                                      config_.debounce_ms,
                                      500}),
      ride_state_(static_cast<uint32_t>(config_.stop_timeout_s) * 1000u),
      tasks_{{"pulses", 0, 0, pulseTask, this, 0},
             {"state", 100, 0, stateTask, this, 0},
             {"battery", 1000, 0, batteryTask, this, 0},
             {"display", 50, 0, displayTask, this, 0}},
      scheduler_(tasks_, 4) {}

void AppController::begin() {
  Serial.begin(115200);
  const uint32_t serial_started = millis();
  while (!Serial && static_cast<uint32_t>(millis() - serial_started) < 1500u) yield();

  Serial.println();
  Serial.print("BikeComp FW ");
  Serial.println(FW_VERSION);
  Serial.println("Hall simulator: button D0 -> GND");

  wheel_sensor_.begin(kHallPin, FALLING);
  const bool display_ok = display_.begin(config_);
  Serial.print("OLED 0x3C: ");
  Serial.println(display_ok ? "OK" : "NOT FOUND; counting remains active");
  battery_.begin(config_, millis());
  Serial.print("Battery: ");
  Serial.print(battery_.snapshot().millivolts);
  Serial.print(" mV, ");
  Serial.print(battery_.snapshot().percent);
  Serial.println("%");
  ride_state_.reset(millis());
}

void AppController::loop() {
  wheel_sensor_.pollPin();
  scheduler_.run(millis());
  yield();
}

void AppController::pulseTask(void* context, uint32_t now_ms) {
  static_cast<AppController*>(context)->processPulses(now_ms);
}

void AppController::stateTask(void* context, uint32_t now_ms) {
  static_cast<AppController*>(context)->updateState(now_ms);
}

void AppController::displayTask(void* context, uint32_t) {
  static_cast<AppController*>(context)->updateDisplay();
}

void AppController::batteryTask(void* context, uint32_t now_ms) {
  static_cast<AppController*>(context)->updateBattery(now_ms);
}

void AppController::applyRideUpdate(const RideUpdate& update) {
  if (update.moving_delta_ms != 0) trip_computer_.addMovingTime(update.moving_delta_ms);
  trip_computer_.setRideState(update.state);
  if (update.state != RideState::kMoving) trip_computer_.setCurrentSpeed(0);
}

void AppController::processPulses(uint32_t now_ms) {
  PulseEvent event;
  while (wheel_sensor_.pop(event)) {
    const PulseDecision decision = pulse_filter_.process(event.timestamp_us, event.returned_passive);
    if (!decision.accepted) {
      Serial.print("Pulse rejected: ");
      Serial.println(static_cast<unsigned>(decision.rejection));
      continue;
    }

    applyRideUpdate(ride_state_.onPulse(now_ms));
    uint16_t speed = 0;
    if (!decision.first_pulse) {
      speed = speed_calculator_.onInterval(config_.wheel_circumference_mm,
                                           decision.interval_us,
                                           event.timestamp_us,
                                           config_.smoothing_enabled,
                                           config_.smoothing_window);
    }
    trip_computer_.onRevolution(config_.wheel_circumference_mm, speed);

    Serial.print("Revolution ");
    Serial.print(trip_computer_.snapshot().revolutions);
    Serial.print(", speed x100=");
    Serial.println(speed);
  }
}

void AppController::updateState(uint32_t now_ms) {
  applyRideUpdate(ride_state_.update(now_ms));
  const uint16_t speed = speed_calculator_.updateForTimeout(
      micros(), static_cast<uint32_t>(config_.stop_timeout_s) * 1000000u);
  if (ride_state_.state() == RideState::kMoving) trip_computer_.setCurrentSpeed(speed);
}

void AppController::updateDisplay() {
  DisplaySnapshot snapshot;
  snapshot.trip = trip_computer_.snapshot();
  snapshot.battery = battery_.snapshot();
  display_.render(snapshot);
}

void AppController::updateBattery(uint32_t now_ms) {
  if (!battery_.update(now_ms)) return;
  const BatterySnapshot& snapshot = battery_.snapshot();
  Serial.print("Battery raw=");
  Serial.print(battery_.lastRawAverage());
  Serial.print(", spread=");
  Serial.print(battery_.lastRawSpread());
  Serial.print(", mV=");
  Serial.print(snapshot.millivolts);
  Serial.print(", pct=");
  Serial.print(snapshot.percent);
  Serial.print(", usb=");
  Serial.println(snapshot.usb_present ? 1 : 0);
}

}  // namespace bike
