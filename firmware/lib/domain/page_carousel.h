#pragma once

#include <stdint.h>

#include "config.h"
#include "types.h"

namespace bike {

class PageCarousel {
 public:
  void configure(const DeviceConfig& config, uint32_t now_ms);
  bool update(uint32_t now_ms);

  DisplayPage currentPage() const { return current_page_; }
  uint8_t pageCount() const { return page_count_; }

 private:
  bool contains(DisplayPage page) const;
  uint8_t indexOf(DisplayPage page) const;
  DisplayPage validPinnedPage() const;

  DisplayPage pages_[kDisplayPageCount] = {DisplayPage::kTrip};
  uint8_t page_count_ = 1;
  DisplayPage current_page_ = DisplayPage::kTrip;
  bool auto_switch_ = true;
  uint8_t pinned_page_ = 0;
  uint32_t period_ms_ = 4000;
  uint32_t last_switch_ms_ = 0;
  bool configured_ = false;
};

}  // namespace bike
