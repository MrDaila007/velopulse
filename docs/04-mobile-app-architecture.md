# 04. Архитектура мобильного приложения

Платформа v1.0: **Android** (minSdk 24, targetSdk 35). Технология: **Flutter 3.24+ / Dart 3.5+**.
Задел на iOS сохраняется: платформозависимый код изолирован в одном слое.

Требования-источники: ТЗ §18–§31, §43, §45.

---

## 1. Стек технологий

| Задача | Выбор | Обоснование |
| --- | --- | --- |
| UI-фреймворк | Flutter 3.24+ | Единая кодовая база, путь на iOS (ТЗ §19) |
| BLE | `flutter_blue_plus` ^1.32 | Наиболее зрелая поддержка Android 12+ разрешений, работает с 128-битными UUID, стабильные стримы состояния |
| Состояние | `flutter_riverpod` ^2.5 + `riverpod_generator` | Явные зависимости, тестируемость, автоматическая утилизация подписок |
| Модели | `freezed` + `json_serializable` | Иммутабельные модели, copyWith для черновиков конфигурации |
| Локальное хранилище | `shared_preferences` (настройки) + `path_provider` (файлы профилей) | Достаточно для v1.0, без БД |
| Навигация | `go_router` ^14 | Deep links, типизированные маршруты |
| Разрешения | `permission_handler` ^11 | Единая точка для BLUETOOTH_SCAN/CONNECT/LOCATION |
| Логирование | `logger` + собственный ring-buffer для экспорта диагностики | Экспорт отчёта (ТЗ §27) |
| Тесты | `flutter_test`, `mocktail` | Unit + widget + golden-тесты кодеков |

Альтернатива Kotlin + Compose (ТЗ §19) отклонена: см. ADR-011.

## 2. Слои

```text
┌──────────────────────────────────────────────────────────────────────┐
│ PRESENTATION  screens/ · widgets/ · theme/                           │
│   Экраны читают провайдеры, не знают про BLE и байты                 │
└──────────────────────────┬───────────────────────────────────────────┘
┌──────────────────────────▼───────────────────────────────────────────┐
│ APPLICATION (Riverpod)                                               │
│   ScanController · ConnectionController · TelemetryProvider           │
│   ConfigDraftController · CommandController · DiagnosticsController   │
└──────────────────────────┬───────────────────────────────────────────┘
┌──────────────────────────▼───────────────────────────────────────────┐
│ DOMAIN (чистый Dart, без Flutter и BLE)                              │
│   entities: Telemetry, DeviceConfig, DeviceInfo, CommandResult        │
│   validators: ConfigValidator (зеркало ТЗ §31)                       │
│   services: UnitFormatter, TirePresets, ConfigProfileCodec           │
└──────────────────────────┬───────────────────────────────────────────┘
┌──────────────────────────▼───────────────────────────────────────────┐
│ DATA                                                                 │
│   BikeComputerRepository (единственный вход в устройство)             │
│   protocol/  telemetry_codec · config_codec · command_codec · uuids   │
│   ble/       BleTransport (обёртка flutter_blue_plus)                 │
│   local/     PreferencesStore · ProfileFileStore                     │
└──────────────────────────┬───────────────────────────────────────────┘
┌──────────────────────────▼───────────────────────────────────────────┐
│ PLATFORM  permissions · bluetooth adapter state · file picker        │
└──────────────────────────────────────────────────────────────────────┘
```

Правило: **байты живут только в `data/protocol`**. Выше передаются доменные объекты.
Это позволяет тестировать всю логику без BLE-адаптера.

## 3. Структура каталогов

