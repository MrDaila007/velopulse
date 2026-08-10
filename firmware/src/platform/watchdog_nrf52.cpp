#include "platform/watchdog.h"

#include <nrf_wdt.h>

#include "watchdog_config.h"

namespace bike {

void watchdogConfigure(uint32_t timeout_ms) {
  // RUN_SLEEP (only the SLEEP bit set): the counter keeps running through
  // low-power idle's short delay() calls, so a genuine wedge there still
  // trips a reset instead of being excused by "the CPU was asleep". The HALT
  // bit stays clear, so a debugger-halted CPU pauses the counter — stepping
  // through code under a debugger doesn't trigger a spurious watchdog reset.
  nrf_wdt_behaviour_set(NRF_WDT, NRF_WDT_BEHAVIOUR_RUN_SLEEP);
  nrf_wdt_reload_value_set(NRF_WDT, watchdogTimeoutMsToCrv(timeout_ms));
  nrf_wdt_reload_request_enable(NRF_WDT, NRF_WDT_RR0);
}

void watchdogStart() { nrf_wdt_task_trigger(NRF_WDT, NRF_WDT_TASK_START); }

void watchdogFeed() {
  if (!nrf_wdt_started(NRF_WDT)) return;
  nrf_wdt_reload_request_set(NRF_WDT, NRF_WDT_RR0);
}

bool watchdogStarted() { return nrf_wdt_started(NRF_WDT); }

}  // namespace bike
