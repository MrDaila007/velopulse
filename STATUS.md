# Статус проекта

Обновлено: 2026-07-31

## Текущий этап

Э4 — BLE-интеграция. Э4.1–4.14 программно реализованы: advertising, команды,
encryption и 5-минутное pairing window синхронизированы с Android flow. Базовая
интеграция Android-приложения с реальным XIAO подтверждена пользователем: новый APK
обнаруживает BikeComp, подключается и работает. Полный hardware gate Э5 ещё открыт.

## Готово

- PlatformIO-проект для XIAO nRF52840 Sense и native-окружения.
- Фильтрация импульсов, fixed-point скорость, дистанция, средняя/максимальная скорость.
- Автостарт и автопауза, неблокирующий scheduler, ISR с кольцевым буфером.
- Минимальный OLED-интерфейс, проверенный на устройстве с кнопкой вместо датчика Холла.
- GUI/headless-симулятор OLED и pixel-golden тесты состояний IDLE/MOV/PAUSE.
- `PageCarousel` с настройкой порядка, маски, периода и фиксированной страницы.
- Общий C++-форматтер пяти нижних страниц и индикатора батареи.
- Общая C++-разметка: прошивка и симулятор исполняют один и тот же код отрисовки.
- Pixel-golden кадры пяти страниц, IDLE/PAUSE, неизвестного и низкого заряда.
- Два compile-time OLED-профиля из общего renderer: primary SSD1306 128×64 с
  Logisoso 38 и совместимый 128×32 с неизменными пикселями; BLE/config не менялись.
- `BatteryModel`: пересчёт ADC, калибровка, EMA, SoC LUT, монотонность и гистерезис.
- `BatteryManager`: 16 ADC-выборок и min/max rejection для внешнего делителя на P0.31.
- Аппаратная документация приведена к фактической схеме NologoTech Super-nRF52840.
- Внешний делитель 1 MΩ / 1 MΩ на P0.31 распаян и проверен: 4.016–4.037 В, 84–86% при зарядке.
- Автономная работа от Li-Po без USB и сохранение индикации заряда подтверждены пользователем.
- Установившийся разброс 16 выборок: 16–25 ADC counts (примерно 19–29 мВ).
- `DisplayPower`: приглушение на половине тайм-аута, выключение и пробуждение.
- `DisplayManager`: управление контрастом и SSD1306 power-save; корректный импульс
  колеса немедленно возвращает экран в яркий режим.
- `DisplayBurnInGuard`: независимо от auto-off раз в 60 с циклически смещает общий
  renderer на один пиксель по четырём позициям; работает для 128×64 и 128×32,
  wrap-safe и не изменяет splash/test patterns.
- `AmbientLightModel`: EMA 1/8, raw normalization, уровни 5/15/35/65/100%,
  гистерезис 5%, выдержка 2 с и invalid fallback. `AmbientLightManager` раз в
  секунду коммутирует D3, на следующем tick усредняет 16 ADC-выборок с min/max
  rejection; `brightness_pct` остаётся пользовательским максимумом.
- Канонический little-endian codec 48-байтовой `DeviceConfig` без зависимости от
  C++ padding; валидируются все диапазоны, маска/порядок страниц и имя устройства.
- `StorageManager`: `/cfg_a/b` и `/odo_a/b`, заголовок `BKCP`, version, sequence,
  CRC32, выбор свежего слота, чередование записей и пропуск неизменённых данных.
- При повреждении одного слота используется второй; при повреждении обоих defaults
  записываются в слот A. Одометр и общее число оборотов восстанавливаются при старте.
- Serial при старте показывает mount status, source, sequence, version, recovery/
  defaults, migration и восстановленное значение одометра.
- `OdometerSavePolicy`: независимые триггеры дистанции (`odometer_save_interval_m`),
  settle 30 с после `MOVING→PAUSED`, OLED off, deep sleep, `FORCE_SAVE`, critical
  battery ≤5% (однократно), USB disconnect и reboot; ошибка Flash не останавливает
  поездку. `AppController` пишет одометр и логирует trigger/result/sequence/counters.
