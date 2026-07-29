# Э3. Хранение данных

## Выполнено

- [x] 3.1 CRC32 и эталонный native-тест.
- [x] 3.2 Canonical 48-byte DeviceConfig, defaults и validator.
- [x] 3.3 InternalFS, RecordHeader, version, CRC и A/B slots.
- [x] 3.4 Startup load, newest sequence, fallback и defaults.
- [x] 3.7 Counters записей, пропусков, ошибок и recovery.
- [x] Embedded corruption/fallback тест на XIAO.
- [x] Два startup без лишней записи: config/odo sequence остаётся 1.

## Текущий инкремент 3.5. Автосохранение

### Реализация

- [ ] Выделить policy/controller без зависимости от InternalFS.
- [ ] Сохранять каждые `odometer_save_interval_m`, default 500 м.
- [ ] Сохранять через 30 секунд после `MOVING → PAUSED`.
- [ ] Сохранять перед OLED off.
- [ ] Сохранять перед deep sleep.
- [ ] Предусмотреть API для будущей команды `FORCE_SAVE`.
- [ ] Сохранять однократно при battery ≤5%.
- [ ] Сохранять при USB disconnect и перед reboot.
- [ ] Продолжать поездку при ошибке Flash.
- [ ] Логировать trigger, result, sequence и counters в Serial.

### Тесты и DoD

- [ ] Native-тесты каждого trigger и его граничного значения.
- [ ] Неизменённый odometer не увеличивает sequence.
- [ ] За 5 км при 500 м выполняется ровно 10 записей.
- [ ] Embedded-тест чередует `/odo_a/b` и восстанавливает старый слот.
- [ ] Ненулевой odometer/revolutions переживает два reboot на XIAO.
- [ ] 10 power-loss попыток не уничтожают обе копии.
- [ ] Native, nRF build, embedded compile и OLED simulator зелёные.
- [ ] После embedded-теста возвращена production firmware.

## Остаток Э3

- [ ] 3.6 Добавить и протестировать migration hook v1 → v2.
- [ ] Персистировать/экспортировать storage counters в diagnostics.
- [ ] Проверить полный record и config migration на старых fixtures.
- [ ] Обновить `STATUS.md` и закрыть milestone M3.
