# Статус проекта

Обновлено: 2026-07-30

## Текущий этап

Э3 — хранение данных. Реализованы A/B-хранилище, экономное автосохранение
одометра, migration hook flash-записей v1 → v2 и экспорт counters в diagnostics.

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
- `BatteryModel`: пересчёт ADC, калибровка, EMA, SoC LUT, монотонность и гистерезис.
- `BatteryManager`: 16 ADC-выборок и min/max rejection для внешнего делителя на P0.31.
- Аппаратная документация приведена к фактической схеме NologoTech Super-nRF52840.
- Внешний делитель 1 MΩ / 1 MΩ на P0.31 распаян и проверен: 4.016–4.037 В, 84–86% при зарядке.
- Автономная работа от Li-Po без USB и сохранение индикации заряда подтверждены пользователем.
- Установившийся разброс 16 выборок: 16–25 ADC counts (примерно 19–29 мВ).
- `DisplayPower`: приглушение на половине тайм-аута, выключение и пробуждение.
- `DisplayManager`: управление контрастом и SSD1306 power-save; корректный импульс
  колеса немедленно возвращает экран в яркий режим.
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
  для будущего `GET_DIAGNOSTIC`. Serial dump при старте;
  `AppController::diagnosticSnapshot()`.
- BLE Э4.1–4.6: `BleManager` GATT + live Device Info на Read (uptime, flags,
  bonded, pairing window); FICR serial; `reset_reason` из `NRF_POWER->RESETREAS`;
  `boot_count` в `/boot_cnt`; Config Read seed; Telemetry/CommandResult/ErrorLog —
  stubs; Config Write/Command → `ERR_NOT_SUPPORTED`; ADV Flags + Service UUID,
  Scan Response name + Tx Power; имя `BikeComp-XXXX` → serial (`ble_identity`);
  ack одометра только при успехе Flash; hot-path Serial за
  `BIKECOMP_HOTPATH_SERIAL` (по умолчанию выкл).

## Проверки

- `pio test -e native`: 54 тестов проходят (включая oneshot-ack, `ble_identity`,
  reset_reason map, boot_count, Device Info flags/uptime).
- `pio run -e xiao_ble_sense`: сборка проходит.
- Boot smoke на XIAO (`/dev/ttyACM0`): `BLE GATT: OK`, `BLE ADV name: BikeComp-D210`
  (не литерал `XXXX`), `OLED OK`, `selftest=0x3F`, `heap/16≈12695`, устройство
  стабильно после SoftDevice init.
- Embedded A/B-тест прошёл на XIAO: mount, A/B-чередование, чтение и fallback после
  повреждения свежего слота; `/test_storage_a/b` удалены после теста.
- Embedded odometer A/B на `/test_odo_a/b`: чередование и fallback после повреждения
  свежего слота; тестовые файлы удалены после теста (`pio test -e xiao_ble_sense`).
- DoD Э3.5 reboot на `/dev/ttyACM0`: seed 424242 mm / 77 rev переживает два reboot
  production (`Odometer: source=A, sequence=3`, те же значения оба раза).
- Production восстановлена после embedded-тестов (`pio run -e xiao_ble_sense -t upload`).
- Ранее: два старта подряд прочитали config/odometer из слота A с `sequence=1`,
  без роста sequence и лишней записи.
- Прошивка загружалась на XIAO; OLED и импульсы кнопки подтверждены пользователем.
- Сборка `424fbc7` с общей C++-разметкой загружена на XIAO через `/dev/ttyACM0`.
- Сборка `4335e65` с энергосбережением OLED загружена через `/dev/ttyACM0`.
- На устройстве подтверждены приглушение через 30 секунд, выключение через 60 секунд
  и немедленное пробуждение по импульсу кнопки D0.
- На реальном OLED подтверждены постоянная скорость, батарея справа сверху и
  автоматическая смена пяти нижних значений.
- `./simulator/test.sh`: 3 набора тестов и 9 golden-кадров проходят.

## Ограничения

- Реальный датчик Холла и повторяемый стенд импульсов ещё не проверены.
- Штатные NPR-позиции делителя не используются; внешний делитель P0.31 подтверждён.
- Автосохранение одометра: native + на XIAO подтверждены odo A/B embedded и
  ненулевой odometer после двух reboot. Остаётся ручная проверка 10× power-loss
  (чтобы не уничтожить обе копии `/odo_a|b`).
- BLE: handlers Config/Command — stub `ERR_NOT_SUPPORTED` до Э4.8–4.11; live
  Telemetry notify — Э4.7; стандартные DIS/BAS (`0x180A`/`0x180F`) отложены
  (SoftDevice attr-table risk на текущем стеке; приложение их не использует).
  `flash_write_count` RAM-only до персиста counters. `sd_softdevice_disable` при
  fail init не вызываем (ломает USB CDC); teardown = `Advertising.stop()`.
  `kSelftestWatchdogOk` не ставится — Watchdog ещё не init. Pairing enforcement
  (отклонение новых bonds после окна) — Э4.12; прототип
  `BIKECOMP_OPEN_PAIRING=1` держит флаг окна открытым.
- Migration hook — заготовка identity v1→v2; реальное расширение payload потребует
  обновления `migrate_*` и, при изменении BLE-структуры, `protocol/`.

## Следующий шаг

Остаток DoD Э3.5: 10× power-loss вручную (хотя бы один слот `/odo_a|b` жив),
затем закрыть M3. Дальше: Э4.7 Telemetry notify (seq + adaptive rate) или
Э4.8 Config Write; персист diagnostics counters. Ручной nRF Connect: Device Info
uptime/boot_count/reset_reason, имя `BikeComp-<hex>`, pairing для encrypted chars.