- `storage_migration`: `migrateConfigV1ToV2` / `migrateOdometerV1ToV2` (identity stub
  при неизменном payload); `RecordHeader.version` = 2. При load запись v1
  мигрируется, перезаписывается как v2 (force write); будущая version отвергается.
  Счётчики `config_migrations` / `odometer_migrations`.
- `diagnostics`: snapshot §6.1 (`raw_pulse_count`, debounce/overspeed rejects,
  `isr_overflow`, `flash_write_count` ← `StorageCounters::writes`,
  `free_heap` через `dbgHeapFree()`, i2c/selftest). Little-endian 16-byte payload
  возвращается через `GET_DIAGNOSTIC`. Serial dump при старте;
  `AppController::diagnosticSnapshot()`.
- BLE Э4.1–4.14: `BleManager` GATT + live Device Info на Read (uptime, flags,
  bonded, pairing window); FICR serial; `reset_reason` из `NRF_POWER->RESETREAS`;
  `boot_count` в `/boot_cnt`; live Telemetry notify (seq + adaptive 1 Гц /
  0.2 Гц Read refresh; sensor-test 5 Гц); Config Write pending queue +
  validation/apply + Config Read notify. Safe Command `0x01–0x0B`: strict parser,
  single-slot queue и выполнение в main loop (`RESET_TRIP/MAX`, `FORCE_SAVE`, OLED,
  display/sensor/battery tests, diagnostics); корректные `Command Result` status,
  `detail` и 16-byte payload; sensor-test завершается по timeout/disconnect.
  ADV Flags + Service UUID, Scan Response name + Tx Power; fast 30 мс первые 30 с,
  slow 1000 мс, постоянная реклама по умолчанию либо stop через 5 минут; рестарт
  после disconnect/движения, runtime-применение имени и policy. Имя `BikeComp-XXXX` →
  serial (`ble_identity`); ack одометра только при успехе Flash; hot-path Serial
  за `BIKECOMP_HOTPATH_SERIAL` (по умолчанию выкл).
  Dangerous Command `0x20–0x40`: двухфазный request/confirm, аппаратный 32-bit
  nonce, TTL 30 с, binding к command/payload/connection, обязательный bonded link,
  single-slot execution и token/error statuses. Реализованы reset odometer/factory,
  reboot с отложенным reset, battery calibration, set odometer и runtime pairing
  window; параметры сохраняются до ответа OK.
  Pairing/bonding: LESC Just Works, Flash bond store Bluefruit, encrypted GATT
  permissions, release-default `BIKECOMP_OPEN_PAIRING=0`; новые pairing requests
  после 5 минут отклоняются disconnect + defensive bond revoke, сохранённые bonds
  допускаются. Dangerous commands требуют одновременно bonded и encrypted link.
  Android preflight показывает typed `notPaired` по Device Info без reconnect-loop.
  Error Log: RAM-кольцо 16 событий, Read/Notify последних 4 в хронологическом порядке;
  регистрируются I²C/Flash/config reject/pairing reject/ISR overflow/sensor stuck/
  watchdog/critical battery. Sensor-test использует нормативные Telemetry fields и
  принудительный интервал 200 мс; timeout, stop и disconnect завершают режим.
- Неблокирующая USB Serial-консоль: `open-pairing`, `dump-config`, `reset-odo`,
  `selftest`; CR/LF, ограничение длины и восстановление после переполнения проверены
  native-тестами. `dump-config` возвращает читаемые поля и точный 48-byte wire payload.
