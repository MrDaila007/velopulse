#include "scheduler.h"

#include <climits>
#include <stdint.h>

namespace bike {

void Scheduler::run(uint32_t now_ms) {
  for (size_t i = 0; i < count_; ++i) {
    ScheduledTask& task = tasks_[i];
    if (task.callback == nullptr || !isDue(now_ms, task.next_due_ms)) continue;
    task.callback(task.context, now_ms);
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
  for (size_t i = 0; i < count_; ++i) {
    const ScheduledTask& task = tasks_[i];
    if (task.callback == nullptr) continue;
    if (task.period_ms == 0) return now_ms;
    if (isDue(now_ms, task.next_due_ms)) return now_ms;
    if (task.next_due_ms < next) {
      next = task.next_due_ms;
    }
  }
  return next;
}

}  // namespace bike
