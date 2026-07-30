# BikeComp — задачи

Обновлено: 2026-07-30

## Состояние этапов

| Этап | Результат | Состояние | Подробности |
| --- | --- | --- | --- |
| Э0 | Репозиторий, toolchain, CI | Частично | [Firmware](tasks/firmware/README.md#э0-подготовка-и-каркас) |
| Э1 | Подтверждённое железо | Частично | [Hardware](tasks/hardware/README.md) |
| Э2 | Базовая прошивка | Частично | [Firmware](tasks/firmware/README.md#э2-базовая-прошивка) |
| Э3 | Данные переживают reboot | В работе | [Storage](tasks/firmware/storage.md) |
| Э4 | BLE-контракт | Частично | [BLE](tasks/firmware/ble.md) |
| Э5 | MVP Android-приложения | Не начат | [Mobile](tasks/mobile/README.md#э5-mvp-приложение) |
| Э6 | Полное приложение | Не начат | [Mobile](tasks/mobile/README.md#э6-расширенное-приложение) |
| Э7 | Испытания и v1.0 | Не начат | [Verification](tasks/verification/README.md) |

## Текущий инкремент

Э3.5 hardware DoD частично закрыт на XIAO (`/dev/ttyACM0`): odo A/B embedded,
ненулевой odometer после 2 reboot, production восстановлена. Остаётся 10×
power-loss вручную → M3. BLE Э4.1–4.6 (GATT + live Device Info: uptime,
boot_count, reset_reason, flags); дальше Э4.7 Telemetry notify / Э4.8 Config Write.

## Порядок и зависимости

1. Закрыть Storage Э3 (остаток DoD 3.5: 10× power-loss) и аппаратные долги Hall/стенда.
2. BLE structures/fixtures/GATT/Device Info Э4.1–4.6 — выполнены.
3. Реализовать BLE firmware — Э4.7–4.14.
4. После fixtures параллельно начать Flutter codecs и FakeBleTransport.
5. Завершить MVP Э5 и расширенное приложение Э6.
6. Выполнить soak, power и field tests Э7, затем собрать v1.0.

## Папки задач

- [Обзор и правила](tasks/README.md)
- [Firmware: Э0 и Э2](tasks/firmware/README.md)
- [Firmware Storage: Э3](tasks/firmware/storage.md)
- [Firmware BLE: Э4](tasks/firmware/ble.md)
- [Hardware: Э1](tasks/hardware/README.md)
- [Mobile: Э5–Э6](tasks/mobile/README.md)
- [Verification и release: Э7](tasks/verification/README.md)
- [После v1.0](tasks/future/README.md)

Выполненным отмечается только пункт, подтверждённый кодом, тестом или протоколом
аппаратной проверки. Частичная реализация остаётся незакрытой с пояснением.
