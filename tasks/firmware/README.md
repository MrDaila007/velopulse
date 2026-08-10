# Firmware — подготовка и базовая прошивка

## Э0. Подготовка и каркас

- [ ] 0.1 Завершить `.gitignore`, README и добавить лицензию.
- [x] 0.2 Разместить документацию в `docs/` и `protocol/`.
- [x] 0.3 Настроить PlatformIO и XIAO nRF52840 build/upload.
- [x] 0.4 Зафиксировать платформу, C++ standard и библиотеки.
- [x] 0.5 Завершить каркас всех модулей. BLE (`ble_manager`) и diagnostics
  (`diagnostics.{h,cpp}`) реализованы; `StorageCounters` внутри diagnostics
  остаётся RAM-only (см. «Общие firmware-долги»).
- [x] 0.6 Настроить native environment и host-тест CRC32.
- [ ] 0.7 Инициализировать Flutter-проект — отслеживается в `tasks/mobile/`.
- [x] 0.8 Настроить CI для firmware и Flutter.

## Э2. Базовая прошивка

- [ ] 2.1 Добавить измерение времени задач в `Scheduler`.
  Кооперативный scheduler и wrap-safe интервалы уже реализованы; `ScheduledTask`
  (`scheduler.h:10-17`) хранит только `run_count`, `run()` (`scheduler.cpp:8-22`)
  не замеряет длительность, а `run_count` наружу вообще не отдаётся
  (Serial/diagnostics/BLE). Нужны per-task duration/overrun.
- [x] 2.2 Реализовать WheelSensor: ISR, ring buffer, configurable edge.
- [x] 2.3 Реализовать PulseFilter: debounce, overspeed, stuck, counters.
- [x] 2.4 Реализовать fixed-point SpeedCalculator, smoothing и timeout.
- [x] 2.5 Реализовать TripComputer: trip/odo/time/avg/max/reset.
- [x] 2.6 Реализовать RideState: auto start/pause.
- [ ] 2.7 Завершить DisplayManager. Не хватает BLE-индикатора: `DisplaySnapshot`/
  `DisplayFrame` (`include/types.h:49-72`) не содержат поля состояния BLE, ноль
  упоминаний BLE в `display_layout.cpp`. `BleManager::bleConnected()`
  (`src/ble_manager.h:72`) уже доступен для чтения — не хватает только wiring
  и отрисовки.
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
- [ ] Добавить watchdog и контролируемую инъекцию зависания. WDT нигде не
  инициализируется и не кормится (нет `NRF_WDT`/`nrf_wdt_*` в дереве); бит
  `kSelftestWatchdogOk` (`diagnostics.h:20`) объявлен, но никогда не
  выставляется (`src/app_controller.cpp:265`). Декодирование
  `ResetReason::kWatchdog` и `ErrorLogCode::kWatchdogReset` уже реализованы и
  ждут инициализации WDT.
- [x] Добавить диагностический snapshot всех counters: `diagnostics.{h,cpp}`,
  `GET_DIAGNOSTIC`, Serial dump при старте. Персист `StorageCounters`
  (`storage_manager.h:170`) между reboot — отдельный открытый пункт ниже.
- [ ] Персистировать `StorageCounters` между reboot. Сейчас поле `counters_`
  только инкрементируется в памяти, без serialize/load-пути — `flash_write_count`
  и остальные счётчики обнуляются на каждом старте.
- [x] Добавить feature-flag deep sleep и wake от GPIO/USB (код; полевая верификация — Э7).
- [ ] Исключить длительные блокировки из production loop и проверить ISR review.
  Найдено: `delay(delay_ms)` до ~1000 мс в low-power idle
  (`src/app_controller.cpp:579`); синхронные flash-записи (remove+write+flush+close
  в `InternalFsBackend::write`) прямо на scheduler-пути при автосохранении
  одометра/ambient-калибровки (`app_controller.cpp:1411,1419,1426`); несколько
  `delay(2)` в `printGpioProbe()`, вызываемой из Serial-консоли (`:478,495,500`).