```text
mobile-app/
├── pubspec.yaml
├── lib/
│   ├── main.dart
│   ├── app.dart                       # MaterialApp + go_router + тема
│   ├── core/
│   │   ├── result.dart                # Result<T, AppError>
│   │   ├── app_error.dart             # типизированные ошибки + сообщения RU
│   │   ├── logging.dart
│   │   └── constants.dart
│   ├── domain/
│   │   ├── entities/
│   │   │   ├── telemetry.dart
│   │   │   ├── device_config.dart
│   │   │   ├── device_info.dart
│   │   │   ├── command_result.dart
│   │   │   ├── diagnostics_snapshot.dart
│   │   │   └── enums.dart             # RideState, SensorState, PowerState, CmdStatus
│   │   ├── validators/config_validator.dart
│   │   └── services/
│   │       ├── unit_formatter.dart
│   │       ├── tire_presets.dart
│   │       └── config_profile_codec.dart
│   ├── data/
│   │   ├── protocol/
│   │   │   ├── ble_uuids.dart         # зеркало protocol/uuids.md
│   │   │   ├── device_info_codec.dart
│   │   │   ├── telemetry_codec.dart
│   │   │   ├── config_codec.dart
│   │   │   ├── command_codec.dart
│   │   │   └── result_codec.dart
│   │   ├── ble/
│   │   │   ├── ble_transport.dart     # scan/connect/discover/read/write/subscribe
│   │   │   ├── ble_connection_state.dart
│   │   │   └── reconnect_policy.dart
│   │   ├── local/
│   │   │   ├── preferences_store.dart # последнее устройство, единицы, флаги UI
│   │   │   └── profile_file_store.dart
│   │   └── bike_computer_repository.dart
│   ├── application/
│   │   ├── providers.dart             # корневые провайдеры
│   │   ├── scan_controller.dart
│   │   ├── connection_controller.dart
│   │   ├── telemetry_provider.dart
│   │   ├── config_draft_controller.dart
│   │   ├── command_controller.dart
│   │   └── diagnostics_controller.dart
│   └── presentation/
│       ├── screens/
│       │   ├── scan/            (1) поиск устройств
│       │   ├── connecting/      (2) подключение
│       │   ├── dashboard/       (3) главный экран
│       │   ├── settings_bike/   (4) настройки велосипеда
│       │   ├── settings_display/(5) настройки дисплея
│       │   ├── settings_sensor/ (6) настройки датчика + тест
│       │   ├── settings_power/  (7) настройки питания
│       │   ├── diagnostics/     (8) диагностика
│       │   ├── maintenance/     (9) обслуживание
│       │   └── device_info/     (10) информация об устройстве
│       ├── widgets/
│       │   ├── speed_gauge.dart
│       │   ├── metric_tile.dart
│       │   ├── connection_badge.dart
│       │   ├── dirty_config_banner.dart
│       │   ├── confirm_dangerous_dialog.dart
│       │   ├── validated_number_field.dart
│       │   └── magnet_pass_indicator.dart
│       └── theme/
└── test/
    ├── protocol/     # golden-тесты на protocol/fixtures/
    ├── domain/       # валидатор, форматтер, профили
    ├── application/  # контроллеры на FakeBleTransport
    └── widget/
```

## 4. Модель состояния подключения

```text
             ┌──────────────┐
             │ bluetoothOff │◀── адаптер выключен (в любой момент)
             └──────┬───────┘
                    │ включён
┌───────────┐  scan ┌──────────┐ select ┌──────────────┐
│   idle    │──────▶│ scanning │───────▶│  connecting  │
└───────────┘       └──────────┘        └──────┬───────┘
      ▲                                        │ services discovered
      │ disconnect                    ┌────────▼─────────┐
      │                               │  synchronizing   │ read info+config, MTU, subscribe
      │                               └────────┬─────────┘
      │                                        │ ok
      │      ┌────────────────┐  разрыв ┌──────▼──────┐
      └──────│  reconnecting  │◀────────│    ready    │
             └────────┬───────┘         └─────────────┘
                      │ исчерпаны попытки → failed(reason)
                      ▼
              ┌───────────────┐
              │    failed     │ (показать причину + кнопку «повторить»)
              └───────────────┘
  Отдельная терминальная ветка: incompatibleProtocol — соединение сохраняется,
  доступно только чтение Device Information (ТЗ §32).
```

