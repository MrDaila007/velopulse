# 09. Мобильное приложение: фактическое состояние кода

Документ фиксирует **как приложение реализовано сейчас** (ветка `dev`, код в `mobile-app/`),
в отличие от [`04-mobile-app-architecture.md`](04-mobile-app-architecture.md), который
описывает исходный архитектурный план Э5/Э6. Часть решений плана в код не попала или
попала в другом виде — расхождения отмечены по ходу. Актуальный чек-лист по этапам —
[`tasks/mobile/README.md`](../tasks/mobile/README.md).

---

## 1. Стек и параметры сборки

| Параметр | Значение |
| --- | --- |
| Flutter/Dart SDK | `^3.12.2` |
| Application ID / Bundle ID | `app.bikecomp.mobile` |
| Android | `minSdk 24`, `compileSdk/targetSdk 36` |
| iOS | `13.0+` |
| Версия приложения | `pubspec.yaml: version` (`1.1.0+3` на момент документа) |
| Локализация | только `ru` (`l10n.yaml` → `app_ru.arb`; `MaterialApp.locale` жёстко `ru`) |
| State management | Riverpod 3.x + `riverpod_generator`, code-gen (`providers.g.dart`) |
| Модели | Freezed 3.x + `json_serializable` |
| Навигация | `go_router` 17.x, `StatefulShellRoute` с 4 табами + отдельный `/connecting` |

Fake BLE-транспорт включается только флагом сборки `--dart-define=BIKECOMP_FAKE_BLE=true`
(`application/providers.dart:24-31`) — недоступен как runtime-настройка.

## 2. Слои и файлы (как есть)

В отличие от плана (по файлу на структуру: `telemetry_codec.dart`, `config_codec.dart` и т.д.),
кодеки объединены в один файл, модели — в два файла:

```text
lib/
├── core/                      result.dart · app_error.dart
├── domain/
│   ├── entities/               models.dart (все protocol-структуры и enum'ы)
│   │                            companion_models.dart (CompanionSnapshot, WeatherReading, prefs)
│   ├── validators/              config_validator.dart
│   └── services/                unit_formatter.dart · tire_presets.dart
├── data/
│   ├── protocol/                ble_uuids.dart · protocol_codecs.dart (все структуры v1)
│   ├── ble/                     ble_transport.dart (интерфейс) · reactive_ble_transport.dart
│   │                            fake_ble_transport.dart · ble_discovery_filter.dart
│   ├── local/                   preferences_store.dart · firmware_migration_store.dart
│   ├── weather/                 open_meteo_client.dart · weather_cities.dart
│   ├── bike_computer_repository.dart
│   ├── companion_sync_service.dart
│   ├── ride_log_recorder.dart
│   └── log_exporter.dart
├── application/                 providers.dart (все Riverpod-провайдеры и контроллеры)
│                                 app_states.dart (freezed-состояния)
├── platform/                     android_ble_platform.dart
└── presentation/
    ├── bikecomp_app.dart         (MaterialApp, роутинг, shell, ConnectionBadge)
    └── screens/                  scan · dashboard · settings · maintenance
    └── widgets/                  app_version_footer · companion_settings_card
```

Правило «байты только в `data/protocol`» соблюдается: `ProtocolCodecs` — единственное место
кодирования/декодирования, все смещения и биты флагов сверены с
[`protocol/data-structures.md`](../../protocol/data-structures.md) (device info, telemetry,
config, companion — совпадают побайтово).

### Функциональность сверх исходного плана

Не описана в `04-mobile-app-architecture.md`, но реализована:

- **Companion sync** (`companion_sync_service.dart`, `weather/`) — раз в 15 минут (и сразу
  при подключении) приложение пишет на характеристику `…000B` время, часовой пояс и погоду
  (Open-Meteo, без ключа), если устройство поддерживает эту характеристику
  (`BleUuids.companionWrite` в discovery). Настраивается карточкой `CompanionSettingsCard` на
  экране «Настройки» (город, показывать часы/погоду, °F).
- **Firmware migration backup/restore** (`local/firmware_migration_store.dart`) — на экране
  «Обслуживание» можно сохранить конфиг + одометр в `SharedPreferences` и восстановить их на
  новое/перепрошитое устройство (двухфазная запись одометра с токеном).
- **Экспорт лога сессии** (`log_exporter.dart`) — JSON с device info, конфигом, диагностикой,
  error log и накопленными телеметрическими сэмплами (`RideLogRecorder`, до 3600 записей),
  расшаривается через `share_plus`.

### Функциональность из плана, которая не реализована

