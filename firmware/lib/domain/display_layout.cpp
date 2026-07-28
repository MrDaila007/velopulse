#include "display_layout.h"

namespace bike {

void drawDisplayFrame(DisplayCanvas& canvas, const DisplayFrame& frame) {
  canvas.setFont(DisplayFont::kSpeed);
  canvas.drawText(0, 21, frame.speed);

  canvas.setFont(DisplayFont::kSmall);
  canvas.drawFrame(91, 0, 12, 8);
  canvas.drawBox(103, 2, 2, 4);
  if (frame.battery_fill_width != 0) {
    canvas.drawBox(93, 2, frame.battery_fill_width, 4);
  }
  canvas.drawText(107, 7, frame.battery_percent);
  canvas.drawText(88, 20, frame.units);
  canvas.drawText(0, 31, frame.lower);
}

}  // namespace bike
