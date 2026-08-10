#include "scheduler.h"

#include <climits>
#include <stdint.h>

namespace bike {

void Scheduler::run(uint32_t now_ms) {
  for (size_t i = 0; i < count_; ++i) {
    ScheduledTask& task = tasks_[i];
    if (task.callback == nullptr || !isDue(now_ms, task.next_due_ms)) continue;

    const bool measure = micros_fn_ != nullptr;
    const uint32_t start_us = measure ? micros_fn_() : 0u;
    task.callback(task.context, now_ms);
    if (measure) {
      // Unsigned subtraction wraps correctly across a micros() rollover.
      const uint32_t duration_us = micros_fn_() - start_us;
      task.last_duration_us = duration_us;
      if (duration_us > task.max_duration_us) task.max_duration_us = duration_us;
      if (task.budget_us != 0 && duration_us > task.budget_us) {
        ++task.overrun_count;
      }
    }

    ++task.run_count;
    if (task.period_ms == 0) {
      task.next_due_ms = now_ms;
    } else {
      do {
        task.next_due_ms += task.period_ms;
      } while (isDue(now_ms, task.next_due_ms));
    }
  }
}

void Scheduler::setTaskPeriod(size_t index, uint32_t period_ms, uint32_t now_ms) {
  if (index >= count_) return;
  ScheduledTask& task = tasks_[index];
  if (task.period_ms == period_ms) return;
  task.period_ms = period_ms;
  task.next_due_ms = now_ms;
}

uint32_t Scheduler::nextDueMs(uint32_t now_ms) const {
  uint32_t next = UINT32_MAX;
  uint32_t next_delta = UINT32_MAX;
  for (size_t i = 0; i < count_; ++i) {
    const ScheduledTask& task = tasks_[i];
    if (task.callback == nullptr) continue;
    if (task.period_ms == 0) return now_ms;
    if (isDue(now_ms, task.next_due_ms)) return now_ms;
    // Not due yet, so (next_due_ms - now_ms) is the forward distance in the
    // wrap-safe half of the uint32 range isDue() uses; comparing raw
    // next_due_ms values instead (as this used to) picks the wrong task
    // across a millis() rollover.
    const uint32_t delta = task.next_due_ms - now_ms;
    if (delta < next_delta) {
      next_delta = delta;
      next = task.next_due_ms;
    }
  }
  return next;
}

}  // namespace bike
