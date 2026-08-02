#pragma once

#include <stddef.h>
#include <stdint.h>

namespace bike {

using TaskCallback = void (*)(void* context, uint32_t now_ms);

struct ScheduledTask {
  const char* name = nullptr;
  uint32_t period_ms = 0;
  uint32_t next_due_ms = 0;
  TaskCallback callback = nullptr;
  void* context = nullptr;
  uint32_t run_count = 0;
};

class Scheduler {
 public:
  Scheduler(ScheduledTask* tasks, size_t count) : tasks_(tasks), count_(count) {}
  void run(uint32_t now_ms);
  void setTaskPeriod(size_t index, uint32_t period_ms, uint32_t now_ms);
  uint32_t nextDueMs(uint32_t now_ms) const;

  static bool isDue(uint32_t now_ms, uint32_t due_ms) {
    return static_cast<int32_t>(now_ms - due_ms) >= 0;
  }

 private:
  ScheduledTask* tasks_;
  size_t count_;
};

}  // namespace bike
