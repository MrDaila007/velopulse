# Реестр UUID — Bike Computer Configuration Service

Версия документа: 1.0 · Версия протокола: **1.0**

Все UUID — 128-битные, из единого базового диапазона проекта. UUID **зафиксированы** и не
меняются при изменении `minor`-версии протокола. Изменение UUID допускается только вместе
с инкрементом `major`.

## Базовый UUID

```text
7C9A-xxxx-4B7D-4F2E-9C1A-2E6D5F8B31A4   ← схема
7C9AxxxxA-...                            ← неверно, не использовать
```

Каноническая форма (RFC 4122, старший байт первым при записи в текстовом виде):

```text
7C9AXXXX-4B7D-4F2E-9C1A-2E6D5F8B31A4
```

где `XXXX` — 16-битный идентификатор объекта из таблицы ниже.

## Таблица UUID

| Объект | XXXX | Полный UUID |
| --- | --- | --- |
| Service: Bike Computer Configuration | `0001` | `7C9A0001-4B7D-4F2E-9C1A-2E6D5F8B31A4` |
| Char: Device Information | `0002` | `7C9A0002-4B7D-4F2E-9C1A-2E6D5F8B31A4` |
| Char: Telemetry | `0003` | `7C9A0003-4B7D-4F2E-9C1A-2E6D5F8B31A4` |
| Char: Config Read | `0004` | `7C9A0004-4B7D-4F2E-9C1A-2E6D5F8B31A4` |
| Char: Config Write | `0005` | `7C9A0005-4B7D-4F2E-9C1A-2E6D5F8B31A4` |
| Char: Command | `0006` | `7C9A0006-4B7D-4F2E-9C1A-2E6D5F8B31A4` |
| Char: Command Result | `0007` | `7C9A0007-4B7D-4F2E-9C1A-2E6D5F8B31A4` |
| Char: Error Log (опционально) | `0008` | `7C9A0008-4B7D-4F2E-9C1A-2E6D5F8B31A4` |
| Зарезервировано: Ride Log (v2+) | `0009` | `7C9A0009-4B7D-4F2E-9C1A-2E6D5F8B31A4` |
| Зарезервировано: Firmware Update meta | `000A` | `7C9A000A-4B7D-4F2E-9C1A-2E6D5F8B31A4` |

## Свойства характеристик

| Характеристика | Read | Write | Write w/o Resp | Notify | Требует шифрования |
| --- | :---: | :---: | :---: | :---: | :---: |
| Device Information | ✔ | — | — | — | нет |
| Telemetry | ✔ | — | — | ✔ | да (чтение — да) |
| Config Read | ✔ | — | — | ✔ | да |
| Config Write | — | ✔ | — | — | **да** |
| Command | — | ✔ | — | — | **да** |
| Command Result | ✔ | — | — | ✔ | да |
| Error Log | ✔ | — | — | ✔ | да |

`Device Information` доступна без шифрования, чтобы приложение могло проверить версию
протокола до сопряжения (ТЗ §32).

## Стандартные сервисы

Дополнительно устройство публикует стандартные сервисы для совместимости с системными
BLE-утилитами (не являются частью контракта, приложение их не использует):

| Сервис | UUID | Примечание |
| --- | --- | --- |
| Generic Access | `0x1800` | Имя устройства, appearance |
| Device Information Service | `0x180A` | Manufacturer, Model, FW Rev |
| Battery Service | `0x180F` | Battery Level (0…100) — удобно для сторонних приложений |

## Реклама

| Элемент | Где | Данные |
| --- | --- | --- |
| Flags | ADV | LE General Discoverable, BR/EDR not supported |
| Complete List of 128-bit Service UUIDs | ADV | `7C9A0001-…` (17 байт) |
| Complete Local Name | Scan Response | `BikeComp-XXXX`, до 20 символов |
| Tx Power Level | Scan Response | для оценки дистанции (опционально) |

Приложение **обязано** фильтровать результаты сканирования по Service UUID `7C9A0001-…`
(ТЗ §21).

## Правила изменения реестра

1. Новый объект получает следующий свободный `XXXX`; повторное использование запрещено.
2. Удаление объекта = инкремент `major` версии протокола.
3. Добавление новой характеристики без изменения существующих = инкремент `minor`.
4. Изменения вносятся сначала в этот файл, затем в `ble_protocol.h` (прошивка) и
   `ble_uuids.dart` (приложение). Расхождение считается дефектом сборки.
