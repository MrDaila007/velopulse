# 08. Миграция прошивки на Zephyr RTOS

Обновлено: 2026-08-02

Цель: перенести firmware BikeComp с **PlatformIO + Arduino (Adafruit nRF52)** на
**Zephyr RTOS v4.1** для Seeed XIAO nRF52840 Sense / Super-nRF52840, сохранив
паритет с текущим статусом (`STATUS.md`, этап Э4) и неизменность `protocol/`.

---

## 1. Мотивация

| Аспект | Arduino (текущий) | Zephyr (целевой) |
| --- | --- | --- |
| RTOS | кооперативный loop + ISR | preemptive kernel, work queues |
| BLE | Adafruit Bluefruit / SoftDevice S140 | Zephyr Bluetooth host |
| FS | Adafruit InternalFS | LittleFS на выделенной flash-партиции |
| Тестируемость | domain на хосте (`native`) | тот же domain + `west build` smoke |
| Долгосрочно | зависимость от Adafruit core | upstream board `xiao_ble`, Nordic HAL |

Миграция **не меняет** BLE wire format, `DeviceConfig` codec и мобильное приложение.

---

## 2. Архитектура после миграции

```text
┌─────────────────────────────────────────────────────────────┐
│  firmware-zephyr/app/src/app_controller.cpp                 │
│  (оркестрация — общая логика с Arduino-сборкой)             │
└───────────────┬─────────────────────────────┬───────────────┘
                │                             │
┌───────────────▼──────────────┐   ┌──────────▼────────────────┐
│ firmware/lib/domain/       │   │ firmware-zephyr/app/src/ │
│ (29 модулей, без изменений)  │   │ platform/ + services/    │
└──────────────────────────────┘   └──────────┬────────────────┘
                                            │
                               ┌────────────▼────────────┐
                               │ Zephyr: GPIO, ADC, I2C, │
                               │ BT, LittleFS, USB CDC   │
                               └─────────────────────────┘
```

Правило: **вся бизнес-логика остаётся в `firmware/lib/domain/`**. Zephyr-адаптеры
реализуют те же интерфейсы, что Arduino `firmware/src/`.

---

## 3. Маппинг платформы

| Arduino / Adafruit | Zephyr | Файл адаптера |
| --- | --- | --- |
| `millis()` / `micros()` | `k_uptime_get_32()` / `k_cycle_get_64()` | `platform/time_console.cpp` |
| `Serial` (USB CDC) | `printk` + USB CDC ACM | `platform/time_console.cpp` |
| `attachInterrupt` + ring buffer | `gpio_callback` + ISR | `services/wheel_sensor.cpp` |
| `analogRead` P0.31 / A2 | SAADC `adc_read_dt` | `platform/adc_io.cpp` |
| `InternalFS` | LittleFS `/bikecomp` | `platform/littlefs_backend.cpp` |
| `U8g2` + `Wire` | I2C + SSD1306 (Z2) | `services/display_manager_*.cpp` |
| Bluefruit GATT | `CONFIG_BT` GATT (Z3) | `services/ble_manager_zephyr.cpp` |
| `NRF_POWER->RESETREAS` | `hwinfo_get_reset_cause()` | `platform/time_console.cpp` |
| `NVIC_SystemReset()` | `sys_reboot()` | `platform/time_console.cpp` |
| `dbgHeapFree()` | `k_mem_free_get()` | `platform/time_console.cpp` |

### Распиновка (Super-nRF52840)

Совпадает с `firmware/include/board_pins.h`; в Zephyr задаётся overlay
`firmware-zephyr/app/boards/super_nrf52840.overlay`:

| Сигнал | Arduino | nRF pin | DT alias |
| --- | --- | --- | --- |
| Hall | D0 | P1.02 | `hall-sensor` |
| LDR ADC | A2 | P1.04 | `ambient-adc` |
| LDR power | D3 | P0.28 | `ambient-power` |
| OLED I2C | D4/D5 | P0.05/P0.04 | `&i2c0` (board default) |
| Battery ADC | pin 32 | P0.31 | `battery-adc` |

---

## 4. Матрица паритета (STATUS.md → Zephyr)

