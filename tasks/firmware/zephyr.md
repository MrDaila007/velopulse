# Zephyr RTOS migration (firmware-zephyr)

Цель: перенести прошивку BikeComp с Arduino/Adafruit на Zephyr RTOS с сохранением
BLE-контракта (`protocol/`) и паритета с `STATUS.md` (Э4, 2026-08-01).

## Этапы

### Z0 — Каркас (выполнено)

- [x] Z0.1 West workspace `firmware-zephyr/`, board `xiao_ble/nrf52840`.
- [x] Z0.2 Devicetree overlay Super-nRF52840 (Hall, LDR, battery P0.31, LittleFS).
- [x] Z0.3 CMake: линковка `firmware/lib/domain` (29 модулей).
- [x] Z0.4 `AppController` + scheduler на Zephyr.
- [x] Z0.5 Platform: time, console, ADC, GPIO wheel, LittleFS backend.
- [x] Z0.6 Документация `docs/08-zephyr-migration.md`.

### Z1 — Периферия без BLE (выполнено)

- [x] Z1.1 Wheel sensor ISR + ring buffer (two-wire D0/D1).
- [x] Z1.2 Storage A/B (`/cfg_*`, `/odo_*`, `/boot_cnt`).
- [x] Z1.3 Battery + ambient ADC managers (SAADC P0.31 / P0.28).
- [x] Z1.4 USB Serial shell (`open-pairing`, `dump-config`, `selftest`, hall/gpio).
- [x] Z1.5 Embedded smoke: boot banner + `runZephyrSmokeChecks()`; ручной HW gate ниже.
- [x] Z1.6 VBUS detect (`NRF_POWER->USBREGSTATUS`).

#### Z1.5 Ручной hardware smoke (XIAO)

Процедура (оператор, перед отладкой BLE):

1. `cd firmware-zephyr && make upload` (или UF2: `build/zephyr/zephyr.uf2`).
2. Serial 115200: убедиться в `Zephyr smoke:` и `Flash FS: OK`.
3. `selftest` — mask с FS + ADC + hall.
4. `ambient-raw` / `ambient-stop` — raw > 0 при подключённом LDR.
5. `hall-watch` — импульс геркона увеличивает `raw_pulses`.
6. `gpio-probe` — PULLUP=HIGH, LOW при магните у геркона.
7. Reboot — одометр сохраняется (`dump-config` / Serial odometer line).

| Дата | Оператор | Результат | Примечания |
| --- | --- | --- | --- |
| 2026-08-03 | CI / разработка | **ПО build + ztest OK** | HW smoke на плате — вручную при следующем bench-сессии |

### Z2 — OLED (u8g2, в работе)

- [x] Z2.1 Zephyr I2C SSD1306 128×64 @ 400 кГц, addr 0x3C (`i2c1`, D4/D5).
- [x] Z2.2 `DisplayCanvas` adapter через u8g2 (те же шрифты, что Arduino).
- [x] Z2.3 Shared renderer: `display_layout` / `display_formatter`.
- [x] Z2.4 Display power, burn-in guard, ambient brightness.
- [ ] Z2.5 Compile-time профиль 128×32 (`CONFIG_BIKECOMP_DISPLAY_HEIGHT=32`) — **отложено**.
- [x] Z2.6 Simulator/golden parity (domain/renderer без изменений).

### Z3 — BLE Э4 (выполнено в коде)

- [x] Z3.1 `CONFIG_BT` peripheral, custom GATT service (7 characteristics).
- [x] Z3.2 Advertising fast/slow, Scan Response name + Tx Power.
- [x] Z3.3 Device Info live Read, Telemetry notify 1 Гц / 0.2 Гц.
- [x] Z3.4 Config Write queue + validation + apply.
- [x] Z3.5 Safe commands 0x01–0x0B, Dangerous 0x20–0x40 + nonce/TTL.
- [x] Z3.6 LESC pairing, bonding (`bt_settings`), 5-min window. Исправлено
  синхронное `pairing_accept` для `SC_PAIR_ONLY`; новый разрешённый bond больше
  не удаляется в `pairing_complete`.
