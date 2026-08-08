# Бинарные структуры BLE-протокола

Версия протокола: **1.0** · Порядок байт: **little-endian** (ТЗ §33)

Этот файл — нормативный контракт. Прошивка (`firmware/include/ble_protocol.h`) и приложение
(`mobile-app/lib/data/protocol/*.dart`) обязаны совпадать с ним побайтно. Расхождение
проверяется golden-тестами на фикстурах из `protocol/fixtures/`.

---

## 1. Общие правила

1. Все многобайтовые целые — little-endian.
2. Структуры упакованы без выравнивания: в C++ используется `#pragma pack(push,1)` /
   `__attribute__((packed))`; в Dart — ручная сборка через `ByteData`.
3. Каждая структура начинается с `struct_version` (uint8) — версия **этой** структуры,
   независимая от версии протокола. Правило: получатель, встретивший больший
   `struct_version`, читает известный ему префикс и игнорирует хвост; встретивший меньший —
   заполняет отсутствующие поля значениями по умолчанию.
4. Все строки — ASCII, дополняются нулями до фиксированной длины; терминирующий ноль
   не обязателен, если строка занимает всё поле.
5. Поля `reserved` пишутся нулями и игнорируются при чтении.
6. Контрольная сумма не используется: целостность обеспечивает канальный уровень BLE
   (ТЗ §33 допускает).
7. Единицы: скорости в `0.01 км/ч`, дистанция поездки в см, одометр в метрах, время в
   секундах, напряжение в мВ.

### MTU

Приложение обязано запросить ATT MTU **247** байт после подключения. Минимально
необходимый MTU для протокола — **51** (максимальная структура 48 байт + 3 байта ATT header).
Если согласованный MTU < 51, приложение переходит в режим «только чтение» и показывает
ошибку `ERR_MTU_TOO_SMALL`: fragmentation в v1.0 не реализуется.

### Типы в таблицах

`u8`, `u16`, `u32`, `i16` — беззнаковые/знаковые целые; `char[N]` — ASCII-строка;
`u8[N]` — массив байт.

---

## 2. Device Information (UUID `…0002`, Read)

Размер: **48 байт**, `struct_version = 1`.

| Off | Тип | Поле | Описание |
| --: | --- | --- | --- |
| 0 | u8 | `struct_version` | `1` |
| 1 | u8 | `proto_major` | Мажорная версия протокола (`1`) |
| 2 | u8 | `proto_minor` | Минорная версия протокола (`0`) |
| 3 | u8 | `hw_revision` | Ревизия платы (`1`) |
| 4 | char[16] | `model` | `"BIKECOMP-XIAO"` |
| 20 | char[12] | `fw_version` | Semver, напр. `"1.0.0"` |
| 32 | u8[8] | `serial` | Nordic DEVICEID (FICR), little-endian |
| 40 | u32 | `uptime_s` | Секунды с момента старта |
| 44 | u8 | `reset_reason` | См. §8.1 |
| 45 | u16 | `boot_count` | Число перезагрузок (из Flash) |
| 47 | u8 | `flags` | См. ниже |

`flags` (Device Info):

| Бит | Значение |
| --- | --- |
| 0 | `config_valid` — конфигурация загружена из Flash (0 = применены defaults) |
| 1 | `display_ok` — дисплей отвечает по I²C |
| 2 | `fs_ok` — файловая система смонтирована |
| 3 | `bonded` — текущее соединение с bonded-устройством |
| 4 | `pairing_window_open` — открыто окно нового сопряжения |
| 5 | `usb_connected` |
| 6 | `deep_sleep_supported` |
| 7 | reserved |

---

## 3. Telemetry (UUID `…0003`, Read + Notify)

Размер: **36 байт**, `struct_version = 1`. Частота notify: 1 Гц при активной подписке
(ТЗ §14); 0.2 Гц, если подписки нет (актуализация значения для Read).

| Off | Тип | Поле | Ед. | Описание |
| --: | --- | --- | --- | --- |
| 0 | u8 | `struct_version` | — | `1` |
| 1 | u8 | `flags` | — | См. ниже |
| 2 | u16 | `speed_x100` | 0.01 км/ч | Текущая скорость, `0` при паузе |
| 4 | u16 | `avg_speed_x100` | 0.01 км/ч | По времени движения |
| 6 | u16 | `max_speed_x100` | 0.01 км/ч | Максимум за поездку |
| 8 | u32 | `trip_distance_cm` | см | Дистанция текущей поездки |
| 12 | u32 | `moving_time_s` | с | Время движения |
| 16 | u32 | `odometer_m` | м | Общий пробег |
| 20 | u16 | `battery_mv` | мВ | Отфильтрованное напряжение |
| 22 | u8 | `battery_pct` | % | 0…100 |
| 23 | u8 | `ride_state` | — | См. §8.2 |
| 24 | u32 | `revolutions` | шт | Корректные обороты за поездку |
| 28 | u32 | `last_pulse_age_ms` | мс | Время с последнего импульса (`0xFFFFFFFF` = не было) |
| 32 | u16 | `seq` | — | Счётчик пакетов, растёт на 1, обнаружение пропусков |
| 34 | u8 | `sensor_state` | — | См. §8.3 |
| 35 | u8 | `power_state` | — | См. §8.4 |