Состояние на 2026-08-02 после подготовки миграции (ветка `cursor/zephyr-migration-a729`).

| Возможность STATUS.md | Arduino | Zephyr | Фаза |
| --- | :---: | :---: | --- |
| Domain logic + native 83 tests | ✓ | ✓ (shared) | Z0 |
| Pulse filter, speed, trip, ride FSM | ✓ | ✓ | Z0 |
| Scheduler 6 tasks | ✓ | ✓ | Z0 |
| Storage A/B + CRC + migration v1→v2 | ✓ | ✓ | Z1 |
| Odometer save policy | ✓ | ✓ | Z0 |
| Boot counter `/boot_cnt` | ✓ | ✓ | Z1 |
| Battery model + ADC 16-sample | ✓ | ✓ | Z1 |
| Ambient light model + manager | ✓ | ✓ | Z1 |
| Display layout/formatter (domain) | ✓ | ✓ (shared) | Z0 |
| OLED render 128×64/32 | ✓ | — | Z2 |
| Display power / burn-in | ✓ | логика ✓, HW — | Z2 |
| BLE GATT 7 characteristics | ✓ | — | Z3 |
| Advertising / pairing window | ✓ | — | Z3 |
| Safe + dangerous commands | ✓ | — | Z3 |
| Error log + sensor test | ✓ | — | Z3 |
| Serial console commands | ✓ | частично | Z1 |
| Diagnostics GET_DIAGNOSTIC | ✓ | ✓ (encode) | Z0 |
| Deep sleep | отложено | отложено | — |

---

## 5. Порядок портирования

```text
Z0 Каркас + domain + AppController
 │
 ├─▶ Z1 GPIO/ADC/FS + hardware smoke
 │
 ├─▶ Z2 OLED (I2C SSD1306 + DisplayCanvas)
 │
 └─▶ Z3 BLE (самый большой объём: ~600 LOC Bluefruit → Zephyr BT)
      │
      └─▶ Z4 CI + Android gate + docs update
```

**Риски:**

1. **BLE** — bonding, LESC, encrypted permissions, staged writes из ISR context.
2. **Flash layout** — партиция `bikecomp_storage` не должна пересечься с bootloader
   и существующими Arduino InternalFS данными (разные FS — миграция данных отдельная задача).
3. **Hall ISR** — latency GPIO callback vs текущий lightweight ISR.

**Низкий риск:** domain layer и `pio test -e native` остаются эталоном на всём пути.

---

## 6. Сборка и прошивка

```bash
cd firmware-zephyr
./scripts/bootstrap.sh
./scripts/build.sh
# UF2: build/zephyr/zephyr.uf2
```

Board targets:

- `xiao_ble/nrf52840` — базовая XIAO BLE
- `xiao_ble/nrf52840/sense` — Sense (IMU не используется в BikeComp v1)

Overlay Super-nRF52840 подключается автоматически в `scripts/build.sh`.

---

## 7. Миграция данных Flash

Arduino InternalFS и Zephyr LittleFS используют **разные** области и форматы
монтирования. Прямого чтения `/cfg_a` из Arduino FS в Zephyr нет.

Варианты (решение на Z4):

1. **Clean start** — Zephyr загружает defaults (как при первом включении Arduino).
2. **One-shot importer** — Serial/BLE команда читает legacy blob (если сохранён
   совместимый layout) — только при необходимости полевых устройств.
3. **Общий wire format** — payload 48+16 байт header идентичен; меняется только
   носитель.

---

## 8. CI (план)

Добавить job в GitHub Actions:

```yaml
- name: Zephyr build
  run: |
    cd firmware-zephyr
    ./scripts/bootstrap.sh
    ./scripts/build.sh
```

Требует кэша `deps/zephyr` и Zephyr SDK (~2 ГБ).

---

## 9. Связанные документы

- [`firmware-zephyr/README.md`](../firmware-zephyr/README.md) — quick start
- [`tasks/firmware/zephyr.md`](../tasks/firmware/zephyr.md) — чеклист задач
- [`docs/03-firmware-architecture.md`](03-firmware-architecture.md) — текущая Arduino-архитектура
- [`STATUS.md`](../STATUS.md) — эталон функциональности