`ConnectionController` — `StateNotifier<ConnectionState>`, где `ConnectionState` —
freezed union. Все экраны реагируют на состояние: при `!ready` элементы записи
блокируются, отображается баннер статуса.

### Политика переподключения (ТЗ §30)

```dart
// reconnect_policy.dart
const delays = [1, 2, 4, 8, 15, 15, 15];  // секунды, далее 15 с бесконечно
// сбрасывается при успешном подключении;
// приостанавливается, когда приложение в фоне;
// прекращается при явном disconnect пользователем.
```

## 5. Репозиторий устройства

`BikeComputerRepository` — единственная точка доступа к устройству. Экраны и контроллеры
не обращаются к `flutter_blue_plus` напрямую.

```dart
abstract class BikeComputerRepository {
  Stream<ConnectionState> get connectionState;
  Stream<Telemetry>       get telemetry;      // broadcast, throttle 4 Гц max
  Stream<DeviceConfig>    get configUpdates;  // notify Config Read
  Stream<int>             get rssi;           // локальные измерения, 1 Гц

  Future<Result<DeviceInfo>>     readDeviceInfo();
  Future<Result<DeviceConfig>>   readConfig();
  Future<Result<void>>           writeConfig(DeviceConfig cfg);   // write-then-verify
  Future<Result<CommandResult>>  sendCommand(DeviceCommand cmd);
  Future<Result<CommandResult>>  sendDangerousCommand(DeviceCommand cmd); // 2 фазы
  Future<Result<DiagnosticsSnapshot>> readDiagnostics();

  Future<void> setTelemetrySubscribed(bool value);  // экономия энергии
  Future<void> disconnect();
}
```

### Реализация `writeConfig` (ТЗ §30, §31)

```text
1. Локальная валидация (ConfigValidator) — при ошибке возвращаем ERR без похода в BLE.
2. Кодирование в 48 байт.
3. Write with response на характеристику Config Write.
4. Ожидание Command Result{id=0xF0} с тайм-аутом 3 с.
5. status != OK → вернуть типизированную ошибку с field_id.
6. status == OK → дождаться notify Config Read (или явно перечитать) и сверить
   с отправленным. Расхождение → ошибка «устройство применило другие значения».
7. Только после сверки — обновить «сохранённое» состояние в UI.
```

### Реализация `sendDangerousCommand`

```text
1. Отправить команду без токена → ожидается NEEDS_CONFIRM + nonce.
2. Вернуть в UI объект PendingConfirmation(nonce, ttl=30 c, description).
3. UI показывает предупреждение, требует ввод слова подтверждения (ТЗ §28).
4. Повторная отправка с токеном; при ERR_TOKEN_EXPIRED — начать заново.
```

## 6. Управление конфигурацией: черновик и dirty-состояние

Проблема из ТЗ §30: «не терять введённые, но ещё не отправленные настройки».

```dart
@freezed
class ConfigDraftState with _$ConfigDraftState {
  const factory ConfigDraftState({
    required DeviceConfig? deviceConfig,   // последнее прочитанное с устройства
    required DeviceConfig? draft,          // с изменениями пользователя
    required Map<String, String> fieldErrors,
    required bool isWriting,
    required DateTime? lastSyncedAt,
  }) = _ConfigDraftState;
}
```

Правила:

* `isDirty = draft != deviceConfig` — считается по значению (freezed equality).
* Черновик **живёт в памяти приложения** при разрыве соединения и не перетирается
  автоматически при повторном чтении конфигурации.
* Если после переподключения `deviceConfig` изменился, а черновик грязный, показывается
  диалог: «Применить ваши несохранённые изменения / отбросить и взять с устройства».