- Отдельный **generic dangerous-command flow** (`sendDangerousCommand` из плана §5) — не
  существует. Есть только `_sendCommandDirect` (общий safe-путь) и один hardcoded двухфазный
  сценарий — `setOdometerMeters`, используемый исключительно в firmware-migration restore.
  `resetOdometer` (`0x20`), `factoryReset` (`0x21`), `reboot` (`0x22`), `SET_BATTERY_CAL`
  (`0x23`), `openPairingWindow` (`0x40`) объявлены в `DeviceCommandId`, но нигде не вызываются
  из UI.
- **ConfigProfileCodec** / экспорт-импорт JSON-профиля конфигурации с diff — отсутствует.
- Экраны 6–10 из плана (полные sensor/power/display settings, diagnostics, device info) —
  не выделены отдельно; их содержимое частично влито в один экран «Настройки»
  (`settings_screen.dart`: колесо/шины, единицы, яркость/тайм-ауты дисплея, power save,
  deep sleep, page switch — всё на одном экране с draft/dirty/conflict-логикой).
  `MagnetPassIndicator` и лог интервалов импульсов не реализованы; тест датчика на экране
  «Обслуживание» ограничен счётчиком оборотов, состоянием и возрастом импульса.
- Диагностика/self-test/version screen отдельным экраном нет — только `getDiagnostic()` при
  экспорте лога.

## 3. Экраны и навигация (как есть)

4 таба через `StatefulShellRoute` + один вне-табовый route:

| Route | Экран | Содержимое |
| --- | --- | --- |
| `/scan` | `ScanScreen` | Автосканирование при простое/ошибке, список найденных устройств (имя, RSSI, bond-статус), последнее устройство + reconnect, forget |
| `/connecting` | `ConnectingScreen` | Прогресс по `SyncStage` (mtu → discovery → deviceInfo → pairing → subscriptions), авто-редирект на `/dashboard` при `ready`/`readOnly` |
| `/dashboard` | `DashboardScreen` | Скорость крупно, 6 метрик (trip/avg/max/moving time/odometer/battery), статус-чипы (ride/sensor/link/RSSI), быстрые действия |
| `/settings` | `SettingsScreen` | Один экран: окружность колеса + пресеты шин, единицы, яркость/тайм-ауты, power save, deep sleep, page switch, `CompanionSettingsCard` (время/погода) |
| `/maintenance` | `MaintenanceScreen` | Reset trip, OLED on/off, display test (3 паттерна), force save, экспорт лога, firmware backup/restore, sensor test с таймером 60 с |

`BikeCompShell` — общий `Scaffold` с `ConnectionBadge`, кнопкой disconnect и
`NavigationBar`; слушает `lastMessage`/`lastError` из состояния и показывает snackbar.
`didChangeAppLifecycleState` дергает `setForeground()` — на фоне отменяется телеметрия и
таймер reconnect.

## 4. Связь и состояние

`ConnectionController` (Riverpod, `keepAlive`) — единая FSM на все состояния
(`app_states.dart`: `idle/scanning/connecting/synchronizing/ready/readOnly/reconnecting/
failed/bluetoothOff/permissionRequired/incompatibleProtocol`). Ключевые механизмы:

- Автоподключение к «запомненному» устройству прямо во время скана (`scan()`,
  `providers.dart:272-296`).
- Синхронизация (`_synchronize`): MTU (247, `readOnly` если `<51`) → discovery всех required
  характеристик → device info → проверка `protoMajor == 1` → bonding/pairing window → config →
  сохранение драфта → подписки → telemetry → `ready`.
- Реконнект с экспоненциальной задержкой `[1, 2, 4, 8, 15]` c, только пока приложение на
  переднем плане и не было явного `disconnect()`.
- `BikeComputerRepositoryImpl` — единственная точка доступа к устройству; `writeConfig`
  делает write-then-verify (до 4 попыток при `BUSY`, ожидание notify или повторное чтение при
  таймауте), `_sendCommandDirect` — общий безопасный путь для команд.
- `ConfigDraftController` — черновик конфигурации, персистится в `SharedPreferences`
  (namespaced по `deviceId`), конфликт «черновик vs устройство» решается диалогом
  «применить/отбросить».

## 5. Протокол ⇄ приложение — сверка

