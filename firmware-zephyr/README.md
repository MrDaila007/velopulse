# BikeComp Zephyr firmware

Параллельная ветка прошивки на **Zephyr RTOS** для Seeed XIAO nRF52840 Sense /
Super-nRF52840. Цель — довести функциональность до уровня Arduino-сборки,
описанной в `STATUS.md` (этап Э4, 2026-08-01).

## Статус миграции

| Подсистема | Arduino | Zephyr | Примечание |
| --- | --- | --- | --- |
| Domain (`firmware/lib/domain`) | ✓ | ✓ | Общий код, 29 модулей |
| Scheduler / AppController | ✓ | ✓ | Портирован |
| Wheel sensor (GPIO ISR) | ✓ | ✓ | Two-wire D0 drive + D1 sense |
| Storage A/B + CRC | ✓ | ✓ | LittleFS 24 KiB; BLE settings NVS 8 KiB |
| Battery ADC | ✓ | ✓ | SAADC P0.31 |
| Ambient LDR | ✓ | ✓ | D3/P0.29 power + A2/P0.28 ADC |
| USB Serial console | ✓ | ✓ | CDC RX + hall/gpio commands |
| Board LEDs | ✓ | ✓ | RGB/charge suppress, status LED hook |
| VBUS detect | ✓ | ✓ | `usbPresent()` via USBREG |
| OLED SSD1306 | ✓ | ✓ | u8g2 + shared `display_layout` |
| BLE GATT (Э4) | ✓ | код готов | pairing/config restore требуют hardware gate |

Подробный план, матрица паритета и порядок портирования — в
[`docs/08-zephyr-migration.md`](../docs/08-zephyr-migration.md).

## Быстрый старт

Требования: Python 3.12+, CMake, Ninja и Zephyr SDK. Zephyr 4.4 и остальные
локальные пути можно обнаружить автоматически:

```bash
cd firmware-zephyr
make env              # найти установки и записать игнорируемый .env
make env-show         # проверить, какие пути будут использованы
make                  # или ./scripts/build.sh
make upload           # прошивка по USB serial (как pio run -t upload)
make monitor          # serial console 115200
# make clean / make rebuild / make uf2
```

Прошивка:

- **Serial (рекомендуется):** `make upload` — adafruit-nrfutil по USB, без drag-and-drop UF2.
  Нужен пакет PIO `tool-adafruit-nrfutil` или переменная `ADAFRUIT_NRFUTIL`.
- **UF2 вручную:** `build/zephyr/zephyr.uf2` (двойной Reset → копирование на диск `XIAO BLE`).

После обновления со старой Zephyr-разметки первый запуск переформатирует бывший
общий storage в отдельные NVS и LittleFS. Старые Zephyr bonds/config будут удалены;
после нового pairing конфигурацию следует восстановить из мобильного бэкапа.

```bash
UPLOAD_PORT=/dev/ttyACM0 make upload-nobuild   # без пересборки
./scripts/upload.sh --no-touch -p /dev/ttyACM0 # уже в bootloader
```

Переменные:

```bash
BOARD=xiao_ble/nrf52840/sense ./scripts/build.sh
BUILD_DIR=/tmp/bikecomp-zephyr-build ./scripts/build.sh
BIKECOMP_SEARCH_ROOTS=/mnt/dev:/opt make env
```

`.env` содержит абсолютные пути только текущей машины и не попадает в Git;
структура файла приведена в [`.env.example`](.env.example). Значения из shell или
командной строки имеют приоритет. Без `.env` скрипты сначала проверяют стандартные
места, затем ищут маркеры Zephyr, SDK и U8g2 внутри `HOME`, `/opt` и `/data`
(глубина ограничена). Корни поиска задаются colon-separated переменной
`BIKECOMP_SEARCH_ROOTS`; поиск можно отключить через `BIKECOMP_AUTO_DISCOVER=0`.

## Тесты

Доменная логика проверяется двумя путями:

```bash
cd firmware && pio test -e native          # Unity, 127 тестов (полное покрытие domain)
cd firmware-zephyr && make test            # ztest unittest, 29 сценариев (Twister)
```

`make test` запускает `west twister` на `tests/domain/` (host `unit_testing`, без SDK).
Полный набор domain-тестов остаётся в Arduino native; ztest дублирует критичные
сценарии (codec, motion, protocol fixtures) для Zephyr CI.

## Структура

```text
firmware-zephyr/
├── west.yml
├── scripts/
│   ├── bootstrap.sh
│   ├── configure-env.sh
│   ├── env.sh
│   ├── build.sh
│   ├── upload.sh
│   ├── serial_upload.py
│   ├── monitor.sh
│   └── test.sh
├── tests/
│   └── domain/           # ztest host unittest (Twister type: unit)
└── app/
    ├── CMakeLists.txt
    ├── prj.conf
    ├── boards/super_nrf52840.overlay
    ├── include/          # platform.h, board_pins.h
    └── src/
        ├── app_controller.*
        ├── platform/     # time, ADC, LittleFS
        └── services/     # wheel, battery, ambient, BLE/display stubs
```

Общие заголовки и домен: `firmware/include`, `firmware/lib/domain`.