* Валидация полей — при каждом изменении (`ConfigValidator`), кнопка «Сохранить» неактивна
  при наличии ошибок → выполняется критерий ТЗ §43.13 («не отправляет некорректные значения»).
* Черновик персистится в `shared_preferences` при уходе приложения в фон, чтобы выжить
  убийство процесса Android.

### ConfigValidator (зеркало ТЗ §31)

```dart
class ConfigValidator {
  static const wheelCircumference = IntRange(500, 3000);
  static const maxSpeedKmh        = IntRange(20, 200);
  static const stopTimeoutS       = IntRange(1, 30);
  static const displayTimeoutS    = IntRange(10, 600);   // + 0 = никогда
  static const brightnessPct      = IntRange(1, 100);
  static const pageSwitchPeriodS  = IntRange(1, 60);
  static const lowBatteryPct      = IntRange(5, 50);
  static const deepSleepTimeoutS  = IntRange(60, 3600);  // + 0 = выкл
  static const odoSaveIntervalM   = IntRange(100, 5000);
  static const smoothingWindow    = IntRange(2, 5);
  static const debounceMs         = IntRange(0, 50);
  // device_name: 3..15 символов, [A-Za-z0-9-_ ]
  // enabled_pages_mask: минимум одна страница
  // page_order: перестановка 0..4 без повторов
}
```

Значения границ и defaults продублированы в `protocol/data-structures.md §4` — источник
истины там; тест `config_validator_matches_protocol_test.dart` сверяет их с фикстурами.

## 7. Экраны

### 7.1. Карта экранов и MVP

| № | Экран | MVP (ТЗ §45) | Содержание |
| --: | --- | :---: | --- |
| 1 | Поиск устройств | ✔ | Список с именем, RSSI, id, статусом сопряжения; последнее устройство; автоподключение; «забыть» |
| 2 | Подключение | ✔ | Прогресс этапов (connect → MTU → discover → read → subscribe), ошибки |
| 3 | Главный (dashboard) | ✔ | Крупная скорость, метрики, статусы, быстрые действия |
| 4 | Настройки велосипеда | ✔ | Окружность колеса + пресеты шин, единицы измерения |
| 5 | Настройки дисплея | ✔ | Яркость, тайм-аут, карусель, страницы, тест дисплея |
| 6 | Настройки датчика | — | Лимит скорости, тайм-аут остановки, сглаживание, фронт, debounce, тест |
| 7 | Настройки питания | — | Тайм-аут сна, реклама, порог низкого заряда, калибровка АКБ |
| 8 | Диагностика | — | Версии, uptime, счётчики, self-test, экспорт отчёта |
| 9 | Обслуживание | ✔ | Сброс поездки, сброс одометра, профили конфигурации, factory reset |
| 10 | Информация об устройстве | — | Модель, serial, HW/FW/protocol, bond-статус |

MVP-навигация: 4 таба (Поиск/Главный/Настройки/Обслуживание), где «Настройки» объединяет
экраны 4 и 5. Полная версия разносит настройки по разделам.

### 7.2. Главный экран (ТЗ §22)

```text
┌──────────────────────────────────────────┐
│ BikeComp-4F2A   ● подключено   -62 dBm   │  ← ConnectionBadge
├──────────────────────────────────────────┤
│                                          │
│              24.8                        │  ← SpeedGauge (крупно, 72 sp)
│              км/ч                        │     цвет по состоянию датчика
│                                          │
├──────────────────────────────────────────┤
│ Дистанция          18.42 км              │
│ Средняя            19.7 км/ч             │  ← MetricTile ×7
│ Максимальная       42.3 км/ч             │
│ Время движения     01:12:36              │
│ Одометр            1 234.5 км            │
│ Аккумулятор        82 %  3.92 В          │
│ Датчик             OK · 8 421 об.        │
├──────────────────────────────────────────┤
│ [Сбросить поездку] [OLED вкл] [OLED выкл]│
│ [Обновить] [Настройки]                   │
└──────────────────────────────────────────┘
```

