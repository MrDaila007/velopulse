#include <Arduino.h>

#include "app_controller.h"

namespace {
bike::AppController app;
}

void setup() { app.begin(); }

void loop() { app.loop(); }
