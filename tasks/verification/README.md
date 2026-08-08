# Э7. Проверка, оптимизация и релиз

## Задачи

- [ ] 7.1 Измерить current во всех режимах и убрать лишние wakeups.
  Реализовано: `PowerManager` low-power idle, `power-status`, замедленный scheduler.
  Ожидаемый idle с OLED off: 0.6–1.5 мА (ADR-009). Замер на стенде — вручную.
- [ ] 7.2 Проверить deep sleep и wake от Hall/USB.
  Код: `xiao_ble_sense_deep_sleep`, GPIO SENSE P0.03 + VBUS. 50 циклов — вручную.
- [ ] 7.3 Выполнить 100 BLE connect/disconnect и connection 2 ч.
- [ ] 7.4 Выполнить 500 save cycles и проверить flash counters.
- [ ] 7.5 Проверить vibration, bumps, Hall gaps и false pulses.
- [ ] 7.6 Провести ≥3 rides суммарно ≥60 км, включая влажные условия.
- [ ] 7.7 Проверить watchdog/reset reason/data recovery.
- [ ] 7.8 Закрыть firmware/app acceptance с протоколами.
- [ ] 7.9 Собрать signed release artifacts, tags и changelog.

## Приёмка firmware — 17 пунктов

- [ ] 20 стабильных boot и корректный boot_count.
- [ ] SSD1306: splash, speed, battery и пять страниц.
- [ ] Каждый оборот регистрируется ровно один раз.
- [ ] Ложные импульсы фильтруются.
- [ ] Погрешность скорости ≤2%.
- [ ] Distance/odometer переживают reboot.
- [ ] Reset trip не сбрасывает odometer.
- [ ] Auto start работает менее чем за 1 с.
- [ ] Auto pause соответствует timeout ±0.3 с.
- [x] OLED auto off работает.
  `tools/oled_gate_serial.py` на XIAO: dim на 30 с, off на 60 с, wake через
  `wake-display` (D0 pulse — отдельная ручная проверка).
- [ ] Новый оборот включает OLED менее чем за 300 мс.
- [ ] OLED auto brightness: raw dark/room/outdoor откалиброваны; максимумы
  10/60/100% соблюдаются без мерцания.
  LDR не подключён (`valid=0`); `tools/ldr_calibrate.py` готов к сборке делителя.
- [ ] Цепь LDR потребляет в среднем ≤20 мкА при цикле измерения 1 Гц.
- [ ] Fill/checkerboard/text и четыре фазы burn-in shift проверены на 128×64.
- [ ] Battery level сверена на трёх уровнях.
- [ ] BLE обнаруживается с 5 м.
- [ ] Все config fields читаются/записываются.
- [ ] Все invalid boundaries отклоняются с правильным field.
- [ ] RESET_TRIP работает через BLE.
- [ ] 2 км без телефона считаются корректно.

## Приёмка приложения — 15 пунктов

- [ ] Scan, connect и reconnect.
- [ ] Speed, distance, time и battery совпадают с устройством.
- [ ] Config read/write, wheel circumference, OLED и timeouts.
- [ ] Maintenance запускает все три OLED test patterns с реальным CommandResult.
- [ ] Trip reset и результаты всех 12 commands.
- [ ] Invalid config не отправляется.
- [ ] Incompatible major блокирует write.
- [ ] Все error messages понятны и проверены.

## Release regression и DoD

- [ ] Native, embedded, Flutter tests/analyze зелёные.
- [ ] BLE soak 100 cycles пройден.
- [ ] Idle current с OLED off ≤1.5 мА.
- [ ] GPS distance error ≤2% на дистанции ≥20 км.
- [ ] Нет reset/data loss за ≥60 км полевых испытаний.
- [ ] Контрольная поездка перед релизом ≥10 км.
- [ ] Protocol fixtures совпадают на firmware/mobile.
- [ ] Собраны firmware `.uf2`/`.hex`, signed APK, tags и changelog.
