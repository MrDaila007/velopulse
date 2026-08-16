# BikeComp firmware 0.2.0

Версии прошивки, протокола BLE и мобильного приложения задаются в корневом
[`version.toml`](../version.toml). После изменения запустите:

```bash
python3 ../tools/sync_versions.py
```

Первый инкремент прошивки Super-nRF52840 с bootloader XIAO: подсчёт импульсов, fixed-point скорость,
дистанция, время движения, автостарт/автопауза и экран SSD1306 с пятью страницами.

## Подключение стенда

| Компонент | Super-nRF52840 |
| --- | --- |
| OLED VCC / GND | 3V3 / GND |
| OLED SDA / SCL | D4 / D5 |
| Геркон (reed) | D0 ↔ геркон ↔ D1 (D0 = drive LOW, D1 = sense) |
| LDR ADC | D2/A2 через узел LDR / 22 кΩ / 100 нФ |
| LDR power | D3, включается только на время измерения |

Геркон: прошивка держит **D0** в LOW и читает **D1** с внутренней подтяжкой
(`BIKECOMP_HALL_TWO_WIRE=1`). Альтернатива — один контакт на D1, второй на GND
(`BIKECOMP_HALL_TWO_WIRE=0`). OLED должен иметь адрес `0x3C`.

Профиль выбирается при сборке: `xiao_ble_sense` — основной SSD1306 128×64,
`xiao_ble_sense_128x32` — совместимый SSD1306 128×32. Оба используют I²C `0x3C`.

## Сборка и прошивка

Цели Makefile (из каталога `firmware/`):

```bash
make                    # сборка ENV=xiao_ble_sense
make test               # pio test -e native
make upload             # USB serial DFU (nrfutil / двойной Reset)
make flash-stlink       # SWD через ST-Link, бутлодер не стирается
make dfu-ble            # BLE OTA с этого ПК (нужен bleak)
make monitor            # Serial 115200
make help
make build ENV=xiao_ble_sense_128x32
make build ENV=xiao_ble_sense_deep_sleep
```

Те же `flash-stlink` / `dfu-ble` доступны из `firmware-zephyr/` (прокси в Arduino-сборку).

Эквивалент без make:

```bash
pio test -e native
pio run -e xiao_ble_sense
pio run -e xiao_ble_sense -t upload
./scripts/flash_stlink.sh
python3 scripts/ble_dfu.py
pio device monitor -b 115200
```

### USB serial / UF2

Штатный путь, если CDC живой: `make upload` или двойной Reset и копирование UF2.
USB-C Super-nRF52840 может не перечисляться; бутлодер Adafruit UF2 при этом
остаётся, и прошивать можно по SWD или BLE.

### ST-Link (SWD)

Нужны OpenOCD и ST-Link (SWDIO, SWCLK, GND; питание 3.3 В или USB платы).

```bash
make flash-stlink
# или: ./scripts/flash_stlink.sh xiao_ble_sense_128x32
```

Скрипт пишет `firmware.hex` без mass-erase бутлодера и обновляет CRC16 настроек
Adafruit на `0xFF000`, иначе после reset приложение может не стартовать.

### BLE OTA

После рабочей прошивки с `BLEDfu` плата рекламирует `BikeComp-XXXX` и сервис
`00001530-1212-EFDE-1523-785FEABCD123`. Это аварийный Adafruit DFU, не Nordic
Secure DFU v1.1.

**С телефона:** nRF Connect или nRF Device Firmware Update → Connect → DFU →
`.pio/build/xiao_ble_sense/firmware.zip`. Имя в бутлодере — `AdaDFU` / `DfuTarg`.
OLED в бутлодере гаснет — это нормально.

**С ПК** (BlueZ, пакет `bleak`):

```bash
make dfu-ble
# или: python3 scripts/ble_dfu.py .pio/build/xiao_ble_sense/firmware.zip
```