`flags` (Telemetry):

| Бит | Значение |
| --- | --- |
| 0 | `moving` — устройство в состоянии движения |
| 1 | `display_on` |
| 2 | `charging` |
| 3 | `usb_connected` |
| 4 | `low_battery` |
| 5 | `smoothing_enabled` |
| 6 | `units_imperial` (только информативно, значения всегда метрические) |
| 7 | `charge_status_unknown` — статус зарядки недостоверен |

Замечания:
* `RSSI` в структуре отсутствует — измеряется приложением локально (ТЗ §14).
* При изменении окружности колеса `revolutions` не сбрасывается; сброс — только вместе с поездкой.

---

## 4. Configuration (UUID `…0004` Read/Notify, `…0005` Write w/ Response)

Размер: **48 байт**, `struct_version = 1`. Обе характеристики используют одну структуру.

| Off | Тип | Поле | Диапазон | Default | ТЗ |
| --: | --- | --- | --- | --- | --- |
| 0 | u8 | `struct_version` | `1` | `1` | — |
| 1 | u8 | `flags` | см. ниже | `0b0000_1111` | §15 |
| 2 | u16 | `wheel_circumference_mm` | 500…3000 | 2100 | §4.1 |
| 4 | u8 | `max_speed_kmh` | 20…200 | 100 | §4.3 |
| 5 | u8 | `stop_timeout_s` | 1…30 | 3 | §5.2 |
| 6 | u16 | `display_timeout_s` | 10…600, `0` = никогда | 60 | §7.2 |
| 8 | u16 | `deep_sleep_timeout_s` | 60…3600, `0` = выкл | 900 | §10.2 |
| 10 | u8 | `brightness_pct` | 1…100 | 60 | §7.4 |
| 11 | u8 | `page_switch_period_s` | 1…60 | 4 | §6.2 |
| 12 | u8 | `enabled_pages_mask` | биты 0…4, ≥1 бит | `0x1F` | §6.3 |
| 13 | u8 | `low_battery_pct` | 5…50 | 20 | §8.3 |
| 14 | u16 | `odometer_save_interval_m` | 100…5000 | 500 | §9.2 |
| 16 | u8 | `smoothing_window` | 2…5 | 3 | §4.5 |
| 17 | u8 | `debounce_ms` | 0…50 | 3 | §24 |
| 18 | u8 | `active_edge` | 0=FALLING, 1=RISING, 2=CHANGE | 0 | §24 |
| 19 | u8 | `pinned_page` | 0…4 (используется при выкл. карусели) | 0 | §6.3 |
| 20 | u16 | `batt_cal_scale_permille` | 800…1200 | 1000 | §2.5 |
| 22 | i16 | `batt_cal_offset_mv` | −500…+500 | 0 | §2.5 |
| 24 | u8[5] | `page_order` | перестановка 0…4 | `{0,1,2,3,4}` | §6.3 |
| 29 | u8 | `reserved_page` | `0` | `0` | — |
| 30 | char[16] | `device_name` | ASCII 3…15, `[A-Za-z0-9-_ ]` | `"BikeComp-XXXX"` | §15 |
| 46 | u16 | `reserved` | `0` | `0` | — |

`flags` (Configuration):

| Бит | Имя | Default | Описание |
| --- | --- | --- | --- |
| 0 | `smoothing_enabled` | 1 | Сглаживание скорости (ТЗ §4.5) |
| 1 | `auto_page_switch` | 1 | Карусель нижней строки (ТЗ §6.3) |
| 2 | `display_auto_off` | 1 | Автовыключение OLED (ТЗ §7.2) |
| 3 | `ble_always_advertise` | 1 | Постоянная реклама (ТЗ §10.3) |
| 4 | `units_imperial` | 0 | Единицы отображения: 0 = км, 1 = мили |
| 5 | `sensor_invert` | 0 | Инверсия логики датчика (ТЗ §24) |
| 6 | `power_save_mode` | 0 | Агрессивное энергосбережение |
| 7 | `deep_sleep_enabled` | 0 | Глубокий сон (ТЗ §10.2, в v1.0 по умолчанию выкл) |

