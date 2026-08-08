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
| Storage A/B + CRC | ✓ | ✓ | LittleFS на выделенной партиции |
| Battery ADC | ✓ | ✓ | SAADC P0.31 |
| Ambient LDR | ✓ | ✓ | D3/P0.29 power + A2/P0.28 ADC |
| USB Serial console | ✓ | ✓ | CDC RX + hall/gpio commands |
| Board LEDs | ✓ | ✓ | RGB/charge suppress, status LED hook |
| VBUS detect | ✓ | ✓ | `usbPresent()` via USBREG |
| OLED SSD1306 | ✓ | ✓ | u8g2 + shared `display_layout` |
| BLE GATT (Э4) | ✓ | заглушка | `CONFIG_BT` выключен; см. `docs/08-zephyr-migration.md` |

Подробный план, матрица паритета и порядок портирования — в
[`docs/08-zephyr-migration.md`](../docs/08-zephyr-migration.md).

## Быстрый старт

Требования: Python 3.10+, CMake, Ninja, Zephyr SDK (`ZEPHYR_SDK_INSTALL_DIR`).

```bash
cd firmware-zephyr
make                  # или ./scripts/build.sh
make upload           # прошивка по USB serial (как pio run -t upload)
make monitor          # serial console 115200
# make clean / make rebuild / make uf2
```

Прошивка:

- **Serial (рекомендуется):** `make upload` — adafruit-nrfutil по USB, без drag-and-drop UF2.
  Нужен пакет PIO `tool-adafruit-nrfutil` или переменная `ADAFRUIT_NRFUTIL`.
- **UF2 вручную:** `build/zephyr/zephyr.uf2` (двойной Reset → копирование на диск `XIAO BLE`).

```bash
UPLOAD_PORT=/dev/ttyACM0 make upload-nobuild   # без пересборки
./scripts/upload.sh --no-touch -p /dev/ttyACM0 # уже в bootloader
```

Переменные:

```bash
BOARD=xiao_ble/nrf52840/sense ./scripts/build.sh
BUILD_DIR=/tmp/bikecomp-zephyr-build ./scripts/build.sh
```

## Тесты

Доменная логика проверяется двумя путями:

```bash
cd firmware && pio test -e native          # Unity, 90 тестов (полное покрытие domain)
cd firmware-zephyr && make test            # ztest unittest, 17 сценариев (Twister)
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