Если после SWD-заливки connect сразу рвётся: на ПК остался старый bond.

```bash
bluetoothctl remove ED:CC:18:B1:9F:7D   # адрес из scan
```

Затем перезапустите плату (pairing window 5 минут) и снова `make dfu-ble`.
Слабый RSSI (~−80 dBm) сильно замедляет заливку; поднесите плату ближе к ноутбуку.

После загрузки первый импульс запускает поездку и начисляет один оборот. Скорость
появляется после второго корректного импульса. Импульсы быстрее лимита 100 км/ч
отбрасываются; через 3 секунды без импульсов устройство переходит в `PAUSED`.

## Экран

Основная сборка рассчитана на 128×64: скорость выводится шрифтом Logisoso 38,
единицы и батарея находятся в верхней строке, а метрика карусели — в нижней зоне.
Совместимая 128×32 сборка сохраняет прежний пиксельный интерфейс. При заряде ≤20%
нижняя строка периодически показывает `LOW BATT`; пять страниц меняются раз в 4 секунды.

По умолчанию через 30 секунд без корректных импульсов колеса экран снижает яркость,
а через 60 секунд выключается командой SSD1306 power-save. Первый корректный
импульс немедленно включает экран; после wake применяется эффективный контраст. Значение
`display_timeout_s = 0` отключает оба перехода.
Production-сборки используют LDR для пяти уровней автоматической яркости. Значение
`brightness_pct` задаёт верхний предел; при `BIKECOMP_AMBIENT_LIGHT=0` оно снова
задаёт фиксированную яркость. Схема и процедура калибровки неизвестного LDR описаны
в `docs/05-hardware-design.md §3.1`.
Перед основным чтением внутренний pull-down проверяет наличие делителя; отсутствующий
LDR даёт `raw=0`, `valid=0` и возвращает пользовательский максимум. Контраст
вычисляется как `8 + brightness_pct × 247 / 100`. Serial diagnostics выводит
`raw`, `filtered`, auto/effective percent и признак `valid`. Для калибровки:
`ambient-raw` (лог раз в секунду) и `ambient-stop`.

## Serial console

Команды (115200, CR/LF): `open-pairing`, `dump-config`, `reset-odo`, `selftest`,
`ambient-raw`, `ambient-stop`, `display-state`, `wake-display`, `power-status`, `status`,
`test-on` / `test-off` (USB regression protocol, см. `tools/usb_regression.py`). Скрипты в `tools/`.

Протокол BLE **1.1**: характеристика `Companion Write` (`000B`) — время и погода с телефона
(шапка OLED `14:32`, под батареей `+18C` и `R40%`). Без геолокации: город задаётся в приложении.

Разметка и форматирование находятся в общих C++-модулях `display_layout.cpp` и
`display_formatter.cpp`. Эти же файлы напрямую компилирует OLED-симулятор для
обеих геометрий.

## Батарея Super-nRF52840

Внешний делитель подключается: `BAT+ → 1 MΩ → P0.31 → 1 MΩ → GND`. P0.31 —
задняя площадка платы. Калибровка пока номинальная:
`scale=1000`, `offset=0`; диагностический Serial выводит raw, spread, mV, percent и VBUS.

## Энергосбережение

- `power_save_mode` (config flags bit 6) — немедленный low-power idle после выключения OLED.
- `deep_sleep_timeout_s` — задержка до low-power idle (0 = выкл); при `deep_sleep_enabled`
  телеметрия показывает `kDeepSleepPending`.
- `BIKECOMP_FEATURE_DEEP_SLEEP=1` (профиль `xiao_ble_sense_deep_sleep`) — System OFF с
  пробуждением от магнита на D1 или USB.

USB regression (host sends fixture lines, firmware executes and answers OK/FAIL):

```bash
python3 tools/usb_regression.py --port /dev/ttyACM0
python3 -m unittest tools/test_usb_regression.py
```