* Значения обновляются из `telemetryProvider` (throttle до 4 Гц для UI, notify 1 Гц).
* При потере соединения значения приглушаются и показывается время последнего обновления
  («данные от 12:41:07»), а не устаревшие цифры как актуальные.
* «Сбросить поездку» — безопасная команда, но с диалогом подтверждения (одно нажатие).

### 7.3. Настройки велосипеда (ТЗ §23)

* Поле окружности в мм с валидацией 500…3000 и кнопкой «выбрать по шине».
* Пресеты (значения — стартовые, корректируются пользователем):

| Профиль | Окружность, мм |
| --- | --: |
| 700×23C | 2096 |
| 700×25C | 2105 |
| 700×28C | 2136 |
| 700×32C | 2155 |
| 26″ × 1.95 | 2055 |
| 27.5″ × 2.10 | 2148 |
| 29″ × 2.10 | 2288 |

* Раздел «Как измерить точно»: метод отката колеса (отметить точку, прокатить один оборот
  с весом райдера, измерить расстояние) — короткая инструкция с иллюстрацией.
* Единицы измерения: переключатель км/мили, влияет **только на отображение** в приложении
  и на устройстве; данные в протоколе всегда метрические.

### 7.4. Настройки датчика и тест (ТЗ §24)

Экран теста должен показывать (все данные — из телеметрии в режиме 5 Гц):

* текущее логическое состояние пина (`sensor_state`);
* число импульсов (`revolutions` + `raw_pulse_count` из диагностики);
* время с последнего импульса (`last_pulse_age_ms`);
* расчётную тестовую скорость (`speed_x100`);
* `MagnetPassIndicator` — виджет-вспышка при каждом росте счётчика оборотов,
  плюс лог последних 10 интервалов в мс (помогает выявить двойные срабатывания).

### 7.5. Обслуживание (ТЗ §28)

* Сохранить/загрузить профиль конфигурации в JSON-файл (`ConfigProfileCodec`),
  формат: `{"schema":1,"proto":"1.0","config":{...},"exported_at":"..."}`;
  при загрузке — валидация и показ diff «текущее → из файла» перед записью.
* Экспорт диагностического отчёта (текстовый файл: устройство, версии, счётчики, лог
  приложения за сессию) через системный share-sheet.
* Сброс одометра: трёхступенчатый барьер — предупреждение → повторное подтверждение →
  ввод слова `СБРОС` → двухфазный протокол с токеном.
* Проверка и обновление прошивки — заглушки с пометкой «в будущей версии» (ТЗ §28).

## 8. Разрешения и особенности Android

| Версия Android | Разрешения |
| --- | --- |
| ≤ 11 (API ≤ 30) | `BLUETOOTH`, `BLUETOOTH_ADMIN`, `ACCESS_FINE_LOCATION` (обязательно для сканирования) |
| ≥ 12 (API ≥ 31) | `BLUETOOTH_SCAN` (`neverForLocation`), `BLUETOOTH_CONNECT` |

Обработка: перед первым сканированием — экран-объяснение, затем запрос. Отдельные состояния
UI для «разрешение отклонено навсегда» (кнопка «Открыть настройки») и «Bluetooth выключен»
(кнопка «Включить»). Проверяется также «службы геолокации выключены» на Android ≤ 11 —
частая причина пустого списка сканирования.

Дополнительно:
* Foreground-сервис в v1.0 **не используется**: телеметрия не нужна в фоне, приложение —
  конфигуратор. При уходе в фон подписки отменяются.
* `WRITE_EXTERNAL_STORAGE` не нужен — экспорт через share-sheet и SAF.

## 9. Обработка ошибок и сообщения пользователю (ТЗ §43.15)

Все ошибки — типизированные (`AppError`), с человекочитаемым русским текстом и, где
уместно, предлагаемым действием.

