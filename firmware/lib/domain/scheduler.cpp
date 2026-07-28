#include "scheduler.h"

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

}  // namespace bike
