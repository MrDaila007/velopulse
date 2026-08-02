# BikeComp Zephyr firmware

Параллельная ветка прошивки на **Zephyr RTOS** для Seeed XIAO nRF52840 Sense /
Super-nRF52840. Цель — довести функциональность до уровня Arduino-сборки,
описанной в `STATUS.md` (этап Э4, 2026-08-01).

## Статус миграции

| Подсистема | Arduino | Zephyr | Примечание |
| --- | --- | --- | --- |
| Domain (`firmware/lib/domain`) | ✓ | ✓ | Общий код, 29 модулей |
| Scheduler / AppController | ✓ | ✓ | Портирован |
| Wheel sensor (GPIO ISR) | ✓ | ✓ | `gpio-keys` + ring buffer |
| Storage A/B + CRC | ✓ | ✓ | LittleFS на выделенной партиции |
| Battery ADC | ✓ | ✓ | SAADC P0.31 |
| Ambient LDR | ✓ | ✓ | D3 power + A2 ADC |
| USB Serial console | ✓ | частично | `printk`/CDC, shell-команды — в работе |
| OLED SSD1306 | ✓ | заглушка | Headless: счёт идёт, `display_ok=false` |
| BLE GATT (Э4) | ✓ | заглушка | `CONFIG_BT` выключен; см. `docs/08-zephyr-migration.md` |

Подробный план, матрица паритета и порядок портирования — в
[`docs/08-zephyr-migration.md`](../docs/08-zephyr-migration.md).

## Быстрый старт

Требования: Python 3.10+, CMake, Ninja, [Zephyr SDK](https://github.com/zephyrproject-rtos/sdk-ng) 0.16.x (`ZEPHYR_SDK_INSTALL_DIR`).

```bash
cd firmware-zephyr
./scripts/bootstrap.sh          # один раз: west + Zephyr v4.1.0
export ZEPHYR_SDK_INSTALL_DIR=~/zephyr-sdk-0.16.8  # путь к SDK
./scripts/build.sh              # xiao_ble/nrf52840 + Super overlay
```

Прошивка UF2: `build/zephyr/zephyr.uf2` (двойной клик Reset → drag-and-drop).

Переменные:

```bash
BOARD=xiao_ble/nrf52840/sense ./scripts/build.sh
BUILD_DIR=/tmp/bikecomp-zephyr-build ./scripts/build.sh
```

## Тесты

Доменная логика по-прежнему проверяется Arduino native-тестами:

```bash
cd firmware && pio test -e native
```

После включения Zephyr BLE/display добавить `west build` smoke и
hardware gate по `tasks/firmware/zephyr.md`.

## Структура

```text
firmware-zephyr/
├── west.yml
├── scripts/
│   ├── bootstrap.sh
│   └── build.sh
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