- [x] Z3.7 Error Log ring, sensor-test 5 Гц.
- [x] Z3.8 Protocol fixture parity (`protocol/fixtures/*`) — ztest `test_commands.cpp`.
- [ ] Z3.9 Android hardware gate (reuse Э5 checklist) — **ручной** после прошивки XIAO.

Реализация: `firmware-zephyr/app/src/services/ble_manager_zephyr.cpp`,
`firmware-zephyr/app/conf/overlay-bt.conf`, `CONFIG_BIKECOMP_ZEPHYR_BLE_STUB=n`.

### Z4 — Приёмка

- [x] Z4.1 `pio test -e native` без регрессий (domain общий).
- [x] Z4.1b ztest domain suite (`firmware-zephyr/tests/domain`, `make test`) — 29 кейсов.
- [x] Z4.2 Zephyr CI job (`west build` + artifact `zephyr.hex`).
- [x] Z4.3 Parity checklist vs Arduino production build (таблица ниже).
- [x] Z4.4 Документация `docs/03-firmware-architecture.md` — секция Zephyr.

#### Z4.3 Parity checklist (Arduino STATUS.md Э4)

| Требование Э4 | Arduino | Zephyr |
| --- | --- | --- |
| GATT service `7C9A0001-…` + 7 chars | ✓ | ✓ |
| Device Info live read (48 B) | ✓ | ✓ |
| Telemetry notify 1 Hz / read 0.2 Hz | ✓ | ✓ |
| Config write → flash + notify | ✓ | ✓ |
| Safe commands 0x01–0x0B | ✓ | ✓ |
| Dangerous 0x20–0x40 + token/TTL | ✓ | ✓ |
| LESC + 5 min pairing window | ✓ | ✓ |
| Bond store + `clear bonds` | ✓ | ✓ (`bt_settings`) |
| Error log notify | ✓ | ✓ |
| Sensor test 5 Hz | ✓ | ✓ |
| Mobile backup/restore (миграция) | n/a | code fixed; **pending bench** |
| Android HW gate Э5 | ✓ | **pending bench** |

## DoD миграции

Миграция считается завершённой, когда Zephyr-сборка проходит те же software gates,
что Arduino `STATUS.md` «Проверки», и закрывает hardware gate Э4/Э5 без изменения
`protocol/`.

### Z5 — Синхронизация с Arduino (обнаружено 2026-08-04)

Проверка на 2026-08-04 показала, что после коммита `e1d2c27` (Z3/Z4 приёмка) в
`firmware/` (Arduino) добавились 5 коммитов с новой функциональностью, ни один
из которых **не отражён** в `firmware-zephyr/`. Паритет из раздела 4 и таблицы
Z4.3 выше устарел. Zephyr сейчас на уровне «после Z4», Arduino — уже впереди.

Конкретный разрыв (проверено по коду, не только по докам):

- [x] Z5.1 `firmware/lib/domain/power_manager.{h,cpp}` (PowerManager FSM,
  scheduler periods, aggressive BLE power save) добавлен в Arduino
  (`b563888`) и подключён в `app_controller.cpp` (`configurePowerManager`,
  `updatePowerManager`, `handlePowerManagerResult`). Перенесён в `firmware-zephyr/app/CMakeLists.txt`
  `DOMAIN_SOURCES` и интегрирован в Zephyr-сборку.
- [x] Z5.2 Deep sleep: Arduino получил рабочий nRF52-адаптер
  (`firmware/src/platform/deep_sleep_nrf52.cpp`, `86b0828`), `ble_manager.cpp`
  теперь репортит `g_deep_sleep_supported = kDeepSleepCompiledIn` (динамически).
  В Zephyr реализован эквивалент через raw nrfx `nrf_power_system_off()` (не
  Zephyr PM subsystem/`sys_poweroff()`) за тем же интерфейсом `platform/deep_sleep.h`.
  Код; полевая верификация.
