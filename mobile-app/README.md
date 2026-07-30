# BikeComp Mobile MVP

Android-приложение для поиска, настройки и обслуживания велокомпьютера BikeComp.
Application ID — `app.bikecomp.mobile`, `minSdk 24`, `compileSdk/targetSdk 36`.

## Запуск

Сначала подготовьте локальный toolchain из корня репозитория:

```bash
./tool/bootstrap-mobile.sh
cd mobile-app
../tool/flutterw pub get
../tool/flutterw run --dart-define=BIKECOMP_FAKE_BLE=true
```

Без `BIKECOMP_FAKE_BLE=true` используется production transport на
`flutter_reactive_ble`. Fake нельзя случайно включить runtime-настройкой.

## Архитектура

- `lib/data/protocol/` — единственное место, где доменные значения превращаются в
  little-endian байты protocol v1;
- `BleTransport` изолирует BLE-плагин и deterministic fake;
- `BikeComputerRepository` возвращает только доменные объекты и выполняет
  write-then-verify;
- Riverpod-контроллеры управляют FSM подключения и per-device config draft;
- `lib/presentation/` содержит четыре Material 3 раздела и временный connecting route.

Fake предоставляет профили idle/moving/paused/low-battery, sensor test 5 Гц и
инъекции ошибок. Для сценарных тестов transport переопределяется Riverpod provider.

## Проверки

```bash
../tool/flutterw pub run build_runner build
../.tooling/flutter/bin/dart format --set-exit-if-changed lib test
../tool/flutterw analyze
../tool/flutterw test
../tool/flutterw build apk --debug
../tool/flutterw build apk --release
```

Golden codec-тесты читают нормативные fixtures из `../protocol/fixtures`. Аппаратная
приёмка BLE, bonding, reconnect и permission flows выполняется отдельно после