Байтовые структуры и биты флагов в `protocol_codecs.dart` и геттерах `models.dart`
(`bonded`, `pairingWindowOpen`, `deepSleepSupported`, `displayOn`, `charging`, `lowBattery`,
`smoothingEnabled`, `unitsImperial` и т.д.) сверены построчно с
[`protocol/data-structures.md`](../../protocol/data-structures.md) — совпадений по офсетам и
битам не найдено расхождений. `ConfigValidator` дублирует диапазоны §4 документа один в один
(включая `zeroOrRange` для `display_timeout_s`/`deep_sleep_timeout_s`).

## 6. Тесты

10 файлов, 1207 строк, `flutter_test` + `mocktail`:

`ble_discovery_filter_test`, `config_validator_test`, `connection_controller_test`,
`connection_error_test`, `fake_repository_test`, `log_export_test`, `preferences_store_test`,
`protocol_codecs_test` (golden-тесты на фикстурах `protocol/fixtures/`),
`reactive_ble_transport_test`, `widget_test`.

`flutter`/`dart` в среде ревью недоступны — тесты и `flutter analyze` не запускались, только
статический разбор кода.

---

## 7. Известные проблемы (код-ревью, 2026-08-04, ветка `dev`)

Ревью — статическое (без запуска `flutter analyze`/`flutter test`), покрывает BLE-транспорт,
кодеки протокола, `ConnectionController`, `ConfigValidator`, локальные хранилища и экраны.

1. **Незавершённые опасные команды в `BikeComputerRepositoryImpl`.**
   Прошивка поддерживает `RESET_ODOMETER` (`0x20`), `FACTORY_RESET` (`0x21`), `REBOOT`
   (`0x22`), `SET_BATTERY_CAL` (`0x23`), `OPEN_PAIRING_WINDOW` (`0x40`) — двухфазные команды
   с токеном (см. §5.1). В приложении генерик-обработки `NEEDS_CONFIRM` для произвольной
   команды нет: `_sendCommandDirect` (`data/bike_computer_repository.dart:289-329`) при
   `status == needsConfirm` возвращает `Success` и не отправляет подтверждение — только
   `setOdometerMeters` (`:346-381`) реализует полный цикл, и то лишь для firmware-migration
   restore. Пока эти команды не вызываются из UI, поведение безопасно, но это открытый
   функциональный пробел: при добавлении экрана для `factoryReset`/`reboot` наивный вызов
   `sendCommand` завершится «успехом» без реального выполнения действия на устройстве.

2. **Несогласованная очистка ресурсов при несовместимом протоколе устройства.**
   `_synchronize` (`application/providers.dart:400-409`): при `info.protoMajor != 1` метод
   просто выставляет состояние `incompatibleProtocol` и `return`, не бросая исключение — в
   отличие от ветки `notPaired`, которая кидает `AppErrors.notPaired` и тем самым запускает
   `_disposeRepository()`/`_cancelLink()` в `catch` внутри `connectDevice`. В результате BLE-
   соединение и созданный `BikeComputerRepositoryImpl` остаются активными до следующего вызова
   `connectDevice`. Возможно, это намеренно (чтобы показать причину, не разрывая линк), но
   стоит явно задокументировать в коде или выровнять с остальными терминальными ошибками.

3. **`LogExporter._diagnosticJson` теряет часть диагностики и содержит фиктивное поле.**
   `data/log_exporter.dart:75-82` экспортирует только `rawPulseCount`, `rejectedDebounce`,
   `rejectedOverspeed`, `isrOverflow`. Поля `flashWriteCount`, `freeHeapBytes`,
   `i2cErrorCount`, `selftestMask` из `DiagnosticSnapshot` в лог не попадают, а вместо этого
   всегда пишется `"speedIntervalCorrected": 0` — поля с таким именем в модели вообще нет.
   `test/log_export_test.dart:148` явно проверяет этот ноль, то есть поведение осознанное
   (заглушка на будущее?), но по факту это забытое/неиспользуемое поле, которое стоит либо
   убрать, либо заменить реальными диагностическими данными, полезными в отчёте.

4. **Возможно отсутствующий `return` при пустом логе поездки на экспорте.**
   `presentation/screens/maintenance_screen.dart:88-96`: при
   `notifier.rideLogSampleCount == 0` показывается snackbar «лог пуст», но `return` после
   него нет — экспорт продолжается (в лог всё равно попадут diagnostic/error log без
   `telemetrySamples`). Может быть намеренным, но стоит явно подтвердить это поведение —
   иначе выглядит как забытый ранний выход.

### Не проверено

Сборка и тесты не запускались в среде ревью (нет `flutter`/`dart`) — перед мерджем
рекомендуется прогнать `flutter analyze` и `flutter test` в `mobile-app/`, особенно
`connection_controller_test.dart` и `protocol_codecs_test.dart`.