- `mobile-app`: Flutter 3.44.7, application ID `app.bikecomp.mobile`, minSdk 24,
  compileSdk/targetSdk 36; четыре Material 3 раздела и connecting overlay-route.
  Реализованы protocol v1 Freezed-модели/codecs, полный ConfigValidator,
  `FakeBleTransport`, production `flutter_reactive_ble` transport, Android
  permissions/adapter/location/bond layer, FSM/reconnect, domain repository,
  write-then-verify и versioned per-device dirty drafts. Устранена lifecycle race,
  где `unawaited(stopScan())` закрывал только что созданный scan stream. Discovery
  выполняет нефильтрованный platform scan с локальной проверкой service UUID и
  fallback по имени `BikeComp-*`; после connect полный GATT-контракт обязателен.
  Device Info читается до protected GATT operations; encrypted Config Read завершает
  pairing до subscriptions. Ошибки sync и cancel освобождают единственный BLE link.
  Settings показывает «Максимальная яркость» с пояснением автоматики; Maintenance
  отправляет fill/checkerboard/text через существующую команду `DISPLAY_TEST`,
  без изменения BLE wire format.
  Dashboard, MVP settings, defaults и maintenance не включают diagnostics/export/
  dangerous commands Э6.
  Воспроизводимый Flutter/JDK/Android SDK/NDK/CMake toolchain лежит в игнорируемой
  `.tooling/` и разворачивается `tool/bootstrap-mobile.sh`.

## Проверки

- `pio test -e native`: 81/81 тестов проходят, включая автояркость, advertising,
  safe/dangerous framing, shared fixtures, Config Write, diagnostics и Serial parser.
- `pio run -e xiao_ble_sense`: primary 128×64 собирается, RAM 17 424 Б, Flash 171 160 Б.
- `pio run -e xiao_ble_sense_128x32`: compatible build собирается, RAM 16 912 Б,
  Flash 171 096 Б.
- Boot smoke на XIAO (`/dev/ttyACM0`): `BLE GATT: OK`, `BLE ADV name: BikeComp-D210`
  (не литерал `XXXX`), `OLED OK`, `selftest=0x3F`, `heap/16≈12695`, устройство
  стабильно после SoftDevice init.
- Embedded A/B-тест прошёл на XIAO: mount, A/B-чередование, чтение и fallback после
  повреждения свежего слота; `/test_storage_a/b` удалены после теста.
- Embedded odometer A/B на `/test_odo_a/b`: чередование и fallback после повреждения
  свежего слота; тестовые файлы удалены после теста (`pio test -e xiao_ble_sense`).
- DoD Э3.5 reboot на `/dev/ttyACM0`: seed 424242 mm / 77 rev переживает два reboot
  production (`Odometer: source=A, sequence=3`, те же значения оба раза).
- Mobile automatic gate: `dart format`, `flutter analyze` — no issues;
  `flutter test` — 52/52; актуальный release APK собран.
- Release APK: application ID `app.bikecomp.mobile`, minSdk 24, target/compileSdk 36,
  54,049,637 байт (`a31959c20eeb972c60070b30567f6f12bf94fc12a58051a64bc3203e7944f1c2`).
- Android-устройства через ADB и Bluetooth controller на хосте нет;
  локальные результаты не являются аппаратной приёмкой BLE.
- Primary production-сборка SSD1306 128×64 загружена на XIAO. Serial smoke:
  `i2c_err=0`, `isr_ovf=0`, `selftest=0x3F`, `heap/16=12630`. Пользователь подтвердил
  крупную разметку и работу экрана. Ранее проверенный профиль 128×32 остаётся
  совместимой сборкой; `reset-odo` на пользовательских данных не выполнялся.
- Ранее: два старта подряд прочитали config/odometer из слота A с `sequence=1`,
  без роста sequence и лишней записи.
- Пользовательский лог nRF Connect от 2026-07-31 подтверждает обнаружение
  `BikeComp-D210`, connect, service discovery, bonding и чтение Device Info,
  Telemetry, Config Read, Command Result и пустого Error Log. Ошибки
  `GATT READ NOT PERMIT` относятся к попыткам чтения descriptor у write-only chars;
  Config Write, команды и bond persistence этим логом не проверялись. Лог
  заканчивается без disconnect: пока nRF Connect держит единственное peripheral
  connection, firmware штатно не рекламируется и другое приложение его не найдёт.
- Android hardware smoke от 2026-07-31: после освобождения BLE-соединения в
  nRF Connect пользователь подтвердил, что новый release APK обнаруживает устройство,
  подключается и работает. Это закрывает базовую end-to-end интеграцию, но не заменяет
  измерение поиска ≤5 с, серию 10/10 и остальные пункты аппаратного gate.
