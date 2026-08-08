# BikeComp — задачи

Обновлено: 2026-08-08

> **Важно:** `main` отстаёт от `dev` (~60 коммитов). Актуальный код и статус — на ветке **`dev`** (`v0.2.0-beta.3`). Таблица ниже отражает состояние **`dev`**.

## Состояние этапов

| Этап | Результат | Состояние | Подробности |
| --- | --- | --- | --- |
| Э0 | Репозиторий, toolchain, CI | Частично | [Firmware](tasks/firmware/README.md#э0-подготовка-и-каркас) |
| Э1 | Подтверждённое железо | Частично | [Hardware](tasks/hardware/README.md) |
| Э2 | Базовая прошивка | Частично | [Firmware](tasks/firmware/README.md#э2-базовая-прошивка) |
| Э3 | Данные переживают reboot | Почти готов | [Storage](tasks/firmware/storage.md) |
| Э4 | BLE-контракт | Частично (код готов) | [BLE](tasks/firmware/ble.md) |
| Э5 | MVP Android-приложения | В работе | [Mobile](tasks/mobile/README.md#э5-mvp-приложение) |
| Э6 | Полное приложение | Не начат | [Mobile](tasks/mobile/README.md#э6-расширенное-приложение) |
| Э7 | Испытания и v1.0 | Не начат | [Verification](tasks/verification/README.md) |

## Релизы

| Тег | Дата | Содержание |
| --- | --- | --- |
| `v0.2.0-beta.1` | 2026-08-03 | Первый публичный beta: BLE 1.1, Flutter MVP, power manager, A/B storage |
| `v0.2.0-beta.2` | 2026-08-05 | CI hardening, Zephyr Z5 parity, companion sync layout fixes |
| `v0.2.0-beta.3` | 2026-08-06 | Speed fix (Hall ISR + SpeedIntervalGuard), Zephyr speed port |

## Текущий инкремент

**Hardware gate Э4/Э5** — software часть закрыта на `dev`, аппаратная приёмка открыта.

Закрыто на `dev`:
- Э3.5 автосохранение одометра (код + native + embedded A/B + 2 reboot).
- Э4.1–4.14 BLE (GATT, commands, pairing, Error Log, Serial).
- Э5 automatic gate (52 Flutter tests, release APK, fake transport).
- Dual OLED 128×64/128×32, burn-in guard, ambient light software gate.
- CI/CD (firmware, mobile, Zephyr, release workflow).
- Beta.3 speed regression fix.

Открыто:
- 10× power-loss для Э3.5.
- Hall/стенд DoD (калибровка ≤2%, 100 slow/fast revolutions).
- LDR hardware: делитель, калибровка, плавность переходов.
- OLED 128×64 extended gate (LOW BATT, patterns, burn-in phases).
- Android hardware gate (≤5 с поиск, 10/10 connect, bond/reconnect, write-then-verify, MVP-команды).
- BLE hardware DoD (bonding reboot, closed window, 4-bond LRU).
- Zephyr field validation.
- Слияние `dev` → `main`.

Ветка `feat/zephyr-usb-profile` (не влита): USB profile migration, `load-config`/`set-odo-mm`, LittleFS rework.

## Порядок и зависимости

1. Собрать LDR D2/D3, выбрать резистор, калибровать, загрузить production 128×64.
2. Завершить расширенный OLED 128×64 gate.
3. Закрыть Storage Э3 (10× power-loss) и долги Hall/стенда.
4. Завершить BLE hardware DoD Э4.
5. Провести полный Android hardware gate Э5.
6. После hardware gate — начать Э6.
7. Soak, power, field tests Э7 → v1.0.
8. Слить `dev` → `main`; рассмотреть merge `feat/zephyr-usb-profile`.

## Папки задач

- [Обзор и правила](tasks/README.md)
- [Firmware: Э0 и Э2](tasks/firmware/README.md)
- [Firmware Storage: Э3](tasks/firmware/storage.md)
- [Firmware BLE: Э4](tasks/firmware/ble.md)
- [Hardware: Э1](tasks/hardware/README.md)
- [Mobile: Э5–Э6](tasks/mobile/README.md)
- [Verification и release: Э7](tasks/verification/README.md)
- [После v1.0](tasks/future/README.md)

Выполненным отмечается только пункт, подтверждённый кодом, тестом или протоколом аппаратной проверки. Частичная реализация остаётся незакрытой с пояснением.
