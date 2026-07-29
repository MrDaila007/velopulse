# Э3. Хранение данных

## Выполнено

- [x] 3.1 CRC32 и эталонный native-тест.
- [x] 3.2 Canonical 48-byte DeviceConfig, defaults и validator.
- [x] 3.3 InternalFS, RecordHeader, version, CRC и A/B slots.
- [x] 3.4 Startup load, newest sequence, fallback и defaults.
- [x] 3.6 Migration hook v1 → v2: `migrateConfigV1ToV2` /
  `migrateOdometerV1ToV2`, load+rewrite, future-version reject.
- [x] 3.7 Counters записей, пропусков, ошибок, recovery и migrations.
- [x] Embedded corruption/fallback тест на XIAO.
- [x] Два startup без лишней записи: config/odo sequence остаётся 1.

## Текущий инкремент 3.5. Автосохранение

### Реализация

- [x] Выделить policy/controller без зависимости от InternalFS.
- [x] Сохранять каждые `odometer_save_interval_m`, default 500 м.
- [x] Сохранять через 30 секунд после `MOVING → PAUSED`.
- [x] Сохранять перед OLED off.
- [x] Сохранять перед deep sleep.
- [x] Предусмотреть API для будущей команды `FORCE_SAVE`.
- [x] Сохранять однократно при battery ≤5%.
- [x] Сохранять при USB disconnect и перед reboot.
- [x] Продолжать поездку при ошибке Flash.
- [x] Логировать trigger, result, sequence и counters в Serial.

### Тесты и DoD

- [x] Native-тесты каждого trigger и его граничного значения.
- [x] Неизменённый odometer не увеличивает sequence.
- [x] За 5 км при 500 м выполняется ровно 10 записей.
- [ ] Embedded-тест чередует `/odo_a/b` и восстанавливает старый слот.
- [ ] Ненулевой odometer/revolutions переживает два reboot на XIAO.
- [ ] 10 power-loss попыток не уничтожают обе копии.
- [x] Native и nRF build зелёные (UI/simulator не затрагивались).
- [ ] После embedded-теста возвращена production firmware.

## Остаток Э3

- [x] 3.6 Добавить и протестировать migration hook v1 → v2.
- [x] Экспортировать storage counters в diagnostics snapshot
  (`diagnostics.{h,cpp}`, Serial dump, payload stub для GET_DIAGNOSTIC §6.1).
  Персист counters между reboot — ещё нет.
- [x] Проверить полный record и config migration на старых fixtures.
- [ ] Обновить `STATUS.md` и закрыть milestone M3 (после DoD 3.5 на железе).
