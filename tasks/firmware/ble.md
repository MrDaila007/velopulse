# Э4. BLE-протокол

- [x] 4.1 Создать `ble_protocol.h` и `static_assert` размеров.
- [x] 4.2 Реализовать codec всех BLE-структур.
- [x] 4.3 Сгенерировать shared `.hex`/`.json` fixtures и golden-тесты.
- [x] 4.4 Реализовать GATT service, characteristics, descriptors и permissions
  (`BleManager`: Bluefruit service + 7 chars, CCCD/User Desc, SECMODE per uuids.md;
  минимальная реклама + Scan Response name/Tx Power; payload/handlers — Э4.5–4.13;
  write stubs → ERR_NOT_SUPPORTED; ADV name resolves BikeComp-XXXX → serial).
- [x] 4.5 Реализовать fast/slow advertising и Scan Response.
  (`ble_advertising`: fast 30 ms / 30 s, slow 1000 ms; постоянная реклама по
  умолчанию либо timeout 5 мин; Scan Response name/TxPower; безопасный deferred
  restart после disconnect, restart по движению и runtime refresh config;
  native policy test, production build/upload и XIAO selftest `0x3F`.)
- [x] 4.6 Реализовать Device Information, FICR serial, uptime/reset reason.
  (`BleManager` Read-authorize: live `uptime_s` + flags; FICR serial; `mapNrfResetReason`
  из `RESETREAS`; `boot_count` в `/boot_cnt`; pairing window 5 мин /
  `BIKECOMP_OPEN_PAIRING`; bonded из `BLEConnection::bonded()`; USB live через
  `noteUsbPresent`).
- [x] 4.7 Реализовать Telemetry notify, sequence и adaptive rate.
  (`ble_telemetry`: 1 Гц notify / 0.2 Гц Read refresh / 5 Гц sensor-test hook;
  `BleManager::serviceTelemetry` + seq; `AppController` task `ble` 100 мс;
  live trip/battery/display/pulse → `TelemetryPacket`).
- [x] 4.8 Реализовать Config Read/Write, pending queue и validation/apply.
  (`ble_config_write`: BLE callback stages 48 B write; `AppController` validates,
  saves A/B config, applies runtime settings; `Command Result` OK/ERR_RANGE/ERR_BUSY/
  ERR_STORAGE + Config Read notify; advertising name refresh).
- [x] 4.9 Реализовать safe commands `0x01–0x0B`.
  (`ble_command`: strict framing/range checks + single-slot queue; `AppController`:
  trip/max reset, force-save с `ERR_STORAGE`, OLED on/off/test, sensor test 5 Гц с
  timeout/disconnect, battery/self-test и 16-byte `GET_DIAGNOSTIC`; shared fixtures
  проверяются firmware/mobile tests; mobile builder добавляет обязательные payload.)
- [x] 4.10 Реализовать dangerous command nonce, TTL и connection binding.
  (strict parser для `0x20–0x40`; аппаратный RNG; TTL 30 с; binding к command,
  parameter payload и connection; bonded-only; single-slot execution; reset/factory/
  reboot/calibration/odometer/open-pairing; общие request/NEEDS_CONFIRM fixtures.)
- [x] 4.11 Реализовать Command Result statuses и `field_id`.
  (Config Write и все safe/dangerous paths возвращают OK либо точный status/detail;
  diagnostics payload и nonce передаются нормативным little-endian codec.)
- [x] 4.12 Реализовать pairing/bonding, 5-minute window и encryption.
  (LESC Just Works + Bluefruit Flash bonds; encrypted GATT permissions; release
  default закрывает окно через 5 минут; raw security-event gate отклоняет новые
  pairing requests и отзывает непредвиденный bond; runtime reopen/factory reset;
  dangerous commands требуют bonded+secured; mobile preflight даёт `notPaired`.
  Hardware DoD и лимит 4 bonds/LRU остаются открыты ниже.)
- [x] 4.13 Реализовать Error Log и sensor test telemetry 5 Гц.
  (`ErrorLogBuffer`: RAM-кольцо 16 событий, wire snapshot последних 4; Read/Notify
  обслуживается вне BLE callback. Реальные I²C/Flash/config/pairing/ISR/sensor/
  watchdog/battery события; sensor-test Telemetry 200 мс + timeout/stop/disconnect;
  78/78 native, production build/upload и XIAO selftest `0x3F`.)
- [x] 4.14 Добавить `open-pairing`, `dump-config`, `selftest` в Serial.
  (Неблокирующий parser с CR/LF, trim, bounded buffer и overflow recovery;
  `reset-odo` из архитектуры §12.4; config fields + wire payload, diagnostic mask;
  78/78 native, production build/upload и безопасный Serial smoke на XIAO.)

## DoD Э4

- [x] nRF Connect читает все доступные для Read характеристики побайтно по
  спецификации (пользовательский лог 2026-07-31: Device Info, Telemetry, Config
  Read, Command Result, Error Log; service + 7 custom characteristics обнаружены).
- [x] Adafruit BLEDfu (buttonless OTA) зарегистрирован рядом с BikeComp GATT;
  первая заливка при мёртвом USB — `scripts/flash_stlink.sh`.
- [ ] Валидная config write применяется и сохраняется.
- [ ] Невалидная config write возвращает `ERR_RANGE` и верный field.
- [ ] RESET_TRIP работает через BLE.
- [ ] Dangerous command проходит NEEDS_CONFIRM → token → OK/expired.
- [ ] Bonding переживает reboot; закрытое окно блокирует новые pairing.
- [ ] Bond store ограничен четырьмя устройствами с LRU-заменой.
- [ ] Устройство считает поездку без телефона.
- [ ] `protocol/*`, firmware structs и fixtures синхронизированы.

## Процесс изменения протокола

- [ ] Изменение начинается с PR в `protocol/`.
- [ ] Обновляются version, field table и fixtures.
- [ ] Затем синхронно обновляются firmware и mobile codecs.
- [ ] Golden-тесты обеих сторон обязательны до merge.