| `AppError` | Сообщение | Действие |
| --- | --- | --- |
| `bluetoothOff` | «Bluetooth выключен» | «Включить» |
| `permissionDenied` | «Нужно разрешение на поиск устройств рядом» | «Разрешить» / «Открыть настройки» |
| `deviceNotFound` | «Велокомпьютер не найден. Прокрутите колесо, чтобы разбудить устройство» | «Искать снова» |
| `connectionLost` | «Соединение потеряно, переподключение…» | автоматически |
| `serviceMissing` | «Устройство не поддерживает протокол велокомпьютера» | «Отключиться» |
| `incompatibleProtocol` | «Прошивка использует протокол 2.0, приложение — 1.0. Настройка недоступна» | «Обновить приложение» |
| `mtuTooSmall` | «Телефон не поддерживает нужный размер пакета. Доступен только просмотр» | — |
| `validationFailed(field)` | «Значение поля вне допустимого диапазона: 500–3000 мм» | подсветка поля |
| `deviceRejected(field)` | «Устройство отклонило значение "Окружность колеса"» | подсветка поля |
| `writeTimeout` | «Устройство не ответило. Проверяем фактическое состояние…» | авто-перечитывание |
| `storageError` | «Устройство не смогло сохранить настройки» | «Повторить» |
| `notPaired` | «Требуется сопряжение. Перезапустите устройство и подключитесь в течение 5 минут» | «Повторить» |
| `tokenExpired` | «Время подтверждения истекло» | «Начать заново» |

## 10. Тестирование приложения

| Уровень | Что покрываем | Инструменты |
| --- | --- | --- |
| Golden-тесты кодеков | Кодирование/декодирование всех структур на фикстурах `protocol/fixtures/` | `flutter_test` |
| Unit (domain) | `ConfigValidator` (границы и краевые значения), `UnitFormatter`, `TirePresets`, `ConfigProfileCodec` | `flutter_test` |
| Unit (application) | Контроллеры на `FakeBleTransport`: сценарии разрыва, тайм-аута записи, `ERR_RANGE`, двухфазного подтверждения, несовместимой версии | `mocktail` |
| Widget | Dashboard при разных состояниях, блокировка кнопки «Сохранить» при ошибке валидации, диалог опасной команды | `flutter_test` |
| Ручные | Реальное устройство: сценарии из [06-testing-and-acceptance.md](06-testing-and-acceptance.md) | чек-листы |

`FakeBleTransport` — эмулятор устройства на Dart: реализует все характеристики, генерирует
телеметрию по заданному профилю поездки, умеет отвечать ошибками и «падать» по команде.
Позволяет разрабатывать и тестировать всё приложение без железа — критично для параллельной
работы над прошивкой и приложением.

## 11. Соответствие критериям приёмки приложения (ТЗ §43)

| № | Критерий | Где реализовано |
| --: | --- | --- |
| 1 | Находит велокомпьютер по BLE | `ScanController` + фильтр по Service UUID |
| 2 | Подключается | `ConnectionController.connect` |
| 3 | Переподключается после разрыва | `ReconnectPolicy`, §4 |
| 4–6 | Скорость, дистанция, время, заряд | Dashboard + `telemetryProvider` |
| 7 | Читает конфигурацию | `readConfig` при синхронизации |
| 8–10 | Меняет окружность, OLED, тайм-ауты | Экраны 4, 5, 6 + `writeConfig` |
| 11 | Сброс поездки | `CommandController`, Dashboard/Обслуживание |
| 12 | Показывает результат каждой команды | `CommandResult` → snackbar/диалог |
| 13 | Не отправляет некорректные значения | `ConfigValidator` + блокировка кнопки |
| 14 | Обрабатывает несовместимый протокол | состояние `incompatibleProtocol` |
| 15 | Понятные сообщения об ошибках | таблица §9 |