### Правила записи

1. Запись принимается **только целиком** (48 байт). Частичная запись отклоняется со
   статусом `ERR_LENGTH`.
2. Устройство валидирует все поля; при первой ошибке отклоняет **весь пакет** и возвращает
   `Command Result` с `status = ERR_RANGE` и `detail = field_id` (см. §8.5).
3. Успешная запись: сохранение во Flash → применение → `Command Result{status=OK}` →
   notify на `Config Read` с фактической конфигурацией.
4. Приложение считает запись успешной только после `Command Result{status=OK}` (ТЗ §30).
5. Изменение `device_name` вступает в силу после перезапуска рекламы; устройство
   перезапускает её автоматически, соединение при этом не разрывается.

---

## 5. Command (UUID `…0006`, Write w/ Response)

Размер: **4…20 байт**, `struct_version = 1`.

| Off | Тип | Поле | Описание |
| --: | --- | --- | --- |
| 0 | u8 | `struct_version` | `1` |
| 1 | u8 | `command_id` | См. §5.1 |
| 2 | u8 | `flags` | бит0 `has_token` — в payload передан токен подтверждения |
| 3 | u8 | `payload_len` | 0…16 |
| 4 | u8[N] | `payload` | Зависит от команды |

### 5.1. Коды команд

| ID | Команда | Payload первого запроса | Payload подтверждения | ТЗ |
| --: | --- | --- | --- | --- |
| `0x01` | `RESET_TRIP` | — | не требуется | §5.5 |
| `0x02` | `RESET_MAX_SPEED` | — | не требуется | §16 |
| `0x03` | `FORCE_SAVE` | — | не требуется | §16 |
| `0x04` | `DISPLAY_ON` | — | не требуется | §16 |
| `0x05` | `DISPLAY_OFF` | — | не требуется | §16 |
| `0x06` | `DISPLAY_TEST` | u8 `pattern` (0=all on, 1=шахматка, 2=текст) | не требуется | §16 |
| `0x07` | `SENSOR_TEST_START` | u16 `duration_s` (1…120) | не требуется | §24 |
| `0x08` | `SENSOR_TEST_STOP` | — | не требуется | §24 |
| `0x09` | `BATTERY_TEST` | — | не требуется | §16 |
| `0x0A` | `START_DIAGNOSTIC` | — | не требуется | §16 |
| `0x0B` | `GET_DIAGNOSTIC` | — | не требуется | §27 |
| `0x20` | `RESET_ODOMETER` | — | u32 `token` | §5.5 |
| `0x21` | `FACTORY_RESET` | — | u32 `token` | §16 |
| `0x22` | `REBOOT` | — | u32 `token` | §16 |
| `0x23` | `SET_BATTERY_CAL` | u16 `scale`, i16 `offset` | u16 `scale`, i16 `offset`, u32 `token` | §16 |
| `0x30` | `SET_ODOMETER` | u32 `odometer_m` | u32 `odometer_m`, u32 `token` | сервис |
| `0x40` | `OPEN_PAIRING_WINDOW` | u16 `duration_s` | u16 `duration_s`, u32 `token` | §17 |

Диапазон `0x01…0x1F` — безопасные команды, `0x20…0x4F` — опасные (требуют токена).

### 5.2. Протокол подтверждения опасных команд (ТЗ §16, §28)

```text
Приложение                                   Устройство
    │  Command{id=0x20, flags=0, len=0}          │
    │───────────────────────────────────────────▶│  генерирует nonce, TTL 30 с
    │  Result{status=NEEDS_CONFIRM, token=N}     │
    │◀───────────────────────────────────────────│
    │  (UI: предупреждение + ввод слова          │
    │   подтверждения пользователем)             │
    │  Command{id=0x20, flags=1, payload=N}      │
    │───────────────────────────────────────────▶│  проверяет N и TTL
    │  Result{status=OK}                         │
    │◀───────────────────────────────────────────│
```

Для параметризованной команды приложение повторяет исходный payload без изменений и
добавляет `token` последними четырьмя байтами. Диапазоны: `scale` = 800…1200 ‰,
`offset` = −500…500 мВ, `duration_s` для окна сопряжения = 1…3600 с.

Правила:
* Nonce — 32 бита, генерируется аппаратным RNG, привязан к `command_id`, исходному
  payload и соединению.
