#pragma once

#include "types.h"

namespace bike {

class DisplayFormatter {
 public:
  static DisplayFrame format(const DisplaySnapshot& snapshot, DisplayPage page);
};

}  // namespace bike
