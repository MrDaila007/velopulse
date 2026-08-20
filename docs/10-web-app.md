# 10. ПК веб-компаньон

Локальное веб-приложение в [`web-app/`](../web-app/) для **Chrome/Edge на ПК**.
Повторяет BLE-функции Flutter-компаньона и добавляет USB CDC-консоль для отладки.

Связанные документы: [`04-mobile-app-architecture.md`](04-mobile-app-architecture.md),
[`09-mobile-app-current-state.md`](09-mobile-app-current-state.md), [`protocol/ble-protocol.md`](../protocol/ble-protocol.md).

---

## Требования

| Параметр | Значение |
| --- | --- |
| Браузер | Chrome или Edge (Chromium) на ПК |
| Web Bluetooth | secure context: `https://` или `http://localhost` |
| Web Serial | Chrome/Edge на ПК (USB CDC) |
| Устройство | BikeComp с прошивкой BLE protocol v1.x |

**Не поддерживается:** Safari/iOS, Firefox (нет Web Bluetooth), мобильные браузеры.

### Linux

Web Bluetooth в Chrome на Linux **выключен по умолчанию**. Нужно одно из:

1. Запуск через `npm run web:open` / `npm run web:open` (скрипт передаёт флаги Chrome), **или**
2. Вручную: `chrome://flags` → **Experimental Web Platform features** → **Enabled** → перезапуск Chrome.

Проверка в консоли (`F12`):

```js
navigator.bluetooth
```

Должен быть объект, не `undefined`. Также нужен работающий **BlueZ** (`systemctl status bluetooth`).

## Запуск

```bash
cd web-app
npm install
npm run dev
```

Откройте **`http://localhost:5173/scan` в системном Google Chrome или Microsoft Edge**.

> **Важно:** встроенный браузер Cursor/VS Code **не поддерживает** Web Bluetooth и Web Serial,
> даже на `localhost`. Используйте внешний Chrome:
>
> ```bash
> npm run open
> ```

### Fake BLE (без железа)

```bash
VITE_FAKE_BLE=true npm run dev
```

Или включите чекбокс «Fake BLE» на экране «Скан».

### Сборка и тесты

```bash
npm run build
npm test
```

Тесты кодеков сверяются с золотыми фикстурами в [`protocol/fixtures/`](../protocol/fixtures/).

## Экраны

| Маршрут | Назначение |
| --- | --- |
| `/scan` | Web Bluetooth scan/connect, reconnect, forget |
| `/dashboard` | Телеметрия, быстрые команды |
| `/settings` | Конфигурация BLE (48 B) по секциям, черновик в localStorage, companion clock/weather |
| `/maintenance` | Сервисные команды, sensor test, reboot, backup/restore, export log |
| `/debug` | USB serial CDC, live LDR (`ambient-raw`), raw GATT hex |

## Архитектура

```text
src/
  protocol/     LE-кодеки и UUID (единственное место для байтов)
  transport/    WebBluetooth · FakeBle · WebSerial
  domain/       типы, валидатор, форматтеры
  services/     repository, connection FSM, companion sync, backup
  ui/           React-экраны
```

Правило как в мобильном приложении: **байты только в `protocol/`**.

## Ограничения Web Bluetooth

- Подключение требует жеста пользователя (`requestDevice`).
- Bonding/шифрование в Chrome ограничены по сравнению с Android Flutter.
- При `pairing closed` откройте окно сопряжения через USB-команду `open-pairing`
  на вкладке «Отладка» или перезагрузите устройство.

## USB debug

Web Serial @ 115200, те же ASCII-команды, что в [`serial_console`](../firmware/lib/domain/serial_console.cpp)
и [`tools/serial_client.py`](../tools/serial_client.py).

Пресеты: `status`, `dump-config`, `selftest`, `open-pairing`, `hall-status`, `ambient-raw`, `ambient-stop`, `csc-status`, `csc-pair`, `csc-forget`, и др.

## Карта полей: BLE Config vs USB

Все настраиваемые параметры прошивки v1.x — в GATT **Configuration** (48 B), см.
[`protocol/data-structures.md`](../protocol/data-structures.md) §4. Веб-компаньон покрывает все поля
на экране «Настройки» (вкладки: Колесо, Датчик, Дисплей, Питание, Устройство). Companion clock/weather —
отдельная характеристика, вкладка Companion.

| Область | BLE (настройки) | USB только (отладка) |
| --- | --- | --- |
| Колесо / скорость | `wheel_circumference_mm`, `units_imperial`, `max_speed_kmh`, `smoothing_*`, `stop_timeout_s` | — |
| Датчик Холла | `debounce_ms`, `active_edge`, `sensor_invert` | `hall-status`, `hall-watch` |
| Дисплей | `brightness_pct` (потолок LDR), таймауты, страницы 0–7 (`enabled_pages_mask` биты 5–6 = погода, бит 7 = CAD при C3), `page_order`, `pinned_page` | — |
| LDR калибровка | **не доступна** (только `brightness_pct`) | `ambient-raw` / `ambient-stop` — live raw/filtered/pct; пороги в `platformio.ini` или `ldr_calibrate.py` |
| Питание | `low_battery_pct`, `batt_cal_*`, sleep/advertise, `odometer_save_interval_m` | `power-status` |
| Устройство | `device_name` | — |
| Companion | companion write (не в Config 48 B) | — |

Черновик настроек сохраняется в `localStorage` (`bikecomp.v1.configDraft.{deviceId}`). При
расхождении с устройством показывается баннер конфликта.

## Паритет с mobile-app

Реализовано:

- Scan/connect FSM, version gate `protoMajor === 1`
- Dashboard telemetry, включая каденс C3 и статус S3/CSC
- Settings: все поля BLE Config 48 B (6 секций), черновик + conflict banner, LDR helper на дисплее
- Companion sync (Open-Meteo, интервал 15 мин)
- Maintenance: safe commands, sensor test, reboot (dangerous token), backup/restore, session log JSON
- Debug: USB serial, live LDR panel, raw BLE

Не входит в v1 web MVP: OTA/DFU, ride history, замена Flutter-приложения.