* TTL = 30 с; по истечении — `ERR_TOKEN_EXPIRED`, нужно начинать заново.
* Одновременно активен только один nonce; новый запрос отменяет предыдущий.
* При разрыве соединения nonce аннулируется.
* Опасные команды требуют зашифрованного соединения с bonded-устройством.

### 5.3. Режим теста датчика

`SENSOR_TEST_START` переводит устройство в режим, где телеметрия отправляется с частотой
5 Гц и включает расширенные поля в `Error Log`-канале (уровень пина, число импульсов,
время последнего импульса, тестовая скорость — ТЗ §24). Режим завершается автоматически
по `duration_s`, командой `SENSOR_TEST_STOP` или при разрыве соединения.

---

## 6. Command Result (UUID `…0007`, Read + Notify)

Размер: **9…25 байт**, `struct_version = 1`. Отправляется на каждую команду и на каждую
запись конфигурации.

| Off | Тип | Поле | Описание |
| --: | --- | --- | --- |
| 0 | u8 | `struct_version` | `1` |
| 1 | u8 | `command_id` | Эхо команды; `0xF0` = результат записи конфигурации |
| 2 | u8 | `status` | См. §8.5 |
| 3 | u8 | `detail` | `field_id` при `ERR_RANGE`, битовая маска подтестов при self-test |
| 4 | u32 | `token` | Nonce при `NEEDS_CONFIRM`, иначе `0` |
| 8 | u8 | `payload_len` | 0…16 |
| 9 | u8[N] | `payload` | Данные ответа (для `GET_DIAGNOSTIC`, `BATTERY_TEST` и т. п.) |

### 6.1. Payload для `GET_DIAGNOSTIC` (16 байт)

| Off | Тип | Поле |
| --: | --- | --- |
| 0 | u32 | `raw_pulse_count` |
| 4 | u16 | `rejected_debounce` |
| 6 | u16 | `rejected_overspeed` |
| 8 | u16 | `isr_overflow` |
| 10 | u16 | `flash_write_count` |
| 12 | u16 | `free_heap_bytes / 16` |
| 14 | u8 | `i2c_error_count` |
| 15 | u8 | `selftest_mask` |

`selftest_mask`: бит0 I²C-дисплей, бит1 Hall-пин, бит2 ADC, бит3 файловая система,
бит4 конфигурация валидна, бит5 BLE-стек, бит6 watchdog, бит7 reserved. 1 = OK.

---

## 7. Error Log (UUID `…0008`, Read + Notify, опционально)

Кольцевой буфер из 16 последних событий. Один notify содержит до 4 записей.

| Off | Тип | Поле |
| --: | --- | --- |
| 0 | u8 | `struct_version` = 1 |
| 1 | u8 | `entry_count` (1…4) |
| 2 | entry[N] | по 8 байт каждая |

Запись (8 байт): `u32 uptime_s`, `u8 code`, `u8 severity` (0=info,1=warn,2=error),
`u16 detail`.

Коды событий: `0x01` I²C timeout, `0x02` ошибка Flash, `0x03` CRC конфигурации,
`0x04` переполнение ISR-буфера, `0x05` датчик залип, `0x06` перезагрузка по watchdog,
`0x07` критический заряд, `0x08` отклонена запись конфигурации, `0x09` отклонено сопряжение.

---

## 8. Companion Snapshot (UUID `…000B`, Write w/ Response)

Размер: **15 байт**, `struct_version = 1`. Телефон передаёт время и краткую погоду;
устройство держит soft RTC и отображает данные в шапке OLED.

| Off | Тип | Поле | Описание |
| --: | --- | --- | --- |
| 0 | u8 | `struct_version` | `1` |
| 1 | u32 | `unix_time` | UTC epoch seconds |
| 5 | i16 | `tz_offset_min` | Смещение от UTC, минуты |
| 7 | i16 | `temp_c_x10` | Температура ×10; `0x7FFF` = нет данных |
| 9 | u8 | `pop_pct` | Вероятность осадков 0–100; `0xFF` = нет данных |
| 10 | u8 | `flags` | См. ниже |
| 11 | u32 | `valid_until` | UTC epoch; после — stale |

`flags` (Companion):

| Бит | Значение |
| --- | --- |
| 0 | `time_valid` |
| 1 | `weather_valid` |
| 2 | `rain_now` |
| 3 | `rain_soon` |
| 4 | `stale` |

Добавление характеристики — инкремент **proto minor** до `1.1`.

---

## 9. Перечисления

### 9.1. `reset_reason`

