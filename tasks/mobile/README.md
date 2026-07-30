# Mobile application

## Э5. MVP-приложение

> На 2026-07-30 код и fake-tested automatic gate готовы. Пункты 5.1–5.15
> намеренно остаются незакрытыми до приёмки с firmware Э4.7–Э4.12 и двумя
> поколениями Android; fake-проверка не считается аппаратным завершением этапа.

- [ ] 5.1 Создать freezed domain models и enums.
- [ ] 5.2 Реализовать codecs и golden-тесты shared fixtures.
- [ ] 5.3 Реализовать ConfigValidator и boundary tests.
- [ ] 5.4 Реализовать FakeBleTransport, ride profiles и error injection.
- [ ] 5.5 Реализовать scan/connect/MTU/discover/read/write/subscribe.
- [ ] 5.6 Реализовать Android 12+/legacy permissions и adapter states.
- [ ] 5.7 Реализовать ConnectionController FSM и reconnect policy.
- [ ] 5.8 Создать scan screen, RSSI, UUID filter, remember/forget device.
- [ ] 5.9 Создать connecting/synchronizing screen.
- [ ] 5.10 Создать dashboard: gauge, metrics, states и quick actions.
- [ ] 5.11 Создать MVP settings и tire presets.
- [ ] 5.12 Реализовать dirty draft, local persistence и write-then-verify.
- [ ] 5.13 Создать maintenance: trip reset, OLED, sensor test, defaults.
- [ ] 5.14 Обработать incompatible protocol major.
- [ ] 5.15 Реализовать typed errors и понятные сообщения.

## Э6. Расширенное приложение

- [ ] 6.1 Экран diagnostics, versions, counters, RSSI и self-test.
- [ ] 6.2 Sensor test, MagnetPassIndicator и interval log.
- [ ] 6.3 Полные sensor settings.
- [ ] 6.4 Power/sleep/advertising/battery calibration settings.
- [ ] 6.5 Полные display page/order/pinned settings.
- [ ] 6.6 Device information screen.
- [ ] 6.7 JSON profile export/import с diff.
- [ ] 6.8 Diagnostic report export/share.
- [ ] 6.9 Трёхступенчатый dangerous reset odometer.
- [ ] 6.10 Skeleton/empty/error states, dark theme и accessibility.

## DoD Э5 — автоматический gate

- [x] Code generation повторяется без diff.
- [x] `dart format --set-exit-if-changed`, `flutter analyze` и 34 теста проходят.
- [x] Debug и release APK собираются с application ID `app.bikecomp.mobile`,
  minSdk 24 и targetSdk 36.
- [x] Config draft персистится асинхронно и namespaced по device ID.
- [x] Некорректные поля блокируют BLE write; команды не показывают успех без
  `CommandResult`.

## DoD Э5 — аппаратный gate

- [ ] Поиск ≤ 5 с и 10/10 подключений к firmware Э4.7–Э4.12.
- [ ] Bonding переживает reboot; reconnect работает после потери связи.
- [ ] Пять обязательных настроек проходят write-then-verify на устройстве.
- [ ] Все MVP-команды подтверждены реальным `CommandResult`.
- [ ] Permission/adapter/location flows проверены на Android ≤11 и ≥12.

## DoD Э6

- [ ] Все 10 экранов из ТЗ реализованы.
- [ ] Profile export/import и diagnostic export работают.
- [ ] Dangerous commands имеют полный многошаговый confirm flow.