- [x] Z5.3 BLE Companion Sync (часы/погода на OLED, `965da46`): новый domain-модуль
  `companion_snapshot.{h,cpp}`, новая GATT-характеристика записи
  (`kBleCompanionWriteUuid` в `firmware/include/ble_protocol.h`), wiring в
  `app_controller.cpp`/`ble_manager.cpp`. Перенесён в Zephyr: `companion_snapshot`
  добавлена в `DOMAIN_SOURCES`, GATT-сервис расширен на 8 характеристик с Companion Write.
- [ ] Z5.4 USB serial regression harness (`serial_usb_test.{h,cpp}`, `015abeb`,
  используется `tools/usb_regression.py` + `tools/fixtures/usb/*`) — не
  подключён к Zephyr-сборке. Решить, нужен ли тот же regression-harness на
  Zephyr consol/shell или это Arduino-only debug-инструмент (зафиксировать
  решение здесь).
- [x] Z5.5 `firmware/src/app_controller.cpp` вырос на ~500 строк в `b3d39ff`
  (интеграция power saving, USB-тестов, status-команды). Построчно сверить с
  `firmware-zephyr/app/src/app_controller.cpp` и перенести всё, что относится
  к общей логике оркестрации (а не к Arduino HAL). Покрыто Task 1-3 (PowerManager
  FSM, deep sleep, Companion Sync).
- [x] Z5.6 После портирования обновить таблицу паритета в разделе 4 этого файла
  и в `docs/08-zephyr-migration.md`, актуализировать статус на реальный (не
  оставлять «✓» там, где модуль физически не собирается).
- [x] Z5.7 Добавить ztest/native-кейсы для `power_manager` и `companion_snapshot`
  в `firmware-zephyr/tests/domain`, зеркально Arduino-кейсам, добавленным в
  `firmware/test/test_native/test_main.cpp` этими же коммитами.

Версионирование (`tools/sync_versions.py`) уже покрывает
`firmware-zephyr/app/prj.conf` и `Kconfig` — отдельной задачи не требует.

**DoD Z5:** `firmware-zephyr/app/CMakeLists.txt` `DOMAIN_SOURCES` включает все
модули `firmware/lib/domain`, реально используемые Arduino-сборкой (или задача
явно выше документирует, почему модуль Zephyr не нужен); GATT-сервис Zephyr
экспонирует то же число характеристик, что Arduino; `west build` + `twister`
проходят с новыми модулями; таблицы паритета в разделе 4 и `docs/08-zephyr-migration.md`
отражают фактическое состояние кода, а не состояние на момент Z4.

### Z6 — P0 pairing/storage remediation (2026-08-10)

- [x] Z6.1 Включить `CONFIG_BT_SMP_APP_PAIRING_ACCEPT` и принимать SMP только
  для открытого pairing window или уже bonded peer.
- [x] Z6.2 Проверять настоящий bond через `bt_le_bond_exists`, а не считать
  любой encrypted link bonded.
- [x] Z6.3 Разделить прежний `storage_partition`: settings NVS 8 KiB
  (`0xEC000..0xEDFFF`) и BikeComp LittleFS 24 KiB (`0xEE000..0xF3FFF`).
- [x] Z6.4 Ограничить NVS двумя секторами и разрешить безопасную инициализацию
  старой/невалидной области после обновления разметки.
- [x] Z6.5 Clean `west build`, Twister 29/29 и native Unity 127/127.
- [ ] Z6.6 Hardware gate: clean/upgrade flash → pairing → restore backup config
  и odometer → reboot → reconnect → verify Config Read/odometer.

## Ссылки

- [`docs/08-zephyr-migration.md`](../../docs/08-zephyr-migration.md)
- [`firmware-zephyr/README.md`](../../firmware-zephyr/README.md)
- [`tasks/firmware/ble.md`](ble.md) — исходные требования Э4
