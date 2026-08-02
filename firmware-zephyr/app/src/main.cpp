#include <zephyr/kernel.h>

#include "app_controller.h"

namespace {
bike::AppController app;
}

int main() {
  app.begin();
  while (true) {
    app.loop();
  }
  return 0;
}
