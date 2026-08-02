# Zephyr RTOS migration (firmware-zephyr)

Цель: перенести прошивку BikeComp с Arduino/Adafruit на Zephyr RTOS с сохранением
BLE-контракта (`protocol/`) и паритета с `STATUS.md` (Э4, 2026-08-01).

## Этапы

### Z0 — Каркас (выполнено)

- [x] Z0.1 West workspace `firmware-zephyr/`, board `xiao_ble/nrf52840`.
- [x] Z0.2 Devicetree overlay Super-nRF52840 (Hall, LDR, battery P0.31, LittleFS).
- [x] Z0.3 CMake: линковка `firmware/lib/domain` (29 модулей).
- [x] Z0.4 `AppController` + scheduler на Zephyr.
- [x] Z0.5 Platform: time, console, ADC, GPIO wheel, LittleFS backend.
- [x] Z0.6 Документация `docs/08-zephyr-migration.md`.

### Z1 — Периферия без BLE (в работе)

- [x] Z1.1 Wheel sensor ISR + ring buffer.
- [x] Z1.2 Storage A/B (`/cfg_*`, `/odo_*`, `/boot_cnt`).
- [x] Z1.3 Battery + ambient ADC managers.
- [ ] Z1.4 USB Serial shell (`open-pairing`, `dump-config`, `selftest`, …).
- [ ] Z1.5 Embedded smoke на XIAO (boot, mount, pulse, odometer reboot).
- [ ] Z1.6 VBUS detect для `usb_present` (nRF USBREG).

### Z2 — OLED (не начато)

- [ ] Z2.1 Zephyr I2C SSD1306 128×64 @ 400 кГц, addr 0x3C.
- [ ] Z2.2 `DisplayCanvas` adapter (U8g2 или Zephyr display API).
- [ ] Z2.3 Shared renderer: `display_layout` / `display_formatter`.
- [ ] Z2.4 Display power, burn-in guard, ambient brightness.
- [ ] Z2.5 Compile-time профиль 128×32.
- [ ] Z2.6 Simulator/golden parity (без изменений в domain).

### Z3 — BLE Э4 (не начато)

- [ ] Z3.1 `CONFIG_BT` peripheral, custom GATT service (7 characteristics).
- [ ] Z3.2 Advertising fast/slow, Scan Response name + Tx Power.
- [ ] Z3.3 Device Info live Read, Telemetry notify 1 Гц / 0.2 Гц.
- [ ] Z3.4 Config Write queue + validation + apply.
- [ ] Z3.5 Safe commands 0x01–0x0B, Dangerous 0x20–0x40 + nonce/TTL.
- [ ] Z3.6 LESC pairing, bonding (`bt_settings`), 5-min window.
- [ ] Z3.7 Error Log ring, sensor-test 5 Гц.
- [ ] Z3.8 Protocol fixture parity (`protocol/fixtures/*`).
- [ ] Z3.9 Android hardware gate (reuse Э5 checklist).

### Z4 — Приёмка

- [ ] Z4.1 `pio test -e native` без регрессий (domain общий).
- [ ] Z4.2 Zephyr CI job (`west build`).
- [ ] Z4.3 Parity checklist vs Arduino production build.
- [ ] Z4.4 Документация `docs/03-firmware-architecture.md` — секция Zephyr.

## DoD миграции

Миграция считается завершённой, когда Zephyr-сборка проходит те же software gates,
что Arduino `STATUS.md` «Проверки», и закрывает hardware gate Э4/Э5 без изменения
`protocol/`.

## Ссылки

- [`docs/08-zephyr-migration.md`](../../docs/08-zephyr-migration.md)
- [`firmware-zephyr/README.md`](../../firmware-zephyr/README.md)
- [`tasks/firmware/ble.md`](ble.md) — исходные требования Э4
