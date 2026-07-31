# BikeComp — задачи

Обновлено: 2026-07-31

## Состояние этапов

| Этап | Результат | Состояние | Подробности |
| --- | --- | --- | --- |
| Э0 | Репозиторий, toolchain, CI | Частично | [Firmware](tasks/firmware/README.md#э0-подготовка-и-каркас) |
| Э1 | Подтверждённое железо | Частично | [Hardware](tasks/hardware/README.md) |
| Э2 | Базовая прошивка | Частично | [Firmware](tasks/firmware/README.md#э2-базовая-прошивка) |
| Э3 | Данные переживают reboot | В работе | [Storage](tasks/firmware/storage.md) |
| Э4 | BLE-контракт | Частично | [BLE](tasks/firmware/ble.md) |
| Э5 | MVP Android-приложения | В работе | [Mobile](tasks/mobile/README.md#э5-mvp-приложение) |
| Э6 | Полное приложение | Не начат | [Mobile](tasks/mobile/README.md#э6-расширенное-приложение) |
| Э7 | Испытания и v1.0 | Не начат | [Verification](tasks/verification/README.md) |

## Текущий инкремент

Э3.5 hardware DoD частично закрыт на XIAO (`/dev/ttyACM0`): odo A/B embedded,
ненулевой odometer после 2 reboot, production восстановлена. Остаётся 10×
power-loss вручную → M3. BLE Э4.1–4.14 программно выполнен: advertising, команды,
nonce/TTL/binding, encryption и 5-минутное pairing window; 78/78 native, XIAO build
и 50/50 Flutter tests проходят. Э4.13 Error Log/sensor-test
и Э4.14 Serial-консоль выполнены; Serial проверен на XIAO (`dump-config`, `selftest`,
`open-pairing`). Текущая production загружена на XIAO. Подтверждённая lifecycle race
старого scan transport устранена; Android UUID scan filter заменён локальным,
Device Info/pairing/subscriptions упорядочены, link cleanup проверен тестами. Новый
release APK установлен: пользователь подтвердил discovery/connect и работу с реальным
XIAO. Базовая интеграция с приложением достигнута. Дальше полный Android hardware
gate. Э5 проходит fake/automatic gate, но остаётся незакрытым до полной аппаратной
приёмки.

## Порядок и зависимости

1. Закрыть Storage Э3 (остаток DoD 3.5: 10× power-loss) и аппаратные долги Hall/стенда.
2. BLE Э4.1–4.14 выполнены; завершить оставшийся hardware DoD Э4.
3. Discovery/connect нового APK подтверждены; провести полный Android hardware gate.
4. Flutter codecs/FakeBleTransport и automatic gate Э5 — выполнены.
5. После hardware gate Э5 начать Э6.
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
