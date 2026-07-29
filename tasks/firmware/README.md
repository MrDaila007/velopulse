# Firmware — подготовка и базовая прошивка

## Э0. Подготовка и каркас

- [ ] 0.1 Завершить `.gitignore`, README и добавить лицензию.
- [x] 0.2 Разместить документацию в `docs/` и `protocol/`.
- [x] 0.3 Настроить PlatformIO и XIAO nRF52840 build/upload.
- [x] 0.4 Зафиксировать платформу, C++ standard и библиотеки.
- [ ] 0.5 Завершить каркас всех модулей. Частично: BLE/diagnostics отсутствуют.
- [x] 0.6 Настроить native environment и host-тест CRC32.
- [ ] 0.7 Инициализировать Flutter-проект — отслеживается в `tasks/mobile/`.
- [ ] 0.8 Настроить CI для firmware и Flutter.

## Э2. Базовая прошивка

- [ ] 2.1 Добавить измерение времени задач в `Scheduler`.
  Кооперативный scheduler и wrap-safe интервалы уже реализованы.
- [x] 2.2 Реализовать WheelSensor: ISR, ring buffer, configurable edge.
- [x] 2.3 Реализовать PulseFilter: debounce, overspeed, stuck, counters.
- [x] 2.4 Реализовать fixed-point SpeedCalculator, smoothing и timeout.
- [x] 2.5 Реализовать TripComputer: trip/odo/time/avg/max/reset.
- [x] 2.6 Реализовать RideState: auto start/pause.
- [ ] 2.7 Завершить DisplayManager. Не хватает BLE-индикатора.
- [x] 2.8 Реализовать PageCarousel, mask/order/pinned page.
- [x] 2.9 Реализовать 4/1 Гц render, dim/off и wake OLED.
- [x] 2.10 Реализовать BatteryManager: ADC, EMA, SoC, monotonicity.
- [x] 2.11 Реализовать splash и продолжение работы без OLED.
- [ ] 2.12 Откалибровать скорость на стенде до погрешности ≤2%.

## Остаток DoD Э2

- [ ] Подтвердить 100 медленных оборотов без дублей.
- [ ] Подтвердить 100 быстрых оборотов без пропусков.
- [ ] Проверить реальный Hall и ложные импульсы.
- [ ] Подтвердить скорость относительно эталона с погрешностью ≤2%.
- [ ] Зафиксировать результаты измерений в аппаратном протоколе.

## Общие firmware-долги

- [ ] Реализовать boot count и reset reason.
- [ ] Добавить watchdog и контролируемую инъекцию зависания.
- [ ] Добавить диагностический snapshot всех counters.
- [ ] Добавить feature-flag deep sleep и wake от GPIO/USB.
- [ ] Исключить длительные блокировки из production loop и проверить ISR review.
