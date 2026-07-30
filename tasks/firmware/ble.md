# Э4. BLE-протокол

- [x] 4.1 Создать `ble_protocol.h` и `static_assert` размеров.
- [x] 4.2 Реализовать codec всех BLE-структур.
- [x] 4.3 Сгенерировать shared `.hex`/`.json` fixtures и golden-тесты.
- [x] 4.4 Реализовать GATT service, characteristics, descriptors и permissions
  (`BleManager`: Bluefruit service + 7 chars, CCCD/User Desc, SECMODE per uuids.md;
  минимальная реклама + Scan Response name/Tx Power; payload/handlers — Э4.5–4.13;
  write stubs → ERR_NOT_SUPPORTED; ADV name resolves BikeComp-XXXX → serial).
- [ ] 4.5 Реализовать fast/slow advertising и Scan Response.
  (частично: intervals 30/1000 ms + fast timeout 30 s + name/TxPower уже есть;
  остаётся polish/DoD Э4.5).
- [x] 4.6 Реализовать Device Information, FICR serial, uptime/reset reason.
  (`BleManager` Read-authorize: live `uptime_s` + flags; FICR serial; `mapNrfResetReason`
  из `RESETREAS`; `boot_count` в `/boot_cnt`; pairing window 5 мин /
  `BIKECOMP_OPEN_PAIRING`; bonded из `BLEConnection::bonded()`; USB live через
  `noteUsbPresent`).
- [ ] 4.7 Реализовать Telemetry notify, sequence и adaptive rate.
- [ ] 4.8 Реализовать Config Read/Write, pending queue и validation/apply.
- [ ] 4.9 Реализовать safe commands `0x01–0x0B`.
- [ ] 4.10 Реализовать dangerous command nonce, TTL и connection binding.
- [ ] 4.11 Реализовать Command Result statuses и `field_id`.
- [ ] 4.12 Реализовать pairing/bonding, 5-minute window и encryption.
- [ ] 4.13 Реализовать Error Log и sensor test telemetry 5 Гц.
- [ ] 4.14 Добавить `open-pairing`, `dump-config`, `selftest` в Serial.

## DoD Э4

- [ ] nRF Connect читает все характеристики побайтно по спецификации.
- [ ] Валидная config write применяется и сохраняется.
- [ ] Невалидная config write возвращает `ERR_RANGE` и верный field.
- [ ] RESET_TRIP работает через BLE.
- [ ] Dangerous command проходит NEEDS_CONFIRM → token → OK/expired.
- [ ] Bonding переживает reboot; закрытое окно блокирует новые pairing.
- [ ] Устройство считает поездку без телефона.
- [ ] `protocol/*`, firmware structs и fixtures синхронизированы.

## Процесс изменения протокола

- [ ] Изменение начинается с PR в `protocol/`.
- [ ] Обновляются version, field table и fixtures.
- [ ] Затем синхронно обновляются firmware и mobile codecs.
- [ ] Golden-тесты обеих сторон обязательны до merge.
