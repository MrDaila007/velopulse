# Firmware — подготовка и базовая прошивка

## Э0. Подготовка и каркас

- [ ] 0.1 Завершить `.gitignore`, README и добавить лицензию.
- [x] 0.2 Разместить документацию в `docs/` и `protocol/`.
- [x] 0.3 Настроить PlatformIO и XIAO nRF52840 build/upload.
- [x] 0.4 Зафиксировать платформу, C++ standard и библиотеки.
- [x] 0.5 Завершить каркас всех модулей. BLE (`ble_manager`) и diagnostics
  (`diagnostics.{h,cpp}`) реализованы; `StorageCounters` персистируется между
  reboot (см. «Общие firmware-долги»).
- [x] 0.6 Настроить native environment и host-тест CRC32.
- [ ] 0.7 Инициализировать Flutter-проект — отслеживается в `tasks/mobile/`.
- [x] 0.8 Настроить CI для firmware и Flutter.

## Э2. Базовая прошивка

- [x] 2.1 Добавить измерение времени задач в `Scheduler`. `ScheduledTask`
  (`scheduler.h`) хранит `last_duration_us`/`max_duration_us`/`overrun_count`
  против per-task `budget_us`, замеряется через инжектируемый `MicrosFn`
  (домен остаётся Arduino-независимым); `AppController` подключает `micros()`
  и именованные бюджеты с headroom под flash-запись. Serial-команда `sched`
  печатает статистику каждой задачи. Заодно исправлен смежный баг:
  `Scheduler::nextDueMs` сравнивал сырые `next_due_ms` вместо wrap-safe
  дельты и мог занизить срочность задачи почти на 2^31 мс вокруг переполнения
  `millis()`.
- [x] 2.2 Реализовать WheelSensor: ISR, ring buffer, configurable edge.
- [x] 2.3 Реализовать PulseFilter: debounce, overspeed, stuck, counters.
- [x] 2.4 Реализовать fixed-point SpeedCalculator, smoothing и timeout.
- [x] 2.5 Реализовать TripComputer: trip/odo/time/avg/max/reset.
- [x] 2.6 Реализовать RideState: auto start/pause.
- [x] 2.7 Завершить DisplayManager. BLE-индикатор реализован: `DisplaySnapshot`/
  `DisplayFrame` (`include/types.h`) содержат `ble_connected`, `display_layout.cpp`
  рисует `"BLE"` при подключении, `AppController::updateDisplay()` подключает
  `BleManager::bleConnected()`. Simulator gate: сценарий `ble_connected`,
  24 golden-кадра.
- [x] 2.8 Реализовать PageCarousel, mask/order/pinned page.
- [x] 2.9 Реализовать 4/1 Гц render, dim/off и wake OLED.
- [x] 2.10 Реализовать BatteryManager: ADC, EMA, SoC, monotonicity.
- [x] 2.11 Реализовать splash и продолжение работы без OLED.
- [ ] 2.12 Откалибровать скорость на стенде до погрешности ≤2%.
- [x] 2.13 Добавить primary SSD1306 128×64 и совместимый 128×32 build/layout;
  обе сборки и 22 simulator golden-кадра (11 сценариев × 2 профиля, включая
  weather-страницы) проходят.
- [x] 2.14 Добавить защиту OLED от выгорания: default dim/off и четырёхфазный
  pixel shift общей разметки раз в 60 с для обеих панелей.

## Остаток DoD Э2

- [ ] Подтвердить 100 медленных оборотов без дублей.
- [ ] Подтвердить 100 быстрых оборотов без пропусков.
- [ ] Проверить реальный Hall и ложные импульсы.
- [ ] Подтвердить скорость относительно эталона с погрешностью ≤2%.
- [ ] Зафиксировать результаты измерений в аппаратном протоколе.

## Общие firmware-долги

- [x] Реализовать boot count и reset reason: `lib/domain/boot_counter.*` (`/boot_cnt`),
  чтение и очистка `NRF_POWER->RESETREAS` в `src/app_controller.cpp:132-135`.
- [x] Добавить watchdog и контролируемую инъекцию зависания. nRF52 WDT через
  `src/platform/watchdog_nrf52.cpp` (HAL напрямую — `nrfx_wdt` драйвер не
  собран в этом core); конфигурация до, старт после всех блокирующих шагов
  `begin()`; кормление безусловно первой строкой `loop()` и перед входом в
  deep sleep. `kSelftestWatchdogOk` выставляется при успешном старте.
  Serial-команда `wdt-hang` вешает `loop()` для проверки реального сброса на
  железе (аппаратно ещё не подтверждено — см. `STATUS.md`). Таймаут по
  умолчанию 8 с (`BIKECOMP_WDT_TIMEOUT_MS`), отключается сборочным флагом
  `BIKECOMP_FEATURE_WATCHDOG=0`.
- [x] Добавить диагностический snapshot всех counters: `diagnostics.{h,cpp}`,
  `GET_DIAGNOSTIC`, Serial dump при старте. Персист `StorageCounters`
  (`storage_manager.h:118`) между reboot — см. пункт ниже.
- [x] Персистировать `StorageCounters` между reboot: новый A/B слот `/cnt_a|b`
  (`storage_manager.{h,cpp}`), payload version 1, 48 байт. `StorageManager::begin()`
  восстанавливает `counters_` из свежего слота; `AppController::persistStorageCounters()`
  сохраняет снимок перед deep sleep и перед reboot (Serial и BLE `CommandId::kReboot`).
- [x] Добавить feature-flag deep sleep и wake от GPIO/USB (код; полевая верификация — Э7).
- [ ] Исключить длительные блокировки из production loop и проверить ISR review.
  Low-power idle `delay()` теперь ограничен `kMaxIdleDelayChunkMs` = 50 мс
  (`clampIdleDelayMs`, `idle_delay.{h,cpp}`) вместо до ~1000 мс. Остаются: синхронные
  flash-записи (remove+write+flush+close в `InternalFsBackend::write`) прямо на
  scheduler-пути при автосохранении одометра/ambient-калибровки/counters
  (`app_controller.cpp:1409,1451,1472`); несколько `delay(2)` в `printGpioProbe()`,
  вызываемой из Serial-консоли (`:522,539,544`).
  watchdog-таймаут 8 с и `sched` (см. 2.1) остаются сетью безопасности и
  измерительным инструментом для оставшихся блокировок.
