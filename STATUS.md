# Статус проекта

Обновлено: 2026-08-08

## Снимок веток

| Ветка | Состояние | Комментарий |
| --- | --- | --- |
| `main` | **Устарела** (последний коммит 2026-07-29) | Э2: базовая прошивка без BLE, mobile и автосохранения |
| `dev` | **Активная разработка** | `v0.2.0-beta.3`, BLE Э4, Flutter MVP, CI/CD, Zephyr experimental |
| `feat/zephyr-usb-profile` | WIP | USB profile migration, `load-config`/`set-odo-mm` через Serial, LittleFS rework |
| `cursor/zephyr-migration-a729` | Слита в `dev` | Zephyr scaffold, ambient light, BLE-порт |
| `cursor/cicd-firmware-mobile-060b` | Слита в `dev` | GitHub Actions, iOS build (TestFlight снят в beta.2) |

Актуальный код, тесты и релизные артефакты — на **`dev`**. `main` отстаёт примерно на 60+ коммитов.

Версии продукта задаются в [`version.toml`](version.toml) (на `dev`): firmware Arduino `0.2.0`, Zephyr `0.2.0-zephyr`, mobile `1.1.0+5`, BLE protocol `1.1`.

## Текущий этап

**Э4 → Э5.** BLE-интеграция (Э4.1–4.14) реализована в коде и прошла software gate. Flutter Android MVP собран; базовая end-to-end интеграция с реальным XIAO подтверждена (discovery, connect, работа). Полный hardware gate Э4/Э5 и релиз **v1.0** ещё открыты.

Публичные beta-релизы: `v0.2.0-beta.1` (2026-08-03), `beta.2` (2026-08-05), `beta.3` (2026-08-06). Beta.3 исправляет регрессию live speed = 0 при работающей дистанции (Hall ISR + `SpeedIntervalGuard`).

## Готово

### Прошивка Arduino (PlatformIO)

- PlatformIO-проект для XIAO nRF52840 Sense / Super-nRF52840 и native-окружения.
- Фильтрация импульсов, fixed-point скорость, дистанция, средняя/максимальная скорость.
- Автостарт и автопауза, неблокирующий scheduler, ISR с кольцевым буфером.
- Двухпроводный геркон: D0 (drive LOW) + D1 (sense) или однопроводный на D1.
- Минимальный OLED-интерфейс, проверенный на устройстве (кнопка / геркон).
- GUI/headless-симулятор OLED и pixel-golden тесты состояний IDLE/MOV/PAUSE.
- `PageCarousel` с настройкой порядка, маски, периода и фиксированной страницы.
- Два compile-time OLED-профиля: primary SSD1306 **128×64** (Logisoso 38) и совместимый **128×32**.
- `DisplayBurnInGuard`: четырёхфазный pixel shift раз в 60 с независимо от auto-off.
- `BatteryModel` / `BatteryManager`: ADC P0.31, EMA, SoC LUT, внешний делитель 1 MΩ/1 MΩ.
- `DisplayPower` / `DisplayManager`: dim/off/wake, SSD1306 power-save.
- `AmbientLightModel` / `AmbientLightManager`: LDR на D2/D3, EMA, 5 уровней, manual cap, invalid fallback.
- `PowerManager` FSM: агрессивное энергосбережение BLE, display policy, deep sleep (опциональный build).
- `StorageManager`: A/B-слоты `/cfg_a/b`, `/odo_a/b`, CRC32, recovery, migration v1→v2.
- `OdometerSavePolicy`: автосохранение по дистанции, паузе, OLED off, deep sleep, critical battery, USB disconnect, reboot.
- `diagnostics`: snapshot §6.1, `GET_DIAGNOSTIC`, Serial dump при старте.
- **BLE Э4.1–4.14**: GATT, advertising, telemetry, config write/read, safe/dangerous commands, pairing window 5 мин, Error Log, Serial console.
- BLE Companion Sync: время и погода на OLED (protocol 1.1, char `000B`).
- USB Serial regression protocol + host runner (`tools/usb_regression.py`).

### Мобильное приложение (Flutter)

