#include <Arduino.h>

// Isolated wheel-input probe: deliberately does not include BikeComp code.
constexpr uint8_t kDrivePin = D0;
constexpr uint8_t kSensePin = D1;

void setup() {
  Serial.begin(115200);
  pinMode(kDrivePin, OUTPUT);
  digitalWrite(kDrivePin, LOW);
  pinMode(kSensePin, INPUT_PULLUP);
  delay(100);
  Serial.println("HALL_PROBE D0=LOW D1/P0.03 INPUT_PULLUP");
}

void loop() {
  static int previous = -1;
  const int current = digitalRead(kSensePin);
  if (current != previous) {
    previous = current;
    Serial.print("D1=");
    Serial.println(current == HIGH ? "HIGH" : "LOW");
  }
  delay(1);
}