| Код | Значение |
| --: | --- |
| 0 | `UNKNOWN` |
| 1 | `POWER_ON` |
| 2 | `PIN_RESET` |
| 3 | `WATCHDOG` |
| 4 | `SOFT_RESET` (команда `REBOOT`) |
| 5 | `LOCKUP` |
| 6 | `WAKE_FROM_SLEEP` |
| 7 | `BROWNOUT` |

### 9.2. `ride_state`

| Код | Значение |
| --: | --- |
| 0 | `IDLE` — поездка не начата |
| 1 | `MOVING` |
| 2 | `PAUSED` — автопауза |

### 9.3. `sensor_state`

| Код | Значение |
| --: | --- |
| 0 | `OK` — импульсы приходят |
| 1 | `IDLE` — импульсов нет, состояние штатное |
| 2 | `STUCK` — постоянный активный уровень |
| 3 | `NO_SIGNAL` — импульсов не было с момента старта |

### 9.4. `power_state`

| Код | Значение |
| --: | --- |
| 0 | `ACTIVE` |
| 1 | `SHORT_STOP` |
| 2 | `IDLE_DISPLAY_OFF` |
| 3 | `DEEP_SLEEP_PENDING` |
| 4 | `BLE_CONFIG` |
| 5 | `CHARGING` |

### 9.5. `status` (Command Result)

| Код | Значение | Действие приложения |
| --: | --- | --- |
| 0 | `OK` | Успех |
| 1 | `ERR_UNKNOWN_COMMAND` | Проверить версию протокола |
| 2 | `ERR_LENGTH` | Дефект кодека — сообщить об ошибке |
| 3 | `ERR_STRUCT_VERSION` | Несовместимая структура |
| 4 | `ERR_RANGE` | Показать поле `detail` как невалидное |
| 5 | `NEEDS_CONFIRM` | Запросить подтверждение, повторить с токеном |
| 6 | `ERR_TOKEN_INVALID` | Начать подтверждение заново |
| 7 | `ERR_TOKEN_EXPIRED` | Начать подтверждение заново |
| 8 | `ERR_NOT_PAIRED` | Предложить сопряжение |
| 9 | `ERR_BUSY` | Повторить через 1 с (до 3 попыток) |
| 10 | `ERR_STORAGE` | Показать ошибку сохранения |
| 11 | `ERR_HARDWARE` | Показать в диагностике |
| 12 | `ERR_NOT_SUPPORTED` | Функция отсутствует в этой прошивке |

### 9.6. `field_id` (для `ERR_RANGE`)

Совпадает со смещением поля в структуре Configuration. Приложение сопоставляет смещение
с полем UI по таблице §4 и подсвечивает конкретный ввод.

---

## 10. Зеркала структур в коде

### C++ (`firmware/include/ble_protocol.h`)

```cpp
#pragma pack(push, 1)
struct TelemetryPacket {
    uint8_t  struct_version;   // 1
    uint8_t  flags;
    uint16_t speed_x100;
    uint16_t avg_speed_x100;
    uint16_t max_speed_x100;
    uint32_t trip_distance_cm;
    uint32_t moving_time_s;
    uint32_t odometer_m;
    uint16_t battery_mv;
    uint8_t  battery_pct;
    uint8_t  ride_state;
    uint32_t revolutions;
    uint32_t last_pulse_age_ms;
    uint16_t seq;
    uint8_t  sensor_state;
    uint8_t  power_state;
};
static_assert(sizeof(TelemetryPacket) == 36, "Telemetry layout mismatch");
#pragma pack(pop)
```

Аналогичные `static_assert` обязательны для всех структур — это первая линия защиты
контракта.

### Dart (`mobile-app/lib/data/protocol/telemetry_codec.dart`)

Кодеки пишутся вручную через `ByteData` с `Endian.little`; каждый кодек покрыт тестом
на фикстуре из `protocol/fixtures/`.

## 11. Фикстуры

`protocol/fixtures/` содержит эталонные пакеты в hex с ожидаемой расшифровкой в JSON:

```text
fixtures/
├── device_info_v1_nominal.hex / .json
├── telemetry_v1_moving.hex / .json
├── telemetry_v1_paused.hex / .json
├── config_v1_defaults.hex / .json
├── config_v1_imperial.hex / .json
├── command_reset_trip.hex / .json
├── command_reset_odo_request.hex / .json
├── command_reset_odo_with_token.hex / .json
├── result_ok.hex / .json
├── result_needs_confirm_reset_odo.hex / .json
├── result_err_range_wheel.hex / .json
└── companion_v1_nominal.hex / .json
```

Обе стороны обязаны иметь тест: «декодировать `.hex` → сравнить с `.json`» и
«закодировать `.json` → сравнить с `.hex`». Это гарантирует совместимость без физического
устройства.
