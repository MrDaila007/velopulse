#include "page_carousel.h"

namespace bike {

namespace {
bool isValidPage(uint8_t page) { return page < kDisplayPageCount; }
}  // namespace

bool PageCarousel::contains(DisplayPage page) const {
  for (uint8_t i = 0; i < page_count_; ++i) {
    if (pages_[i] == page) return true;
  }
  return false;
}

uint8_t PageCarousel::indexOf(DisplayPage page) const {
  for (uint8_t i = 0; i < page_count_; ++i) {
    if (pages_[i] == page) return i;
  }
  return 0;
}

DisplayPage PageCarousel::validPinnedPage() const {
  if (isValidPage(pinned_page_)) {
    const DisplayPage requested = static_cast<DisplayPage>(pinned_page_);
    if (contains(requested)) return requested;
  }
  return pages_[0];
}

void PageCarousel::configure(const DeviceConfig& config, uint32_t now_ms) {
  const DisplayPage previous = current_page_;
  uint8_t enabled = config.enabled_pages_mask & kValidEnabledPagesMask;
  if (enabled == 0) enabled = 0x01;

  bool added[kDisplayPageCount] = {};
  page_count_ = 0;
  for (uint8_t i = 0; i < kConfigurablePageOrderCount; ++i) {
    const uint8_t page = config.page_order[i];
    if (!isValidPage(page) || added[page] || (enabled & (1u << page)) == 0) continue;
    pages_[page_count_++] = static_cast<DisplayPage>(page);
    added[page] = true;
  }
  for (uint8_t page = 0; page < kDisplayPageCount; ++page) {
    if (!added[page] && (enabled & (1u << page)) != 0) {
      pages_[page_count_++] = static_cast<DisplayPage>(page);
    }
  }

  auto_switch_ = config.auto_page_switch;
  pinned_page_ = config.pinned_page;
  period_ms_ = static_cast<uint32_t>(
      config.page_switch_period_s == 0 ? kDefaultPageSwitchPeriodS
                                       : config.page_switch_period_s) *
               1000u;
  current_page_ = auto_switch_
                      ? (configured_ && contains(previous) ? previous : pages_[0])
                      : validPinnedPage();
  configured_ = true;
  last_switch_ms_ = now_ms;
}

bool PageCarousel::update(uint32_t now_ms) {
  if (!configured_) return false;
  if (!auto_switch_) {
    const DisplayPage requested = validPinnedPage();
    if (requested == current_page_) return false;
    current_page_ = requested;
    return true;
  }
  if (page_count_ <= 1) return false;

  const uint32_t elapsed = now_ms - last_switch_ms_;
  if (elapsed < period_ms_) return false;
  const uint32_t steps = elapsed / period_ms_;
  const uint8_t current_index = indexOf(current_page_);
  current_page_ = pages_[(current_index + steps) % page_count_];
  last_switch_ms_ += steps * period_ms_;
  return true;
}

}  // namespace bike
