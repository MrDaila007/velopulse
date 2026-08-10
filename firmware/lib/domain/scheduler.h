#pragma once

#include <stddef.h>
#include <stdint.h>

namespace bike {

using TaskCallback = void (*)(void* context, uint32_t now_ms);

// Mirrors Arduino's micros() signature so AppController can pass it straight
// through without an adapter lambda. nullptr disables duration measurement.
using MicrosFn = uint32_t (*)();

struct ScheduledTask {
  const char* name = nullptr;
  uint32_t period_ms = 0;
  uint32_t next_due_ms = 0;
  TaskCallback callback = nullptr;
  void* context = nullptr;
  uint32_t run_count = 0;
  // Fields below are only populated when the Scheduler is constructed with a
  // MicrosFn. budget_us == 0 means "no budget enforced" for that task.
  uint32_t budget_us = 0;
  uint32_t last_duration_us = 0;
  uint32_t max_duration_us = 0;
  uint32_t overrun_count = 0;
};

class Scheduler {
 public:
  explicit Scheduler(ScheduledTask* tasks, size_t count,
                      MicrosFn micros_fn = nullptr)
      : tasks_(tasks), count_(count), micros_fn_(micros_fn) {}
  void run(uint32_t now_ms);
  void setTaskPeriod(size_t index, uint32_t period_ms, uint32_t now_ms);
  uint32_t nextDueMs(uint32_t now_ms) const;

  static bool isDue(uint32_t now_ms, uint32_t due_ms) {
    return static_cast<int32_t>(now_ms - due_ms) >= 0;
  }

 private:
  ScheduledTask* tasks_;
  size_t count_;
  MicrosFn micros_fn_;
};

}  // namespace bike