- Flutter 3.44.7, `app.bikecomp.mobile`, minSdk 24, targetSdk 36.
- Protocol v1 Freezed-модели/codecs, ConfigValidator, FakeBleTransport.
- Production `flutter_reactive_ble` transport, Android permissions/bond layer.
- FSM/reconnect, write-then-verify, per-device dirty drafts.
- Четыре Material 3 раздела: scan, dashboard, settings, maintenance.
- Power save / deep sleep settings UI, companion sync (clock/weather).

### Zephyr (experimental)

- `firmware-zephyr/`: Z1–Z5 порты domain-логики, BLE, PowerManager, deep sleep, companion sync.
- CI собирает `zephyr.{hex,uf2,elf}`; field validation ещё не пройдена.
- Ветка `feat/zephyr-usb-profile`: расширенный LittleFS partition, serial profile restore.

### Инфраструктура

- GitHub Actions: firmware native tests, Flutter analyze/test, Zephyr west build, release workflow.
- Unified versioning (`version.toml` + `tools/sync_versions.py`).
- Serial gate tools: `oled_gate_serial.py`, `ldr_calibrate.py`, `android_gate.sh`.
- 3D-модель корпуса: `3d-models/velopulse.3mf` (dev, 2026-08-06).

## Проверки

- `pio test -e native`: **83/83** (dev) — ambient, BLE, storage, diagnostics, Serial.
- `pio run -e xiao_ble_sense` (128×64): RAM ~17.4 КБ, Flash ~171 КБ.
- `pio run -e xiao_ble_sense_128x32`: собирается, совместимые пиксели.
- `flutter test`: **52/52**; release APK собирается.
- `./simulator/test.sh`: 6 групп, **18 golden-кадров** (128×32 + 128×64).
- Boot smoke XIAO: `BLE GATT: OK`, `selftest=0x3F`, `i2c_err=0`.
- Android hardware smoke 2026-07-31: discovery/connect нового APK с реальным XIAO подтверждены.
- Odometer A/B: seed 424242 mm / 77 rev переживает 2 reboot (sequence=3).
- Ambient без LDR: `raw=0`, `valid=0`, manual cap — fallback подтверждён.
- Beta.3 speed fix: Hall ISR edge capture восстановлен, `SpeedIntervalGuard` сбрасывается при resume.

## Ограничения

- **`main` не отражает текущее состояние** — для работы использовать `dev`.
- Реальный датчик Холла/геркон и повторяемый стенд импульсов не прошли полный DoD (калибровка ≤2%).
- SSD1306 128×64: базовый smoke пройден; полный gate (LOW BATT, patterns, burn-in phases) открыт.
- LDR-делитель не собран; калибровка dark/room/outdoor и ток ≤20 мкА ждут железа.
- Автосохранение: 10× power-loss вручную не проверено.
- BLE hardware DoD: bonding после reboot, closed-window, bond LRU (4 устройства), sensor-test 5 Гц на телефоне.
- Mobile hardware gate: поиск ≤5 с, 10/10 connect, write-then-verify 5 настроек, MVP-команды, Android ≤11/≥12.
- Zephyr — experimental; primary target остаётся Arduino firmware + Android APK.
- iOS: CI build есть, TestFlight workflow снят; field validation не выполнена.
- ANT+, каденс, пульс — вне scope v1.0 (см. `bike-tz.md` §1.3).

## В работе (ветки вне main)

| Тема | Ветка | Статус |
| --- | --- | --- |
| USB profile + serial config restore | `feat/zephyr-usb-profile` | Код готов, не влит в `dev` |
| Ambient auto-calibration (design) | `dev` (`ed0d3b3`) | Спека написана, реализация не начата |
| Zephyr LittleFS storage backend | `feat/zephyr-usb-profile` | Rework partition + backend |

## Следующий шаг

1. Слить `dev` → `main` (или продолжать на `dev` до v1.0).
2. Собрать LDR-делитель, калибровать, загрузить production 128×64.
3. Закрыть OLED gate (LOW BATT, patterns, dim/off/wake, burn-in, pulse smoke).
4. Закрыть Android hardware gate (10/10, bond/reconnect, write-then-verify, MVP-команды).
5. Ручная проверка 10× power-loss для Э3.5.
6. Soak/power/field tests → v1.0.