- Прошивка загружалась на XIAO; OLED и импульсы кнопки подтверждены пользователем.
- Сборка `424fbc7` с общей C++-разметкой загружена на XIAO через `/dev/ttyACM0`.
- Сборка `4335e65` с энергосбережением OLED загружена через `/dev/ttyACM0`.
- На устройстве подтверждены приглушение через 30 секунд, выключение через 60 секунд
  и немедленное пробуждение по импульсу кнопки D0.
- На реальном OLED 128×32 подтверждены постоянная скорость, батарея справа сверху и
  автоматическая смена пяти нижних значений.
- `./simulator/test.sh`: 6 групп проверок и 18 golden-кадров для 128×32/128×64.
- Hardware smoke SSD1306 128×64 от 2026-07-31: primary firmware загружена на XIAO;
  Serial вернул `i2c_err=0`, `isr_ovf=0`, `selftest=0x3F`, `heap/16=12630`;
  крупная разметка и работа экрана подтверждены пользователем.
- Сборка с автояркостью не загружена: на момент проверки XIAO отсутствовал в
  `/dev/ttyACM*` и USB. Последней аппаратно подтверждённой остаётся production
  128×64 до добавления LDR.

## Ограничения

- Реальный датчик Холла и повторяемый стенд импульсов ещё не проверены.
- SSD1306 128×64 подключён и прошёл базовый hardware smoke; ещё не зафиксированы
  `LOW BATT`, test patterns, полный dim/off цикл и четыре фазы burn-in pixel shift.
- LDR-делитель ещё не собран: raw dark/room/outdoor, итоговый резистор,
  плавность переходов, максимумы 10/60/100% и средний ток ≤20 мкА ждут
  аппаратной проверки.
- Штатные NPR-позиции делителя не используются; внешний делитель P0.31 подтверждён.
- Автосохранение одометра: native + на XIAO подтверждены odo A/B embedded и
  ненулевой odometer после двух reboot. Остаётся ручная проверка 10× power-loss
  (чтобы не уничтожить обе копии `/odo_a|b`).
- BLE: pairing enforcement и persistence собраны, но reboot/closed-window сценарии
  ещё не приняты на телефоне; лимит 4 bonds с LRU пока не реализован. Prototype
  override `BIKECOMP_OPEN_PAIRING=1` остаётся opt-in. Error Log Read/Notify и sensor
  test 5 Гц ждут BLE hardware DoD. Стандартные DIS/BAS (`0x180A`/`0x180F`) отложены;
  приложение их не использует. `flash_write_count` RAM-only до персиста counters.
  `sd_softdevice_disable` при fail init не вызываем (ломает USB CDC); teardown =
  `Advertising.stop()`. `kSelftestWatchdogOk` не ставится — Watchdog ещё не init.
- Mobile hardware gate не завершён. Исправленный release APK установлен, и базовые
  discovery/connect подтверждены на реальном телефоне и XIAO. Нужны измерение поиска
  ≤5 с, 10/10 connect, bonding после reboot, write-then-verify пяти настроек, все
  MVP-команды, reconnect и permission flows на Android ≤11 и ≥12.
- `flutter_reactive_ble` 5.5.0 пока применяет legacy Kotlin Gradle Plugin;
  Flutter 3.44.7 собирает APK с предупреждением. CompileSdk библиотеки принудительно
  36; до будущего обновления Flutter нужно отслеживать Built-in Kotlin миграцию.
- Migration hook — заготовка identity v1→v2; реальное расширение payload потребует
  обновления `migrate_*` и, при изменении BLE-структуры, `protocol/`.

## Следующий шаг

Собрать LDR-делитель, измерить raw dark/room/outdoor, выбрать резистор и загрузить
production 128×64. Затем завершить OLED gate (`LOW BATT`, три test patterns,
dim/off/wake, четыре фазы pixel shift, импульсы) и Android gate (поиск ≤5 с,
10/10 подключений, bond/reconnect, пять write-then-verify и MVP-команды).
Отдельный долг — 10× power-loss для Э3.5.
